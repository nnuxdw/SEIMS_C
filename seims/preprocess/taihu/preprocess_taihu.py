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
from collections import deque

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

    # ---------- 空间匹配湖泊 ----------
    unique_lake_ids = np.unique(hl_data[hl_valid])
    unique_lake_ids = unique_lake_ids.astype(np.int64)

    # 跟踪已被分配到某个湖泊子流域的单元格，防止重复分配
    assigned = np.zeros((n_rows, n_cols), dtype=np.bool_)
    matched_lake_ids = []

    for lake_id in unique_lake_ids:
        lake_id = int(lake_id)
        hl_lake_mask = (hl_data == lake_id) & hl_valid

        # hydrolake 湖泊与 Watershed 湖泊 (-1) 的重叠区域
        overlap_mask = hl_lake_mask & ws_lake_mask & ws_valid & (~assigned)

        if not np.any(overlap_mask):
            # 该湖泊在 Watershed 中没有对应的湖泊区，跳过
            continue

        # 坡面栅格：Watershed 中值等于湖泊 ID 负值的单元格
        # （Watershed 编码规则：坡面值 = -hydrolake_id）
        slope_mask = (ws_data == -lake_id) & ws_valid & (~assigned)

        # 合并坡面 + 湖泊 → 湖泊子流域，统一用 -lake_id 作为栅格值
        # 这样坡面栅格（本身就是 -lake_id）和湖泊栅格值完全一致
        combined_mask = slope_mask | overlap_mask

        if not np.any(combined_mask):
            continue

        output_data[combined_mask] = -lake_id
        assigned[combined_mask] = True
        matched_lake_ids.append(lake_id)

    if len(matched_lake_ids) == 0:
        ws_ds = None
        hl_ds = None
        raise RuntimeError("没有匹配到任何湖泊子流域")

    # ---------- BFS 区域生长：将剩余 -1 湖泊栅格合并到相邻子流域 ----------
    # 主匹配循环只处理了与 hydrolake 重叠的湖泊栅格。但很多 -1 湖泊栅格
    # 虽属同一子流域（被相同坡面栅格包围），却未被 hydrolake 覆盖，
    # 会在 TIF 中留下 NoData 缝隙，导致 Polygonize 无法将湖泊与坡面融合成
    # 一个面要素。这里用 BFS 从已匹配的单元格向邻接的 -1 单元格扩散，
    # 保证湖泊-坡面栅格值完全一致。
    #
    # 注意：此时已匹配单元格的值为 -lake_id（负值），剩余湖泊栅格值为 -1。
    # 使用独立的 visited 数组避免 lake_id==1 时 -lake_id==-1 导致的重复入队。

    visited = assigned.copy()
    matched_indices = np.where(assigned)
    queue = deque()

    for r, c in zip(matched_indices[0], matched_indices[1]):
        queue.append((int(r), int(c)))

    # 8 邻域方向
    _directions = [(-1, -1), (-1, 0), (-1, 1),
                   (0, -1),           (0, 1),
                   (1, -1),  (1, 0),  (1, 1)]

    while queue:
        r, c = queue.popleft()
        current_id = output_data[r, c]

        for dr, dc in _directions:
            nr, nc = r + dr, c + dc
            if 0 <= nr < n_rows and 0 <= nc < n_cols:
                if (not visited[nr, nc]) and (output_data[nr, nc] == -1):
                    output_data[nr, nc] = current_id
                    visited[nr, nc] = True
                    queue.append((nr, nc))

    # ---------- 临时编号：从原始最大子流域 ID + 1 开始 ----------
    # 找到原始 watershed 中非湖泊子流域（正值）的最大 ID，
    # 湖泊子流域的临时 ID 从 max_existing_id + 1 开始递增，
    # 避免与已有子流域 ID 冲突。最终统一编号在后续步骤中完成。
    positive_mask = (ws_data > 0) & ws_valid
    if np.any(positive_mask):
        max_existing_id = int(np.max(ws_data[positive_mask]))
    else:
        max_existing_id = 0

    temp_id_map = {}
    sorted_lake_ids = sorted(set(matched_lake_ids))
    for i, lake_id in enumerate(sorted_lake_ids):
        temp_id = max_existing_id + i + 1
        temp_id_map[lake_id] = temp_id

    # 重映射 output_data 中的湖泊子流域值（-lake_id → temp_id）
    for lake_id, temp_id in temp_id_map.items():
        output_data[output_data == -lake_id] = temp_id

    # 将剩余所有 ID < 0 的栅格设为 NoData
    # 这些可能是未匹配的湖泊栅格（-1）或其他残留的负值子流域，
    # 均不纳入湖泊子流域的输出范围。
    negative_mask = (output_data < 0) & ws_valid
    negative_count = int(np.sum(negative_mask))
    if negative_count > 0:
        print("        警告：发现 {} 个 ID<0 的残留栅格，已设为 NoData".format(
            negative_count))
        output_data[negative_mask] = -9999

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


