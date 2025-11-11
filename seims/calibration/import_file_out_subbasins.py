# -*- coding: utf-8 -*-
"""
读取形如：
214
49-54-57-...
225
174-184-...
...
457
430-438-443-...
的 txt 文件，并把每个站点的上游列表写入对应 MongoDB 库的 FILE_OUT 集合：
- OUTPUTID="m_chInundationArea" 的 SUBBASIN = 上游列表字符串
- OUTPUTID="QRECH"            的 SUBBASIN = 站点ID字符串
"""

from pymongo import MongoClient, ReturnDocument
from pathlib import Path



# === 可选：不存在时是否创建文档（True=自动upsert，False=只更新已存在的文档） ===
UPSERT_IF_MISSING = False


def parse_station_upstreams(txt_path: str):
    """
    解析两行一组：第一行站点ID（纯数字），第二行上游列表（以-分隔的大串）。
    允许空行，会自动跳过；允许第二行末尾有空格。
    返回 dict: { station_id(int): upstream_str(str) }
    """
    p = Path(txt_path)
    if not p.exists():
        raise FileNotFoundError(f"文件不存在: {txt_path}")

    mapping = {}
    with p.open("r", encoding="utf-8") as f:
        lines = [ln.strip() for ln in f if ln.strip() != ""]

    i = 0
    while i < len(lines):
        # 找到一个纯数字的行作为 station_id
        if lines[i].isdigit():
            station_id = int(lines[i])
            if i + 1 >= len(lines):
                raise ValueError(f"站点 {station_id} 后缺少上游列表行")
            upstream_line = lines[i + 1].strip()
            # 允许把行尾多余的连字符去掉
            upstream_line = upstream_line.strip("-")
            mapping[station_id] = upstream_line
            i += 2
        else:
            # 兼容：若首行不是纯数字，尝试跳过（也可以直接抛错）
            i += 1
    return mapping


def write_to_mongo(mapping: dict, mongo_uri: str, upsert_if_missing: bool = False):
    """
    对于每个 station_id：
      db = f"poyang_lake1_longterm_model_{station_id}"
      coll = db.FILE_OUT
      - 更新 OUTPUTID="m_chInundationArea" 的 SUBBASIN = upstream_str
      - 更新 OUTPUTID="QRECH" 的 SUBBASIN = str(station_id)
    """
    client = MongoClient(mongo_uri)

    for station_id, upstream_str in mapping.items():
        db_name = f"poyang_lake1_longterm_model_{station_id}"
        coll = client[db_name]["FILE_OUT"]

        # 1) m_chInundationArea
        res1 = coll.update_one(
            {"OUTPUTID": "m_chInundationArea"},
            {"$set": {"SUBBASIN": upstream_str}},
            upsert=upsert_if_missing,
        )

        # 2) QRECH
        res2 = coll.update_one(
            {"OUTPUTID": "QRECH"},
            {"$set": {"SUBBASIN": str(station_id)}},
            upsert=upsert_if_missing,
        )

        # 打印结果
        def _summ(r):
            if r.matched_count:
                return f"matched={r.matched_count}, modified={r.modified_count}"
            return f"upserted_id={r.upserted_id}" if r.upserted_id else "no-match"

        print(
            f"[{db_name}] "
            f"m_chInundationArea -> {_summ(res1)} | "
            f"QRECH -> {_summ(res2)}"
        )

    client.close()


if __name__ == "__main__":
    # === 必填：txt 路径 ===
    TXT_PATH = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\站点上游列表.txt"
    MONGO_URI = "mongodb://localhost:27017"
    # MONGO_URI = "mongodb://172.21.124.127:27019"
    mapping = parse_station_upstreams(TXT_PATH)
    print(f"解析到 {len(mapping)} 个站点。示例：", list(mapping.items())[:3])
    write_to_mongo(mapping, MONGO_URI, upsert_if_missing=UPSERT_IF_MISSING)
