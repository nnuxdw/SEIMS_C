#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""将 watershed.tif 转换为带 SUBBASINID 字段的 watershed.shp。"""

from __future__ import print_function

import os
import sys

from osgeo import gdal
from osgeo import ogr
from osgeo import osr


def set_traditional_axis_order(spatial_ref):
    """校大卫: 兼容 GDAL 2/3 的 GIS 坐标轴顺序。"""
    if spatial_ref is not None and \
            hasattr(spatial_ref, "SetAxisMappingStrategy") and \
            hasattr(osr, "OAMS_TRADITIONAL_GIS_ORDER"):
        spatial_ref.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)


def delete_shapefile(path):
    """校大卫: 删除同名 Shapefile 的全部组成文件，允许重复执行脚本。"""
    if not os.path.exists(path):
        return

    driver = ogr.GetDriverByName("ESRI Shapefile")
    if driver.DeleteDataSource(path) != 0 and os.path.exists(path):
        raise IOError("无法覆盖已有 Shapefile：{}".format(path))


def watershed_tif_to_shp(input_tif, output_shp, overwrite=True):
    """校大卫: 将单波段 watershed 栅格直接矢量化为子流域面。"""
    if not os.path.isfile(input_tif):
        raise IOError("输入 watershed.tif 不存在：{}".format(input_tif))
    if os.path.normcase(os.path.abspath(input_tif)) == \
            os.path.normcase(os.path.abspath(output_shp)):
        raise ValueError("输入与输出路径不能相同")

    output_dir = os.path.dirname(os.path.abspath(output_shp))
    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)

    if os.path.exists(output_shp):
        if not overwrite:
            raise IOError("输出 Shapefile 已存在：{}".format(output_shp))
        delete_shapefile(output_shp)

    raster_ds = gdal.Open(input_tif, gdal.GA_ReadOnly)
    if raster_ds is None or raster_ds.RasterCount != 1:
        raster_ds = None
        raise IOError("无法打开单波段 watershed 栅格：{}".format(input_tif))

    spatial_ref = None
    projection = raster_ds.GetProjection()
    if projection:
        spatial_ref = osr.SpatialReference()
        spatial_ref.ImportFromWkt(projection)
        set_traditional_axis_order(spatial_ref)

    driver = ogr.GetDriverByName("ESRI Shapefile")
    vector_ds = driver.CreateDataSource(output_shp)
    if vector_ds is None:
        raster_ds = None
        raise IOError("无法创建输出 Shapefile：{}".format(output_shp))

    layer_name = os.path.splitext(os.path.basename(output_shp))[0]
    layer = vector_ds.CreateLayer(
        layer_name, srs=spatial_ref, geom_type=ogr.wkbPolygon,
        options=["ENCODING=UTF-8"]
    )
    integer_field_type = getattr(ogr, "OFTInteger64", ogr.OFTInteger)
    if layer.CreateField(ogr.FieldDefn("SUBBASINID", integer_field_type)) != 0:
        vector_ds = None
        raster_ds = None
        raise RuntimeError("无法创建 SUBBASINID 字段")

    # 校大卫: 使用栅格自身的有效像元掩膜，NoData 不会被写为矢量面。
    band = raster_ds.GetRasterBand(1)
    result = gdal.Polygonize(band, band.GetMaskBand(), layer, 0, [])
    feature_count = layer.GetFeatureCount()

    vector_ds = None
    raster_ds = None

    if result != 0 or feature_count <= 0 or not os.path.isfile(output_shp):
        raise RuntimeError("watershed 栅格矢量化失败：{}".format(input_tif))

    return feature_count


