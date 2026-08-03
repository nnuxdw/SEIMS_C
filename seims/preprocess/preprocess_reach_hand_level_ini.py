from typing import Dict, Optional, Union
from pymongo import MongoClient, UpdateOne
from pymongo.errors import BulkWriteError
import pandas as pd
import os
def set_lake_hand_level_ini(
    mongo_uri: str,
    mapping,
    db_name: str,
    coll_name: str,
    subbasin_field: str,
    target_field: str,
    default_value,
    create_index: bool = True,
    dry_run: bool = False,
):
    """
       仅修改 Is_Lake==1 或 Is_Res==1 的记录：
       - 先把所有湖/库的 target_field 设为 default_value（若提供）；
       - 再按 mapping 覆盖指定 SUBBASIN 的 target_field。
       """
    client = MongoClient(mongo_uri)
    coll = client[db_name][coll_name]

    if create_index:
        coll.create_index(subbasin_field)
        coll.create_index("Is_Lake")
        coll.create_index("Is_Res")

    lake_or_res_filter = {"$or": [{"Is_Lake": 1}, {"Is_Res": 1}]}
    non_lake_res_filter = {"Is_Lake": 0, "Is_Res": 0}

    print("---- quick stats ----")
    print("total docs:", coll.estimated_document_count())
    print("lake/res count:", coll.count_documents(lake_or_res_filter))
    print("non-lake/res count:", coll.count_documents(non_lake_res_filter))


    # --- 1) 非湖/库设为 -1 ---
    if dry_run:
        to_match = coll.count_documents(non_lake_res_filter)
        print(f"[DRY-RUN] would set {target_field}=-1 for {to_match} non-lake/res docs")
    else:
        res_non = coll.update_many(non_lake_res_filter, {"$set": {target_field: default_value}})
        print(f"[OK] non-lake/res set to {default_value}: matched={res_non.matched_count}, modified={res_non.modified_count}")

    # --- 2) mapping 覆盖 ---
    norm_map: Dict[int, Union[int, float]] = {}
    for k, v in mapping.items():
        try:
            ki = int(float(k))
        except Exception:
            raise ValueError(f"SUBBASINID 无法转为整数: {k!r}")
        norm_map[ki] = v

    ops = []
    for sbid, level in norm_map.items():
        ops.append(
            UpdateOne(
                {subbasin_field: sbid, **lake_or_res_filter},
                {"$set": {target_field: level}},
                upsert=False,
            )
        )

    if not ops:
        print("[INFO] no mapping entries to update.")
        client.close()
        return

    if dry_run:
        print(f"[DRY-RUN] would bulk update {len(ops)} lake/res docs with mapping overrides.")
        client.close()
        return

    try:
        res_map = coll.bulk_write(ops, ordered=False)
        print(f"[OK] mapping overrides: matched={res_map.matched_count}, modified={res_map.modified_count}")
    except BulkWriteError as bwe:
        print("[ERROR] bulk write:", bwe.details)
    finally:
        client.close()



def count_hand_levels(csv_path: str) -> Dict[int, int]:
    """
    读取 HAND 结果 CSV，计算每个 Subbasin 的 HAND level 数量。

    参数:
        csv_path: CSV 文件路径

    返回:
        dict，key 为 SubbasinID (int)，value 为该子流域的 HAND level 数
    """
    df = pd.read_csv(csv_path)

    # 确保列名一致
    if "Subbasin" not in df.columns or "Flood_Level" not in df.columns:
        raise ValueError("CSV 缺少必要的列: Subbasin 或 Flood_Level")

    # 按 Subbasin 分组，统计 Flood_Level 的数量
    counts = df.groupby("Subbasin")["Flood_Level"].count()

    # 转成 dict
    return counts.to_dict()

if __name__ == '__main__':

    db_name = "taihu_1_longterm_model"
    collection = "REACHES"
    subbasin_field = "SUBBASINID"
    target_field= "Lake_Hand_Level_Ini"
    if os.name == 'nt':  # Windows
        # csv_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\InundationMap.csv"
        csv_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\workspace\rundata\InundationMap.csv"
        mongo_uri = "mongodb://localhost:27017"
    else:  # Linux/Unix
        csv_path = r"/data/user/xiaodw/software/WISE_V20160219/data/MSL_1/workspace/rundata/InundationMap.csv"
        mongo_uri = "mongodb://172.21.124.127:27019"

    # 从InundationMap.csv中统计每个Subbasin有多少层级HAND
    level_counts_map = count_hand_levels(csv_path)
    level_counts_map_ini = {}
    ratio = 0.1
    for subbasinid, nlevels in level_counts_map.items():
        ini_level = int(nlevels * ratio)   # 截断取整
        level_counts_map_ini[subbasinid] = ini_level

    # 仅查看将要更新多少条（不落库）
    set_lake_hand_level_ini(
        mongo_uri=mongo_uri,
        mapping=level_counts_map_ini,
        db_name=db_name,
        coll_name=collection,
        subbasin_field=subbasin_field,
        target_field=target_field,
        default_value = 1,
        create_index = True,
        dry_run=False)


