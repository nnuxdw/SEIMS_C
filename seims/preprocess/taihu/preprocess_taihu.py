#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
使用太湖流域面状 Shapefile 裁剪以下数据：

    burnedfillDEM.tif
    Final_fdr.tif
    Watershed.tif
    stream.tif
    stream_split.shp

兼容 Python 3.6，依赖 GDAL/OGR（osgeo）。
"""

from __future__ import print_function

import math
import os
import sys
import threading
from collections import deque
from concurrent.futures import ThreadPoolExecutor, as_completed

import numpy as np
from osgeo import gdal
from osgeo import ogr
from osgeo import osr


def enable_exceptions():
    """让 GDAL/OGR 发生错误时抛出 Python 异常。"""
    gdal.UseExceptions()

    if hasattr(ogr, "UseExceptions"):
        ogr.UseExceptions()


def set_traditional_axis_order(spatial_ref):
    """兼容 GDAL 2/3 的坐标轴顺序。"""
    if spatial_ref is None:
        return

    if hasattr(spatial_ref, "SetAxisMappingStrategy") and \
            hasattr(osr, "OAMS_TRADITIONAL_GIS_ORDER"):
        spatial_ref.SetAxisMappingStrategy(
            osr.OAMS_TRADITIONAL_GIS_ORDER
        )


def geometry_is_valid_quietly(geometry):
    """静默检查几何有效性，避免自相交环输出 Warning 1。"""
    if geometry is None or geometry.IsEmpty():
        return False

    pushed_handler = False

    try:
        gdal.PushErrorHandler("CPLQuietErrorHandler")
        pushed_handler = True
    except Exception:
        pass

    try:
        return bool(geometry.IsValid())
    except Exception:
        return False
    finally:
        if pushed_handler:
            try:
                gdal.PopErrorHandler()
            except Exception:
                pass


def repair_polygon_geometry(geometry):
    """
    使用 Buffer(0) 修复面几何自相交，兼容旧版 GDAL/OGR。

    不调用 Geometry.MakeValid()，避免旧版 Python 绑定报错：
    type object 'object' has no attribute '__getattr__'。
    """
    if geometry is None or geometry.IsEmpty():
        return None

    candidate = geometry.Clone()

    if geometry_is_valid_quietly(candidate):
        return candidate

    try:
        repaired = candidate.Buffer(0)
    except Exception:
        return None

    if repaired is None or repaired.IsEmpty():
        return None

    if not geometry_is_valid_quietly(repaired):
        return None

    return repaired


def ensure_file(path, description):
    if not os.path.isfile(path):
        raise IOError("{}不存在：{}".format(description, path))


def ensure_output_directory(output_path):
    """若输出目录不存在，则创建该目录。"""
    output_dir = os.path.dirname(os.path.abspath(output_path))

    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)


def ensure_different_paths(input_path, output_path):
    """防止覆盖模式下误删原始输入数据。"""
    if os.path.normcase(os.path.abspath(input_path)) == \
            os.path.normcase(os.path.abspath(output_path)):
        raise ValueError(
            "输入与输出路径不能相同：{}；请更换输出目录或修改输出文件名".format(
                input_path
            )
        )


def delete_existing_raster(path, overwrite):
    if not os.path.exists(path):
        return

    if not overwrite:
        raise IOError(
            "输出文件已经存在：{}；请将该步骤的 overwrite 设为 True".format(
                path
            )
        )

    driver = gdal.GetDriverByName("GTiff")
    result = driver.Delete(path)

    if result != 0 and os.path.exists(path):
        raise IOError("无法删除已有栅格：{}".format(path))


def delete_existing_shapefile(path, overwrite):
    if not os.path.exists(path):
        return

    if not overwrite:
        raise IOError(
            "输出文件已经存在：{}；请将该步骤的 overwrite 设为 True".format(
                path
            )
        )

    driver = ogr.GetDriverByName("ESRI Shapefile")
    result = driver.DeleteDataSource(path)

    if result != 0 and os.path.exists(path):
        raise IOError("无法删除已有矢量文件：{}".format(path))


def _find_multipart_subbasin_ids(shp_path):
    """
    读取子流域 SHP，一次性返回所有多面（MultiPolygon / GeometryCollection）
    要素对应的 SUBBASINID 集合。若 SHP 不存在或无法读取则返回空集合。
    """
    if not os.path.exists(shp_path):
        return set()

    shp_ds = ogr.Open(shp_path, gdal.GA_ReadOnly)
    if shp_ds is None:
        return set()

    layer = shp_ds.GetLayer(0)
    if layer is None:
        shp_ds = None
        return set()

    defn = layer.GetLayerDefn()
    subbasin_idx = defn.GetFieldIndex("SUBBASINID")

    multipart_ids = set()
    layer.ResetReading()
    for feature in layer:
        geom = feature.GetGeometryRef()
        if geom is None:
            feature = None
            continue
        geom_type = geom.GetGeometryType()
        # MultiPolygon(6) / GeometryCollection(7) 及其 25D / ZM 变体
        is_multi = (
            geom_type == ogr.wkbMultiPolygon
            or geom_type == ogr.wkbMultiPolygon25D
            or geom_type == ogr.wkbGeometryCollection
            or geom_type == ogr.wkbGeometryCollection25D
            or geom_type == ogr.wkbMultiPolygonZM
            or geom_type == ogr.wkbGeometryCollectionZM
        )
        if not is_multi:
            feature = None
            continue

        if subbasin_idx >= 0:
            val = feature.GetField("SUBBASINID")
            if val is not None:
                multipart_ids.add(int(val))
        feature = None

    shp_ds = None
    return multipart_ids


def _copy_raster(src_path, dst_path, overwrite):
    """用 GDAL 驱动复制栅格文件。"""
    if os.path.abspath(src_path) == os.path.abspath(dst_path):
        return
    delete_existing_raster(dst_path, overwrite)
    driver = gdal.GetDriverByName("GTiff")
    src_ds = gdal.Open(src_path, gdal.GA_ReadOnly)
    if src_ds is None:
        raise IOError("无法打开源栅格：{}".format(src_path))
    dst_ds = driver.CreateCopy(dst_path, src_ds, options=["TILED=YES", "COMPRESS=LZW"])
    if dst_ds is None:
        src_ds = None
        raise IOError("无法复制栅格到：{}".format(dst_path))
    dst_ds = None
    src_ds = None


def _copy_shapefile(src_path, dst_path, overwrite):
    """用 OGR 驱动复制 Shapefile（含 .shp/.dbf/.shprj 等）。"""
    if os.path.abspath(src_path) == os.path.abspath(dst_path):
        return
    delete_existing_shapefile(dst_path, overwrite)
    driver = ogr.GetDriverByName("ESRI Shapefile")
    src_ds = ogr.Open(src_path, gdal.GA_ReadOnly)
    if src_ds is None:
        raise IOError("无法打开源 Shapefile：{}".format(src_path))
    dst_ds = driver.CopyDataSource(src_ds, dst_path)
    if dst_ds is None:
        src_ds = None
        raise IOError("无法复制 Shapefile 到：{}".format(dst_path))
    dst_ds = None
    src_ds = None


def compute_pixel_area_sqm(geotransform, projection_wkt, n_rows, n_cols):
    """
    计算栅格的「平均像元面积」，单位：平方米（m²）。

    策略：
        1. 尝试从投影 WKT 中解析线性单位（米、英尺等）。
        2. 若是投影坐标系（线性单位为米），直接返回 |dx*dy|。
        3. 若是地理坐标系（单位为度），则用栅格中心附近的平均纬度
           做球面近似，换算为 m²：
               dx_m = dx_deg * (π/180) * R * cos(lat)
               dy_m = dy_deg * (π/180) * R
               area = dx_m * dy_m
    """
    dx = abs(geotransform[1])
    dy = abs(geotransform[5])
    raw_area = dx * dy

    if not projection_wkt:
        return raw_area, "unknown"

    srs = osr.SpatialReference(wkt=projection_wkt)
    if srs is None:
        return raw_area, "unknown"

    # 是否是地理坐标系（单位为度）
    is_geographic = srs.IsGeographic() == 1

    unit_name = ""
    unit_factor = 1.0  # 到 米 的换算系数
    if is_geographic:
        try:
            unit_name = srs.GetAttrValue("UNIT") or ""
        except Exception:
            unit_name = ""
        # 地理坐标的 ANGLEUNIT
        unit_name = unit_name.lower()
        # 绝大多数为度
        if "metre" in unit_name or "meter" in unit_name:
            is_geographic = False
            unit_factor = 1.0
        # 地理坐标系：用球面近似换算
    else:
        try:
            # 投影坐标系：GetLinearUnits 返回 (name, meters_per_unit)
            result = srs.GetLinearUnits()
            if result is not None:
                unit_name, unit_factor = result
        except Exception:
            unit_name = ""
            unit_factor = 1.0

    if is_geographic:
        # 球面半径（米），WGS84 半长轴 ≈ 6378137
        R = 6378137.0
        # 取栅格中心纬度
        x_center = geotransform[0] + geotransform[1] * (n_cols / 2.0)
        y_center = geotransform[3] + geotransform[5] * (n_rows / 2.0)
        lat_center = y_center  # 中心纬度
        lat_rad = math.radians(lat_center)
        dx_m = dx * math.pi / 180.0 * R * math.cos(lat_rad)
        dy_m = dy * math.pi / 180.0 * R
        return dx_m * dy_m, "degree→m²(approx)"

    return raw_area * (unit_factor * unit_factor), "meter"


def read_mask_union(mask_path):
    """读取流域面，并将多个面要素合并为一个裁剪几何。"""
    data_source = ogr.Open(mask_path, 0)

    if data_source is None:
        raise IOError("无法打开流域范围 Shapefile：{}".format(mask_path))

    layer = data_source.GetLayer(0)

    if layer is None:
        data_source = None
        raise RuntimeError("流域范围 Shapefile 中没有可用图层")

    mask_srs = layer.GetSpatialRef()

    if mask_srs is None:
        data_source = None
        raise RuntimeError("流域范围 Shapefile 缺少坐标系（.prj）")

    mask_srs = mask_srs.Clone()
    set_traditional_axis_order(mask_srs)

    merged = None
    layer.ResetReading()

    for feature in layer:
        geometry = feature.GetGeometryRef()

        if geometry is None or geometry.IsEmpty():
            continue

        geometry = geometry.Clone()

        # 修复无效几何（如自相交），防止 Union 产生无效结果
        if hasattr(geometry, "IsValid") and not geometry.IsValid():
            repaired = geometry.MakeValid()
            if repaired is not None and not repaired.IsEmpty():
                geometry = repaired

        if merged is None:
            merged = geometry
        else:
            merged = merged.Union(geometry)

    data_source = None

    if merged is None or merged.IsEmpty():
        raise RuntimeError("流域范围 Shapefile 中没有有效面几何")

    # 最终再做一次几何修复，确保裁剪时不会出现拓扑异常
    if hasattr(merged, "IsValid") and not merged.IsValid():
        repaired = merged.MakeValid()
        if repaired is None or repaired.IsEmpty():
            repaired = merged.Buffer(0)
        if repaired is not None and not repaired.IsEmpty():
            merged = repaired

    return merged, mask_srs


def spatial_ref_from_wkt(wkt, description):
    if not wkt:
        raise RuntimeError("{}缺少投影信息".format(description))

    spatial_ref = osr.SpatialReference()
    spatial_ref.ImportFromWkt(wkt)
    set_traditional_axis_order(spatial_ref)

    return spatial_ref


def transform_geometry(geometry, source_srs, target_srs):
    """将几何转换到目标坐标系；输入几何不会被修改。"""
    result = geometry.Clone()

    source = source_srs.Clone()
    target = target_srs.Clone()

    set_traditional_axis_order(source)
    set_traditional_axis_order(target)

    if source.IsSame(target):
        return result

    transformation = osr.CoordinateTransformation(source, target)
    transform_result = result.Transform(transformation)

    if transform_result != 0:
        raise RuntimeError("流域范围坐标转换失败")

    return result


def calculate_snapped_bounds(dataset, mask_geometry, mask_srs):
    """
    计算与输入栅格原始像元网格严格对齐的最小裁剪外包范围。

    返回：
        (xmin, ymin, xmax, ymax)
    """
    geotransform = dataset.GetGeoTransform()

    if geotransform is None:
        raise RuntimeError("输入栅格缺少地理变换参数")

    if abs(geotransform[2]) > 1.0e-12 or \
            abs(geotransform[4]) > 1.0e-12:
        raise RuntimeError("当前脚本不支持带旋转参数的栅格")

    pixel_width = geotransform[1]
    pixel_height = abs(geotransform[5])

    if pixel_width <= 0 or pixel_height <= 0:
        raise RuntimeError("输入栅格像元大小无效")

    raster_srs = spatial_ref_from_wkt(
        dataset.GetProjection(), "输入栅格"
    )

    mask_in_raster_srs = transform_geometry(
        mask_geometry, mask_srs, raster_srs
    )

    min_x, max_x, min_y, max_y = mask_in_raster_srs.GetEnvelope()

    origin_x = geotransform[0]
    origin_y = geotransform[3]

    col_start = int(math.floor((min_x - origin_x) / pixel_width))
    col_end = int(math.ceil((max_x - origin_x) / pixel_width))
    row_start = int(math.floor((origin_y - max_y) / pixel_height))
    row_end = int(math.ceil((origin_y - min_y) / pixel_height))

    col_start = max(0, col_start)
    row_start = max(0, row_start)
    col_end = min(dataset.RasterXSize, col_end)
    row_end = min(dataset.RasterYSize, row_end)

    if col_start >= col_end or row_start >= row_end:
        raise RuntimeError("流域范围与输入栅格没有空间重叠")

    xmin = origin_x + col_start * pixel_width
    xmax = origin_x + col_end * pixel_width
    ymax = origin_y - row_start * pixel_height
    ymin = origin_y - row_end * pixel_height

    return xmin, ymin, xmax, ymax


def choose_nodata_and_output_type(band):
    """
    优先沿用原始 NoData 和数据类型。

    如果输入没有 NoData，则选择一个可表达负数的输出类型，避免把
    Final_fdr 的合法值 255 错当成 NoData。
    """
    source_nodata = band.GetNoDataValue()
    source_type = band.DataType

    if source_nodata is not None:
        return source_nodata, source_type, True

    if source_type == gdal.GDT_Byte:
        return -9999, gdal.GDT_Int16, False

    if source_type == gdal.GDT_UInt16:
        return -9999, gdal.GDT_Int32, False

    if source_type == gdal.GDT_UInt32:
        return -9999.0, gdal.GDT_Float64, False

    if source_type == gdal.GDT_Int16:
        return -32768, source_type, False

    if source_type == gdal.GDT_Int32:
        return -2147483648, source_type, False

    if source_type in (gdal.GDT_Float32, gdal.GDT_Float64):
        return -9999.0, source_type, False

    return -9999.0, gdal.GDT_Float64, False


def clip_raster(input_path, output_path, mask_path, mask_geometry,
                mask_srs, overwrite):
    """按流域面裁剪单波段栅格。"""
    ensure_file(input_path, "输入栅格")
    delete_existing_raster(output_path, overwrite)

    source = gdal.Open(input_path, gdal.GA_ReadOnly)

    if source is None:
        raise IOError("无法打开输入栅格：{}".format(input_path))

    if source.RasterCount != 1:
        raster_count = source.RasterCount
        source = None

        raise RuntimeError(
            "脚本当前按单波段水文栅格设计，文件包含{}个波段：{}".format(
                raster_count, input_path
            )
        )

    bounds = calculate_snapped_bounds(source, mask_geometry, mask_srs)

    geotransform = source.GetGeoTransform()
    x_resolution = geotransform[1]
    y_resolution = abs(geotransform[5])

    band = source.GetRasterBand(1)
    nodata, output_type, has_source_nodata = \
        choose_nodata_and_output_type(band)

    options_args = {
        "format": "GTiff",
        "outputBounds": bounds,
        "xRes": x_resolution,
        "yRes": y_resolution,
        "outputType": output_type,
        "dstNodata": nodata,
        "resampleAlg": gdal.GRA_NearestNeighbour,
        "cutlineDSName": mask_path,
        "cropToCutline": False,
        "multithread": True,
        "errorThreshold": 0.0,
        "creationOptions": [
            "TILED=YES",
            "COMPRESS=LZW",
            "BIGTIFF=IF_SAFER",
        ],
        "warpOptions": [
            "NUM_THREADS=ALL_CPUS",
            "CUTLINE_ALL_TOUCHED=FALSE",
        ],
    }

    if has_source_nodata:
        options_args["srcNodata"] = nodata

    warp_options = gdal.WarpOptions(**options_args)

    result = gdal.Warp(
        output_path,
        source,
        options=warp_options
    )

    source = None

    if result is None:
        raise RuntimeError("栅格裁剪失败：{}".format(input_path))

    result.FlushCache()
    result = None

    check = gdal.Open(output_path, gdal.GA_ReadOnly)

    if check is None or check.RasterXSize <= 0 or check.RasterYSize <= 0:
        check = None
        raise RuntimeError("输出栅格验证失败：{}".format(output_path))

    info = {
        "cols": check.RasterXSize,
        "rows": check.RasterYSize,
        "nodata": check.GetRasterBand(1).GetNoDataValue(),
        "type": gdal.GetDataTypeName(check.GetRasterBand(1).DataType),
    }

    check = None

    return info


def append_line_parts(geometry, multi_line):
    """递归提取相交结果中的线要素，忽略仅接触边界产生的点。"""
    if geometry is None or geometry.IsEmpty():
        return

    geometry_name = geometry.GetGeometryName()

    if geometry_name is None:
        geometry_name = ""

    geometry_name = geometry_name.upper()

    is_single_line = (
        ("LINESTRING" in geometry_name and
         "MULTILINESTRING" not in geometry_name) or
        "LINEARRING" in geometry_name
    )

    if is_single_line:
        if geometry.GetPointCount() >= 2 and geometry.Length() > 0:
            multi_line.AddGeometry(geometry.Clone())

        return

    if geometry.GetGeometryCount() > 0:
        for index in range(geometry.GetGeometryCount()):
            append_line_parts(
                geometry.GetGeometryRef(index),
                multi_line
            )


def clip_stream_vector(input_path, output_path, mask_geometry,
                       mask_srs, overwrite):
    """按流域面真实裁剪河网线，并保留原属性字段。"""
    ensure_file(input_path, "输入河网 Shapefile")
    delete_existing_shapefile(output_path, overwrite)

    input_ds = ogr.Open(input_path, 0)

    if input_ds is None:
        raise IOError("无法打开河网 Shapefile：{}".format(input_path))

    input_layer = input_ds.GetLayer(0)

    if input_layer is None:
        input_ds = None
        raise RuntimeError("河网 Shapefile 中没有可用图层")

    stream_srs = input_layer.GetSpatialRef()

    if stream_srs is None:
        input_ds = None
        raise RuntimeError("河网 Shapefile 缺少坐标系（.prj）")

    stream_srs = stream_srs.Clone()
    set_traditional_axis_order(stream_srs)

    mask_in_stream_srs = transform_geometry(
        mask_geometry, mask_srs, stream_srs
    )

    driver = ogr.GetDriverByName("ESRI Shapefile")
    output_ds = driver.CreateDataSource(output_path)

    if output_ds is None:
        input_ds = None
        raise IOError("无法创建输出河网 Shapefile：{}".format(output_path))

    layer_name = os.path.splitext(os.path.basename(output_path))[0]

    output_layer = output_ds.CreateLayer(
        layer_name,
        srs=stream_srs,
        geom_type=ogr.wkbMultiLineString,
        options=["ENCODING=UTF-8"]
    )

    if output_layer is None:
        output_ds = None
        input_ds = None
        raise RuntimeError("无法创建输出河网图层")

    input_definition = input_layer.GetLayerDefn()

    for field_index in range(input_definition.GetFieldCount()):
        field_definition = input_definition.GetFieldDefn(field_index)

        if output_layer.CreateField(field_definition) != 0:
            output_ds = None
            input_ds = None

            raise RuntimeError(
                "无法复制属性字段：{}".format(field_definition.GetName())
            )

    output_definition = output_layer.GetLayerDefn()

    input_layer.SetSpatialFilter(mask_in_stream_srs)
    input_layer.ResetReading()

    output_count = 0

    for input_feature in input_layer:
        source_geometry = input_feature.GetGeometryRef()

        if source_geometry is None or source_geometry.IsEmpty():
            continue

        clipped_geometry = source_geometry.Intersection(mask_in_stream_srs)

        if clipped_geometry is None or clipped_geometry.IsEmpty():
            continue

        output_geometry = ogr.Geometry(ogr.wkbMultiLineString)
        append_line_parts(clipped_geometry, output_geometry)

        if output_geometry.IsEmpty():
            continue

        output_feature = ogr.Feature(output_definition)

        for field_index in range(output_definition.GetFieldCount()):
            field_value = input_feature.GetField(field_index)

            if field_value is not None:
                output_feature.SetField(field_index, field_value)

        output_feature.SetGeometry(output_geometry)

        if output_layer.CreateFeature(output_feature) != 0:
            output_feature = None
            output_ds = None
            input_ds = None
            raise RuntimeError("写入裁剪后的河网要素失败")

        output_feature = None
        output_count += 1

    input_layer.SetSpatialFilter(None)
    output_layer.SyncToDisk()

    output_ds = None
    input_ds = None

    if not os.path.isfile(output_path):
        raise RuntimeError("输出河网验证失败：{}".format(output_path))

    return output_count


def calculate_d8_flow_accumulation(fdr_path, output_path,
                                   overwrite=True,
                                   include_self=True):
    """
    根据单方向 ESRI D8 FDR 栅格计算汇流累积量。

    FDR 编码：
        1=东，2=东南，4=南，8=西南，
        16=西，32=西北，64=北，128=东北。
    """
    ensure_file(fdr_path, "单方向 FDR 栅格")
    ensure_different_paths(fdr_path, output_path)
    ensure_output_directory(output_path)
    delete_existing_raster(output_path, overwrite)

    fdr_ds = gdal.Open(fdr_path, gdal.GA_ReadOnly)

    if fdr_ds is None:
        raise IOError("无法打开 FDR 栅格：{}".format(fdr_path))

    if fdr_ds.RasterCount != 1:
        fdr_ds = None
        raise RuntimeError("FDR 必须是单波段栅格")

    fdr_band = fdr_ds.GetRasterBand(1)
    fdr_data = fdr_band.ReadAsArray()

    if fdr_data is None:
        fdr_ds = None
        raise RuntimeError("无法读取 FDR 栅格")

    n_rows = fdr_ds.RasterYSize
    n_cols = fdr_ds.RasterXSize
    total_cells = n_rows * n_cols
    fdr_nodata = fdr_band.GetNoDataValue()

    d8_directions = {
        1: (0, 1),
        2: (1, 1),
        4: (1, 0),
        8: (1, -1),
        16: (0, -1),
        32: (-1, -1),
        64: (-1, 0),
        128: (-1, 1),
    }

    fdr_flat = fdr_data.reshape(-1)

    if fdr_nodata is None:
        valid_mask = np.ones(total_cells, dtype=np.bool_)
    elif np.isnan(fdr_nodata):
        valid_mask = ~np.isnan(fdr_flat)
    else:
        valid_mask = fdr_flat != fdr_nodata

    if np.issubdtype(fdr_data.dtype, np.floating):
        valid_mask = valid_mask & np.isfinite(fdr_flat)

    valid_indices = np.flatnonzero(valid_mask)
    valid_cell_count = len(valid_indices)

    if valid_cell_count == 0:
        fdr_ds = None
        raise RuntimeError("FDR 栅格中没有有效格点")

    if np.issubdtype(fdr_data.dtype, np.floating):
        valid_values = fdr_flat[valid_indices]

        if np.any(np.abs(valid_values - np.rint(valid_values)) > 1.0e-6):
            fdr_ds = None
            raise RuntimeError("FDR 中存在非整数方向编码")

    unique_codes = np.unique(
        np.rint(fdr_flat[valid_indices]).astype(np.int32)
    )

    invalid_codes = []

    for code in unique_codes:
        code = int(code)

        if code != 0 and code not in d8_directions:
            invalid_codes.append(code)

    if len(invalid_codes) > 0:
        fdr_ds = None

        raise RuntimeError(
            "FDR 中存在不符合单方向 ESRI D8 编码的值：{}。"
            "单方向 FDR 应仅包含 0、1、2、4、8、16、32、64、128。".format(
                invalid_codes
            )
        )

    downstream = np.full(total_cells, -1, dtype=np.int32)
    in_degree = np.zeros(total_cells, dtype=np.int32)

    for flat_index in valid_indices:
        flat_index = int(flat_index)

        row = flat_index // n_cols
        col = flat_index % n_cols
        flow_code = int(round(fdr_flat[flat_index]))

        if flow_code == 0:
            continue

        delta_row, delta_col = d8_directions[flow_code]

        target_row = row + delta_row
        target_col = col + delta_col

        if target_row < 0 or target_row >= n_rows:
            continue

        if target_col < 0 or target_col >= n_cols:
            continue

        target_index = target_row * n_cols + target_col

        if not valid_mask[target_index]:
            continue

        downstream[flat_index] = target_index
        in_degree[target_index] += 1

    accumulation = np.zeros(total_cells, dtype=np.float64)
    accumulation[valid_indices] = 1.0

    queue = deque()

    for flat_index in valid_indices:
        flat_index = int(flat_index)

        if in_degree[flat_index] == 0:
            queue.append(flat_index)

    processed_count = 0

    while queue:
        current_index = queue.popleft()
        processed_count += 1

        target_index = downstream[current_index]

        if target_index < 0:
            continue

        accumulation[target_index] += accumulation[current_index]
        in_degree[target_index] -= 1

        if in_degree[target_index] == 0:
            queue.append(int(target_index))

    if processed_count != valid_cell_count:
        remaining_count = valid_cell_count - processed_count
        fdr_ds = None

        raise RuntimeError(
            "FDR 中存在环路或不合理的流向关系，仍有 {} 个格点未完成累积计算。".format(
                remaining_count
            )
        )

    if include_self:
        output_values = accumulation
    else:
        output_values = np.maximum(accumulation - 1.0, 0.0)

    output_nodata = -9999.0

    output_array = np.full(
        total_cells,
        output_nodata,
        dtype=np.float64
    )

    output_array[valid_indices] = output_values[valid_indices]
    output_array = output_array.reshape((n_rows, n_cols))

    geotransform = fdr_ds.GetGeoTransform()
    projection = fdr_ds.GetProjection()

    driver = gdal.GetDriverByName("GTiff")

    output_ds = driver.Create(
        output_path,
        n_cols,
        n_rows,
        1,
        gdal.GDT_Float64,
        options=[
            "TILED=YES",
            "COMPRESS=LZW",
            "BIGTIFF=IF_SAFER",
        ]
    )

    if output_ds is None:
        fdr_ds = None
        raise RuntimeError("无法创建汇流累积量栅格：{}".format(output_path))

    output_ds.SetGeoTransform(geotransform)
    output_ds.SetProjection(projection)

    output_band = output_ds.GetRasterBand(1)
    output_band.SetNoDataValue(output_nodata)
    output_band.WriteArray(output_array)
    output_band.FlushCache()

    output_ds.FlushCache()

    output_ds = None
    fdr_ds = None

    return {
        "valid_cell_count": valid_cell_count,
        "max_accumulation": float(
            np.max(output_values[valid_indices])
        ),
    }


def append_polygon_parts(geometry, multi_polygon):
    """递归提取面几何，并写入 MultiPolygon。"""
    if geometry is None or geometry.IsEmpty():
        return

    # 不使用 ogr.wkbFlatten，兼容旧版 GDAL/OGR。
    geometry_name = geometry.GetGeometryName()

    if geometry_name is None:
        geometry_name = ""

    geometry_name = geometry_name.upper()

    if geometry_name == "POLYGON":
        multi_polygon.AddGeometry(geometry.Clone())
        return

    if geometry.GetGeometryCount() > 0:
        for index in range(geometry.GetGeometryCount()):
            append_polygon_parts(
                geometry.GetGeometryRef(index),
                multi_polygon
            )


def raster_to_dissolved_subbasin_shp(input_path, output_path, overwrite):
    """
    将 subbasin.tif 转为 subbasin.shp。

    NoData 不参与转换。
    同一个栅格值只输出一个要素；离散区域作为同一个 MultiPolygon
    要素的多个 part 保存，不对它们执行 Union。
    """
    ensure_file(input_path, "子流域栅格")
    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)
    delete_existing_shapefile(output_path, overwrite)

    input_ds = gdal.Open(input_path, gdal.GA_ReadOnly)

    if input_ds is None:
        raise IOError("无法打开子流域栅格：{}".format(input_path))

    if input_ds.RasterCount != 1:
        input_ds = None
        raise RuntimeError("子流域栅格必须是单波段：{}".format(input_path))

    input_band = input_ds.GetRasterBand(1)
    projection_wkt = input_ds.GetProjection()
    output_srs = None

    if projection_wkt:
        output_srs = spatial_ref_from_wkt(
            projection_wkt, "子流域栅格"
        )

    memory_driver = ogr.GetDriverByName("Memory")

    if memory_driver is None:
        input_ds = None
        raise RuntimeError("无法获取 Memory 矢量驱动")

    temp_ds = memory_driver.CreateDataSource("subbasin_polygonize")

    if temp_ds is None:
        input_ds = None
        raise RuntimeError("无法创建临时子流域面数据")

    temp_layer = temp_ds.CreateLayer(
        "subbasin_polygonize",
        srs=output_srs,
        geom_type=ogr.wkbPolygon
    )

    if temp_layer is None:
        temp_ds = None
        input_ds = None
        raise RuntimeError("无法创建临时子流域面图层")

    integer_field_type = getattr(ogr, "OFTInteger64", ogr.OFTInteger)

    if temp_layer.CreateField(
            ogr.FieldDefn("GRID_VALUE", integer_field_type)
    ) != 0:
        temp_ds = None
        input_ds = None
        raise RuntimeError("无法创建临时栅格值字段")

    # 自动排除 NoData；0 若不是 NoData，仍是合法子流域 ID。
    result = gdal.Polygonize(
        input_band,
        input_band.GetMaskBand(),
        temp_layer,
        0,
        []
    )

    if result != 0:
        temp_ds = None
        input_ds = None
        raise RuntimeError("子流域栅格矢量化失败：{}".format(input_path))

    # 关键修复：
    # Polygonize 已经自动合并边相连的同值像元。
    # 对仅角点相接的面再做 Union，可能产生 Ring Self-intersection。
    # 因此只按 ID 收集面，最后写成同一个 MultiPolygon 要素。
    geometry_parts_by_id = {}

    temp_layer.ResetReading()

    for temp_feature in temp_layer:
        subbasin_id = temp_feature.GetField(0)
        geometry = temp_feature.GetGeometryRef()

        if subbasin_id is None or geometry is None or geometry.IsEmpty():
            continue

        subbasin_id = int(subbasin_id)

        if subbasin_id not in geometry_parts_by_id:
            geometry_parts_by_id[subbasin_id] = []

        geometry_parts_by_id[subbasin_id].append(geometry.Clone())

    temp_ds = None

    if len(geometry_parts_by_id) == 0:
        input_ds = None
        raise RuntimeError("子流域栅格中没有有效值")

    driver = ogr.GetDriverByName("ESRI Shapefile")
    output_ds = driver.CreateDataSource(output_path)

    if output_ds is None:
        input_ds = None
        raise IOError("无法创建子流域 Shapefile：{}".format(output_path))

    layer_name = os.path.splitext(os.path.basename(output_path))[0]

    output_layer = output_ds.CreateLayer(
        layer_name,
        srs=output_srs,
        geom_type=ogr.wkbMultiPolygon,
        options=["ENCODING=UTF-8"]
    )

    if output_layer is None:
        output_ds = None
        input_ds = None
        raise RuntimeError("无法创建子流域输出图层")

    if output_layer.CreateField(
            ogr.FieldDefn("SUBBASINID", integer_field_type)
    ) != 0:
        output_ds = None
        input_ds = None
        raise RuntimeError("无法创建 SUBBASINID 字段")

    output_definition = output_layer.GetLayerDefn()
    output_count = 0

    for subbasin_id in sorted(geometry_parts_by_id):
        multi_polygon = ogr.Geometry(ogr.wkbMultiPolygon)

        for geometry in geometry_parts_by_id[subbasin_id]:
            append_polygon_parts(geometry, multi_polygon)

        if multi_polygon.IsEmpty():
            output_ds = None
            input_ds = None
            raise RuntimeError(
                "子流域 ID {} 没有可写出的面几何".format(subbasin_id)
            )

        output_feature = ogr.Feature(output_definition)
        output_feature.SetField("SUBBASINID", subbasin_id)
        output_feature.SetGeometry(multi_polygon)

        if output_layer.CreateFeature(output_feature) != 0:
            output_feature = None
            output_ds = None
            input_ds = None
            raise RuntimeError(
                "无法写入子流域 ID {}".format(subbasin_id)
            )

        output_feature = None
        output_count += 1

    output_layer.SyncToDisk()
    output_ds = None
    input_ds = None

    if not os.path.isfile(output_path):
        raise RuntimeError(
            "子流域 Shapefile 输出验证失败：{}".format(output_path)
        )

    return output_count


def generate_lake_subbasin(watershed_path, hydrolake_path,
                           output_tif_path, output_shp_path,
                           overwrite=True):
    """
    根据 Watershed.tif 和 Taihu_lakes.tif (hydrolake) 生成湖泊子流域。

    步骤：
        1. 将 Watershed.tif 中值为 -1 的湖泊栅格与 hydrolake 湖泊栅格
           在空间上一一匹配（重叠即匹配）。
        2. 用 hydrolake 的湖泊 ID（栅格值）查找 Watershed.tif 中
           值相同的坡面栅格。
        3. 将坡面栅格与对应的湖泊栅格合并，形成湖泊子流域，
           子流域 ID 使用湖泊 ID。

    输出：
        lake_subbasin.tif  —— 湖泊子流域栅格（值 = 湖泊 ID）
        lake_subbasin.shp  —— 湖泊子流域矢量面（含 SUBBASINID 字段）
    """
    ensure_file(watershed_path, "坡面-湖泊栅格 (Watershed.tif)")
    ensure_file(hydrolake_path, "湖泊 ID 栅格 (hydrolake)")
    ensure_different_paths(watershed_path, output_tif_path)
    ensure_output_directory(output_tif_path)
    delete_existing_raster(output_tif_path, overwrite)
    delete_existing_shapefile(output_shp_path, overwrite)

    # ---------- 读取两个栅格 ----------
    ws_ds = gdal.Open(watershed_path, gdal.GA_ReadOnly)
    if ws_ds is None:
        raise IOError("无法打开 Watershed.tif：{}".format(watershed_path))

    hl_ds = gdal.Open(hydrolake_path, gdal.GA_ReadOnly)
    if hl_ds is None:
        ws_ds = None
        raise IOError("无法打开 hydrolake tif：{}".format(hydrolake_path))

    if ws_ds.RasterCount != 1 or hl_ds.RasterCount != 1:
        ws_ds = None
        hl_ds = None
        raise RuntimeError("Watershed.tif 和 hydrolake tif 必须是单波段栅格")

    ws_band = ws_ds.GetRasterBand(1)
    hl_band = hl_ds.GetRasterBand(1)

    ws_data = ws_band.ReadAsArray()
    hl_data = hl_band.ReadAsArray()

    if ws_data is None or hl_data is None:
        ws_ds = None
        hl_ds = None
        raise RuntimeError("无法读取栅格数据")

    n_rows, n_cols = ws_data.shape
    if hl_data.shape != (n_rows, n_cols):
        ws_ds = None
        hl_ds = None
        raise RuntimeError(
            "两个栅格尺寸不一致：Watershed=({},{}) hydrolake=({},{})".format(
                n_rows, n_cols, hl_data.shape[0], hl_data.shape[1]
            )
        )

    ws_geotransform = ws_ds.GetGeoTransform()
    hl_geotransform = hl_ds.GetGeoTransform()
    if ws_geotransform != hl_geotransform:
        ws_ds = None
        hl_ds = None
        raise RuntimeError("两个栅格的地理变换参数不一致，请先对齐")

    ws_projection = ws_ds.GetProjection()
    hl_projection = hl_ds.GetProjection()
    if ws_projection != hl_projection:
        ws_ds = None
        hl_ds = None
        raise RuntimeError("两个栅格的投影不一致，请先统一投影")

    ws_nodata = ws_band.GetNoDataValue()
    hl_nodata = hl_band.GetNoDataValue()

    # ---------- 准备输出 ----------
    # 以 Watershed 为基准，拷贝一份；湖泊子流域将在其上修改
    if np.issubdtype(ws_data.dtype, np.floating):
        output_data = ws_data.copy()
    else:
        output_data = ws_data.astype(np.int32)

    ws_lake_mask = (ws_data == -1)

    # 处理 Watershed 的 NoData
    if ws_nodata is not None:
        if np.isnan(ws_nodata):
            ws_valid = ~np.isnan(ws_data)
        else:
            ws_valid = (ws_data != ws_nodata)
    else:
        ws_valid = np.ones((n_rows, n_cols), dtype=np.bool_)

    # 处理 hydrolake 的 NoData
    if hl_nodata is not None:
        if np.isnan(hl_nodata):
            hl_valid = ~np.isnan(hl_data)
        else:
            hl_valid = (hl_data != hl_nodata)
    else:
        hl_valid = np.ones((n_rows, n_cols), dtype=np.bool_)

    # ---------- 空间匹配湖泊（向量化，避免逐 ID 遍历全图）----------
    # 策略：
    #   1. overlap = ws_lake_mask & hl_valid  → Watershed -1 像素中有 hydrolake 值的
    #   2. output_data[overlap] = -hl_data[overlap]  → 直接用 hydrolake 值赋 -lake_id
    #   3. matched_lake_ids = unique(hl_data[overlap])  → 匹配到的湖泊 ID
    #   4. 坡面像素 (ws=-lake_id) 已在 output_data 中（copy 自 ws_data），无需再赋值
    overlap = ws_lake_mask & hl_valid
    if not np.any(overlap):
        ws_ds = None
        hl_ds = None
        raise RuntimeError("没有匹配到任何湖泊子流域")

    output_data[overlap] = (-hl_data[overlap].astype(output_data.dtype))
    matched_lake_ids = np.unique(hl_data[overlap].astype(np.int64)).tolist()
    matched_lake_id_set = set(int(x) for x in matched_lake_ids)

    print("        匹配到 {} 个湖泊子流域".format(len(matched_lake_ids)))

    # ---------- 向量化区域生长：将剩余 -1 湖泊栅格合并到相邻子流域 ----------
    # 用 ndimage.label 一次性找到 (unfilled | filled) 的连通分量，
    # 再用 ndimage.maximum 将每个分量的 -lake_id 传播到该分量中的 -1 像素。
    try:
        from scipy import ndimage
    except ImportError:
        ndimage = None

    unfilled = (output_data == -1) & ws_valid
    filled = (output_data < 0) & (output_data != -1) & ws_valid

    if ndimage is not None and np.any(unfilled) and np.any(filled):
        structure = np.ones((3, 3), dtype=np.uint8)
        combined = unfilled | filled
        labeled, n_components = ndimage.label(combined, structure=structure)

        # 每个分量的 -lake_id 值（用 maximum 获取；0 表示该分量无 filled 像素）
        filled_values = np.where(filled, output_data, 0)
        component_vals = ndimage.maximum(
            filled_values, labels=labeled, index=range(1, n_components + 1)
        )
        label_to_val = np.zeros(n_components + 1, dtype=output_data.dtype)
        label_to_val[1:] = component_vals

        # 将分量值赋给未填充的 -1 像素
        unfilled_labels = labeled[unfilled]
        unfilled_vals = label_to_val[unfilled_labels]
        has_val = unfilled_vals != 0
        uf_rows, uf_cols = np.where(unfilled)
        output_data[uf_rows[has_val], uf_cols[has_val]] = unfilled_vals[has_val]
    elif np.any(unfilled):
        print("        警告：scipy 不可用，跳过区域生长；部分 -1 湖泊栅格可能未合并")

    # ---------- 临时编号：从原始最大子流域 ID + 1 开始 ----------
    positive_mask = (ws_data > 0) & ws_valid
    if np.any(positive_mask):
        max_existing_id = int(np.max(ws_data[positive_mask]))
    else:
        max_existing_id = 0

    temp_id_map = {}
    sorted_lake_ids = sorted(matched_lake_id_set)
    for i, lake_id in enumerate(sorted_lake_ids):
        temp_id = max_existing_id + i + 1
        temp_id_map[lake_id] = temp_id

    # ---------- 用查找表一次性完成重映射 + NoData 设置 ----------
    # lookup[old_val - min_val] = new_val
    #   - 正值 → 保持不变
    #   - 匹配的 -lake_id → temp_id
    #   - 未匹配的负值 / 残留 -1 → NoData(-9999)
    all_valid_vals = output_data[ws_valid]
    min_val = int(all_valid_vals.min())
    max_val = int(all_valid_vals.max())
    lookup_size = max_val - min_val + 1
    lookup = np.arange(lookup_size, dtype=np.int64) + min_val

    out_nodata = -9999
    for lake_id, temp_id in temp_id_map.items():
        neg_id = -lake_id
        if min_val <= neg_id <= max_val:
            lookup[neg_id - min_val] = temp_id

    # 所有剩余负值（未匹配的 -lake_id、残留 -1）→ NoData
    neg_vals = np.unique(all_valid_vals[all_valid_vals < 0])
    for v in neg_vals:
        v_int = int(v)
        if v_int not in (-int(lid) for lid in matched_lake_id_set):
            if min_val <= v_int <= max_val:
                lookup[v_int - min_val] = out_nodata

    # 应用查找表（仅对有效像素）
    in_range = ws_valid & (output_data >= min_val) & (output_data <= max_val)
    indices = (output_data[in_range] - min_val).astype(np.int64)
    output_data[in_range] = lookup[indices].astype(output_data.dtype)

    # 统计被设为 NoData 的残留栅格数
    nodata_count = int(np.sum((output_data == out_nodata) & ws_valid))
    if nodata_count > 0:
        print("        警告：发现 {} 个残留栅格已设为 NoData".format(nodata_count))

    # ---------- 写出 TIF ----------
    driver = gdal.GetDriverByName("GTiff")
    out_ds = driver.Create(
        output_tif_path,
        n_cols,
        n_rows,
        1,
        gdal.GDT_Int32,
        options=[
            "TILED=YES",
            "COMPRESS=LZW",
            "BIGTIFF=IF_SAFER",
        ]
    )

    if out_ds is None:
        ws_ds = None
        hl_ds = None
        raise IOError("无法创建输出栅格：{}".format(output_tif_path))

    out_ds.SetGeoTransform(ws_geotransform)
    out_ds.SetProjection(ws_projection)

    # 将未匹配的湖泊区域及原 Watershed NoData 设为输出 NoData
    output_nodata = -9999
    output_band = out_ds.GetRasterBand(1)
    output_band.SetNoDataValue(output_nodata)
    output_data = output_data.astype(np.int32)

    # 同时把原 Watershed 中的 NoData 也设为输出 NoData
    output_data[~ws_valid] = output_nodata

    output_band.WriteArray(output_data)
    output_band.FlushCache()
    out_ds.FlushCache()
    out_ds = None

    ws_ds = None
    hl_ds = None

    # ---------- 转为 SHP ----------
    print("        生成湖泊子流域 TIF：{}".format(output_tif_path))

    feature_count = raster_to_dissolved_subbasin_shp(
        output_tif_path,
        output_shp_path,
        overwrite
    )

    print(
        "        输出 SHP：{}（{} 个湖泊子流域面要素）".format(
            output_shp_path,
            feature_count
        )
    )

    temp_ids = [temp_id_map[lid] for lid in sorted_lake_ids]
    return {
        "matched_lake_count": len(matched_lake_ids),
        "max_existing_id": max_existing_id,
        "temp_ids": temp_ids,
        "temp_id_map": temp_id_map,
    }


def split_disconnected_subbasins(tif_path, shp_path, overwrite=True,
                                output_tif_path=None, output_shp_path=None):
    """
    将 TIF 中每个子流域 ID 对应的离散（不连通）栅格区域拆分为独立的子流域，
    各自分配唯一的新 ID。拆分后重新生成 SHP。

    优化策略：
        1. 先读取 SHP，一次性识别哪些子流域是多面（MultiPolygon）要素。
        2. 仅对这些多面 ID 在其 bounding box 内做 ndimage.label，
           避免对所有 ID 遍历全图。
        3. 为多余连通区域分配新 ID。

    若 output_tif_path / output_shp_path 与输入不同，则保留输入文件不变，
    结果写入输出路径；否则就地覆盖。
    """
    if output_tif_path is None:
        output_tif_path = tif_path
    if output_shp_path is None:
        output_shp_path = shp_path
    try:
        from scipy import ndimage
    except ImportError:
        raise RuntimeError("split_disconnected_subbasins 需要 scipy，请先安装 scipy")

    ensure_file(tif_path, "子流域栅格")

    # ---------- 读取 TIF ----------
    ds = gdal.Open(tif_path, gdal.GA_ReadOnly)
    if ds is None:
        raise IOError("无法打开子流域栅格：{}".format(tif_path))

    if ds.RasterCount != 1:
        ds = None
        raise RuntimeError("子流域栅格必须是单波段：{}".format(tif_path))

    band = ds.GetRasterBand(1)
    data = band.ReadAsArray()
    if data is None:
        ds = None
        raise RuntimeError("无法读取子流域栅格数据")

    nodata = band.GetNoDataValue()
    geotransform = ds.GetGeoTransform()
    projection = ds.GetProjection()
    n_rows, n_cols = data.shape

    source_data = data.astype(np.int32)

    if nodata is not None:
        if np.isnan(nodata):
            valid_mask = ~np.isnan(data)
        else:
            valid_mask = (source_data != int(nodata))
    else:
        valid_mask = np.ones((n_rows, n_cols), dtype=np.bool_)

    unique_ids = np.unique(source_data[valid_mask])
    if len(unique_ids) == 0:
        ds = None
        raise RuntimeError("子流域栅格中没有有效子流域")

    max_id = int(unique_ids.max())
    next_id = max_id + 1
    split_count = 0

    # 8 连通结构元素
    structure = np.ones((3, 3), dtype=np.uint8)

    output_data = source_data.copy()

    # ---------- 一次性识别多面子流域 ----------
    multipart_ids = _find_multipart_subbasin_ids(shp_path)
    print("        SHP 中检测到 {} 个多面子流域，将逐一检查连通性".format(
        len(multipart_ids)))

    if len(multipart_ids) == 0:
        band = None
        ds = None
        print("        无离散子流域需要拆分")
        # 输入=输出时无需写入
        if output_tif_path != tif_path or output_shp_path != shp_path:
            _copy_raster(tif_path, output_tif_path, overwrite)
            _copy_shapefile(shp_path, output_shp_path, overwrite)
        return {"split_count": 0, "total_subbasins": int(len(unique_ids))}

    # ---------- 一次全图扫描：找到所有多面 ID 的像素坐标和 bounding box ----------
    multipart_list = sorted(int(x) for x in multipart_ids)
    multipart_mask = np.isin(source_data, multipart_list) & valid_mask

    if not np.any(multipart_mask):
        band = None
        ds = None
        print("        无离散子流域需要拆分")
        if output_tif_path != tif_path or output_shp_path != shp_path:
            _copy_raster(tif_path, output_tif_path, overwrite)
            _copy_shapefile(shp_path, output_shp_path, overwrite)
        return {"split_count": 0, "total_subbasins": int(len(unique_ids))}

    # 提取所有多面像素的坐标和 ID 值
    mp_rows, mp_cols = np.where(multipart_mask)
    mp_vals = source_data[mp_rows, mp_cols].astype(np.int64)

    # 按 ID 排序后用 reduceat 一次性计算每个 ID 的 bounding box
    sort_idx = np.argsort(mp_vals)
    sorted_vals = mp_vals[sort_idx]
    sorted_rows = mp_rows[sort_idx]
    sorted_cols = mp_cols[sort_idx]

    unique_vals, start_idx, counts = np.unique(
        sorted_vals, return_index=True, return_counts=True
    )
    r_mins = np.minimum.reduceat(sorted_rows, start_idx)
    r_maxs = np.maximum.reduceat(sorted_rows, start_idx)
    c_mins = np.minimum.reduceat(sorted_cols, start_idx)
    c_maxs = np.maximum.reduceat(sorted_cols, start_idx)

    # ---------- 仅对多面 ID 在 bounding box 内做连通标记 ----------
    for i in range(len(unique_vals)):
        sub_id_int = int(unique_vals[i])
        r_min, r_max = int(r_mins[i]), int(r_maxs[i])
        c_min, c_max = int(c_mins[i]), int(c_maxs[i])

        # 在 bounding box 内提取该 ID 的掩码并做连通标记
        sub_data = source_data[r_min:r_max + 1, c_min:c_max + 1]
        sub_mask = (sub_data == sub_id_int)
        labeled, n_components = ndimage.label(sub_mask, structure=structure)

        if n_components <= 1:
            continue

        # 第 1 个连通区域保留原 ID，第 2..N 个分配新 ID
        new_ids_for_this = [sub_id_int]
        sub_view = output_data[r_min:r_max + 1, c_min:c_max + 1]
        for comp in range(2, n_components + 1):
            comp_mask = (labeled == comp)
            sub_view[comp_mask] = next_id
            new_ids_for_this.append(next_id)
            next_id += 1
            split_count += 1

        print("        {}号子流域被拆分为：{}".format(
            sub_id_int, "和".join("{}号".format(x) for x in new_ids_for_this)))

    band = None
    ds = None

    if split_count == 0:
        print("        无离散子流域需要拆分")
        return {"split_count": 0, "total_subbasins": int(len(unique_ids))}

    # ---------- 重写 TIF ----------
    delete_existing_raster(output_tif_path, overwrite)

    driver = gdal.GetDriverByName("GTiff")
    out_ds = driver.Create(
        output_tif_path,
        n_cols,
        n_rows,
        1,
        gdal.GDT_Int32,
        options=[
            "TILED=YES",
            "COMPRESS=LZW",
            "BIGTIFF=IF_SAFER",
        ]
    )
    if out_ds is None:
        raise IOError("无法创建输出栅格：{}".format(output_tif_path))

    out_ds.SetGeoTransform(geotransform)
    out_ds.SetProjection(projection)

    out_band = out_ds.GetRasterBand(1)
    out_nodata = -9999
    out_band.SetNoDataValue(out_nodata)
    output_data[~valid_mask] = out_nodata
    out_band.WriteArray(output_data)
    out_band.FlushCache()
    out_ds.FlushCache()
    out_ds = None

    # ---------- 重新生成 SHP ----------
    feature_count = raster_to_dissolved_subbasin_shp(
        output_tif_path, output_shp_path, overwrite
    )

    total = int(len(unique_ids)) + split_count
    print("        拆分了 {} 个离散区域，当前共 {} 个子流域（{} 个面要素）".format(
        split_count, total, feature_count))

    return {"split_count": split_count, "total_subbasins": total}


def merge_small_subbasins(tif_path, shp_path, min_area, overwrite=True,
                          output_tif_path=None, output_shp_path=None):
    """
    将面积小于 min_area 的子流域合并到相邻的大子流域中。

    优化策略（每轮 O(N)，避免逐 ID 遍历全图）：
        1. 用 np.bincount 一次性计算所有子流域面积。
        2. 用 8 方向数组移位一次性提取所有相邻 ID 对。
        3. 按面积升序处理小面积子流域，用 remap 字典记录合并目标。
        4. 用查找表（lookup）一次性应用所有合并。

    参数：
        min_area — 面积阈值，单位与栅格坐标系一致（投影坐标系为平方米）。

    若 output_tif_path / output_shp_path 与输入不同，则保留输入文件不变，
    结果写入输出路径；否则就地覆盖。
    """
    if output_tif_path is None:
        output_tif_path = tif_path
    if output_shp_path is None:
        output_shp_path = shp_path
    ensure_file(tif_path, "子流域栅格")

    # ---------- 读取 TIF ----------
    ds = gdal.Open(tif_path, gdal.GA_ReadOnly)
    if ds is None:
        raise IOError("无法打开子流域栅格：{}".format(tif_path))

    if ds.RasterCount != 1:
        ds = None
        raise RuntimeError("子流域栅格必须是单波段：{}".format(tif_path))

    band = ds.GetRasterBand(1)
    data = band.ReadAsArray()
    if data is None:
        ds = None
        raise RuntimeError("无法读取子流域栅格数据")

    nodata = band.GetNoDataValue()
    geotransform = ds.GetGeoTransform()
    projection = ds.GetProjection()
    n_rows, n_cols = data.shape

    output_data = data.astype(np.int32)

    if nodata is not None:
        if np.isnan(nodata):
            valid_mask = ~np.isnan(data)
        else:
            valid_mask = (output_data != int(nodata))
    else:
        valid_mask = np.ones((n_rows, n_cols), dtype=np.bool_)

    band = None
    ds = None

    # 计算平均像元面积（m²），自动处理经纬度坐标系
    pixel_area, unit_mode = compute_pixel_area_sqm(
        geotransform, projection, n_rows, n_cols
    )
    print("        面积计算模式：{}，平均像元面积 ≈ {:.4e} m²".format(
        unit_mode, pixel_area))
    # 总有效面积用于诊断
    valid_cell_count = int(np.sum(valid_mask))
    total_sqm = valid_cell_count * pixel_area
    total_km2 = total_sqm / 1e6
    print("        总有效面积 ≈ {:.2f} km²，阈值 {} m² = {:.2f} km²".format(
        total_km2, min_area, min_area / 1e6))

    merged_count = 0

    while True:
        # ---------- 1. bincount 一次性计算所有子流域面积 ----------
        flat_valid = output_data[valid_mask]
        if len(flat_valid) == 0:
            break

        min_val = int(flat_valid.min())
        max_val = int(flat_valid.max())
        shifted = (flat_valid - min_val).astype(np.int64)
        counts = np.bincount(shifted)

        id_areas = {}
        for i in range(len(counts)):
            if counts[i] > 0:
                id_areas[i + min_val] = int(counts[i]) * pixel_area

        # ---------- 2. 筛选小面积子流域 ----------
        small_ids = sorted(
            [uid for uid, area in id_areas.items() if area < min_area],
            key=lambda x: id_areas[x]
        )
        if not small_ids:
            break

        # ---------- 3. 8 方向移位一次性提取所有相邻 ID 对 ----------
        neighbors = {}
        offset = max_val - min_val + 1  # 用于编码 ID 对

        for dr in (-1, 0, 1):
            for dc in (-1, 0, 1):
                if dr == 0 and dc == 0:
                    continue
                r1s, r1e = max(0, dr), min(n_rows, n_rows + dr)
                r2s, r2e = max(0, -dr), min(n_rows, n_rows - dr)
                c1s, c1e = max(0, dc), min(n_cols, n_cols + dc)
                c2s, c2e = max(0, -dc), min(n_cols, n_cols - dc)

                a = output_data[r1s:r1e, c1s:c1e]
                b = output_data[r2s:r2e, c2s:c2e]
                va = valid_mask[r1s:r1e, c1s:c1e]
                vb = valid_mask[r2s:r2e, c2s:c2e]

                diff = (a != b) & va & vb
                if not np.any(diff):
                    continue

                # 编码为 (lo, hi) 去重
                a_shifted = a[diff].astype(np.int64) - min_val
                b_shifted = b[diff].astype(np.int64) - min_val
                lo = np.minimum(a_shifted, b_shifted)
                hi = np.maximum(a_shifted, b_shifted)
                codes = lo * offset + hi
                unique_codes = np.unique(codes)

                for code in unique_codes.tolist():
                    v1 = code // offset + min_val
                    v2 = code % offset + min_val
                    neighbors.setdefault(v1, set()).add(v2)
                    neighbors.setdefault(v2, set()).add(v1)

        # ---------- 4. 按面积升序处理合并 ----------
        small_set = set(small_ids)
        remap = {}  # old_id -> target_id

        def resolve_chain(id_val):
            """沿 remap 链找到最终目标 ID。"""
            seen = set()
            while id_val in remap and id_val not in seen:
                seen.add(id_val)
                id_val = remap[id_val]
            return id_val

        for small_id in small_ids:
            if small_id in remap:
                continue

            nbrs = neighbors.get(small_id, set())
            if not nbrs:
                continue

            # 优先选择非小面积的邻居；若全是小面积，选面积最大的
            best_neighbor = None
            best_area = -1.0

            for nbr in nbrs:
                eff = resolve_chain(nbr)
                if eff == small_id:
                    continue
                # 跳过尚未合并的小面积邻居（优先找大邻居）
                if eff in small_set and eff not in remap:
                    continue
                eff_area = id_areas.get(eff, 0.0)
                if eff_area > best_area:
                    best_area = eff_area
                    best_neighbor = eff

            # 若没有非小面积邻居，退而选择最大的小面积邻居
            if best_neighbor is None:
                for nbr in nbrs:
                    eff = resolve_chain(nbr)
                    if eff == small_id or eff in remap:
                        continue
                    eff_area = id_areas.get(eff, 0.0)
                    if eff_area > best_area:
                        best_area = eff_area
                        best_neighbor = eff

            if best_neighbor is None:
                continue

            remap[small_id] = best_neighbor
            id_areas[best_neighbor] = id_areas.get(best_neighbor, 0.0) + id_areas[small_id]
            merged_count += 1
            print("        {}号子流域被合并到{}号子流域".format(
                small_id, best_neighbor))

        if not remap:
            break

        # ---------- 5. 查找表一次性应用所有合并 ----------
        lookup_min = min_val
        lookup_size = max_val - min_val + 1
        lookup = np.arange(lookup_size, dtype=np.int32) + lookup_min

        for old_id in remap:
            final = resolve_chain(old_id)
            lookup[old_id - lookup_min] = final

        in_range = valid_mask & (output_data >= lookup_min) & (output_data < lookup_min + lookup_size)
        output_data[in_range] = lookup[output_data[in_range] - lookup_min]

    if merged_count == 0:
        print("        无小面积子流域需要合并")
        remaining_ids = np.unique(output_data[valid_mask])
        return {"merged_count": 0, "total_subbasins": int(len(remaining_ids))}

    # ---------- 重写 TIF ----------
    delete_existing_raster(output_tif_path, overwrite)

    driver = gdal.GetDriverByName("GTiff")
    out_ds = driver.Create(
        output_tif_path,
        n_cols,
        n_rows,
        1,
        gdal.GDT_Int32,
        options=[
            "TILED=YES",
            "COMPRESS=LZW",
            "BIGTIFF=IF_SAFER",
        ]
    )
    if out_ds is None:
        raise IOError("无法创建输出栅格：{}".format(output_tif_path))

    out_ds.SetGeoTransform(geotransform)
    out_ds.SetProjection(projection)

    out_band = out_ds.GetRasterBand(1)
    out_nodata = -9999
    out_band.SetNoDataValue(out_nodata)
    output_data[~valid_mask] = out_nodata
    out_band.WriteArray(output_data)
    out_band.FlushCache()
    out_ds.FlushCache()
    out_ds = None

    # ---------- 重新生成 SHP ----------
    feature_count = raster_to_dissolved_subbasin_shp(
        output_tif_path, output_shp_path, overwrite
    )

    remaining_ids = np.unique(output_data[valid_mask])
    total = int(len(remaining_ids))
    print("        合并了 {} 个小面积子流域，剩余 {} 个子流域（{} 个面要素）".format(
        merged_count, total, feature_count))

    return {"merged_count": merged_count, "total_subbasins": total}


def renumber_subbasins(tif_path, shp_path, overwrite=True,
                       output_tif_path=None, output_shp_path=None):
    """
    统一重新编号所有子流域：读取子流域 TIF 中所有非 NoData 的唯一值，
    排序后映射为从 1 开始的连续正整数，同步更新 TIF 和 SHP 的 SUBBASINID。

    步骤：
        1. 读取 TIF，获取所有唯一子流域 ID（非 NoData）。
        2. 将所有 ID 排序并映射为 1, 2, 3, ...。
        3. 用新 ID 重写 TIF 栅格。
        4. 更新 SHP 中每个要素的 SUBBASINID 字段。

    若 output_tif_path / output_shp_path 与输入不同，则保留输入文件不变，
    结果写入输出路径（SHP 从输出 TIF 重新生成）；否则就地覆盖。
    """
    if output_tif_path is None:
        output_tif_path = tif_path
    if output_shp_path is None:
        output_shp_path = shp_path
    ensure_file(tif_path, "子流域栅格")
    ensure_file(shp_path, "子流域 Shapefile")
    # delete_existing_raster(tif_path, overwrite)

    # ---------- 读取 TIF ----------
    ds = gdal.Open(tif_path, gdal.GA_ReadOnly)
    if ds is None:
        raise IOError("无法打开子流域栅格：{}".format(tif_path))

    if ds.RasterCount != 1:
        ds = None
        raise RuntimeError("子流域栅格必须是单波段：{}".format(tif_path))

    band = ds.GetRasterBand(1)
    data = band.ReadAsArray()
    if data is None:
        ds = None
        raise RuntimeError("无法读取子流域栅格数据")

    nodata = band.GetNoDataValue()
    geotransform = ds.GetGeoTransform()
    projection = ds.GetProjection()
    n_rows, n_cols = data.shape
    ds = None

    # 收集所有非 NoData 的唯一值
    if nodata is not None:
        if np.isnan(nodata):
            valid_mask = ~np.isnan(data)
        else:
            valid_mask = (data != nodata)
    else:
        valid_mask = np.ones((n_rows, n_cols), dtype=np.bool_)

    unique_ids = np.unique(data[valid_mask])
    unique_ids = unique_ids.astype(np.int64)

    if len(unique_ids) == 0:
        raise RuntimeError("子流域栅格中没有有效值")

    # ---------- 构建映射：排序后映射为 1, 2, 3, ... ----------
    sorted_ids = sorted(unique_ids.tolist())
    id_mapping = {}  # 旧ID → 新ID
    for new_id, old_id in enumerate(sorted_ids, start=1):
        id_mapping[int(old_id)] = new_id

    # ---------- 用查找表一次性完成 ID 重映射（避免逐 ID 全图扫描）----------
    source_data = data.astype(np.int32)
    new_data = source_data.copy()

    min_val = int(source_data[valid_mask].min())
    max_val = int(source_data[valid_mask].max())
    lookup_size = max_val - min_val + 1
    lookup = np.arange(lookup_size, dtype=np.int64) + min_val

    for old_id, new_id in id_mapping.items():
        if min_val <= old_id <= max_val:
            lookup[old_id - min_val] = new_id

    in_range = valid_mask & (source_data >= min_val) & (source_data <= max_val)
    indices = (source_data[in_range] - min_val).astype(np.int64)
    new_data[in_range] = lookup[indices].astype(new_data.dtype)

    # 已完成读取和计算，释放输入文件句柄后再删除旧 tif
    band = None
    ds = None

    # 删除输出文件（若与输入相同则覆盖输入，否则只删除输出）
    delete_existing_raster(output_tif_path, overwrite)

    # 写回 TIF
    driver = gdal.GetDriverByName("GTiff")
    out_ds = driver.Create(
        output_tif_path,
        n_cols,
        n_rows,
        1,
        gdal.GDT_Int32,
        options=[
            "TILED=YES",
            "COMPRESS=LZW",
            "BIGTIFF=IF_SAFER",
        ]
    )
    if out_ds is None:
        raise IOError("无法创建输出栅格：{}".format(output_tif_path))

    out_ds.SetGeoTransform(geotransform)
    out_ds.SetProjection(projection)

    out_band = out_ds.GetRasterBand(1)
    out_nodata = -9999
    out_band.SetNoDataValue(out_nodata)
    new_data[~valid_mask] = out_nodata
    out_band.WriteArray(new_data)
    out_band.FlushCache()
    out_ds.FlushCache()
    out_ds = None

    print("        已重写 TIF：{}（{} 个子流域，ID 从 1 开始）".format(
        output_tif_path, len(sorted_ids)))

    # ---------- 更新 / 重新生成 SHP ----------
    if output_shp_path == shp_path:
        # 就地更新：直接修改 SUBBASINID 字段
        shp_ds = ogr.Open(shp_path, 1)  # 1 = 可写
        if shp_ds is None:
            raise IOError("无法打开子流域 Shapefile 进行更新：{}".format(shp_path))

        shp_layer = shp_ds.GetLayer(0)
        if shp_layer is None:
            shp_ds = None
            raise RuntimeError("子流域 Shapefile 中没有可用图层")

        # 检查是否有 SUBBASINID 字段
        defn = shp_layer.GetLayerDefn()
        subbasin_idx = defn.GetFieldIndex("SUBBASINID")
        if subbasin_idx < 0:
            shp_ds = None
            raise RuntimeError("子流域 Shapefile 缺少 SUBBASINID 字段")

        updated_count = 0
        shp_layer.ResetReading()
        for feature in shp_layer:
            old_val = feature.GetField("SUBBASINID")
            if old_val is not None:
                old_id = int(old_val)
                if old_id in id_mapping:
                    feature.SetField("SUBBASINID", id_mapping[old_id])
                    shp_layer.SetFeature(feature)
                    updated_count += 1
            feature = None

        shp_layer.SyncToDisk()
        shp_ds = None

        print("        已更新 SHP：{}（更新 {} 个要素）".format(
            shp_path, updated_count))
    else:
        # 输出路径不同于输入：从输出 TIF 重新生成 SHP
        feature_count = raster_to_dissolved_subbasin_shp(
            output_tif_path, output_shp_path, overwrite
        )
        print("        已重新生成 SHP：{}（{} 个面要素）".format(
            output_shp_path, feature_count))

    return {
        "total_subbasins": len(sorted_ids),
        "id_mapping": id_mapping,
    }


def update_stream_linkno_by_subbasin(stream_path, subbasin_path, overwrite=True, max_workers=1):
    """
    根据湖泊子流域面更新河道 Shapefile 的 LINKNO 字段。

    保留每条河道的原始几何，不再用子流域边界裁剪、打断或合并河道。
    对每条河道，计算其与各子流域的相交长度，并将 LINKNO 设置为相交长度
    最大的 SUBBASINID；若河道完全不与任何子流域相交，LINKNO 设为 0。

    多线程仅用于“单条河道—候选子流域”的相交长度计算；Shapefile 的读写
    保持单线程，避免旧版 GDAL/OGR 在并发访问数据源时出现不稳定。
    """
    ensure_file(stream_path, "河道 Shapefile")
    ensure_file(subbasin_path, "湖泊子流域 Shapefile")

    try:
        max_workers = max(1, int(max_workers))
    except (TypeError, ValueError):
        raise ValueError("max_workers 必须是大于等于 1 的整数")

    # ---------- 先将所有数据读入内存 ----------
    # 读取子流域面
    sb_ds = ogr.Open(subbasin_path, 0)
    if sb_ds is None:
        raise IOError("无法打开湖泊子流域 Shapefile：{}".format(subbasin_path))

    sb_layer = sb_ds.GetLayer(0)
    if sb_layer is None:
        sb_ds = None
        raise RuntimeError("湖泊子流域 Shapefile 中没有可用图层")

    sb_srs = sb_layer.GetSpatialRef()
    if sb_srs is None:
        sb_ds = None
        raise RuntimeError("湖泊子流域 Shapefile 缺少坐标系")

    sb_srs = sb_srs.Clone()
    set_traditional_axis_order(sb_srs)

    subbasins = []
    sb_layer.ResetReading()
    for sb_feature in sb_layer:
        sb_id = sb_feature.GetField("SUBBASINID")
        sb_geom = sb_feature.GetGeometryRef()
        if sb_id is None or sb_geom is None or sb_geom.IsEmpty():
            continue
        # 修复子流域面自相交，避免 Intersection 抛 TopologyException。
        # 这里不能调用 MakeValid()，以兼容旧版 GDAL Python 绑定。
        geom_clone = repair_polygon_geometry(sb_geom)
        if geom_clone is None:
            print(
                "        警告：子流域 {} 的面几何无法修复，已跳过".format(
                    int(sb_id)
                )
            )
            continue
        subbasins.append({
            "subbasin_id": int(sb_id),
            "geometry": geom_clone,
        })

    sb_ds = None

    if len(subbasins) == 0:
        raise RuntimeError("湖泊子流域 Shapefile 中没有有效面要素")

    # 读取河道
    st_ds = ogr.Open(stream_path, 0)
    if st_ds is None:
        raise IOError("无法打开河道 Shapefile：{}".format(stream_path))

    st_layer = st_ds.GetLayer(0)
    if st_layer is None:
        st_ds = None
        raise RuntimeError("河道 Shapefile 中没有可用图层")

    st_srs = st_layer.GetSpatialRef()
    if st_srs is None:
        st_ds = None
        raise RuntimeError("河道 Shapefile 缺少坐标系")

    st_srs = st_srs.Clone()
    set_traditional_axis_order(st_srs)

    # 将子流域面转换到河道坐标系（如不同）
    if not sb_srs.IsSame(st_srs):
        coord_trans = osr.CoordinateTransformation(sb_srs, st_srs)
        for sb in subbasins:
            transform_result = sb["geometry"].Transform(coord_trans)
            if transform_result not in (0, None):
                raise RuntimeError("子流域面坐标转换失败")

    # OGR Geometry 对象不在线程间共享。此处先序列化为 WKB，工作线程再各自
    # 重建 Geometry，避免旧版 GDAL/OGR 的线程安全问题。
    # 同时预计算每个子流域的包围盒，用于后续快速过滤。
    subbasin_wkbs = []
    for sb in subbasins:
        try:
            geometry_wkb = bytes(sb["geometry"].ExportToWkb())
        except Exception:
            print("        警告：子流域 {} 无法序列化，已跳过".format(sb["subbasin_id"]))
            continue
        # 预计算包围盒 (minX, maxX, minY, maxY)
        env = sb["geometry"].GetEnvelope()
        subbasin_wkbs.append((sb["subbasin_id"], geometry_wkb, env))

    if len(subbasin_wkbs) == 0:
        raise RuntimeError("没有可用于多线程相交计算的子流域面")

    # 读取所有河道要素及字段定义
    orig_defn = st_layer.GetLayerDefn()
    orig_field_count = orig_defn.GetFieldCount()
    orig_geom_type = st_layer.GetGeomType()
    orig_field_names = [orig_defn.GetFieldDefn(i).GetName()
                        for i in range(orig_field_count)]

    # 检查是否存在 LINKNO 字段
    linkno_idx = orig_defn.GetFieldIndex("LINKNO")

    stream_features = []
    st_layer.ResetReading()
    for st_feature in st_layer:
        geometry = st_feature.GetGeometryRef()
        if geometry is None or geometry.IsEmpty():
            continue
        field_values = []
        for i in range(orig_field_count):
            field_values.append(st_feature.GetField(i))
        geometry_clone = geometry.Clone()
        try:
            geometry_wkb = bytes(geometry_clone.ExportToWkb())
        except Exception:
            geometry_wkb = None
            print("        警告：河道 FID={} 无法序列化，将写入 LINKNO=0".format(st_feature.GetFID()))
        # 预计算包围盒 (minX, maxX, minY, maxY)
        try:
            stream_env = geometry_clone.GetEnvelope()
        except Exception:
            stream_env = None
        stream_features.append({
            "fid": st_feature.GetFID(),
            "geometry": geometry_clone,
            "geometry_wkb": geometry_wkb,
            "envelope": stream_env,
            "field_values": field_values,
        })

    st_ds = None

    if len(stream_features) == 0:
        raise RuntimeError("河道 Shapefile 中没有有效线要素")

    # ---------- 多线程：为每条原始河道选择相交长度最大的子流域 ----------
    total_subbasin_count = len(subbasin_wkbs)
    total_stream_count = len(stream_features)

    def _bbox_overlap(env_a, env_b):
        """快速判断两个包围盒是否重叠。env = (minX, maxX, minY, maxY)。"""
        if env_a is None or env_b is None:
            return True
        return not (env_a[1] < env_b[0] or env_a[0] > env_b[1] or
                    env_a[3] < env_b[2] or env_a[2] > env_b[3])

    def _lineal_length(geom):
        """返回几何中线要素的总长度，忽略仅由点构成的相交结果。"""
        if geom is None or geom.IsEmpty():
            return 0.0
        geom_name = geom.GetGeometryName().upper()
        if "LINESTRING" in geom_name:
            return geom.Length()
        if "MULTI" in geom_name or "GEOMETRYCOLLECTION" in geom_name:
            return sum(_lineal_length(geom.GetGeometryRef(index))
                       for index in range(geom.GetGeometryCount()))
        return 0.0

    def process_stream(stream_index):
        """为一条河道选择重叠长度最大的子流域，不改变其原始几何。"""
        stream = stream_features[stream_index]
        if stream["geometry_wkb"] is None:
            return stream_index, 0, 0.0, 0.0

        try:
            stream_geom = ogr.CreateGeometryFromWkb(stream["geometry_wkb"])
        except Exception:
            stream_geom = None
        if stream_geom is None or stream_geom.IsEmpty():
            return stream_index, 0, 0.0, 0.0

        stream_length = _lineal_length(stream_geom)
        best_subbasin_id = 0
        best_overlap_length = 0.0

        for subbasin_id, subbasin_wkb, subbasin_env in subbasin_wkbs:
            if not _bbox_overlap(stream["envelope"], subbasin_env):
                continue
            try:
                subbasin_geom = ogr.CreateGeometryFromWkb(subbasin_wkb)
                intersection = stream_geom.Intersection(subbasin_geom)
                overlap_length = _lineal_length(intersection)
            except Exception:
                continue

            # 边界重合时可能长度相同；按更小的子流域 ID 打破平局，保证结果稳定。
            if overlap_length > best_overlap_length or (
                    overlap_length == best_overlap_length and
                    overlap_length > 0 and
                    (best_subbasin_id == 0 or subbasin_id < best_subbasin_id)):
                best_subbasin_id = subbasin_id
                best_overlap_length = overlap_length

        return stream_index, best_subbasin_id, best_overlap_length, stream_length

    # 每个结果与 stream_features 下标对应，因而写出时可保留原有要素顺序、属性与几何。
    stream_results = [None] * total_stream_count
    print("        计算原始河道的主归属子流域：{} 条河道 × {} 个子流域，使用 {} 个线程".format(
        total_stream_count, total_subbasin_count, max_workers))

    if max_workers == 1:
        for stream_index in range(total_stream_count):
            stream_results[stream_index] = process_stream(stream_index)
    else:
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            future_to_index = {
                executor.submit(process_stream, stream_index): stream_index
                for stream_index in range(total_stream_count)
            }
            for future in as_completed(future_to_index):
                stream_index = future_to_index[future]
                try:
                    stream_results[stream_index] = future.result()
                except Exception as error:
                    print("        警告：河道 FID={} 的主归属计算失败：{}".format(
                        stream_features[stream_index]["fid"], error))
                    stream_results[stream_index] = (stream_index, 0, 0.0, 0.0)

    # ---------- 保留原始河道，一一写回仅更新 LINKNO ----------
    if not overwrite:
        raise IOError(
            "输出文件已经存在：{}；请将 overwrite 设为 True".format(
                stream_path
            )
        )

    driver = ogr.GetDriverByName("ESRI Shapefile")
    if os.path.exists(stream_path):
        driver.DeleteDataSource(stream_path)

    out_ds = driver.CreateDataSource(stream_path)
    if out_ds is None:
        raise IOError("无法创建输出河道 Shapefile：{}".format(stream_path))

    out_layer = out_ds.CreateLayer(
        os.path.splitext(os.path.basename(stream_path))[0],
        srs=st_srs,
        geom_type=orig_geom_type,
        options=["ENCODING=UTF-8"]
    )

    if out_layer is None:
        out_ds = None
        raise RuntimeError("无法创建输出河道图层")

    for field_idx in range(orig_field_count):
        field_defn = orig_defn.GetFieldDefn(field_idx)
        if out_layer.CreateField(field_defn) != 0:
            out_ds = None
            raise RuntimeError("无法复制河道字段：{}".format(
                field_defn.GetName()))

    if linkno_idx < 0:
        integer_field_type = getattr(ogr, "OFTInteger64", ogr.OFTInteger)
        if out_layer.CreateField(
                ogr.FieldDefn("LINKNO", integer_field_type)) != 0:
            out_ds = None
            raise RuntimeError("无法添加 LINKNO 字段")
        linkno_idx = orig_field_count

    out_defn = out_layer.GetLayerDefn()
    out_count = 0
    no_match_count = 0
    minority_match_count = 0
    for stream_index, result in enumerate(stream_results):
        _, linkno, overlap_length, stream_length = result
        stream = stream_features[stream_index]
        new_feature = ogr.Feature(out_defn)

        for field_idx, field_value in enumerate(stream["field_values"]):
            if field_value is not None:
                new_feature.SetField(field_idx, field_value)
        new_feature.SetField(linkno_idx, linkno)
        if stream["geometry"] is not None:
            new_feature.SetGeometry(stream["geometry"])

        if out_layer.CreateFeature(new_feature) != 0:
            new_feature = None
            out_ds = None
            raise RuntimeError("写入更新后的河道要素失败")

        if linkno == 0:
            no_match_count += 1
        elif stream_length > 0 and overlap_length / stream_length < 0.5:
            minority_match_count += 1
        new_feature = None
        out_count += 1

    out_layer.SyncToDisk()
    out_ds = None

    print(
        "        更新河道 LINKNO：保留 {} 条原始河道；{} 条未匹配（LINKNO=0），"
        "{} 条最大重叠仍不足 50%".format(
            out_count, no_match_count, minority_match_count
        )
    )

    return out_count


def repair_stream_topology(stream_path, subbasin_path, output_path=None,
                            overwrite=True, snap_tolerance=1e-4,
                            min_streams_at_junction=3):
    """
    修复河道拓扑：将同一汇合处的河道端点对齐到子流域共同交点（锚点）。

    核心思路：
        1. 从子流域 SHP 中提取原始顶点。若一个顶点同时位于至少 3 个子流域
           的边界上，则作为公共锚点；其中一个子流域可以仅由边穿过该顶点，
           不要求所有子流域都在该点显式存有顶点。
        2. 将距离不超过 snap_tolerance 的河道端点聚为候选汇合组。只有当组内
           至少有 min_streams_at_junction 条不同河道时，才视为多河道交点。
        3. 将该组全部端点整体移动到距离该组最近的子流域公共顶点；若最近点
           不在所有端点的吸附容差内，则跳过该组。未分组端点不作任何移动。
        4. 写出修复后的河道 SHP，保留原始字段。

    参数：
        stream_path           : 河道 Shapefile 路径。
        subbasin_path         : 子流域 Shapefile 路径。
        output_path           : 输出路径；若为 None 则覆盖 stream_path。
        overwrite             : 是否覆盖已存在的输出文件。
        snap_tolerance        : 端点吸附到锚点的距离容差（度）。
        min_streams_at_junction: 一个汇合组至少应包含的不同河道数；默认 3，
                                 即每条河道与至少两条其他河道相交。
    """
    if output_path is None:
        output_path = stream_path

    if snap_tolerance <= 0:
        raise ValueError("snap_tolerance 必须大于 0")
    try:
        min_streams_at_junction = int(min_streams_at_junction)
    except (TypeError, ValueError):
        raise ValueError("min_streams_at_junction 必须是大于等于 3 的整数")
    if min_streams_at_junction < 3:
        raise ValueError("min_streams_at_junction 必须大于等于 3")

    ensure_file(stream_path, "河道 Shapefile")
    ensure_file(subbasin_path, "子流域 Shapefile")

    # ---------- 1. 读取子流域，构建顶点信息 ----------
    print("        读取子流域顶点，识别多子流域共享拐点 ...")

    sb_ds = ogr.Open(subbasin_path, gdal.GA_ReadOnly)
    if sb_ds is None:
        raise IOError("无法打开子流域 Shapefile：{}".format(subbasin_path))

    sb_layer = sb_ds.GetLayer(0)
    if sb_layer is None:
        sb_ds = None
        raise RuntimeError("子流域 Shapefile 中没有可用图层")

    sb_srs = sb_layer.GetSpatialRef()
    if sb_srs is None:
        sb_ds = None
        raise RuntimeError("子流域 Shapefile 缺少坐标系")

    sb_srs = sb_srs.Clone()
    set_traditional_axis_order(sb_srs)

    # (x, y) -> set(subbasin_id)。候选点来自原始顶点；不能按距离连通聚类，
    # 因为栅格矢量化的阶梯状边界会被错误地合并为一个大簇。
    vertex_to_sbs = {}
    # 边界线段按规则网格索引。这里的网格只加速候选查询，不参与拓扑判定。
    boundary_segments = {}
    boundary_grid_size = max(snap_tolerance, 1e-6)
    boundary_tolerance = 1e-9

    def _add_boundary_segment(sb_id, start, end):
        """将一条边界线段加入其覆盖的空间网格。"""
        minx = min(start[0], end[0])
        maxx = max(start[0], end[0])
        miny = min(start[1], end[1])
        maxy = max(start[1], end[1])
        first_x = int(math.floor(minx / boundary_grid_size))
        last_x = int(math.floor(maxx / boundary_grid_size))
        first_y = int(math.floor(miny / boundary_grid_size))
        last_y = int(math.floor(maxy / boundary_grid_size))
        segment = (sb_id, start[0], start[1], end[0], end[1])
        for cell_x in range(first_x, last_x + 1):
            for cell_y in range(first_y, last_y + 1):
                boundary_segments.setdefault((cell_x, cell_y), []).append(segment)

    def _point_on_segment(x, y, segment):
        """判断点是否位于边界线段上，使用极小容差抵消浮点误差。"""
        _, x1, y1, x2, y2 = segment
        if x < min(x1, x2) - boundary_tolerance or \
                x > max(x1, x2) + boundary_tolerance or \
                y < min(y1, y2) - boundary_tolerance or \
                y > max(y1, y2) + boundary_tolerance:
            return False

        dx = x2 - x1
        dy = y2 - y1
        length_sq = dx * dx + dy * dy
        if length_sq <= boundary_tolerance * boundary_tolerance:
            return math.hypot(x - x1, y - y1) <= boundary_tolerance

        cross = abs((x - x1) * dy - (y - y1) * dx)
        if cross > boundary_tolerance * math.sqrt(length_sq):
            return False

        projection = (x - x1) * dx + (y - y1) * dy
        return -boundary_tolerance <= projection <= \
            length_sq + boundary_tolerance

    def _cluster_nearby_points(points, tolerance):
        """按欧氏距离聚合二维点，返回每个连通簇的原始下标列表。"""
        parent = list(range(len(points)))

        def _find(index):
            while parent[index] != index:
                parent[index] = parent[parent[index]]
                index = parent[index]
            return index

        def _union(first, second):
            first_root = _find(first)
            second_root = _find(second)
            if first_root != second_root:
                parent[second_root] = first_root

        # 规则网格只用于候选查询；最终是否同簇仍由实际欧氏距离判定。
        grid = {}
        for index, point in enumerate(points):
            x, y = point[0], point[1]
            cell_x = int(math.floor(x / tolerance))
            cell_y = int(math.floor(y / tolerance))

            for offset_x in (-1, 0, 1):
                for offset_y in (-1, 0, 1):
                    for other_index in grid.get(
                            (cell_x + offset_x, cell_y + offset_y), []):
                        other = points[other_index]
                        if math.hypot(x - other[0], y - other[1]) <= tolerance:
                            _union(index, other_index)

            grid.setdefault((cell_x, cell_y), []).append(index)

        clusters = {}
        for index in range(len(points)):
            clusters.setdefault(_find(index), []).append(index)
        return list(clusters.values())

    sb_layer.ResetReading()
    for sb_feature in sb_layer:
        sb_id = sb_feature.GetField("SUBBASINID")
        sb_geom = sb_feature.GetGeometryRef()
        if sb_id is None or sb_geom is None or sb_geom.IsEmpty():
            continue
        sb_id = int(sb_id)

        geom_name = sb_geom.GetGeometryName().upper()

        def _collect_polygon_vertices(polygon_geom):
            if polygon_geom is None:
                return
            ring_count = polygon_geom.GetGeometryCount()
            for ring_idx in range(ring_count):
                ring = polygon_geom.GetGeometryRef(ring_idx)
                if ring is None:
                    continue
                n_pts = ring.GetPointCount()
                previous = None
                for pt_idx in range(n_pts):
                    pt = ring.GetPoint(pt_idx)
                    coordinate = (pt[0], pt[1])
                    vertex_to_sbs.setdefault(coordinate, set()).add(sb_id)
                    if previous is not None:
                        _add_boundary_segment(sb_id, previous, coordinate)
                    previous = coordinate

        if "MULTIPOLYGON" in geom_name or "GEOMETRYCOLLECTION" in geom_name:
            for g_idx in range(sb_geom.GetGeometryCount()):
                sub = sb_geom.GetGeometryRef(g_idx)
                if sub is not None and "POLYGON" in sub.GetGeometryName().upper():
                    _collect_polygon_vertices(sub)
        elif "POLYGON" in geom_name:
            _collect_polygon_vertices(sb_geom)

    sb_ds = None

    # 识别多子流域公共交点。候选点必须是至少两个子流域共享的原始顶点，
    # 再检查其是否落在第三个子流域的边界上。这样“两子流域顶点 + 第三个
    # 子流域边界穿过该点”的 T 形连接也会被识别，同时不会把普通单面拐点
    # 误判为三子流域交点；相邻顶点之间也不会按距离合并。
    anchors = []  # [(cx, cy, set_of_sb_ids), ...]
    # 与 anchors 下标一一对应，用于输出实际被采用锚点的拓扑来源。
    # vertex_sbs 是显式存有该顶点的子流域；boundary_sbs 是边界经过该点、
    # 但未将该点显式作为顶点保存的额外子流域。
    anchor_details = []
    vertex_only_anchor_count = 0
    boundary_intersection_anchor_count = 0
    for coordinate, vertex_sbs in vertex_to_sbs.items():
        if len(vertex_sbs) < 2:
            continue
        x, y = coordinate
        shared_sbs = set(vertex_sbs)
        cell = (
            int(math.floor(x / boundary_grid_size)),
            int(math.floor(y / boundary_grid_size)),
        )
        boundary_sbs = set()
        for segment in boundary_segments.get(cell, []):
            sb_id = segment[0]
            if sb_id in shared_sbs:
                continue
            if _point_on_segment(x, y, segment):
                shared_sbs.add(sb_id)
                boundary_sbs.add(sb_id)

        if len(shared_sbs) >= 3:
            anchors.append((x, y, shared_sbs))
            anchor_details.append({
                "vertex_sbs": sorted(vertex_sbs),
                "boundary_sbs": sorted(boundary_sbs),
            })
            if len(vertex_sbs) >= 3:
                vertex_only_anchor_count += 1
            else:
                boundary_intersection_anchor_count += 1

    print("        子流域顶点：{} 个，公共交点：{} 个（顶点共享 {} 个，顶点-边界相交 {} 个）".format(
        len(vertex_to_sbs), len(anchors), vertex_only_anchor_count,
        boundary_intersection_anchor_count))

    if len(anchors) == 0:
        print("        未找到多子流域共享拐点，跳过拓扑修复")
        return {
            "stream_count": 0,
            "snapped_count": 0,
            "junction_group_count": 0,
            "junction_endpoint_count": 0,
            "dropped_degenerate_count": 0,
        }

    # ---------- 2. 读取河道，收集端点 ----------
    print("        读取河道端点 ...")

    st_ds = ogr.Open(stream_path, gdal.GA_ReadOnly)
    if st_ds is None:
        raise IOError("无法打开河道 Shapefile：{}".format(stream_path))

    st_layer = st_ds.GetLayer(0)
    if st_layer is None:
        st_ds = None
        raise RuntimeError("河道 Shapefile 中没有可用图层")

    st_srs = st_layer.GetSpatialRef()
    if st_srs is None:
        st_ds = None
        raise RuntimeError("河道 Shapefile 缺少坐标系")

    st_srs = st_srs.Clone()
    set_traditional_axis_order(st_srs)

    orig_defn = st_layer.GetLayerDefn()
    orig_field_count = orig_defn.GetFieldCount()
    linkno_field_index = orig_defn.GetFieldIndex("LINKNO")

    # 坐标转换：锚点子流域坐标系 → 河道坐标系
    coord_trans = None
    if not sb_srs.IsSame(st_srs):
        coord_trans = osr.CoordinateTransformation(sb_srs, st_srs)
        for i in range(len(anchors)):
            cx, cy, sbs = anchors[i]
            pt = ogr.Geometry(ogr.wkbPoint)
            pt.AddPoint(cx, cy)
            pt.Transform(coord_trans)
            anchors[i] = (pt.GetX(), pt.GetY(), sbs)
            pt = None

    # 收集每条河道的几何、字段和端点信息
    stream_records = []
    st_layer.ResetReading()
    for st_feature in st_layer:
        geom = st_feature.GetGeometryRef()
        if geom is None or geom.IsEmpty():
            stream_records.append({
                "fid": st_feature.GetFID(),
                "geom": None,
                "geom_name": "",
                "field_values": [st_feature.GetField(i) for i in range(orig_field_count)],
                "from_xy": None,
                "to_xy": None,
            })
            continue

        geom_clone = geom.Clone()
        geom_name = geom_clone.GetGeometryName().upper()

        from_xy = None
        to_xy = None
        if "MULTILINESTRING" in geom_name:
            n_parts = geom_clone.GetGeometryCount()
            if n_parts > 0:
                first_line = geom_clone.GetGeometryRef(0)
                if first_line is not None and first_line.GetPointCount() > 0:
                    p = first_line.GetPoint(0)
                    from_xy = (p[0], p[1])
                last_line = geom_clone.GetGeometryRef(n_parts - 1)
                if last_line is not None and last_line.GetPointCount() > 0:
                    p = last_line.GetPoint(last_line.GetPointCount() - 1)
                    to_xy = (p[0], p[1])
        elif "LINESTRING" in geom_name:
            n_pts = geom_clone.GetPointCount()
            if n_pts > 0:
                p = geom_clone.GetPoint(0)
                from_xy = (p[0], p[1])
                p = geom_clone.GetPoint(n_pts - 1)
                to_xy = (p[0], p[1])

        stream_records.append({
            "fid": st_feature.GetFID(),
            "geom": geom_clone,
            "geom_name": geom_name,
            "field_values": [st_feature.GetField(i) for i in range(orig_field_count)],
            "from_xy": from_xy,
            "to_xy": to_xy,
        })

    st_ds = None
    stream_records_by_fid = {rec["fid"]: rec for rec in stream_records}

    # ---------- 3. 按多河道汇合组吸附到最近的公共锚点 ----------
    print("        聚合河道汇合端点并吸附到子流域共同拐点（容差 {} 度）...".format(
        snap_tolerance))

    # 构建锚点数组用于距离计算
    anchor_positions = np.array([(a[0], a[1]) for a in anchors], dtype=np.float64)

    # 诊断：统计端点到最近锚点距离
    min_dist_list = []
    for rec in stream_records:
        for xy in (rec["from_xy"], rec["to_xy"]):
            if xy is None:
                continue
            dists = np.hypot(anchor_positions[:, 0] - xy[0],
                             anchor_positions[:, 1] - xy[1])
            min_dist_list.append(float(dists.min()))
    if min_dist_list:
        min_dist_arr = np.array(min_dist_list)
        print("        诊断：端点到最近锚点距离——最小 {} / 中位数 {} / 90%分位 {} 度".format(
            round(float(min_dist_arr.min()), 8),
            round(float(np.median(min_dist_arr)), 8),
            round(float(np.percentile(min_dist_arr, 90)), 8)))

    # 只使用河道端点的空间关系：不再以端点当前属于哪个子流域作为筛选条件。
    endpoint_entries = []
    for rec in stream_records:
        fid = rec["fid"]
        if rec["from_xy"] is not None:
            endpoint_entries.append({
                "fid": fid,
                "side": "from",
                "xy": rec["from_xy"],
            })
        if rec["to_xy"] is not None:
            endpoint_entries.append({
                "fid": fid,
                "side": "to",
                "xy": rec["to_xy"],
            })

    # endpoint_targets 的键是 (fid, "from" / "to")。一个簇中需有至少 3 条
    # 不同河道，表示每条河道都与至少两条其他河道相交。
    endpoint_targets = {}
    junction_group_count = 0
    junction_endpoint_count = 0
    printed_junction_group_count = 0
    max_detail_log_count = 20
    endpoint_clusters = _cluster_nearby_points(
        [entry["xy"] for entry in endpoint_entries], snap_tolerance)
    for cluster_indices in endpoint_clusters:
        cluster = [endpoint_entries[index] for index in cluster_indices]
        stream_fids = set(entry["fid"] for entry in cluster)
        if len(stream_fids) < min_streams_at_junction:
            continue

        center_x = sum(entry["xy"][0] for entry in cluster) / len(cluster)
        center_y = sum(entry["xy"][1] for entry in cluster) / len(cluster)
        candidate_anchors = []
        for anchor_index, (anchor_x, anchor_y, anchor_sbs) in enumerate(anchors):
            distances = [
                math.hypot(anchor_x - entry["xy"][0],
                           anchor_y - entry["xy"][1])
                for entry in cluster
            ]
            if max(distances) > snap_tolerance:
                continue

            candidate_anchors.append((
                math.hypot(anchor_x - center_x, anchor_y - center_y),
                max(distances),
                sum(distance * distance for distance in distances),
                anchor_index,
            ))

        if not candidate_anchors:
            continue

        _, _, _, anchor_index = min(candidate_anchors)
        anchor_x, anchor_y, _ = anchors[anchor_index]
        for entry in cluster:
            endpoint_targets[(entry["fid"], entry["side"])] = (anchor_x, anchor_y)

        junction_group_count += 1
        junction_endpoint_count += len(cluster)
        if printed_junction_group_count < max_detail_log_count:
            detail = anchor_details[anchor_index]
            endpoint_labels = []
            for entry in sorted(cluster, key=lambda item: (item["fid"], item["side"])):
                rec = stream_records_by_fid[entry["fid"]]
                side_name = "起点" if entry["side"] == "from" else "终点"
                linkno_text = ""
                if linkno_field_index >= 0:
                    linkno_text = "，LINKNO={}".format(
                        rec["field_values"][linkno_field_index])
                endpoint_labels.append("FID {} {}{}".format(
                    entry["fid"], side_name, linkno_text))

            print("          汇合组 {} 条河道、{} 个端点 → ({:.6f},{:.6f})".format(
                len(stream_fids), len(cluster), anchor_x, anchor_y))
            print("            显式共享顶点的子流域：{}".format(
                detail["vertex_sbs"]))
            if detail["boundary_sbs"]:
                print("            顶点同时落在子流域 {} 的边界上（该面未显式存该顶点）".format(
                    detail["boundary_sbs"]))
            else:
                print("            所有相关子流域均显式共享该顶点")
            print("            参与河道端点：{}".format("； ".join(endpoint_labels)))
            printed_junction_group_count += 1

    snapped_count = len(endpoint_targets)
    new_stream_records = []
    dropped_degenerate_count = 0

    # 某些原始河段在端点处有重复（或近乎重复）的顶点。若只替换第一/最后
    # 一个顶点，会得到“新锚点 -> 旧重复顶点 -> 后续河道”的人工折返线。
    # 使用远小于吸附容差的阈值，仅清理这种端点重复，不删除正常折线顶点。
    endpoint_duplicate_tolerance = min(snap_tolerance * 1e-3, 1e-7)

    def _distance_between(first, second):
        return math.hypot(first[0] - second[0], first[1] - second[1])

    def _rebuild_line_part(line_part, snapped_start=None, snapped_end=None,
                           original_start=None, original_end=None):
        """替换端点并剔除端点附近的重复顶点；退化线返回 None。"""
        point_count = line_part.GetPointCount()
        first_index = 0
        last_index = point_count

        if snapped_start is not None and original_start is not None:
            while first_index < last_index:
                point = line_part.GetPoint(first_index)
                if _distance_between(point, original_start) <= endpoint_duplicate_tolerance:
                    first_index += 1
                else:
                    break

        if snapped_end is not None and original_end is not None:
            while last_index > first_index:
                point = line_part.GetPoint(last_index - 1)
                if _distance_between(point, original_end) <= endpoint_duplicate_tolerance:
                    last_index -= 1
                else:
                    break

        coordinates = []
        if snapped_start is not None:
            coordinates.append(snapped_start)

        for point_index in range(first_index, last_index):
            point = line_part.GetPoint(point_index)
            coordinate = (point[0], point[1])
            if not coordinates or \
                    _distance_between(coordinates[-1], coordinate) > endpoint_duplicate_tolerance:
                coordinates.append(coordinate)

        if snapped_end is not None and (
                not coordinates or
                _distance_between(coordinates[-1], snapped_end) > endpoint_duplicate_tolerance):
            coordinates.append(snapped_end)

        if len(coordinates) < 2:
            return None

        if all(_distance_between(coordinates[index - 1], coordinates[index]) <=
               endpoint_duplicate_tolerance
               for index in range(1, len(coordinates))):
            return None

        rebuilt = ogr.Geometry(ogr.wkbLineString)
        for coordinate in coordinates:
            rebuilt.AddPoint(coordinate[0], coordinate[1])
        return rebuilt

    for rec in stream_records:
        geom = rec["geom"]
        geom_name = rec["geom_name"]
        if geom is None:
            new_stream_records.append(rec)
            continue

        fid = rec["fid"]
        snapped_from = endpoint_targets.get((fid, "from"))
        snapped_to = endpoint_targets.get((fid, "to"))

        if snapped_from is None and snapped_to is None:
            new_stream_records.append(rec)
            continue

        # 重建几何
        if "MULTILINESTRING" in geom_name:
            n_parts = geom.GetGeometryCount()
            new_ml = ogr.Geometry(ogr.wkbMultiLineString)
            for part_idx in range(n_parts):
                line_part = geom.GetGeometryRef(part_idx)
                if line_part is None or line_part.GetPointCount() == 0:
                    continue
                new_line = _rebuild_line_part(
                    line_part,
                    snapped_start=snapped_from if part_idx == 0 else None,
                    snapped_end=snapped_to if part_idx == n_parts - 1 else None,
                    original_start=rec["from_xy"] if part_idx == 0 else None,
                    original_end=rec["to_xy"] if part_idx == n_parts - 1 else None,
                )
                if new_line is None:
                    dropped_degenerate_count += 1
                    continue
                new_ml.AddGeometry(new_line)
                new_line = None
            geom = new_ml if new_ml.GetGeometryCount() > 0 else None
        elif "LINESTRING" in geom_name:
            geom = _rebuild_line_part(
                geom,
                snapped_start=snapped_from,
                snapped_end=snapped_to,
                original_start=rec["from_xy"],
                original_end=rec["to_xy"],
            )
            if geom is None:
                dropped_degenerate_count += 1

        rec["geom"] = geom
        if snapped_from is not None:
            rec["from_xy"] = snapped_from
        if snapped_to is not None:
            rec["to_xy"] = snapped_to

        new_stream_records.append(rec)

    print("        汇合组 {} 个，整组吸附 {} 个端点；总计吸附 {} 个端点；删除 {} 条退化河段".format(
        junction_group_count, junction_endpoint_count, snapped_count,
        dropped_degenerate_count))
    if junction_group_count > printed_junction_group_count:
        print("        其余 {} 个汇合组已省略逐项输出（详细日志上限为 {} 组）".format(
            junction_group_count - printed_junction_group_count,
            max_detail_log_count))

    # ---------- 5. 写出修复后的河道 SHP ----------
    print("        写出修复后的河道 SHP ...")

    driver = ogr.GetDriverByName("ESRI Shapefile")
    if os.path.exists(output_path):
        if not overwrite:
            raise IOError("输出文件已存在：{}".format(output_path))
        driver.DeleteDataSource(output_path)

    out_ds = driver.CreateDataSource(output_path)
    if out_ds is None:
        raise IOError("无法创建输出河道 Shapefile：{}".format(output_path))

    out_layer = out_ds.CreateLayer(
        os.path.splitext(os.path.basename(output_path))[0],
        srs=st_srs,
        geom_type=ogr.wkbMultiLineString,
        options=["ENCODING=UTF-8"]
    )

    if out_layer is None:
        out_ds = None
        raise RuntimeError("无法创建输出图层")

    for field_idx in range(orig_field_count):
        field_defn = orig_defn.GetFieldDefn(field_idx)
        if out_layer.CreateField(field_defn) != 0:
            out_ds = None
            raise RuntimeError("无法复制河道字段：{}".format(field_defn.GetName()))

    out_defn = out_layer.GetLayerDefn()
    out_count = 0

    for rec in new_stream_records:
        geom = rec["geom"]
        if geom is None or geom.IsEmpty():
            continue

        new_feature = ogr.Feature(out_defn)
        for i, val in enumerate(rec["field_values"]):
            new_feature.SetField(i, val)

        gname = geom.GetGeometryName().upper()
        if "LINESTRING" in gname and "MULTI" not in gname:
            ml = ogr.Geometry(ogr.wkbMultiLineString)
            ml.AddGeometry(geom)
            geom = ml

        new_feature.SetGeometry(geom)

        if out_layer.CreateFeature(new_feature) != 0:
            new_feature = None
            out_ds = None
            raise RuntimeError("写入河道要素失败")

        new_feature = None
        out_count += 1

    out_layer.SyncToDisk()
    out_ds = None

    print("        拓扑修复完成：{} 条河道，吸附 {} 个端点".format(
        out_count, snapped_count))

    return {
        "stream_count": out_count,
        "snapped_count": snapped_count,
        "junction_group_count": junction_group_count,
        "junction_endpoint_count": junction_endpoint_count,
        "dropped_degenerate_count": dropped_degenerate_count,
    }


def extract_stream_nodes(stream_path, node_path, overwrite=True, precision=6):
    """
    从河道 Shapefile 中提取节点并输出节点 Shapefile，同时为河道添加
    FROM_NODE 和 TO_NODE 字段。

    步骤：
        1. 遍历所有河道要素，提取每条河道的起点和终点。
        2. 按坐标去重（容差由 precision 控制），为每个唯一节点分配 NODEID。
        3. 输出节点 Shapefile（点要素，含 NODEID 字段）。
        4. 在河道 Shapefile 中新增 FROM_NODE 和 TO_NODE 字段。

    对于 MultiLineString，取第一条 LineString 的第一个点作为起点，
    最后一条 LineString 的最后一个点作为终点。

    参数：
        stream_path  : 河道 Shapefile 路径（将被原地修改以添加字段）。
        node_path    : 输出节点 Shapefile 路径。
        overwrite    : 是否覆盖已存在的节点文件。
        precision    : 坐标去重的小数位数（6 ≈ 0.1 m，适用于经纬度）。
    """
    ensure_file(stream_path, "河道 Shapefile")

    # ---------- 1. 读取河道，提取端点 ----------
    ds = ogr.Open(stream_path, 1)  # 可写模式，后续要添加字段
    if ds is None:
        raise IOError("无法打开河道 Shapefile：{}".format(stream_path))

    layer = ds.GetLayer(0)
    if layer is None:
        ds = None
        raise RuntimeError("河道 Shapefile 中没有可用图层")

    srs = layer.GetSpatialRef()
    defn = layer.GetLayerDefn()

    # 收集每条河道的 (fid, from_xy, to_xy)
    stream_endpoints = []
    layer.ResetReading()
    for feature in layer:
        geom = feature.GetGeometryRef()
        if geom is None or geom.IsEmpty():
            stream_endpoints.append((feature.GetFID(), None, None))
            continue

        geom_name = geom.GetGeometryName().upper()
        first_point = None
        last_point = None

        if "MULTILINESTRING" in geom_name:
            n_parts = geom.GetGeometryCount()
            if n_parts > 0:
                first_line = geom.GetGeometryRef(0)
                if first_line is not None and first_line.GetPointCount() > 0:
                    first_point = first_line.GetPoint(0)
                last_line = geom.GetGeometryRef(n_parts - 1)
                if last_line is not None and last_line.GetPointCount() > 0:
                    last_point = last_line.GetPoint(last_line.GetPointCount() - 1)
        elif "LINESTRING" in geom_name:
            n_pts = geom.GetPointCount()
            if n_pts > 0:
                first_point = geom.GetPoint(0)
                last_point = geom.GetPoint(n_pts - 1)

        if first_point is not None:
            from_xy = (round(first_point[0], precision),
                       round(first_point[1], precision))
        else:
            from_xy = None

        if last_point is not None:
            to_xy = (round(last_point[0], precision),
                     round(last_point[1], precision))
        else:
            to_xy = None

        stream_endpoints.append((feature.GetFID(), from_xy, to_xy))
        feature = None

    # ---------- 2. 去重并分配 NODEID ----------
    node_map = {}  # (x, y) -> NODEID
    node_list = []  # [(NODEID, x, y), ...]

    for _, from_xy, to_xy in stream_endpoints:
        for xy in (from_xy, to_xy):
            if xy is None:
                continue
            if xy not in node_map:
                node_id = len(node_list) + 1
                node_map[xy] = node_id
                node_list.append((node_id, xy[0], xy[1]))

    print("        提取到 {} 个唯一节点（{} 条河道）".format(
        len(node_list), len(stream_endpoints)))

    # ---------- 3. 输出节点 Shapefile ----------
    if os.path.exists(node_path):
        if not overwrite:
            ds = None
            raise IOError("输出文件已存在：{}；请将 overwrite 设为 True".format(node_path))
        node_driver = ogr.GetDriverByName("ESRI Shapefile")
        node_driver.DeleteDataSource(node_path)

    node_driver = ogr.GetDriverByName("ESRI Shapefile")
    node_ds = node_driver.CreateDataSource(node_path)
    if node_ds is None:
        ds = None
        raise IOError("无法创建节点 Shapefile：{}".format(node_path))

    node_layer = node_ds.CreateLayer(
        os.path.splitext(os.path.basename(node_path))[0],
        srs=srs,
        geom_type=ogr.wkbPoint,
        options=["ENCODING=UTF-8"]
    )
    if node_layer is None:
        node_ds = None
        ds = None
        raise RuntimeError("无法创建节点图层")

    node_field = ogr.FieldDefn("NODEID", ogr.OFTInteger)
    if node_layer.CreateField(node_field) != 0:
        node_ds = None
        ds = None
        raise RuntimeError("无法添加 NODEID 字段")

    node_defn = node_layer.GetLayerDefn()
    for node_id, x, y in node_list:
        point_geom = ogr.Geometry(ogr.wkbPoint)
        point_geom.AddPoint(x, y)
        node_feature = ogr.Feature(node_defn)
        node_feature.SetField("NODEID", node_id)
        node_feature.SetGeometry(point_geom)
        if node_layer.CreateFeature(node_feature) != 0:
            node_feature = None
            node_ds = None
            ds = None
            raise RuntimeError("写入节点要素失败")
        node_feature = None

    node_layer.SyncToDisk()
    node_ds = None

    print("        已输出节点 Shapefile：{}（{} 个节点）".format(
        node_path, len(node_list)))

    # ---------- 4. 为河道添加 FROM_NODE / TO_NODE 字段 ----------
    # 检查字段是否已存在
    from_idx = defn.GetFieldIndex("FROM_NODE")
    if from_idx < 0:
        if layer.CreateField(ogr.FieldDefn("FROM_NODE", ogr.OFTInteger)) != 0:
            ds = None
            raise RuntimeError("无法添加 FROM_NODE 字段")
        from_idx = layer.GetLayerDefn().GetFieldIndex("FROM_NODE")

    to_idx = defn.GetFieldIndex("TO_NODE")
    if to_idx < 0:
        if layer.CreateField(ogr.FieldDefn("TO_NODE", ogr.OFTInteger)) != 0:
            ds = None
            raise RuntimeError("无法添加 TO_NODE 字段")
        to_idx = layer.GetLayerDefn().GetFieldIndex("TO_NODE")

    # 构建 fid → (from_xy, to_xy) 查找表，避免回写时对每个要素做 O(n) 线性扫描
    endpoints_by_fid = {ep_fid: (ep_from, ep_to)
                        for ep_fid, ep_from, ep_to in stream_endpoints}

    updated_count = 0
    layer.ResetReading()
    for feature in layer:
        fid = feature.GetFID()
        from_xy, to_xy = endpoints_by_fid.get(fid, (None, None))

        from_node = node_map.get(from_xy, 0) if from_xy else 0
        to_node = node_map.get(to_xy, 0) if to_xy else 0

        feature.SetField(from_idx, from_node)
        feature.SetField(to_idx, to_node)
        if layer.SetFeature(feature) != 0:
            print("        警告：河道 FID={} 写入 FROM_NODE/TO_NODE 失败".format(fid))
        else:
            updated_count += 1
        feature = None

    layer.SyncToDisk()
    ds = None

    print("        已更新河道 FROM_NODE/TO_NODE：{}（{} 条河道）".format(
        stream_path, updated_count))

    return {"node_count": len(node_list), "stream_count": updated_count}


def main():
    enable_exceptions()

    gdal.SetConfigOption("GDAL_FILENAME_IS_UTF8", "YES")

    # 所有步骤共用的太湖流域范围面
    mask_path = (
        r"J:\G\data\太湖数据\Data\太湖流域1：25万分级流域边界数据集（2000年）\太湖流域1：25万一级流域边界数据集（2000年）\太湖流域一级.shp"
    )
    # mask_path = (
    #     r"G:\program\seims\SEIMS_HAND\data\taihu\taihu_preprocess\cliped_taihu_data\basin_test_select.shp"
    # )

    input_basepath = (
        r"G:\program\seims\SEIMS_HAND\data\taihu"
        r"\张斌提取的真实平原河网\HydroLakes"
    )

    output_basepath = (
        r"G:\program\seims\SEIMS_HAND\data\taihu"
        r"\taihu_preprocess\cliped_taihu_data"
    )


    ensure_file(mask_path, "太湖流域范围 Shapefile")
    mask_geometry, mask_srs = read_mask_union(mask_path)

    # ------------------------------------------------------------------
    # 第 1 步：裁剪 Watershed.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "Watershed.tif")
    output_path = os.path.join(output_basepath, "subbasin.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[1/12] 裁剪栅格：Watershed.tif")

    info = clip_raster(input_path, output_path, mask_path, mask_geometry, mask_srs, overwrite)
    # 第 2 步：subbasin.tif 转为融合后的子流域面
    # ------------------------------------------------------------------
    input_path = os.path.join(output_basepath, "subbasin.tif")
    output_path = os.path.join(output_basepath, "subbasin.shp")
    overwrite = True

    print("[2/12] 栅格转子流域面：subbasin.tif")

    feature_count = raster_to_dissolved_subbasin_shp(input_path, output_path, overwrite)

    print("        输出：{}（{} 个子流域面要素）".format(output_path, feature_count))

    # ------------------------------------------------------------------
    # 第 3 步：裁剪 Taihu_lakes.tif (hydrolake)
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "Taihu_lakes.tif")
    output_path = os.path.join(output_basepath, "hydrolake.tif")
    cliped_hydrlakes_path = output_path
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[3/12] 裁剪栅格：Taihu_lakes.tif (hydrolake)")

    info = clip_raster(input_path, output_path, mask_path, mask_geometry, mask_srs, overwrite)


    # ------------------------------------------------------------------
    # 第 4 步：生成湖泊子流域
    # ------------------------------------------------------------------
    watershed_path = os.path.join(output_basepath, "subbasin.tif")
    cliped_hydrlakes_path = os.path.join(output_basepath, "hydrolake.tif")
    lake_tif_path = os.path.join(output_basepath, "lake_subbasin.tif")
    lake_shp_path = os.path.join(output_basepath, "lake_subbasin.shp")
    overwrite = True

    print("[4/12] 生成湖泊子流域：subbasin.tif + hydrolake.tif")

    lake_info = generate_lake_subbasin(watershed_path, cliped_hydrlakes_path, lake_tif_path, lake_shp_path, overwrite)

    print(
        "        匹配到 {} 个湖泊子流域，最大已有子流域 ID={}，临时 ID 起始值={}".format(
            lake_info["matched_lake_count"],
            lake_info["max_existing_id"],
            min(lake_info["temp_ids"]) if lake_info["temp_ids"] else "N/A"
        )
    )

    # ------------------------------------------------------------------
    # 第 4.1 步：拆分离散子流域（同一 ID 的不连通区域拆为独立子流域）
    # ------------------------------------------------------------------
    split_tif_path = os.path.join(output_basepath, "lake_subbasin_split.tif")
    split_shp_path = os.path.join(output_basepath, "lake_subbasin_split.shp")
    print("[4.1] 拆分离散子流域：lake_subbasin.tif → lake_subbasin_split.tif")

    split_info = split_disconnected_subbasins(
        lake_tif_path, lake_shp_path, overwrite,
        output_tif_path=split_tif_path, output_shp_path=split_shp_path)

    print("        拆分 {} 个离散区域，共 {} 个子流域".format(
        split_info["split_count"], split_info["total_subbasins"]))

    # ------------------------------------------------------------------
    # 第 4.2 步：合并小面积子流域到相邻的大子流域
    # ------------------------------------------------------------------
    # min_area 单位为平方米（经纬度栅格会自动用球面近似换算）。
    # 10000000 平方米 = 10 平方公里
    min_subbasin_area = 10000
    merged_tif_path = os.path.join(output_basepath, "lake_subbasin_merged.tif")
    merged_shp_path = os.path.join(output_basepath, "lake_subbasin_merged.shp")
    print("[4.2] 合并小面积子流域（阈值 {} m²）：lake_subbasin_split.tif → lake_subbasin_merged.tif".format(
        min_subbasin_area))

    merge_info = merge_small_subbasins(
        split_tif_path, split_shp_path, min_subbasin_area, overwrite,
        output_tif_path=merged_tif_path, output_shp_path=merged_shp_path)

    print("        合并 {} 个小面积子流域，剩余 {} 个子流域".format(
        merge_info["merged_count"], merge_info["total_subbasins"]))

    # ------------------------------------------------------------------
    # 第 5 步：统一重新编号所有子流域（从 1 开始递增）
    # ------------------------------------------------------------------
    final_tif_path = os.path.join(output_basepath, "lake_subbasin_final.tif")
    final_shp_path = os.path.join(output_basepath, "lake_subbasin_final.shp")
    print("[5/12] 统一重新编号所有子流域：lake_subbasin_merged.tif → lake_subbasin_final.tif")

    renumber_info = renumber_subbasins(
        merged_tif_path, merged_shp_path, overwrite,
        output_tif_path=final_tif_path, output_shp_path=final_shp_path)

    print("        共 {} 个子流域已统一编号（从 1 开始）".format(renumber_info["total_subbasins"]))



    # ------------------------------------------------------------------
    # 第 6 步：裁剪 burnedfillDEM.tif
    # ------------------------------------------------------------------

    input_path = os.path.join(input_basepath, "burnedfillDEM.tif")
    output_path = os.path.join(output_basepath, "dem.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[6/12] 裁剪栅格：burnedfillDEM.tif")

    info = clip_raster(input_path, output_path, mask_path, mask_geometry, mask_srs, overwrite)

    # ------------------------------------------------------------------
    # 第 7 步：裁剪 Final_fdr.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "Final_fdr.tif")
    output_path = os.path.join(output_basepath, "Final_flow_dir.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[7/12] 裁剪栅格：Final_fdr.tif")

    info = clip_raster(input_path, output_path, mask_path, mask_geometry, mask_srs, overwrite)

    # ------------------------------------------------------------------
    # 第 8 步：裁剪单方向 fdr.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "fdr.tif")
    output_path = os.path.join(output_basepath, "flow_dir.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[8/12] 裁剪栅格：fdr.tif")

    info = clip_raster(input_path, output_path, mask_path, mask_geometry, mask_srs, overwrite)


    # ------------------------------------------------------------------
    # 第 9 步：裁剪 stream.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "stream.tif")
    output_path = os.path.join(output_basepath, "stream_link.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[9/12] 裁剪栅格：stream.tif")

    info = clip_raster(input_path, output_path, mask_path, mask_geometry, mask_srs, overwrite)

    # ------------------------------------------------------------------
    # 第 10 步：裁剪 stream_split.shp
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "stream_split.shp")
    output_path = os.path.join(output_basepath, "stream_link.shp")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[10/12] 裁剪河网：stream_split.shp")

    # feature_count = clip_stream_vector(input_path, output_path, mask_geometry, mask_srs, overwrite)

    # print("        输出：{}（{} 条河道要素）".format(output_path, feature_count))


    # ------------------------------------------------------------------
    # 第 11 步：根据已有湖泊子流域更新河道 LINKNO 字段
    # ------------------------------------------------------------------
    lake_shp_path = os.path.join(output_basepath, "lake_subbasin_final.shp")
    stream_shp_path = os.path.join(output_basepath, "stream_link.shp")
    overwrite = True
    # max_workers = min(8, max(1, os.cpu_count() or 1))

    max_workers = 14

    # ------------------------------------------------------------------
    # 第 11 步：修复河道拓扑（吸附交点到子流域拐点）
    # ------------------------------------------------------------------
    # 先修复拓扑：将河道交点对齐到子流域拐点；随后仅按各河道在子流域内的
    # 最大重叠长度更新 LINKNO，不再使用子流域边界裁剪或打断河道。
    # snap_tolerance: 多河道汇合组到最近子流域公共顶点的最大吸附距离（度）。
    # 0.003 度约为 300 米；需覆盖河道交点的定位误差，但不应跨越相邻汇合点。
    # 至少 3 条不同河道才视作拓扑汇合点（每条河道与至少两条其他河道相交）。
    snap_tolerance = 0.003        # ≈ 300 米
    min_streams_at_junction = 3

    print("[11/12] 修复河道拓扑：吸附交点到子流域拐点")
    snaped_stream_shp = os.path.join(output_basepath, "stream_link_snaped.shp")
    topology_info = repair_stream_topology(
        stream_shp_path, lake_shp_path,
        output_path=snaped_stream_shp, snap_tolerance=snap_tolerance,
        min_streams_at_junction=min_streams_at_junction)

    print("        吸附 {} 个端点".format(
        topology_info["snapped_count"]))
    print("        合并 {} 个河道汇合组（{} 个端点）".format(
        topology_info.get("junction_group_count", 0),
        topology_info.get("junction_endpoint_count", 0)))
    print("        删除 {} 条端点吸附后退化的短河段".format(
        topology_info.get("dropped_degenerate_count", 0)))

    # ------------------------------------------------------------------
    # 第 11.1 步：根据湖泊子流域更新河道 LINKNO
    # ------------------------------------------------------------------
    print("[11.1/12] 根据湖泊子流域更新河道 LINKNO")

    updated_count = update_stream_linkno_by_subbasin(snaped_stream_shp, lake_shp_path, overwrite, max_workers)

    print("        已更新 {} 条河道的 LINKNO 字段".format(updated_count))

    # ------------------------------------------------------------------
    # 第 11.2 步：提取河道节点，添加 FROM_NODE / TO_NODE 字段
    # ------------------------------------------------------------------
    node_shp_path = os.path.join(output_basepath, "node.shp")
    overwrite = True
    print("[11.2/12] 提取河道节点：FROM_NODE / TO_NODE")

    node_info = extract_stream_nodes(snaped_stream_shp, node_shp_path, overwrite)

    print("        节点 {} 个，河道 {} 条".format(
        node_info["node_count"], node_info["stream_count"]))


    """
    # ------------------------------------------------------------------
    # 第 12 步：根据单方向 FDR.tif 计算汇流累积量
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "FDR.tif")
    output_path = os.path.join(output_basepath, "acc.tif")

    include_self = True
    overwrite = True

    print("[12/12] 计算单方向 FDR 汇流累积量：FDR.tif")

    accumulation_info = calculate_d8_flow_accumulation(input_path, output_path, overwrite=overwrite, include_self=include_self)

    print(
        "        输出：{}（有效格点数：{}，最大累积量：{}）".format(
            output_path,
            accumulation_info["valid_cell_count"],
            accumulation_info["max_accumulation"]
        )
    )
    """
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print("错误：{}".format(error), file=sys.stderr)
        sys.exit(1)
