from __future__ import annotations

import numpy as np
import pandas as pd
import rasterio


def _read_table_whitespace(txt_path: str) -> pd.DataFrame:
    """
    读取你这种“空格/制表符对齐”的 txt。
    """
    # sep=r"\s+" 可以吃掉任意数量空格/Tab
    return pd.read_csv(txt_path, sep=r"\s+", engine="python")


def handid_tif_to_floodlevel_tif(
    handid_tif: str,
    mapping_txt: str,
    out_tif: str,
    *,
    # 两种指定方式：优先用列名；列名不存在就用列序号（从0开始）
    id_col: str = "HRU_ID",
    flood_col: str = "Flood_Level",
    id_col_idx: int = 0,       # 第1列
    flood_col_idx: int = 2,    # 第3列（Flood_Level）
    band: int = 1,
    out_dtype: str = "int16",  # Flood_Level 是 int，int16 通常够；不够可改 int32
    out_nodata: int = -9999,   # 输出 NoData
):
    """
    输入：
      - handid_tif：像元值为 HAND id / HRU_ID 的栅格
      - mapping_txt：包含 HRU_ID -> Flood_Level 的表（Flood_Level 为单个整数）
    输出：
      - out_tif：单波段栅格，元数据与输入一致，仅像元值替换为 Flood_Level
    """
    df = _read_table_whitespace(mapping_txt)

    # 取映射列：优先按列名，其次按列序号
    if id_col in df.columns and flood_col in df.columns:
        ids = df[id_col].to_numpy()
        levels = df[flood_col].to_numpy()
    else:
        # 没有标准列名就按列位置取
        if df.shape[1] <= max(id_col_idx, flood_col_idx):
            raise ValueError(
                f"映射表列数不足：当前列数={df.shape[1]}，"
                f"但你指定了 id_col_idx={id_col_idx}, flood_col_idx={flood_col_idx}"
            )
        ids = df.iloc[:, id_col_idx].to_numpy()
        levels = df.iloc[:, flood_col_idx].to_numpy()

    # 清洗：id 转 int，level 转 int
    ids_num = pd.to_numeric(ids, errors="coerce")
    levels_num = pd.to_numeric(levels, errors="coerce")

    valid = np.isfinite(ids_num) & np.isfinite(levels_num)

    ids = ids_num[valid].astype(np.int64)
    levels = levels_num[valid].astype(np.int64)



    if ids.size == 0:
        raise ValueError("映射表中没有解析到有效的 HRU_ID / Flood_Level，请检查 txt 内容。")

    # 去重：如果一个 id 在表里出现多次，保留最后一个
    tmp = pd.DataFrame({"id": ids, "lvl": levels}).drop_duplicates("id", keep="last")
    ids = tmp["id"].to_numpy(dtype=np.int64)
    levels = tmp["lvl"].to_numpy(dtype=np.int64)

    # 排序 + searchsorted 做快速映射
    order = np.argsort(ids)
    ids_sorted = ids[order]
    levels_sorted = levels[order]

    with rasterio.open(handid_tif) as src:
        profile = src.profile.copy()
        profile.update(count=1, dtype=out_dtype, nodata=out_nodata, BIGTIFF="IF_SAFER")

        in_nodata = src.nodata

        with rasterio.open(out_tif, "w", **profile) as dst:
            # 分块处理，适合大图
            for _, window in src.block_windows(band):
                hand = src.read(band, window=window)

                # 输入 nodata mask
                if in_nodata is None:
                    mask_in = np.zeros(hand.shape, dtype=bool)
                else:
                    mask_in = (hand == in_nodata)

                # 转 int64 以便匹配
                hand_int = hand.astype(np.int64, copy=False)

                # 映射
                idx = np.searchsorted(ids_sorted, hand_int)
                idx_clip = np.clip(idx, 0, len(ids_sorted) - 1)

                matched = (ids_sorted[idx_clip] == hand_int)

                out = np.full(hand.shape, out_nodata, dtype=np.int64)
                out[matched] = levels_sorted[idx_clip[matched]]

                # 透传输入 nodata
                out[mask_in] = out_nodata

                dst.write(out.astype(np.dtype(out_dtype), copy=False), 1, window=window)

    return out_tif

if __name__ == "__main__":
    ## 为了SCI绘制HAND图，把表示hand id的tif，转成表示hand 层级的tif，该方法要用python3.7
    # handid_tif = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\sci_figure_study_region\hru_in_1171.tif"
    # mapping_txt = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\FloodStep.txt"
    # out_tif = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\sci_figure_study_region\hand_level_in_1171_and_upstream.tif"

    handid_tif = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\sci_figure_study_region\hru_in_1044.tif"
    mapping_txt = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\FloodStep.txt"
    out_tif = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\sci_figure_study_region\hand_level_in_1044.tif"
    handid_tif_to_floodlevel_tif(
        handid_tif=handid_tif,
        mapping_txt=mapping_txt,
        out_tif=out_tif,
        out_dtype="float32",
        out_nodata=-9999
    )

    print("✅ done")
