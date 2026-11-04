# -*- coding: utf-8 -*-
from __future__ import absolute_import, unicode_literals

import csv
import os
import subprocess
import sys
from datetime import datetime
from configparser import ConfigParser

from pymongo import MongoClient

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from utility import read_data_items_from_txt


# ============================================================
# 用户设置
# ============================================================

SEIMS_ROOT = r'D:\SEIMS_C'
WATERSHED_NAME = 'CW_2'

MODEL_DIR = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model'
WORKSPACE_DIR = r'D:\SEIMS_C\data\CW_2\workspace'
CALIBRATION_INI = os.path.join(WORKSPACE_DIR, 'calibration.ini')

MONGO_HOST = '127.0.0.1'
MONGO_PORT = 27017
DATABASE_NAME = 'CW_2_longterm_model'

RUN_MODEL_SCRIPT = os.path.join(
    SEIMS_ROOT,
    'seims',
    'test',
    'demo_runmodel.py'
)

# True：只检查参数名、参数数量和数据库，不写入、不运行
# 第一次建议先设为 True；检查无误后改为 False。
DRY_RUN = True
# DRY_RUN = False

# 选中的共享参数组，共26个值
SELECTED_PARAMETER_VALUES = [
    -0.29471605757582403,
    0.28709574407323546,
    -4.94388206758884,
    2.233353039558394,
    -0.6024840465606199,
    -0.7471097632511163,
    -0.1303869483703164,
    39.17821121559357,
    0.6334189346079515,
    0.04826754263394838,
    33.19742483313223,
    -0.5133809625339951,
    0.8859192373607253,
    2.3393668584112346,
    1.559204703934673,
    229.4144017666391,
    2.0665635144755763,
    5.955885038423687,
    0.3759936469826717,
    0.02886143278633782,
    -0.4460506647706274,
    -0.0007504422040487575,
    0.6380911994841058,
    -1.7254754193404054,
    0.9456319237828157,
    50.37644119512895
]


def read_parameter_names():
    """按率定时的顺序读取参数名称。"""
    if not os.path.exists(CALIBRATION_INI):
        raise FileNotFoundError(
            '找不到 calibration.ini: %s' % CALIBRATION_INI
        )

    cf = ConfigParser()
    cf.read(CALIBRATION_INI)

    if not cf.has_option('CALI_Settings', 'paramrngdef'):
        raise KeyError(
            'calibration.ini 中缺少 CALI_Settings.paramrngdef'
        )

    param_range_def = cf.get(
        'CALI_Settings',
        'paramrngdef'
    )

    if os.path.isabs(param_range_def):
        param_file = param_range_def
    else:
        param_file = os.path.join(
            MODEL_DIR,
            param_range_def
        )

    if not os.path.exists(param_file):
        raise FileNotFoundError(
            '找不到参数范围定义文件: %s' % param_file
        )

    items = read_data_items_from_txt(param_file)

    names = []
    for item in items:
        if len(item) < 3:
            continue
        names.append(item[0])

    print('参数范围文件:', param_file)
    print('读取参数数量:', len(names))

    return names


def backup_current_impacts(collection, parameter_names):
    """写入前备份当前IMPACT值。"""
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    backup_file = os.path.join(
        MODEL_DIR,
        'selected_parameter_impact_backup_%s.csv' % timestamp
    )

    with open(backup_file, 'w', newline='', encoding='utf-8-sig') as f:
        writer = csv.writer(f)
        writer.writerow([
            'Index',
            'NAME',
            'Old_IMPACT',
            'Selected_IMPACT'
        ])

        for idx, (name, new_value) in enumerate(
            zip(parameter_names, SELECTED_PARAMETER_VALUES),
            start=1
        ):
            doc = collection.find_one(
                {'NAME': name},
                {'NAME': 1, 'IMPACT': 1}
            )

            if doc is None:
                old_value = None
            else:
                old_value = doc.get('IMPACT', None)

            writer.writerow([
                idx,
                name,
                old_value,
                new_value
            ])

    print('原IMPACT值已备份到:', backup_file)
    return backup_file


