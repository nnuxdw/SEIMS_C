#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""检查 stream_link.tif 与 flow_dir.tif 的网格和河网流向有效性。

用法：
    python check_stream_flowdir.py <stream_link.tif> <flow_dir.tif> [invalid_points.csv]

第三个参数缺省时，在当前目录输出 stream_flowdir_invalid_points.csv。
"""

from __future__ import print_function

import csv
import math
import os
import sys

import numpy as np
from osgeo import gdal, osr


BLOCK_ROWS = 512
ESRI_D8_CODES = (1, 2, 4, 8, 16, 32, 64, 128)


def open_raster(path, description):
    dataset = gdal.Open(path, gdal.GA_ReadOnly)
    if dataset is None:
        raise RuntimeError('无法打开{}：{}'.format(description, path))
    return dataset


def same_grid(reference_ds, candidate_ds):
    if (reference_ds.RasterXSize, reference_ds.RasterYSize) != \
            (candidate_ds.RasterXSize, candidate_ds.RasterYSize):
        return False, '行列数不一致（stream_link={}×{}，flow_dir={}×{}）'.format(
            reference_ds.RasterXSize, reference_ds.RasterYSize,
            candidate_ds.RasterXSize, candidate_ds.RasterYSize)
    if any(abs(left - right) > 1e-12 for left, right in zip(
            reference_ds.GetGeoTransform(), candidate_ds.GetGeoTransform())):
        return False, '仿射变换不一致（范围、原点或分辨率不同）'
    # 校大卫: 不调用旧版 OSR 绑定中可能缺失的方法；本流程要求两幅栅格使用同一份投影 WKT。
    reference_wkt = ''.join(reference_ds.GetProjection().upper().split())
    candidate_wkt = ''.join(candidate_ds.GetProjection().upper().split())
    if reference_wkt != candidate_wkt:
        return False, '投影或坐标系不一致'
    return True, ''


def create_coordinate_transform(dataset):
    projection_wkt = dataset.GetProjection()
    # 校大卫: 直接检查原始 WKT，完全避免旧版 GDAL Python 绑定缺失 IsEmpty() 等接口的问题。
    if not projection_wkt or not projection_wkt.strip():
        raise RuntimeError('stream_link.tif 缺少投影，无法输出经纬度。')
    normalized_wkt = projection_wkt.upper()
    if 'GEOGCS[' in normalized_wkt or 'GEOGCRS[' in normalized_wkt:
        # 校大卫: 本项目栅格为 WGS84 经纬度网格时，像元中心坐标本身就是经纬度，无需创建 OSR 坐标转换对象。
        return None
    source_srs = osr.SpatialReference()
    source_srs.ImportFromWkt(projection_wkt)
    target_srs = osr.SpatialReference()
    target_srs.ImportFromEPSG(4326)
    return osr.CoordinateTransformation(source_srs, target_srs)


def value_is_nodata(value, nodata):
    if nodata is None:
        return False
    if isinstance(nodata, float) and math.isnan(nodata):
        return bool(np.isnan(value))
    return bool(value == nodata)


def pixel_center(geotransform, row, col):
    pixel_col = col + 0.5
    pixel_row = row + 0.5
    x_value = geotransform[0] + pixel_col * geotransform[1] + pixel_row * geotransform[2]
    y_value = geotransform[3] + pixel_col * geotransform[4] + pixel_row * geotransform[5]
    return x_value, y_value


def format_value(value, nodata):
    if value_is_nodata(value, nodata):
        return 'NoData'
    return str(value.item() if hasattr(value, 'item') else value)


def check(stream_path, flowdir_path, csv_path):
    stream_ds = open_raster(stream_path, 'stream_link.tif')
    flowdir_ds = open_raster(flowdir_path, 'flow_dir.tif')
    grid_ok, reason = same_grid(stream_ds, flowdir_ds)
    if not grid_ok:
        raise RuntimeError('网格检查失败：{}'.format(reason))

    stream_band = stream_ds.GetRasterBand(1)
    flowdir_band = flowdir_ds.GetRasterBand(1)
    stream_nodata = stream_band.GetNoDataValue()
    flowdir_nodata = flowdir_band.GetNoDataValue()
    geotransform = stream_ds.GetGeoTransform()
    coordinate_transform = create_coordinate_transform(stream_ds)
    valid_d8_codes = set(ESRI_D8_CODES)
    statistics = {
        'stream_cells': 0,
        'flowdir_nodata': 0,
        'flowdir_illegal': 0,
        'valid_stream_flowdir': 0,
    }

    output_dir = os.path.dirname(os.path.abspath(csv_path))
    if output_dir and not os.path.isdir(output_dir):
        os.makedirs(output_dir)
    with open(csv_path, 'w', newline='') as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(['longitude', 'latitude', 'row', 'column',
                         'stream_link_value', 'flow_dir_value', 'reason'])
        for y_offset in range(0, stream_ds.RasterYSize, BLOCK_ROWS):
            block_height = min(BLOCK_ROWS, stream_ds.RasterYSize - y_offset)
            stream_data = stream_band.ReadAsArray(
                0, y_offset, stream_ds.RasterXSize, block_height)
            flowdir_data = flowdir_band.ReadAsArray(
                0, y_offset, flowdir_ds.RasterXSize, block_height)
            # 校大卫: 河网编号允许为负值；仅将明确 NoData 和背景值 0 排除，不能以大于零判断河网有效性。
            stream_mask = np.ones(stream_data.shape, dtype=bool)
            if stream_nodata is not None:
                stream_mask &= stream_data != stream_nodata
            stream_mask &= stream_data != 0
            invalid_rows, invalid_cols = np.where(stream_mask)
            statistics['stream_cells'] += int(invalid_rows.size)
            for local_row, col in zip(invalid_rows, invalid_cols):
                row = int(y_offset + local_row)
                col = int(col)
                flowdir_value = flowdir_data[local_row, col]
                if value_is_nodata(flowdir_value, flowdir_nodata):
                    invalid_reason = 'flow_dir 为 NoData'
                    statistics['flowdir_nodata'] += 1
                elif int(flowdir_value) not in valid_d8_codes:
                    invalid_reason = 'flow_dir 不是合法 ESRI D8 编码'
                    statistics['flowdir_illegal'] += 1
                else:
                    statistics['valid_stream_flowdir'] += 1
                    continue
                x_value, y_value = pixel_center(geotransform, row, col)
                if coordinate_transform is None:
                    longitude, latitude = x_value, y_value
                else:
                    longitude, latitude, _ = coordinate_transform.TransformPoint(x_value, y_value)
                writer.writerow([
                    '{:.10f}'.format(longitude), '{:.10f}'.format(latitude),
                    row, col, format_value(stream_data[local_row, col], stream_nodata),
                    format_value(flowdir_value, flowdir_nodata), invalid_reason])

    stream_ds = None
    flowdir_ds = None
    invalid_total = statistics['flowdir_nodata'] + statistics['flowdir_illegal']
    print('网格检查：通过（行列数、仿射变换、投影均一致）')
    print('有效河网像元数：{}'.format(statistics['stream_cells']))
    print('河网像元上 flow_dir 为 NoData 的数量：{}'.format(statistics['flowdir_nodata']))
    print('河网像元上 flow_dir 非法 D8 编码的数量：{}'.format(statistics['flowdir_illegal']))
    print('河网像元上合法 D8 流向的数量：{}'.format(statistics['valid_stream_flowdir']))
    print('不合格总数：{}'.format(invalid_total))
    print('不合格点明细 CSV：{}'.format(csv_path))
    return invalid_total


def main(arguments):
    if len(arguments) not in (3, 4):
        raise SystemExit('用法: check_stream_flowdir.py <stream_link.tif> <flow_dir.tif> [invalid_points.csv]')
    csv_path = arguments[3] if len(arguments) == 4 else 'stream_flowdir_invalid_points.csv'
    invalid_total = check(arguments[1], arguments[2], csv_path)
    if invalid_total > 0:
        raise SystemExit(2)


if __name__ == '__main__':
    main(sys.argv)