def renumber_subbasins(tif_path, shp_path, overwrite=True):
    """
    统一重新编号所有子流域：读取子流域 TIF 中所有非 NoData 的唯一值，
    排序后映射为从 1 开始的连续正整数，同步更新 TIF 和 SHP 的 SUBBASINID。

    步骤：
        1. 读取 TIF，获取所有唯一子流域 ID（非 NoData）。
        2. 将所有 ID 排序并映射为 1, 2, 3, ...。
        3. 用新 ID 重写 TIF 栅格。
        4. 更新 SHP 中每个要素的 SUBBASINID 字段。
    """
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

    # ---------- 重写 TIF ----------
    # 先转为 int32，再进行 ID 映射；用 np.isclose 避免浮点精度问题
    # 使用原始副本作为判断条件，避免 0 -> 1 后又被 1 -> 2 覆盖
    source_data = data.astype(np.int32)
    new_data = source_data.copy()

    for old_id, new_id in id_mapping.items():
        new_data[source_data == old_id] = new_id

    # 已完成读取和计算，释放输入文件句柄后再删除旧 tif
    band = None
    ds = None

    # 此时 tif_path 才是待覆盖的输出文件
    delete_existing_raster(tif_path, overwrite)

    # 写回 TIF
    driver = gdal.GetDriverByName("GTiff")
    out_ds = driver.Create(
        tif_path,
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
        raise IOError("无法创建输出栅格：{}".format(tif_path))

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
        tif_path, len(sorted_ids)))

    # ---------- 更新 SHP ----------
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

    return {
        "total_subbasins": len(sorted_ids),
        "id_mapping": id_mapping,
    }


