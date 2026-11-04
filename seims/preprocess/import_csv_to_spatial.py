#! /usr/bin/env python
# -*- coding: utf-8 -*-
"""
将 CSV 中的地块/空间参数导入 MongoDB GridFS：

    SPATIAL.files
    SPATIAL.chunks

逻辑与原始代码保持一致：
1. 读取 CSV；
2. 跳过第一列和 FID 列；
3. 将 PARAM_1、PARAM_2 等字段合并为多层参数；
4. 文件名统一加前缀 0_ 并转为大写；
5. 若同名文件已存在，删除当前版本；
6. 使用 struct.pack 按 float32 写入 GridFS。
"""

from __future__ import absolute_import, unicode_literals

import os
import sys
from struct import pack

# 保持与原代码相同的工程路径处理方式
if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from gridfs import GridFS
from pygeoc.utils import StringClass

from config import parse_ini_configuration
from db_mongodb import ConnectMongoDB
from utility import read_data_items_from_txt
from text import DBTableNames


def combine_multi_layers_array(data_dict):
    """
    合并多层数组参数。

    输入示例：
        {
            'SOL_OM_1': [1.1, 0.9, 0.4],
            'SOL_OM_2': [1.2, 1.0, 0.5],
            'DEM': [100, 101, 102]
        }

    输出示例：
        {
            'SOL_OM': [
                [1.1, 0.9, 0.4],
                [1.2, 1.0, 0.5]
            ],
            'DEM': [
                [100, 101, 102]
            ]
        }
    """
    comb_data_dict = dict()

    for key, value in list(data_dict.items()):
        key_split = key.split('_')

        # 没有下划线，按单层参数处理
        if len(key_split) <= 1:
            comb_data_dict[key] = [value]
            continue

        try:
            # 最后一个字段为纯数字时，认为它是层号
            # 例如 SOL_OM_1 -> 第0层
            pot_lyr_idx = int(key_split[-1]) - 1
            corename = key[0:key.rfind('_')]

            if pot_lyr_idx < 0:
                pot_lyr_idx = 0

            if corename not in comb_data_dict:
                comb_data_dict[corename] = list()

            # 与原代码保持一致，使用 insert 插入对应层
            comb_data_dict[corename].insert(pot_lyr_idx, value)

        except ValueError:
            # 例如 KG_1D、GWMAX_1D，末尾不是纯数字，
            # 因此仍作为一个独立的单层参数
            comb_data_dict[key] = [value]
            continue

    return comb_data_dict


def read_field_arrays_from_csv(csvf):
    """
    读取 CSV，并整理成 GridFS 所需的二维数组。

    CSV要求：
    - 第一行为字段名；
    - 第一列通常是 FID；
    - 每一行对应一个地块；
    - 除第一列/FID外，其余字段必须为数值。

    返回格式：
        {
            'POC': [[v1, v2, v3, ...]],
            'SOL_OM': [
                [layer1_cell1, layer1_cell2, ...],
                [layer2_cell1, layer2_cell2, ...]
            ]
        }
    """
    data_items = read_data_items_from_txt(csvf)

    if len(data_items) < 2:
        print('CSV文件没有有效数据: %s' % csvf)
        return None

    # 第一行为字段名
    flds = data_items[0]
    flds_array = dict()

    # 从第二行开始读取数据
    for idx, data_item in enumerate(data_items):
        if idx == 0:
            continue

        # 与原代码保持一致：将当前行拼接后提取所有数值
        data_item_values = StringClass.extract_numeric_values_from_string(
            ','.join(data_item)
        )

        for fld_idx, fld_name in enumerate(flds):
            # 跳过第一列
            if fld_idx == 0:
                continue

            # 跳过字段名为 FID 的列
            if StringClass.string_match(fld_name, 'FID'):
                continue

            if fld_name not in flds_array:
                flds_array[fld_name] = list()

            flds_array[fld_name].append(data_item_values[fld_idx])

    return combine_multi_layers_array(flds_array)