def read_boundary_union(boundary_shp):
    """校大卫: 读取并合并流域边界的全部面要素。"""
    if not os.path.isfile(boundary_shp):
        raise IOError("流域边界 Shapefile 不存在：{}".format(boundary_shp))

    boundary_ds = ogr.Open(boundary_shp, gdal.GA_ReadOnly)
    if boundary_ds is None:
        raise IOError("无法打开流域边界 Shapefile：{}".format(boundary_shp))
    layer = boundary_ds.GetLayer(0)
    if layer is None or layer.GetSpatialRef() is None:
        boundary_ds = None
        raise RuntimeError("流域边界 Shapefile 缺少可用图层或坐标系")

    boundary_srs = layer.GetSpatialRef().Clone()
    set_traditional_axis_order(boundary_srs)
    merged = None
    layer.ResetReading()
    for feature in layer:
        geometry = feature.GetGeometryRef()
        if geometry is None or geometry.IsEmpty():
            feature = None
            continue
        geometry = geometry.Clone()
        # 校大卫: Buffer(0) 仅修复自相交环，MultiPolygon 中的孤岛会完整保留。
        if not geometry.IsValid():
            geometry = geometry.Buffer(0)
        if geometry is not None and not geometry.IsEmpty():
            merged = geometry if merged is None else merged.Union(geometry)
        feature = None

    boundary_ds = None
    if merged is None or merged.IsEmpty():
        raise RuntimeError("流域边界 Shapefile 中没有有效面几何")
    if not merged.IsValid():
        merged = merged.Buffer(0)
    if merged is None or merged.IsEmpty():
        raise RuntimeError("流域边界面修复失败")
    return merged, boundary_srs


def read_line_union(line_shp):
    """校大卫: 读取长江线要素并合并为一个用于相交判断的几何。"""
    if not os.path.isfile(line_shp):
        raise IOError("长江线 Shapefile 不存在：{}".format(line_shp))

    line_ds = ogr.Open(line_shp, gdal.GA_ReadOnly)
    if line_ds is None:
        raise IOError("无法打开长江线 Shapefile：{}".format(line_shp))
    layer = line_ds.GetLayer(0)
    if layer is None or layer.GetSpatialRef() is None:
        line_ds = None
        raise RuntimeError("长江线 Shapefile 缺少可用图层或坐标系")

    line_srs = layer.GetSpatialRef().Clone()
    set_traditional_axis_order(line_srs)
    merged = None
    layer.ResetReading()
    for feature in layer:
        geometry = feature.GetGeometryRef()
        if geometry is not None and not geometry.IsEmpty():
            geometry = geometry.Clone()
            merged = geometry if merged is None else merged.Union(geometry)
        feature = None
    line_ds = None
    if merged is None or merged.IsEmpty():
        raise RuntimeError("长江线 Shapefile 中没有有效线几何")
    return merged, line_srs


