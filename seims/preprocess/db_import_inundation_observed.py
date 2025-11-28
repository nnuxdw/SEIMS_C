# -*- coding: utf-8 -*-
from __future__ import absolute_import, unicode_literals

import os
from datetime import datetime

import pandas as pd

from preprocess.db_mongodb import MongoUtil
from preprocess.text import StationFields, DBTableNames, DataValueFields
from preprocess.config import parse_ini_configuration
from preprocess.db_mongodb import ConnectMongoDB


def import_from_csv(csv_path, hydro_clim_db,
                    station_id=1171,
                    type_value='F',
                    tz_offset=0):
    """
    从 CSV 读取“日期 + 面积”，写入 MEASUREMENT 表

    Args:
        csv_path: CSV 文件路径（第一列日期，第二列面积）
        hydro_clim_db: Mongo 中的气候库，如 conn['poyang_lake1_HydroClimate']
        station_id: STATIONID，默认 1171
        type_value: TYPE 字段，默认 'F'
        tz_offset: 时区偏移（小时），这里保持 0
    """
    if not os.path.exists(csv_path):
        raise IOError("CSV 文件不存在: %s" % csv_path)

    coll = hydro_clim_db[DBTableNames.observes]

    # 如果需要重导，可以先删掉旧数据（可选）
    # coll.delete_many({StationFields.id: station_id,
    #                   DataValueFields.type: type_value})

    # 1. 读 CSV
    # 大概率是 GBK / ANSI，带中文表头，所以用 gbk + errors='ignore' 更保险
    df = pd.read_csv(
        csv_path,
        encoding='utf-8',       # 如果发现有问题再改成 'utf-8-sig'
        engine='python'       # 对一些奇怪的分隔符更宽容
    )

    # 只取前两列：日期列 & 面积列
    # 不管表头叫什么，都按 “第一列 = 日期，第二列 = 面积” 来用
    date_col_raw = df.iloc[:, 0]
    area_col_raw = df.iloc[:, 1]

    # 统一转成时间/数字，解析失败的变成 NaT / NaN
    date_col = pd.to_datetime(date_col_raw, errors='coerce')
    area_col = pd.to_numeric(area_col_raw, errors='coerce')

    print("CSV 读入形状：", df.shape)
    print("前 10 行：")
    print(df.head(10))
    print("后 10 行：")
    print(df.tail(10))

    bulk = coll.initialize_ordered_bulk_op()
    count = 0

    for idx, (dt, area_val) in enumerate(zip(date_col, area_col), start=1):
        # 日期或面积为空的行跳过
        if pd.isna(dt) or pd.isna(area_val):
            continue

        # 只要日期部分，时间统一设为 00:00:00
        dt = datetime(dt.year, dt.month, dt.day)
        local_dt = dt
        utc_dt = dt
        tzone = tz_offset  # 0

        doc = {
            StationFields.id: int(station_id),      # STATIONID
            DataValueFields.type: type_value,       # TYPE = 'F'
            DataValueFields.value: float(area_val), # VALUE = 面积
            DataValueFields.local_time: local_dt,   # LOCALDATETIME
            DataValueFields.time_zone: tzone,       # UTCOFFSET
            DataValueFields.utc: utc_dt,            # UTCDATETIME
        }

        bulk.insert(doc)
        count += 1
        if count % 500 == 0:
            MongoUtil.run_bulk(bulk)
            bulk = coll.initialize_ordered_bulk_op()

    if count % 500 != 0:
        MongoUtil.run_bulk(bulk)

    print("成功从 %s 导入 %d 条面积记录到 %s.%s" %
          (os.path.basename(csv_path),
           count,
           hydro_clim_db.name,
           DBTableNames.observes))


"""
读取鄱阳湖全天候面积逐日数据集（2014-2023年）CSV里的淹没面积，并写入 MEASUREMENT 表
"""
def main():
    cfg = parse_ini_configuration()
    client = ConnectMongoDB(cfg.hostname, cfg.port)
    conn = client.get_conn()

    # 气候数据库：poyang_lake1_HydroClimate
    hydroclim_db = conn['poyang_lake1_HydroClimate']

    csv_path = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\鄱阳湖全天候面积逐日数据集（2014-2023年）_数据实体\鄱阳湖.csv"

    import_from_csv(csv_path, hydro_clim_db=hydroclim_db,
                    station_id=1171,
                    type_value='F',
                    tz_offset=0)

    client.close()


if __name__ == '__main__':
    main()