def update_stream_linkno_by_subbasin(stream_path, subbasin_path,
                                      overwrite=True):
    """
    根据湖泊子流域面更新河道 Shapefile 的 LINKNO 字段。

    对每条河道段，计算其与所有子流域面的空间相交关系，
    将 LINKNO 设置为相交长度最大的子流域的 SUBBASINID。
    若河道完全不在任何子流域内，则 LINKNO 设为 0。
    """
    ensure_file(stream_path, "河道 Shapefile")
    ensure_file(subbasin_path, "湖泊子流域 Shapefile")

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

    # 读取所有河道要素及字段定义
    orig_defn = st_layer.GetLayerDefn()
    orig_field_count = orig_defn.GetFieldCount()
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
        stream_features.append({
            "geometry": geometry.Clone(),
            "field_values": field_values,
        })

    st_ds = None

    if len(stream_features) == 0:
        raise RuntimeError("河道 Shapefile 中没有有效线要素")

    # ---------- 在内存中处理 LINKNO ----------
    for sf in stream_features:
        stream_geom = sf["geometry"]

        best_id = 0
        best_length = 0.0

        # 检测每个子流域与河道的相交长度
        for sb in subbasins:
            try:
                intersection = stream_geom.Intersection(sb["geometry"])
            except Exception:
                # 子流域面已在读取阶段修复；个别无效河道跳过该子流域面。
                # 不调用 MakeValid()，以兼容旧版 GDAL Python 绑定。
                continue
            if intersection is None or intersection.IsEmpty():
                continue

            length = intersection.Length()
            if length > best_length:
                best_length = length
                best_id = sb["subbasin_id"]

        # 若相交检测全部为空，尝试用“点在面内”检测（取首点）
        if best_id == 0:
            first_point = None
            geom_name = stream_geom.GetGeometryName().upper()
            if "MULTILINESTRING" in geom_name:
                if stream_geom.GetGeometryCount() > 0:
                    first_line = stream_geom.GetGeometryRef(0)
                    if first_line.GetPointCount() > 0:
                        first_point = first_line.GetPoint(0)
            elif stream_geom.GetPointCount() > 0:
                first_point = stream_geom.GetPoint(0)

            if first_point is not None:
                point_geom = ogr.Geometry(ogr.wkbPoint)
                point_geom.AddPoint(first_point[0], first_point[1])
                for sb in subbasins:
                    if point_geom.Within(sb["geometry"]):
                        best_id = sb["subbasin_id"]
                        break

        # 更新 field_values 中的 LINKNO
        if linkno_idx >= 0:
            sf["field_values"][linkno_idx] = best_id
        else:
            sf["field_values"].append(best_id)

    # ---------- 删除原文件并写出新文件 ----------
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
        geom_type=ogr.wkbMultiLineString,
        options=["ENCODING=UTF-8"]
    )

    if out_layer is None:
        out_ds = None
        raise RuntimeError("无法创建输出河道图层")

    # 复制原字段
    for field_idx in range(orig_field_count):
        field_defn = orig_defn.GetFieldDefn(field_idx)
        if out_layer.CreateField(field_defn) != 0:
            out_ds = None
            raise RuntimeError("无法复制河道字段：{}".format(
                field_defn.GetName()))

    # 添加 LINKNO 字段（如果之前不存在）
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

    for sf in stream_features:
        new_feature = ogr.Feature(out_defn)

        for i in range(orig_field_count):
            if i < len(sf["field_values"]):
                val = sf["field_values"][i]
                if val is not None:
                    new_feature.SetField(i, val)

        # 设置 LINKNO
        new_feature.SetField(linkno_idx, sf["field_values"][linkno_idx])
        new_feature.SetGeometry(sf["geometry"])

        if out_layer.CreateFeature(new_feature) != 0:
            new_feature = None
            out_ds = None
            raise RuntimeError("写入更新后的河道要素失败")

        if sf["field_values"][linkno_idx] == 0:
            no_match_count += 1

        new_feature = None
        out_count += 1

    out_layer.SyncToDisk()
    out_ds = None

    print(
        "        更新河道 LINKNO：共 {} 条，其中 {} 条未匹配到子流域（LINKNO=0）".format(
            out_count,
            no_match_count
        )
    )

    return out_count


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

    # info = clip_raster(input_path,output_path,mask_path,mask_geometry,mask_srs,overwrite)
    # 第 2 步：subbasin.tif 转为融合后的子流域面
    # ------------------------------------------------------------------
    input_path = os.path.join(output_basepath, "subbasin.tif")
    output_path = os.path.join(output_basepath, "subbasin.shp")
    overwrite = True

    print("[2/12] 栅格转子流域面：subbasin.tif")

    # feature_count = raster_to_dissolved_subbasin_shp(input_path,output_path,overwrite)

    # print(
    #     "        输出：{}（{} 个子流域面要素）".format(
    #         output_path,
    #         feature_count
    #     )
    # )

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

    # info = clip_raster(input_path,output_path,mask_path,mask_geometry,mask_srs,overwrite)


    # ------------------------------------------------------------------
    # 第 4 步：生成湖泊子流域
    # ------------------------------------------------------------------
    watershed_path = os.path.join(output_basepath, "subbasin.tif")
    lake_tif_path = os.path.join(output_basepath, "lake_subbasin.tif")
    lake_shp_path = os.path.join(output_basepath, "lake_subbasin.shp")
    overwrite = True

    print("[4/12] 生成湖泊子流域：subbasin.tif + hydrolake.tif")

    # lake_info = generate_lake_subbasin(watershed_path,cliped_hydrlakes_path,lake_tif_path,lake_shp_path,overwrite)

    # print(
    #     "        匹配到 {} 个湖泊子流域，最大已有子流域 ID={}，临时 ID 起始值={}".format(
    #         lake_info["matched_lake_count"],
    #         lake_info["max_existing_id"],
    #         min(lake_info["temp_ids"]) if lake_info["temp_ids"] else "N/A"
    #     )
    # )

    # ------------------------------------------------------------------
    # 第 5 步：统一重新编号所有子流域（从 1 开始递增）
    # ------------------------------------------------------------------
    print("[5/12] 统一重新编号所有子流域：lake_subbasin.tif + lake_subbasin.shp")

    # renumber_info = renumber_subbasins(lake_tif_path,lake_shp_path,overwrite)

    # print(
    #     "        共 {} 个子流域已统一编号（从 1 开始）".format(
    #         renumber_info["total_subbasins"]
    #     )
    # )

    # ------------------------------------------------------------------
    # 第 6 步：裁剪 burnedfillDEM.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "burnedfillDEM.tif")
    output_path = os.path.join(output_basepath, "dem.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[6/12] 裁剪栅格：burnedfillDEM.tif")

    # info = clip_raster(input_path,output_path,mask_path,mask_geometry,mask_srs,overwrite)

    # ------------------------------------------------------------------
    # 第 7 步：裁剪 Final_fdr.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "Final_fdr.tif")
    output_path = os.path.join(output_basepath, "Final_flow_dir.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[7/12] 裁剪栅格：Final_fdr.tif")

    # info = clip_raster(input_path,output_path,mask_path,mask_geometry, mask_srs,overwrite)

    # ------------------------------------------------------------------
    # 第 8 步：裁剪单方向 fdr.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "fdr.tif")
    output_path = os.path.join(output_basepath, "flow_dir.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[8/12] 裁剪栅格：fdr.tif")

    # info = clip_raster(input_path,output_path,mask_path,mask_geometry,mask_srs,overwrite)

    # ------------------------------------------------------------------
    # 第 9 步：裁剪 stream.tif
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "stream.tif")
    output_path = os.path.join(output_basepath, "stream_link.tif")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[9/12] 裁剪栅格：stream.tif")

    # info = clip_raster(input_path,output_path,mask_path,mask_geometry,mask_srs,overwrite)

    # ------------------------------------------------------------------
    # 第 10 步：裁剪 stream_split.shp
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "stream_split.shp")
    output_path = os.path.join(output_basepath, "stream_link.shp")
    overwrite = True

    ensure_different_paths(input_path, output_path)
    ensure_output_directory(output_path)

    print("[10/12] 裁剪河网：stream_split.shp")

    # feature_count = clip_stream_vector(input_path,output_path,mask_geometry,mask_srs,overwrite)

    # print(
    #     "        输出：{}（{} 条河道要素）".format(
    #         output_path,
    #         feature_count
    #     )
    # )

    # ------------------------------------------------------------------
    # 第 11 步：根据湖泊子流域更新河道 LINKNO 字段
    # ------------------------------------------------------------------
    lake_shp_path = os.path.join(output_basepath, "lake_subbasin.shp")
    stream_shp_path = os.path.join(output_basepath, "stream_link.shp")

    print("[11/12] 根据湖泊子流域更新河道 LINKNO")

    updated_count = update_stream_linkno_by_subbasin(
        stream_shp_path,
        lake_shp_path,
        overwrite
    )

    print(
        "        已更新 {} 条河道的 LINKNO 字段".format(updated_count)
    )

    # ------------------------------------------------------------------
    # 第 12 步：根据单方向 FDR.tif 计算汇流累积量
    # ------------------------------------------------------------------
    input_path = os.path.join(input_basepath, "FDR.tif")
    output_path = os.path.join(output_basepath, "acc.tif")

    include_self = True
    overwrite = True

    print("[12/12] 计算单方向 FDR 汇流累积量：FDR.tif")

    accumulation_info = calculate_d8_flow_accumulation(
        input_path,
        output_path,
        overwrite=overwrite,
        include_self=include_self
    )

    print(
        "        输出：{}（有效格点数：{}，最大累积量：{}）".format(
            output_path,
            accumulation_info["valid_cell_count"],
            accumulation_info["max_accumulation"]
        )
    )

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print("错误：{}".format(error), file=sys.stderr)
        sys.exit(1)
