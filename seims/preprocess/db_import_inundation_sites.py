from typing import List, Dict, Any
from pathlib import Path

import pandas as pd
import numpy as np
from pymongo import MongoClient, UpdateOne, ASCENDING
import os

def get_upstream_subbasins(
    csv_path: str,
    id_col: str = "subbasin",
    flag_col: str = "caliparam_sub"
) -> List[int]:
    """
    读取 CSV，筛选 flag_col == 1 的行，返回对应的 subbasin 列（去重并升序）。
    兼容 1/1.0/'1'，忽略空值与非法值。
    """
    csv_path = Path(csv_path)
    if not csv_path.exists():
        raise FileNotFoundError(csv_path)

    # 只读两列更稳妥
    df = pd.read_csv(csv_path, usecols=[id_col, flag_col])

    # 转数值；非法变 NaN
    df[id_col] = pd.to_numeric(df[id_col], errors="coerce")
    df[flag_col] = pd.to_numeric(df[flag_col], errors="coerce")

    # 过滤 caliparam_sub == 1
    mask = (df[flag_col] == 1)
    sub_list = df.loc[mask, id_col].dropna().astype(int).tolist()

    # 去重并排序
    return sorted(set(sub_list))

def insert_F_sites_from_csv(
    mongo_uri: str,
    db_name: str,
    collection: str,
    subbasin_id: int,
    batch_size: int = 1000,
) -> Dict[str, Any]:
    id_col = "subbasin"
    flag_col = "caliparam_sub"
    csv_path = os.path.join(base_path, f"TNH_caliparam_sub_{subbasin_id}.csv")

    subs = get_upstream_subbasins(csv_path, id_col, flag_col)
    coll = MongoClient(mongo_uri)[db_name][collection.strip()]

    ops = []
    matched = modified = upserted = 0

    for sid in subs:
        sid = int(sid)  # 保证原生 int
        base_doc = {
            "STATIONID": sid,
            "NAME": f"F_{sid}",
            "TYPE": "F",
            "LAT": 0.0,
            "LON": 0.0,
            "LOCALX": 0.0,
            "LOCALY": 0.0,
            "UNIT": "m2",
            "ELEVATION": 0.0,
            "ISOUTLET": 0.0,
            "SUBBASINID": sid,
            "BASE_SUBBASINID": int(subbasin_id),
        }

        # 用 TYPE + SUBBASINID 作为匹配条件；存在则更新，不存在则插入（_id 由 Mongo 自动生成）
        ops.append(UpdateOne(
            {"TYPE": "F", "SUBBASINID": sid},
            {"$set": base_doc},
            upsert=True
        ))

        if len(ops) >= batch_size:
            res = coll.bulk_write(ops, ordered=False)
            matched += (res.matched_count or 0)
            modified += (res.modified_count or 0)
            upserted += len(res.upserted_ids or {})
            ops.clear()

    if ops:
        res = coll.bulk_write(ops, ordered=False)
        matched += (res.matched_count or 0)
        modified += (res.modified_count or 0)
        upserted += len(res.upserted_ids or {})

    return {
        "collection": collection.strip(),
        "total_subs": len(subs),
        "matched_docs": matched,  # 命中的现有文档数
        "modified_docs": modified,  # 实际被改动的文档数
        "upserted_new": upserted,  # 新插入的文档数（Mongo 自动生成 _id）
    }

if __name__ == '__main__':
    base_path = 'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model'
    # subbasin_ids = [1171]
    subbasin_ids = [123, 141, 214, 225, 322, 347, 457]
    for subbasin_id in subbasin_ids:
        csv_path = os.path.join(base_path,f"TNH_caliparam_sub_{subbasin_id}.csv")
        upstream_subbasin_ids = get_upstream_subbasins(csv_path,"subbasin","caliparam_sub")
        # stats = insert_F_sites_from_csv(
        #         mongo_uri="mongodb://127.0.0.1:27017",
        #         db_name="poyang_lake1_HydroClimate",
        #         collection="SITES",                   # 如果集合真叫 "SITES "（有空格），也会被 strip()
        #         subbasin_id=subbasin_id
        #     )
        upstream_subbasin_ids_str = "-".join(map(str, upstream_subbasin_ids))
        print(subbasin_id)
        print(upstream_subbasin_ids_str)
