from typing import List, Optional, Dict, Any
from pathlib import Path
import pandas as pd
import numpy as np

from pymongo import MongoClient, UpdateOne, ASCENDING

def to_bson_safe(v):
    """将 pandas/numpy 标量安全转换为 Python 原生类型，NaN/NaT -> None。"""
    # None / NaN / NaT
    try:
        # pd.isna 对 None 返回 False，这里先判 None
        if v is None:
            return None
        if pd.isna(v):
            return None
    except Exception:
        pass

    # numpy -> python
    if isinstance(v, (np.integer,)):
        return int(v)
    if isinstance(v, (np.floating,)):
        return float(v)
    if isinstance(v, (np.bool_,)):
        return bool(v)
    return v


def import_fields_from_csv_to_mongo(
    mongo_uri: str,
    db_name: str,
    collection: str,
    csv_path: str,
    fields: List[str],                  # 需要导入的列名（不含 id 列）
    id_csv_col: str = "subbasin",       # CSV 中的 ID 列
    id_mongo_field: str = "SUBBASINID", # Mongo 集合中的匹配字段
    create_index: bool = True,          # 如无索引则创建
    batch_size: int = 1000,             # 批次大小
    upsert: bool = False,               # 是否在不存在时创建新文档
) -> Dict[str, Any]:
    """
    从 CSV 读取指定列，按 subbasin ↔ SUBBASINID 匹配，向 Mongo 指定集合新增/更新字段。
    返回执行统计信息。
    """
    csv_path = Path(csv_path)
    if not csv_path.exists():
        raise FileNotFoundError(str(csv_path))

    # 1) 读取 CSV（只读需要的列）
    need_cols = [id_csv_col] + list(fields)
    df = pd.read_csv(str(csv_path), usecols=need_cols)

    # 基础校验
    for c in need_cols:
        if c not in df.columns:
            raise ValueError("CSV 缺少列: {}".format(c))

    # 统一 ID 为整数（常见 case：'01' -> 1）
    df[id_csv_col] = pd.to_numeric(df[id_csv_col], errors="raise").astype("int64")

    # 2) 连接 Mongo
    client = MongoClient(mongo_uri)
    coll = client[db_name][collection]

    # 可选：为匹配字段创建索引（若已有不会重复）
    if create_index:
        try:
            coll.create_index([(id_mongo_field, ASCENDING)], name="{}_idx".format(id_mongo_field), background=True)
        except Exception:
            pass

    # 3) DataFrame 预处理：转为 object，NaN -> None（提升整体转换性能）
    df = df.astype(object)
    df = df.where(pd.notnull(df), None)

    # 4) 批量构造 UpdateOne
    ops = []
    matched = 0
    modified = 0
    upserted = 0

    total_rows = len(df)
    csv_ids = set()

    for _, row in df.iterrows():
        sub_id = to_bson_safe(row[id_csv_col])
        csv_ids.add(int(sub_id))

        set_doc = {}
        for col in fields:
            set_doc[col] = to_bson_safe(row[col])

        filt = {id_mongo_field: sub_id}
        ops.append(UpdateOne(filt, {"$set": set_doc}, upsert=upsert))

        if len(ops) >= batch_size:
            res = coll.bulk_write(ops, ordered=False)
            matched += (res.matched_count or 0)
            modified += (res.modified_count or 0)
            upserted += len(res.upserted_ids or {})
            ops.clear()

    # 提交余下的
    if ops:
        res = coll.bulk_write(ops, ordered=False)
        matched += (res.matched_count or 0)
        modified += (res.modified_count or 0)
        upserted += len(res.upserted_ids or {})

    # 5) 统计未匹配（仅在 upsert=False 时有意义）
    mongo_ids = set(x[id_mongo_field] for x in coll.find(
        {id_mongo_field: {"$in": list(csv_ids)}}, {id_mongo_field: 1, "_id": 0}
    ))
    unmatched_ids = sorted(csv_ids - mongo_ids) if not upsert else []

    return {
        "db": db_name,
        "collection": collection,
        "total_csv_rows": total_rows,
        "csv_unique_ids": len(csv_ids),
        "matched_docs": len(mongo_ids),
        "modified_docs": modified,
        "upserted_docs": upserted,
        "unmatched_ids_count": len(unmatched_ids),
        "unmatched_ids_sample": unmatched_ids[:20],
    }

if __name__ == '__main__':
    csv_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\caliparam_sub_141.csv"
    fields_TO_IMPORT = ['LAKEB_1D']
    result = import_fields_from_csv_to_mongo(
        mongo_uri="mongodb://127.0.0.1:27017",
        db_name="poyang_lake1_longterm_model",
        collection="REACHES",  # 你要更新的集合名
        csv_path=csv_path,
        fields=fields_TO_IMPORT,  # 需要导入的列
        id_csv_col="subbasin",  # CSV 中的 id 列
        id_mongo_field="SUBBASINID",  # 集合中的匹配字段
        create_index=True,
        batch_size=2000,
    )
    print(result)
