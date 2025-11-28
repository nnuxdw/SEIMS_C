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

import os
from typing import List, Set

def get_true_upstream_for_subbasin(
    base_path: str,
    downstream_id: int,
    middle_ids: List[int],
):
    """
    计算“下游子流域真正的上游”：
    = 下游子流域的所有上游集合 － middle_ids 及其所有上游集合

    :param base_path: TNH_caliparam_sub_*.csv 所在目录
    :param downstream_id: 下游子流域 id（比如 1171）
    :param middle_ids: 需要从上游中剔除的若干子流域 id
                       （比如 [123, 141, 214, 225, 322, 347, 457]）
    :return: 按从小到大排序后的“真正上游”子流域 id 列表
    """

    # 下游子流域（如 1171）的所有上游（包括它自身）
    down_csv = os.path.join(base_path, f"TNH_caliparam_sub_{downstream_id}.csv")
    upstream_downstream: Set[int] = set(
        get_upstream_subbasins(down_csv, "subbasin", "caliparam_sub")
    )

    # middle_ids 以及它们各自所有上游的并集
    excluded_upstream: Set[int] = set()
    for sid in middle_ids:
        csv_path = os.path.join(base_path, f"TNH_caliparam_sub_{sid}.csv")
        us = get_upstream_subbasins(csv_path, "subbasin", "caliparam_sub")
        excluded_upstream.update(us)

    # 做差集：1171 上游减去中间子流域及其上游
    true_upstream = sorted(upstream_downstream - excluded_upstream)

    # 如果你不想包含 1171 自身，可以在这里再去掉一下
    if downstream_id in true_upstream:
        true_upstream.remove(downstream_id)

    return true_upstream


if __name__ == '__main__':
    base_path = 'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model'
    # subbasin_ids = [1171]
    subbasin_ids = [123, 141, 214, 225, 322, 347, 457]

    for subbasin_id in subbasin_ids:
        csv_path = os.path.join(base_path,f"TNH_caliparam_sub_{subbasin_id}.csv")
        upstream_subbasin_ids = get_upstream_subbasins(csv_path,"subbasin","caliparam_sub")
        stats = insert_F_sites_from_csv(
                mongo_uri="mongodb://172.21.124.127:27019",
                db_name="poyang_lake1_HydroClimate",
                collection="SITES",
                subbasin_id=subbasin_id
            )
        upstream_subbasin_ids_str = "-".join(map(str, upstream_subbasin_ids))
        print(subbasin_id)
        print(upstream_subbasin_ids_str)
    # 找到真正的上游
    true_upstream_for_1171 = get_true_upstream_for_subbasin(base_path,1171,subbasin_ids)
    true_upstream_for_1171_str = "-".join(map(str, true_upstream_for_1171))
    print(1171)
    print(true_upstream_for_1171_str)
