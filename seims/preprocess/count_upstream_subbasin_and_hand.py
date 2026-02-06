import pandas as pd
import re
from typing import Dict, List, Set, Union

def parse_upstream_txt(upstream_txt_path: str, station_ids: Union[List[int], Set[int]]) -> Dict[int, Set[int]]:
    """
    解析“站点上游列表.txt”
    格式示例：
        123
        1-2-3-...-1225
        141
        55-64-...-1191
        ...
    返回：{station_id: set(subbasin_ids)}
    """
    station_ids = set(int(x) for x in station_ids)

    # 允许 txt 里出现的站点行：仅包含一个整数
    station_line_re = re.compile(r"^\s*(\d+)\s*$")

    upstream_map: Dict[int, Set[int]] = {}
    cur_station: int = None

    with open(upstream_txt_path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line:
                continue

            m = station_line_re.match(line)
            if m:
                sid = int(m.group(1))
                cur_station = sid if sid in station_ids else None
                if cur_station is not None and cur_station not in upstream_map:
                    upstream_map[cur_station] = set()
                continue

            # 非站点行：认为是 subbasin 列表片段（用 - 或空白分隔都能吃）
            if cur_station is None:
                continue

            # 提取这一行的所有整数（比 split('-') 更鲁棒）
            nums = re.findall(r"\d+", line)
            upstream_map[cur_station].update(int(x) for x in nums)

    # 若某些站点在 txt 里没有出现，也补一个空集合，避免 KeyError
    for sid in station_ids:
        upstream_map.setdefault(sid, set())

    return upstream_map


def count_station_upstream_subbasin_and_hru(
    inundation_csv_path: str,
    upstream_txt_path: str,
    station_ids: List[int] = None,
    subbasin_col: str = "Subbasin",
    hru_col: str = "HRU_ID",
) -> pd.DataFrame:
    """
    统计每个站点：
    - upstream_subbasin_count: 上游 subbasin 数量（去重）
    - upstream_hru_count: 上游 subbasin 覆盖到的 HRU(HAND) 数量（HRU_ID 去重）
    """
    if station_ids is None:
        station_ids = [123, 141, 214, 225, 322, 347, 457, 1171]

    # 1) 解析上游列表
    upstream_map = parse_upstream_txt(upstream_txt_path, station_ids)

    # 2) 读取 inundationMap.csv
    df = pd.read_csv(inundation_csv_path)

    # 基础校验
    for c in (subbasin_col, hru_col):
        if c not in df.columns:
            raise ValueError(f"CSV 缺少必要列: {c}。当前列有: {list(df.columns)}")

    # 统一类型（防止字符串/浮点导致匹配失败）
    df[subbasin_col] = pd.to_numeric(df[subbasin_col], errors="coerce").astype("Int64")
    df[hru_col] = pd.to_numeric(df[hru_col], errors="coerce").astype("Int64")
    df = df.dropna(subset=[subbasin_col, hru_col])

    # 3) 逐站点统计
    rows = []
    for sid in station_ids:
        ups_subbasins = upstream_map.get(sid, set())
        ups_sub_count = len(ups_subbasins)

        if ups_sub_count == 0:
            ups_hru_count = 0
        else:
            sub_set = set(ups_subbasins)
            ups_hru_count = df.loc[df[subbasin_col].isin(sub_set), hru_col].nunique()

        rows.append({
            "station_id": sid,
            "upstream_subbasin_count": ups_sub_count,
            "upstream_hru_count": int(ups_hru_count),
        })

    return pd.DataFrame(rows).sort_values("station_id").reset_index(drop=True)


# ======= 用法示例 =======
if __name__ == "__main__":
    inundation_csv = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\InundationMap.csv"
    upstream_txt = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\站点上游列表.txt"

    result_df = count_station_upstream_subbasin_and_hru(inundation_csv, upstream_txt)
    print(result_df)
    # result_df.to_csv("station_upstream_counts.csv", index=False, encoding="utf-8-sig")