def filter_subbasins_by_boundary(input_shp, boundary_shp, output_shp,
                                 river_shp=None, force_clip_ids=None,
                                 overwrite=True):
    """校大卫: 按流域边界内外面积比例筛选子流域面。"""
    if not os.path.isfile(input_shp):
        raise IOError("输入子流域 Shapefile 不存在：{}".format(input_shp))
    if os.path.normcase(os.path.abspath(input_shp)) == \
            os.path.normcase(os.path.abspath(output_shp)):
        raise ValueError("输入与输出 Shapefile 路径不能相同")
    # 校大卫: 指定 ID 不应用面积比例舍弃规则，而是强制保留其与流域边界相交的部分。
    force_clip_ids = set(int(value) for value in (force_clip_ids or []))
    if os.path.exists(output_shp):
        if not overwrite:
            raise IOError("输出 Shapefile 已存在：{}".format(output_shp))
        delete_shapefile(output_shp)

    input_ds = ogr.Open(input_shp, gdal.GA_ReadOnly)
    if input_ds is None:
        raise IOError("无法打开子流域 Shapefile：{}".format(input_shp))
    input_layer = input_ds.GetLayer(0)
    if input_layer is None or input_layer.GetSpatialRef() is None:
        input_ds = None
        raise RuntimeError("子流域 Shapefile 缺少可用图层或坐标系")
    subbasin_srs = input_layer.GetSpatialRef().Clone()
    set_traditional_axis_order(subbasin_srs)
    boundary_geometry, boundary_srs = read_boundary_union(boundary_shp)

    if not boundary_srs.IsSame(subbasin_srs):
        transform = osr.CoordinateTransformation(boundary_srs, subbasin_srs)
        if boundary_geometry.Transform(transform) not in (0, None):
            input_ds = None
            raise RuntimeError("流域边界坐标转换失败")

    river_geometry = None
    if river_shp is not None:
        river_geometry, river_srs = read_line_union(river_shp)
        if not river_srs.IsSame(subbasin_srs):
            transform = osr.CoordinateTransformation(river_srs, subbasin_srs)
            if river_geometry.Transform(transform) not in (0, None):
                input_ds = None
                raise RuntimeError("长江线坐标转换失败")

    definition = input_layer.GetLayerDefn()
    id_index = definition.GetFieldIndex("SUBBASINID")
    if id_index < 0:
        input_ds = None
        raise RuntimeError("子流域 Shapefile 缺少 SUBBASINID 字段")

    # 校大卫: 除 -1 外，同一 ID 的不连通面合并统计面积，防止单个孤岛单独触发舍弃规则。
    area_by_id = {}
    retained_minus_one_fids = set()
    has_minus_one = False
    input_layer.ResetReading()
    for feature in input_layer:
        subbasin_id = feature.GetField(id_index)
        geometry = feature.GetGeometryRef()
        if subbasin_id is None or geometry is None or geometry.IsEmpty():
            feature = None
            continue
        total_area = geometry.GetArea()
        if total_area <= 0:
            feature = None
            continue
        intersection = geometry.Intersection(boundary_geometry)
        inside_area = 0.0 if intersection is None or intersection.IsEmpty() \
            else intersection.GetArea()
        key = int(subbasin_id)
        outside_area = max(0.0, total_area - min(inside_area, total_area))
        # 校大卫: -1 是所有湖泊像元的共用编码，不是唯一子流域 ID，必须逐面判断而不能按 ID 汇总。
        # 校大卫: 完全位于边界外的 -1 湖泊面直接舍弃；部分相交时按同一 3 倍面积规则独立判断。
        if key == -1:
            has_minus_one = True
            if outside_area <= 1.0e-10 * max(1.0, total_area) or \
                    outside_area < 3.0 * inside_area:
                retained_minus_one_fids.add(feature.GetFID())
            feature = None
            continue
        if key not in area_by_id:
            area_by_id[key] = [0.0, 0.0]
        area_by_id[key][0] += total_area
        area_by_id[key][1] += min(inside_area, total_area)
        feature = None

    retained_ids = set()
    for subbasin_id, areas in area_by_id.items():
        total_area, inside_area = areas
        outside_area = max(0.0, total_area - inside_area)
        # 校大卫: 非 -1 子流域完全在边界内必保留；仅在边界外面积达到边界内面积 3 倍时舍弃。
        if outside_area <= 1.0e-10 * max(1.0, total_area) or \
                outside_area < 3.0 * inside_area:
            retained_ids.add(subbasin_id)

    driver = ogr.GetDriverByName("ESRI Shapefile")
    output_ds = driver.CreateDataSource(output_shp)
    if output_ds is None:
        input_ds = None
        raise IOError("无法创建筛选后子流域 Shapefile：{}".format(output_shp))
    output_layer = output_ds.CreateLayer(
        os.path.splitext(os.path.basename(output_shp))[0], srs=subbasin_srs,
        geom_type=ogr.wkbPolygon, options=["ENCODING=UTF-8"]
    )
    for field_index in range(definition.GetFieldCount()):
        if output_layer.CreateField(definition.GetFieldDefn(field_index)) != 0:
            output_ds = None
            input_ds = None
            raise RuntimeError("无法复制子流域属性字段")

    output_definition = output_layer.GetLayerDefn()
    retained_feature_count = 0
    truncated_feature_count = 0
    forced_clip_feature_count = 0
    input_layer.ResetReading()
    for feature in input_layer:
        subbasin_id = feature.GetField(id_index)
        geometry = feature.GetGeometryRef()
        if subbasin_id is None or geometry is None:
            feature = None
            continue
        is_forced_clip = int(subbasin_id) in force_clip_ids
        # 校大卫: -1 按单个湖泊面 FID 筛选，其他 ID 沿用按子流域汇总后的保留集合。
        if not is_forced_clip and ((int(subbasin_id) == -1 and
                feature.GetFID() not in retained_minus_one_fids) or \
                (int(subbasin_id) != -1 and
                 int(subbasin_id) not in retained_ids)):
            feature = None
            continue
        output_geometry = geometry.Clone()
        # 校大卫: 指定 ID 始终按流域边界截断并保留内部部分，不再因面积比例被舍弃。
        if is_forced_clip:
            output_geometry = output_geometry.Intersection(boundary_geometry)
            if output_geometry is None or output_geometry.IsEmpty() or \
                    output_geometry.GetArea() <= 0:
                feature = None
                continue
            forced_clip_feature_count += 1
        # 校大卫: 非指定 ID 与长江线相交时，仍按流域边界截断，只保留边界内部部分。
        elif river_geometry is not None and output_geometry.Intersects(river_geometry):
            output_geometry = output_geometry.Intersection(boundary_geometry)
            if output_geometry is None or output_geometry.IsEmpty() or \
                    output_geometry.GetArea() <= 0:
                feature = None
                continue
            truncated_feature_count += 1

        output_feature = ogr.Feature(output_definition)
        for field_index in range(definition.GetFieldCount()):
            output_feature.SetField(field_index, feature.GetField(field_index))
        output_feature.SetGeometry(output_geometry)
        if output_layer.CreateFeature(output_feature) != 0:
            output_feature = None
            output_ds = None
            input_ds = None
            raise RuntimeError("无法写入筛选后的子流域要素")
        output_feature = None
        feature = None
        retained_feature_count += 1

    output_ds = None
    input_ds = None
    if not os.path.isfile(output_shp):
        raise RuntimeError("筛选后子流域 Shapefile 输出验证失败：{}".format(output_shp))
    # 校大卫: -1 为公共湖泊编码，返回统计中按“至少保留一个 -1 面”计为一个 ID。
    retained_id_count = len(retained_ids) + (1 if retained_minus_one_fids else 0)
    total_id_count = len(area_by_id) + (1 if has_minus_one else 0)
    return (retained_id_count, retained_feature_count,
            total_id_count - retained_id_count, truncated_feature_count,
            forced_clip_feature_count)