def import_array_to_mongodb(gfs, array, fname):
    """
    将数组参数写入 MongoDB GridFS。

    参数：
        gfs:
            GridFS对象，对应 SPATIAL.files 和 SPATIAL.chunks。

        array:
            二维数组，格式为：
            [
                [cell1, cell2, cell3, ...],  # 第1层
                [cell1, cell2, cell3, ...]   # 第2层
            ]

        fname:
            GridFS中的文件名，例如：
            0_POC
            0_MAOC
            0_KG_1D

    写入顺序与原代码一致：
        地块0的所有层
        地块1的所有层
        地块2的所有层
        ...
    """
    # 文件名统一转为大写
    fname = fname.upper()

    # 如果同名文件存在，删除当前版本
    if gfs.exists(filename=fname):
        old_file = gfs.get_version(filename=fname)
        gfs.delete(old_file._id)
        print('Delete old GridFS file: %s' % fname)

    # rows = 参数层数
    rows = len(array)

    # cols = 地块数量
    cols = len(array[0])

    # 按原始代码设置元数据
    meta_dict = dict()

    if 'WEIGHT' in fname:
        meta_dict['NUM_SITES'] = rows
        meta_dict['NUM_CELLS'] = cols
        meta_dict['SUBBASIN'] = 0
    else:
        meta_dict['TYPE'] = fname
        meta_dict['ID'] = fname
        meta_dict['DESCRIPTION'] = fname
        meta_dict['SUBBASIN'] = 0
        meta_dict['CELLSIZE'] = 1
        meta_dict['NODATA_VALUE'] = -9999
        meta_dict['NCOLS'] = cols
        meta_dict['NROWS'] = 1
        meta_dict['XLLCENTER'] = 0
        meta_dict['YLLCENTER'] = 0
        meta_dict['LAYERS'] = rows
        meta_dict['CELLSNUM'] = cols
        meta_dict['SRS'] = ''

    # 创建 GridFS 文件
    myfile = gfs.new_file(
        filename=fname,
        metadata=meta_dict
    )

    # 外层循环为地块，内层循环为参数层
    for j in range(0, cols):
        cur_col = list()

        for i in range(0, rows):
            cur_col.append(array[i][j])

        # 每个数按 float32 写入
        fmt = '%df' % rows
        myfile.write(pack(fmt, *cur_col))

    myfile.close()

    print(
        'Import %s done! CELLSNUM=%d, LAYERS=%d'
        % (fname, cols, rows)
    )


def main():
    """
    主程序。

    运行前只需要修改下面三个变量：
        db_name
        csv_file
        prefix
    """

    # ==========================================================
    # 1. 用户需要修改的配置
    # ==========================================================

    # 目标 MongoDB 数据库
    db_name = 'CW_2_longterm_model'

    # 要导入的 CSV 文件完整路径
    csv_file = r'D:\SEIMS_C\data\CW_2\workspace\csv\other_param_SoilC.csv'

    # GridFS 文件名前缀
    # prefix=0 时，POC 会写成 0_POC
    prefix = 0

    # 可选：模型地块总数。
    # 不想检查时设置为 None。
    expected_field_num = 1614
    # 例如：
    # expected_field_num = 32752

    # ==========================================================
    # 2. 检查 CSV 文件
    # ==========================================================

    if not os.path.isfile(csv_file):
        raise IOError('CSV文件不存在: %s' % csv_file)

    # ==========================================================
    # 3. 连接 MongoDB
    # ==========================================================

    seims_cfg = parse_ini_configuration()

    client = ConnectMongoDB(
        seims_cfg.hostname,
        seims_cfg.port
    )

    conn = client.get_conn()

    if db_name not in conn.list_database_names():
        raise ValueError(
            '目标数据库不存在：%s，请检查数据库名称。' % db_name
        )

    db_model_field = conn[db_name]

    # DBTableNames.gridfs_spatial 通常为 SPATIAL
    # 因此 GridFS 自动使用：
    # SPATIAL.files
    # SPATIAL.chunks
    spatial_gfs = GridFS(
        db_model_field,
        DBTableNames.gridfs_spatial
    )

    print('MongoDB host: %s' % seims_cfg.hostname)
    print('MongoDB port: %s' % seims_cfg.port)
    print('Database: %s' % db_name)
    print('CSV file: %s' % csv_file)
    print('GridFS prefix: %s' % DBTableNames.gridfs_spatial)

    # ==========================================================
    # 4. 读取 CSV
    # ==========================================================

    param_arrays = read_field_arrays_from_csv(csv_file)

    if not param_arrays:
        raise ValueError(
            'CSV中没有读取到可导入的参数，请检查表头和数据。'
        )

    print('')
    print('Parameters to import:')

    for key, value in list(param_arrays.items()):
        layers = len(value)
        cells = len(value[0])

        print(
            '  %s -> %d_%s, CELLSNUM=%d, LAYERS=%d'
            % (
                key,
                prefix,
                key.upper(),
                cells,
                layers
            )
        )

        if expected_field_num is not None:
            if cells != expected_field_num:
                raise ValueError(
                    '参数 %s 的数据长度为 %d，'
                    '但模型地块数要求为 %d。'
                    % (key, cells, expected_field_num)
                )

    # ==========================================================
    # 5. 导入 SPATIAL.files 和 SPATIAL.chunks
    # ==========================================================

    print('')
    print('Start importing...')

    for key, value in list(param_arrays.items()):
        import_array_to_mongodb(
            spatial_gfs,
            value,
            '%d_%s' % (prefix, key)
        )

    print('')
    print('All parameters imported successfully.')


if __name__ == '__main__':
    main()