def verify_parameters(collection, parameter_names):
    """检查名称数量以及数据库中是否存在这些参数。"""
    if len(parameter_names) != len(SELECTED_PARAMETER_VALUES):
        raise ValueError(
            '参数名称数量与参数值数量不一致: %d != %d'
            % (
                len(parameter_names),
                len(SELECTED_PARAMETER_VALUES)
            )
        )

    missing_names = []

    print('\n参数映射关系：')
    print('-' * 90)

    for idx, (name, value) in enumerate(
        zip(parameter_names, SELECTED_PARAMETER_VALUES),
        start=1
    ):
        doc = collection.find_one(
            {'NAME': name},
            {'NAME': 1, 'IMPACT': 1}
        )

        if doc is None:
            missing_names.append(name)
            old_value = '<数据库中不存在>'
        else:
            old_value = doc.get('IMPACT', None)

        print(
            '%02d  %-30s  当前IMPACT=%-15s  新IMPACT=%s'
            % (
                idx,
                name,
                str(old_value),
                str(value)
            )
        )

    print('-' * 90)

    if missing_names:
        raise RuntimeError(
            '数据库PARAMETERS中缺少以下参数:\n%s'
            % '\n'.join(missing_names)
        )


def write_selected_impacts(collection, parameter_names):
    """把选中的参数值写入PARAMETERS.IMPACT。"""
    for idx, (name, value) in enumerate(
        zip(parameter_names, SELECTED_PARAMETER_VALUES),
        start=1
    ):
        result = collection.update_one(
            {'NAME': name},
            {'$set': {'IMPACT': float(value)}}
        )

        if result.matched_count != 1:
            raise RuntimeError(
                '写入失败，参数不存在或匹配异常: %s'
                % name
            )

        print(
            '已写入 %02d/%02d: %-30s IMPACT=%s'
            % (
                idx,
                len(parameter_names),
                name,
                value
            )
        )


def run_model():
    """调用当前Python环境运行demo_runmodel.py。"""
    if not os.path.exists(RUN_MODEL_SCRIPT):
        raise FileNotFoundError(
            '找不到模型运行脚本: %s'
            % RUN_MODEL_SCRIPT
        )

    command = [
        sys.executable,
        RUN_MODEL_SCRIPT,
        '-name',
        WATERSHED_NAME
    ]

    print('\n开始运行模型：')
    print(' '.join(command))

    completed = subprocess.run(
        command,
        cwd=os.path.dirname(RUN_MODEL_SCRIPT)
    )

    if completed.returncode != 0:
        raise RuntimeError(
            '模型运行失败，退出码: %d'
            % completed.returncode
        )

    print('模型运行完成，退出码为0。')


def main():
    print('=' * 90)
    print('固定共享参数并重新运行模型')
    print('数据库:', DATABASE_NAME)
    print('DRY_RUN:', DRY_RUN)
    print('=' * 90)

    parameter_names = read_parameter_names()

    client = MongoClient(
        MONGO_HOST,
        MONGO_PORT
    )

    try:
        db = client[DATABASE_NAME]
        collection = db['PARAMETERS']

        verify_parameters(
            collection,
            parameter_names
        )

        if DRY_RUN:
            print(
                '\nDRY_RUN=True：检查完成，未修改数据库，也未运行模型。'
            )
            print(
                '确认上面的26个参数名称和顺序正确后，'
                '把DRY_RUN改成False再运行。'
            )
            return

        backup_current_impacts(
            collection,
            parameter_names
        )

        write_selected_impacts(
            collection,
            parameter_names
        )

        print('\n选中的共享参数已全部写入PARAMETERS.IMPACT。')

    finally:
        client.close()

    run_model()

    print('\n运行后重点检查:')
    print(
        os.path.join(
            MODEL_DIR,
            'OUTPUT0'
        )
    )
    print(
        '先确认新运行结果能够复现共享参数图中的D_1304、'
        'D_1305、D_1306模拟过程，再添加冬季水量平衡诊断输出。'
    )


if __name__ == '__main__':
    main()