def create_basin_boundary(subbasin_shp, output_shp, overwrite=True):
    """校大卫: 融合最终子流域面，生成单要素流域边界 Shapefile。"""
    if os.path.exists(output_shp):
        if not overwrite:
            raise IOError("流域边界 Shapefile 已存在：{}".format(output_shp))
        delete_shapefile(output_shp)

    input_ds = ogr.Open(subbasin_shp, gdal.GA_ReadOnly)
    if input_ds is None:
        raise IOError("无法打开最终子流域 Shapefile：{}".format(subbasin_shp))
    input_layer = input_ds.GetLayer(0)
    if input_layer is None or input_layer.GetSpatialRef() is None:
        input_ds = None
        raise RuntimeError("最终子流域 Shapefile 缺少可用图层或坐标系")
    spatial_ref = input_layer.GetSpatialRef().Clone()
    set_traditional_axis_order(spatial_ref)

    # 校大卫: 收集为 MultiPolygon 后一次性执行 GEOS 级联融合，避免逐面 Union 的二次耗时。
    all_subbasin_polygons = ogr.Geometry(ogr.wkbMultiPolygon)

    def append_polygon_parts(geometry):
        """校大卫: 将 Polygon、MultiPolygon 或 GeometryCollection 中的面追加到融合输入。"""
        if geometry is None or geometry.IsEmpty():
            return
        if geometry.GetGeometryName().upper() == "POLYGON":
            all_subbasin_polygons.AddGeometry(geometry.Clone())
            return
        for part_index in range(geometry.GetGeometryCount()):
            append_polygon_parts(geometry.GetGeometryRef(part_index))

    input_layer.ResetReading()
    for feature in input_layer:
        geometry = feature.GetGeometryRef()
        if geometry is not None and not geometry.IsEmpty():
            append_polygon_parts(geometry)
        feature = None
    input_ds = None
    if all_subbasin_polygons.IsEmpty():
        raise RuntimeError("没有可用于生成流域边界的子流域面")

    # 校大卫: UnionCascaded 基于空间索引树批量融合，面数较多时比逐一 Union 快得多。
    boundary_geometry = all_subbasin_polygons.UnionCascaded()
    if boundary_geometry is None or boundary_geometry.IsEmpty():
        raise RuntimeError("子流域面级联融合失败")
    if not boundary_geometry.IsValid():
        boundary_geometry = boundary_geometry.Buffer(0)
    if boundary_geometry is None or boundary_geometry.IsEmpty():
        raise RuntimeError("流域边界面融合修复失败")

    driver = ogr.GetDriverByName("ESRI Shapefile")
    output_ds = driver.CreateDataSource(output_shp)
    output_layer = output_ds.CreateLayer(
        os.path.splitext(os.path.basename(output_shp))[0], srs=spatial_ref,
        geom_type=ogr.wkbMultiPolygon, options=["ENCODING=UTF-8"]
    )
    output_layer.CreateField(ogr.FieldDefn("BASIN_ID", ogr.OFTInteger))
    output_feature = ogr.Feature(output_layer.GetLayerDefn())
    output_feature.SetField("BASIN_ID", 1)
    output_feature.SetGeometry(boundary_geometry)
    if output_layer.CreateFeature(output_feature) != 0:
        output_feature = None
        output_ds = None
        raise RuntimeError("无法写入流域边界面")
    output_feature = None
    output_ds = None
    if not os.path.isfile(output_shp):
        raise RuntimeError("流域边界 Shapefile 输出验证失败：{}".format(output_shp))
    return boundary_geometry.GetEnvelope()


