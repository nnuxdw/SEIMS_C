#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""对已有流域 DEM 进行保守水文整饰，修复河道和湖泊中的 NoData。

用法：
    python hydrocondition_dem.py <basin.tif> <basin_elv.tif> <basin_Stream.tif>
        <output_dem.tif> [lake_mask.tif] [burn_depth_m]

lake_mask.tif 可使用与 DEM 同网格、以湖泊 ID 编码的栅格；缺省时只填补 NoData，
不进行湖面展平。输出 DEM 供后续重新计算 D8 流向、汇流累积量、河网和子流域使用。
"""

from __future__ import print_function

import os
import sys

import numpy as np
from osgeo import gdal, osr


NODATA = -9999.0
BLOCK_ROWS = 512

# 校大卫: 不传命令行参数时使用此处配置；按实际项目修改 PROJECT_PATH 与可选 LAKE_PATH。
PROJECT_PATH = r'G:\program\seims\SEIMS_HAND\data\taihu_plain_network'
SPATIAL_PATH = os.path.join(PROJECT_PATH, 'data_prepare', 'spatial')
BASIN_PATH = os.path.join(SPATIAL_PATH, 'basin.tif')
DEM_PATH = r"G:\program\seims\SEIMS_HAND\data\taihu\taihu_preprocess\cliped_taihu_data\basin_elv.tif"
STREAM_PATH = os.path.join(SPATIAL_PATH, 'basin_Stream.tif')
OUTPUT_PATH = os.path.join(SPATIAL_PATH, 'basin_elv.tif')
# 校大卫: 若没有与 DEM 同网格的湖泊 ID 栅格，请保持 None；存在时填写例如 os.path.join(SPATIAL_PATH, 'rlake.tif')。
LAKE_PATH = r'G:\program\seims\SEIMS_HAND\data\taihu\张斌提取的真实平原河网\HydroLakes\Taihu_lakes.tif'
# 校大卫: 河道与湖泊统一在周边地形/岸线估算高程基础上下切 2 m，使二者均形成明确的汇水洼地。
BURN_DEPTH_M = 2.0


def open_raster(path, description):
    dataset = gdal.Open(path, gdal.GA_ReadOnly)
    if dataset is None:
        raise RuntimeError('无法打开{}: {}'.format(description, path))
    return dataset


def same_grid(reference, candidate):
    # 校大卫: 整饰 DEM 只能在完全相同的像元网格上叠加流域、河网和湖泊掩膜。
    if (reference.RasterXSize, reference.RasterYSize) != (candidate.RasterXSize, candidate.RasterYSize):
        return False
    if any(abs(a - b) > 1e-12
           for a, b in zip(reference.GetGeoTransform(), candidate.GetGeoTransform())):
        return False
    ref_srs = osr.SpatialReference()
    cur_srs = osr.SpatialReference()
    ref_srs.ImportFromWkt(reference.GetProjection())
    cur_srs.ImportFromWkt(candidate.GetProjection())
    return bool(ref_srs.IsSame(cur_srs))


def data_mask(data, nodata, background=None):
    # 校大卫: 河网与湖泊 ID 允许为负值；只排除其明确 NoData 和约定背景值，不能按大于零筛选。
    mask = np.ones(data.shape, dtype=bool) if nodata is None else data != nodata
    if background is not None:
        mask &= data != background
    return mask


def align_lake_to_basin(lake_path, basin_path, basin_ds):
    """将分类湖泊栅格对齐到 basin.tif，并清除流域外像元。"""
    lake_name = os.path.splitext(os.path.basename(lake_path))[0]
    aligned_path = os.path.join(os.path.dirname(basin_path),
                                lake_name + '_aligned_to_basin.tif')
    geotransform = basin_ds.GetGeoTransform()
    min_x = geotransform[0]
    max_y = geotransform[3]
    max_x = min_x + basin_ds.RasterXSize * geotransform[1]
    min_y = max_y + basin_ds.RasterYSize * geotransform[5]
    source_ds = open_raster(lake_path, '原始湖泊掩膜')
    source_band = source_ds.GetRasterBand(1)
    source_nodata = source_band.GetNoDataValue()
    source_ds = None
    if os.path.exists(aligned_path):
        # 校大卫: 对齐栅格是自动派生的中间成果，重跑时仅覆盖该文件，绝不修改用户提供的原始湖泊栅格。
        gdal.GetDriverByName('GTiff').Delete(aligned_path)
    # 校大卫: 湖泊 ID 属于分类编码，必须用最近邻重采样，保留负值 ID，不能使用双线性等连续型插值。
    options = gdal.WarpOptions(
        format='GTiff', outputBounds=(min_x, min_y, max_x, max_y),
        width=basin_ds.RasterXSize, height=basin_ds.RasterYSize,
        dstSRS=basin_ds.GetProjection(), resampleAlg=gdal.GRA_NearestNeighbour,
        srcNodata=source_nodata, dstNodata=0,
        creationOptions=['COMPRESS=LZW', 'TILED=YES', 'BIGTIFF=IF_SAFER'])
    aligned_ds = gdal.Warp(aligned_path, lake_path, options=options)
    if aligned_ds is None:
        raise RuntimeError('无法将湖泊栅格对齐到 basin.tif: {}'.format(lake_path))
    lake_band = aligned_ds.GetRasterBand(1)
    basin_band = basin_ds.GetRasterBand(1)
    basin_nodata = basin_band.GetNoDataValue()
    for yoff in range(0, basin_ds.RasterYSize, BLOCK_ROWS):
        rows = min(BLOCK_ROWS, basin_ds.RasterYSize - yoff)
        lake_data = lake_band.ReadAsArray(0, yoff, basin_ds.RasterXSize, rows)
        basin_data = basin_band.ReadAsArray(0, yoff, basin_ds.RasterXSize, rows)
        # 校大卫: 对齐后仍以 basin.tif 为唯一建模边界，流域外湖泊像元写为背景值 0，不参与湖面整饰。
        lake_data[~data_mask(basin_data, basin_nodata)] = 0
        lake_band.WriteArray(lake_data, 0, yoff)
    lake_band.SetNoDataValue(0)
    lake_band.FlushCache()
    aligned_ds.FlushCache()
    aligned_ds = None
    print('湖泊网格不一致，已按 basin.tif 裁剪并对齐：{}'.format(aligned_path))
    return aligned_path


def make_output(basin_ds, dem_ds, output_path):
    if os.path.exists(output_path):
        print('输出文件已存在，正在删除: {}'.format(output_path))
        os.remove(output_path)
    driver = gdal.GetDriverByName('GTiff')
    output = driver.Create(
        output_path, dem_ds.RasterXSize, dem_ds.RasterYSize, 1, gdal.GDT_Float32,
        options=['COMPRESS=LZW', 'TILED=YES', 'BIGTIFF=IF_SAFER'])
    if output is None:
        raise RuntimeError('无法创建输出 DEM: {}'.format(output_path))
    output.SetGeoTransform(dem_ds.GetGeoTransform())
    output.SetProjection(dem_ds.GetProjection())
    band = output.GetRasterBand(1)
    band.SetNoDataValue(NODATA)
    basin_band = basin_ds.GetRasterBand(1)
    dem_band = dem_ds.GetRasterBand(1)
    basin_nodata = basin_band.GetNoDataValue()
    dem_nodata = dem_band.GetNoDataValue()
    filled_void_count = 0
    for yoff in range(0, dem_ds.RasterYSize, BLOCK_ROWS):
        rows = min(BLOCK_ROWS, dem_ds.RasterYSize - yoff)
        basin_data = basin_band.ReadAsArray(0, yoff, dem_ds.RasterXSize, rows)
        dem_data = dem_band.ReadAsArray(0, yoff, dem_ds.RasterXSize, rows).astype(np.float32)
        basin_valid = data_mask(basin_data, basin_nodata)
        dem_valid = data_mask(dem_data, dem_nodata)
        filled_void_count += int(np.count_nonzero(basin_valid & ~dem_valid))
        output_data = np.full(dem_data.shape, NODATA, dtype=np.float32)
        output_data[basin_valid & dem_valid] = dem_data[basin_valid & dem_valid]
        # 校大卫: 流域内水体 NoData 暂保留为输出 NoData，随后由 GDAL 插值；流域外始终保持 NoData。
        band.WriteArray(output_data, 0, yoff)
    band.FlushCache()
    return output, filled_void_count


def calculate_lake_levels(output_ds, basin_ds, lake_ds):
    """按湖泊 ID 汇总其岸线邻域高程的均值，返回 {lake_id: water_level}。"""
    output_band = output_ds.GetRasterBand(1)
    basin_band = basin_ds.GetRasterBand(1)
    lake_band = lake_ds.GetRasterBand(1)
    basin_nodata = basin_band.GetNoDataValue()
    lake_nodata = lake_band.GetNoDataValue()
    level_sums = {}
    width = output_ds.RasterXSize
    height = output_ds.RasterYSize
    # 校大卫: 用一像元缓冲带抽取湖岸高程，避免读取整幅太湖栅格造成过高内存占用。
    for yoff in range(0, height, BLOCK_ROWS):
        rows = min(BLOCK_ROWS, height - yoff)
        halo_top = max(0, yoff - 1)
        halo_bottom = min(height, yoff + rows + 1)
        halo_rows = halo_bottom - halo_top
        lake_halo = lake_band.ReadAsArray(0, halo_top, width, halo_rows)
        dem_halo = output_band.ReadAsArray(0, halo_top, width, halo_rows)
        basin_halo = basin_band.ReadAsArray(0, halo_top, width, halo_rows)
        # 校大卫: 四周补一像元无效边框，使湖泊贴近任一栅格边缘时仍可按八邻域安全提取岸线高程。
        lake_halo_padded = np.pad(lake_halo, ((1, 1), (1, 1)), mode='constant', constant_values=0)
        dem_halo_padded = np.pad(dem_halo, ((1, 1), (1, 1)), mode='constant', constant_values=NODATA)
        basin_halo_padded = np.pad(basin_halo, ((1, 1), (1, 1)), mode='constant', constant_values=NODATA)
        lake_valid_halo = data_mask(lake_halo, lake_nodata, background=0)
        basin_valid_halo = data_mask(basin_halo, basin_nodata)
        core_start = yoff - halo_top
        core_end = core_start + rows
        core_lake_ids = lake_halo[core_start:core_end, :]
        core_lake_mask = lake_valid_halo[core_start:core_end, :]
        for drow, dcol in ((-1, -1), (-1, 0), (-1, 1), (0, -1),
                           (0, 1), (1, -1), (1, 0), (1, 1)):
            row_start = core_start + 1 + drow
            row_end = core_end + 1 + drow
            neighbour_lake = data_mask(
                lake_halo_padded[row_start:row_end, 1 + dcol:1 + dcol + width],
                lake_nodata, background=0)
            neighbour_basin = data_mask(
                basin_halo_padded[row_start:row_end, 1 + dcol:1 + dcol + width],
                basin_nodata)
            neighbour_dem = dem_halo_padded[row_start:row_end, 1 + dcol:1 + dcol + width]
            # 校大卫: 仅使用湖外、流域内且已有高程的邻域像元估算湖面，避免湖内插值结果反向影响水位。
            boundary = (core_lake_mask & ~neighbour_lake & neighbour_basin &
                        (neighbour_dem != NODATA))
            if not np.any(boundary):
                continue
            ids = core_lake_ids[boundary].astype(np.int64)
            values = neighbour_dem[boundary].astype(np.float64)
            for lake_id in np.unique(ids):
                selected = values[ids == lake_id]
                total, count = level_sums.get(int(lake_id), (0.0, 0))
                level_sums[int(lake_id)] = (total + float(selected.sum()),
                                             count + int(selected.size))
    return {lake_id: total / count for lake_id, (total, count) in level_sums.items() if count > 0}


def flatten_lakes(output_ds, basin_ds, lake_ds, lake_levels, burn_depth):
    output_band = output_ds.GetRasterBand(1)
    basin_band = basin_ds.GetRasterBand(1)
    lake_band = lake_ds.GetRasterBand(1)
    basin_nodata = basin_band.GetNoDataValue()
    lake_nodata = lake_band.GetNoDataValue()
    flattened_count = 0
    for yoff in range(0, output_ds.RasterYSize, BLOCK_ROWS):
        rows = min(BLOCK_ROWS, output_ds.RasterYSize - yoff)
        output_data = output_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        basin_data = basin_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        lake_data = lake_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        basin_valid = data_mask(basin_data, basin_nodata)
        lake_mask = data_mask(lake_data, lake_nodata, background=0) & basin_valid
        if np.any(lake_mask):
            for lake_id in np.unique(lake_data[lake_mask].astype(np.int64)):
                if int(lake_id) not in lake_levels:
                    continue
                cells = lake_mask & (lake_data == lake_id)
                # 校大卫: 湖面在岸线估算水位基础上下切与河道相同的深度，避免湖泊仍高于入湖河道。
                output_data[cells] = lake_levels[int(lake_id)] - burn_depth
                flattened_count += int(np.count_nonzero(cells))
        output_band.WriteArray(output_data, 0, yoff)
    output_band.FlushCache()
    return flattened_count


def burn_streams(output_ds, basin_ds, stream_ds, burn_depth):
    output_band = output_ds.GetRasterBand(1)
    basin_band = basin_ds.GetRasterBand(1)
    stream_band = stream_ds.GetRasterBand(1)
    basin_nodata = basin_band.GetNoDataValue()
    stream_nodata = stream_band.GetNoDataValue()
    burned_count = 0
    if burn_depth <= 0:
        return burned_count
    for yoff in range(0, output_ds.RasterYSize, BLOCK_ROWS):
        rows = min(BLOCK_ROWS, output_ds.RasterYSize - yoff)
        output_data = output_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        basin_data = basin_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        stream_data = stream_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        basin_valid = data_mask(basin_data, basin_nodata)
        stream_mask = data_mask(stream_data, stream_nodata, background=0) & basin_valid
        stream_mask &= output_data != NODATA
        # 校大卫: 只在遥感河网像元下切固定小深度，不改变河网几何；下游单调约束仍应由河网拓扑/FDR 重建步骤完成。
        output_data[stream_mask] -= burn_depth
        burned_count += int(np.count_nonzero(stream_mask))
        output_band.WriteArray(output_data, 0, yoff)
    output_band.FlushCache()
    return burned_count


def count_remaining_voids(output_ds, basin_ds):
    output_band = output_ds.GetRasterBand(1)
    basin_band = basin_ds.GetRasterBand(1)
    basin_nodata = basin_band.GetNoDataValue()
    remaining = 0
    for yoff in range(0, output_ds.RasterYSize, BLOCK_ROWS):
        rows = min(BLOCK_ROWS, output_ds.RasterYSize - yoff)
        output_data = output_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        basin_data = basin_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        remaining += int(np.count_nonzero(data_mask(basin_data, basin_nodata) &
                                            (output_data == NODATA)))
    return remaining


def restore_exterior_nodata(output_ds, basin_ds):
    """恢复流域外 NoData，防止 FillNodata 越过流域边界外推。"""
    output_band = output_ds.GetRasterBand(1)
    basin_band = basin_ds.GetRasterBand(1)
    basin_nodata = basin_band.GetNoDataValue()
    for yoff in range(0, output_ds.RasterYSize, BLOCK_ROWS):
        rows = min(BLOCK_ROWS, output_ds.RasterYSize - yoff)
        output_data = output_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        basin_data = basin_band.ReadAsArray(0, yoff, output_ds.RasterXSize, rows)
        # 校大卫: GDAL FillNodata 不识别 basin.tif 的建模边界，插值后必须将流域外像元恢复为 NoData。
        output_data[~data_mask(basin_data, basin_nodata)] = NODATA
        output_band.WriteArray(output_data, 0, yoff)
    output_band.FlushCache()


def resolve_parameters(args):
    # 校大卫: 支持脚本内固定配置和外部命令行两种方式，完整命令行参数始终优先。
    if len(args) == 1:
        return BASIN_PATH, DEM_PATH, STREAM_PATH, OUTPUT_PATH, LAKE_PATH, BURN_DEPTH_M
    if len(args) not in (5, 6, 7):
        raise SystemExit('用法: hydrocondition_dem.py <basin.tif> <basin_elv.tif> <basin_Stream.tif> <output_dem.tif> [lake_mask.tif] [burn_depth_m]；不传参数则读取脚本顶部配置。')
    basin_path, dem_path, stream_path, output_path = args[1:5]
    lake_path = args[5] if len(args) >= 6 else None
    burn_depth = float(args[6]) if len(args) == 7 else BURN_DEPTH_M
    return basin_path, dem_path, stream_path, output_path, lake_path, burn_depth


def main(args):
    basin_path, dem_path, stream_path, output_path, lake_path, burn_depth = resolve_parameters(args)
    if os.path.abspath(dem_path) == os.path.abspath(output_path):
        raise RuntimeError('输出路径不能覆盖原始 basin_elv.tif。')

    basin_ds = open_raster(basin_path, '流域掩膜')
    dem_ds = open_raster(dem_path, '原始 DEM')
    stream_ds = open_raster(stream_path, '遥感河网栅格')
    lake_ds = open_raster(lake_path, '湖泊掩膜') if lake_path else None
    # 校大卫: 外部湖泊数据常与建模网格不一致；先按 basin.tif 裁剪和最近邻对齐，再参与分类湖泊整饰。
    if lake_ds is not None and not same_grid(basin_ds, lake_ds):
        lake_ds = None
        lake_path = align_lake_to_basin(lake_path, basin_path, basin_ds)
        lake_ds = open_raster(lake_path, '已对齐湖泊掩膜')
    for description, dataset in (('DEM', dem_ds), ('河网', stream_ds), ('湖泊', lake_ds)):
        if dataset is not None and not same_grid(basin_ds, dataset):
            raise RuntimeError('{}与 basin.tif 网格不一致，停止整饰。'.format(description))

    output_ds, void_count = make_output(basin_ds, dem_ds, output_path)
    output_band = output_ds.GetRasterBand(1)
    # 校大卫: 先只填补流域内原始 NoData；不修改流域外 NoData，从而维持建模边界。
    result = gdal.FillNodata(output_band, None, 0, 0)
    if result != gdal.CE_None:
        raise RuntimeError('GDAL FillNodata 执行失败。')
    output_band.FlushCache()
    restore_exterior_nodata(output_ds, basin_ds)

    flattened_count = 0
    if lake_ds is not None:
        lake_levels = calculate_lake_levels(output_ds, basin_ds, lake_ds)
        flattened_count = flatten_lakes(output_ds, basin_ds, lake_ds, lake_levels, burn_depth)
        print('已按 {} 个湖泊 ID 估算、展平并下切 {} m，共处理 {} 个湖泊像元。'.format(
            len(lake_levels), burn_depth, flattened_count))
    burned_count = burn_streams(output_ds, basin_ds, stream_ds, burn_depth)
    remaining_voids = count_remaining_voids(output_ds, basin_ds)
    output_ds.FlushCache()
    output_ds = None
    lake_ds = None
    stream_ds = None
    dem_ds = None
    basin_ds = None
    print('完成：流域内原始 NoData 像元={}，河网烧蚀像元={}，流域内剩余 NoData={}'.format(
        void_count, burned_count, remaining_voids))
    print('输出整饰 DEM: {}'.format(output_path))
    if remaining_voids > 0:
        raise RuntimeError('整饰后流域内仍有 NoData，不能用于重新计算流向。')


if __name__ == '__main__':
    main(sys.argv)