def rasterize_subbasins(subbasin_shp, boundary_shp, reference_tif,
                        output_tif, overwrite=True):
    """校大卫: 将最终子流域面烧写为 SUBBASINID 栅格，范围严格等于流域边界外包范围。"""
    if os.path.exists(output_tif):
        if not overwrite:
            raise IOError("输出 TIF 已存在：{}".format(output_tif))
        gdal.GetDriverByName("GTiff").Delete(output_tif)

    boundary_ds = ogr.Open(boundary_shp, gdal.GA_ReadOnly)
    boundary_layer = boundary_ds.GetLayer(0)
    feature = boundary_layer.GetNextFeature()
    if feature is None or feature.GetGeometryRef() is None:
        boundary_ds = None
        raise RuntimeError("流域边界 Shapefile 中没有有效面")
    min_x, max_x, min_y, max_y = feature.GetGeometryRef().GetEnvelope()
    feature = None
    boundary_ds = None
    if max_x <= min_x or max_y <= min_y:
        raise RuntimeError("流域边界东西南北范围无效")

    reference_ds = gdal.Open(reference_tif, gdal.GA_ReadOnly)
    if reference_ds is None:
        raise IOError("无法打开参考 watershed.tif：{}".format(reference_tif))
    reference_gt = reference_ds.GetGeoTransform()
    projection = reference_ds.GetProjection()
    reference_ds = None
    base_xres = abs(reference_gt[1])
    base_yres = abs(reference_gt[5])
    width = max(1, int(round((max_x - min_x) / base_xres)))
    height = max(1, int(round((max_y - min_y) / base_yres)))

    output_ds = gdal.GetDriverByName("GTiff").Create(
        output_tif, width, height, 1, gdal.GDT_Int32,
        options=["TILED=YES", "COMPRESS=LZW", "BIGTIFF=IF_SAFER"]
    )
    if output_ds is None:
        raise RuntimeError("无法创建子流域 TIF：{}".format(output_tif))
    # 校大卫: 以边界面 minX/maxX/minY/maxY 精确设定外包范围，范围外预填 NoData。
    output_ds.SetGeoTransform((
        min_x, (max_x - min_x) / width, 0.0,
        max_y, 0.0, -(max_y - min_y) / height
    ))
    output_ds.SetProjection(projection)
    output_band = output_ds.GetRasterBand(1)
    output_band.SetNoDataValue(-9999)
    output_band.Fill(-9999)

    subbasin_ds = ogr.Open(subbasin_shp, gdal.GA_ReadOnly)
    subbasin_layer = subbasin_ds.GetLayer(0)
    # 校大卫: 由 SUBBASINID 属性直接烧写像元，面外区域维持 NoData。
    result = gdal.RasterizeLayer(
        output_ds, [1], subbasin_layer,
        options=["ATTRIBUTE=SUBBASINID", "ALL_TOUCHED=FALSE"]
    )
    output_band.FlushCache()
    output_ds = None
    subbasin_ds = None
    if result != 0 or not os.path.isfile(output_tif):
        raise RuntimeError("子流域 Shapefile 栅格化失败：{}".format(output_tif))
    return width, height

"""
    先按太湖流域边界和面积比例筛选子流域；
    对保留的每个子流域面，检查是否与长江线相交；
    若相交，则用流域边界裁剪该子流域面，只保留边界内部部分；
    不与长江线相交的子流域保持原始完整几何。
"""
def main():
    base_path = r"G:\program\seims\SEIMS_HAND\data\taihu\张斌提取的真实平原河网\HydroLakes"
    input_tif = os.path.join(base_path, "watershed.tif")
    base_path2 = r'G:\program\seims\SEIMS_HAND\data\taihu\taihu_preprocess\cliped_taihu_data'
    output_shp = os.path.join(base_path2, "watershed.shp")
    selected_shp = os.path.join(base_path2, "watershed_selected.shp")
    basin_boundary_shp = os.path.join(base_path2, "basin_boundary.shp")
    selected_tif = os.path.join(base_path2, "watershed_selected.tif")
    boundary_shp = (
        r"J:\G\data\太湖数据\Data\太湖流域1：25万分级流域边界数据集（2000年）"
        r"\太湖流域1：25万一级流域边界数据集（2000年）\太湖流域一级.shp"
    )
    changjiang_shp = os.path.join(base_path2, "changjiang.shp")
    # 校大卫: 在此填写需绕过面积过滤、改为按流域边界截断的 SUBBASINID，例如 [101, 205]。
    force_clip_subbasin_ids = []


    ## 把watershed.tif 转为 watershed.shp
    # count = watershed_tif_to_shp(input_tif, output_shp, overwrite=True)
    # print("转换完成：{}（{} 个面要素）".format(output_shp, count))

    ## 筛选watershed.shp，保留1.完全在流域边界内部的子流域 2.对不完全在流域内部的子流域，当边界外的面积 小于 3倍的边界内面积时，也保留
    # retained_ids, retained_features, discarded_ids, truncated_features, \
    #         forced_clip_features = \
    #     filter_subbasins_by_boundary(
    #         output_shp, boundary_shp, selected_shp,
    #         river_shp=changjiang_shp,
    #         force_clip_ids=force_clip_subbasin_ids, overwrite=True
    #     )
    # print("边界筛选完成：保留 {} 个子流域 ID、{} 个面要素；舍弃 {} 个子流域 ID。".format(
    #     retained_ids, retained_features, discarded_ids
    # ))
    # print("与长江线相交并按流域边界截断的子流域面：{} 个。".format(
    #     truncated_features
    # ))
    # print("指定 ID 强制按流域边界截断并保留的子流域面：{} 个。".format(
    #     forced_clip_features
    # ))
    # print("筛选结果：{}".format(selected_shp))

    boundary_extent = create_basin_boundary(
        selected_shp, basin_boundary_shp, overwrite=True
    )
    print("流域边界：{}（范围 minX={}, maxX={}, minY={}, maxY={}）".format(
        basin_boundary_shp, boundary_extent[0], boundary_extent[1],
        boundary_extent[2], boundary_extent[3]
    ))
    cols, rows = rasterize_subbasins(
        selected_shp, basin_boundary_shp, input_tif, selected_tif,
        overwrite=True
    )
    print("子流域栅格：{}（{} 列 × {} 行；边界外为 NoData）".format(
        selected_tif, cols, rows
    ))


if __name__ == "__main__":
    main()
