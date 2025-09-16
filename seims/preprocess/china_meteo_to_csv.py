
# -*- coding: utf-8 -*-
"""
将中国气象站逐日“20-8时/8-20时累计降水量”（北京时间）转换为 UTC 日尺度数据的脚本。

功能要点
--------
1) 读取 Excel/CSV（列名含中文），重点字段：
   - 年、月、日、区站号
   - 20-8时降水量、8-20时降水量
   - 20-20时降水量质量控制码（仅保留值==0的记录）
2) 过滤并置零特殊编码与异常值：
   - 精确码：32700, 32744, 32766, 999990（以及负值、NaN）
   - 30xxx / 31xxx / 32xxx 全部置零（即 >=30000 且 <40000）
   - 其它 >=30000 的值也置零（保险起见）
3) 数值单位：源数据为 0.1 mm，清洗后 /10 转为 mm。
4) 时段换算：源数据为北京时间（UTC+8）。
   - “8-20时累计降水量” -> UTC 当天 00:00-12:00
   - “20-8时累计降水量” -> UTC 当天 12:00-24:00
   以上两段相加即为 UTC 当天总降水量。
5) 导出 Figure2 风格 CSV：列为 [StationID, DATETIME, Type, VALUE]
   - DATETIME 采用 UTC 的“YYYY/M/D 0:00”形式（每天一行）
   - Type 默认写为 'P'（降水）。

依赖
----
pandas>=1.0（兼容 Py3.6）
"""
import os
import pandas as pd
import numpy as np

# ----------------------------- 用户可修改区 ----------------------------- #
# 中文列名的默认映射（根据你的文件必要时调整）
DEFAULT_COLS = {
    "year": "年",
    "month": "月",
    "day": "日",
    "station": "区站号",
    "p_20_8": "20-8时降水量",
    "p_8_20": "8-20时降水量",
    # "qc_20_20": "20-20时降水量质量控制码",
    # 可选：如果你想在缺失 qc_20_20 时退回到分段质控
    "qc_20_8": "20-8时降水量质量控制码",
    "qc_8_20": "8-20时降水量质量控制码",
}

# 一些常见的精确特殊码
SPECIAL_CODES_EXACT = {32700, 32744, 32766, 999990, 999999}

# ----------------------------- 工具函数 ----------------------------- #
def _read_any(path):
    """兼容读取 Excel/CSV。"""
    ext = os.path.splitext(path)[1].lower()
    if ext in [".xls", ".xlsx", ".xlsm"]:
        return pd.read_excel(path)
    elif ext in [".csv", ".txt"]:
        # 自动尝试常见编码
        for enc in ("utf-8-sig", "utf-8", "gbk", "gb2312"):
            try:
                return pd.read_csv(path, encoding=enc)
            except Exception:
                continue
        # 最后再不带 encoding 重试
        return pd.read_csv(path)
    else:
        raise ValueError("不支持的文件类型：{}".format(ext))

def _coerce_numeric(s):
    """将任意 Series 转为数值，无法解析的置 NaN。"""
    return pd.to_numeric(s, errors="coerce")

def _clean_precip_series(raw):
    """
    清洗降水序列：特殊编码和异常值置 0，随后 /10 转 mm。
    输入单位：0.1 mm；输出单位：mm。
    """
    x = _coerce_numeric(raw).copy()
    # 构造置零掩码
    mask = x.isna() | (x < 0) | (x >= 30000) | x.isin(list(SPECIAL_CODES_EXACT))
    # 置零
    x = x.mask(mask, 0)
    # 转为 mm
    return (x / 10.0).round(3)

def _format_dt_str(dt):
    """返回 YYYY/M/D 0:00（跨平台不依赖 %-m 等格式符）。"""
    return "{}/{}/{} 0:00".format(dt.year, dt.month, dt.day)

# ----------------------------- 核心逻辑 ----------------------------- #
def _to_daily_utc_dataframe(df, cols, type_label="P"):
    # 仅保留 20-20 质控为 0 的记录；若缺失该列，则退回到分段质控全为 0
    if cols.get("qc_20_20") in df.columns:
        df = df[df[cols["qc_20_20"]].fillna(1).astype(int) == 0].copy()
    else:
        # 分别检查两个时间段的质控
        if "qc_20_8" in cols and cols["qc_20_8"] in df.columns:
            qc20_8 = df[cols["qc_20_8"]].fillna(1).astype(int)
            # 只保留 qc=0 的值，其他置 0
            df.loc[qc20_8 != 0, cols["p_20_8"]] = 0.0

        if "qc_8_20" in cols and cols["qc_8_20"] in df.columns:
            qc8_20 = df[cols["qc_8_20"]].fillna(1).astype(int)
            df.loc[qc8_20 != 0, cols["p_8_20"]] = 0.0

    # 清洗数值并转换单位（0.1 mm -> mm）
    p_20_8_mm = _clean_precip_series(df[cols["p_20_8"]])
    p_8_20_mm = _clean_precip_series(df[cols["p_8_20"]])

    # 位移（BJT 20-8 -> UTC 12-24）
    p_20_8_mm_np = np.array(p_20_8_mm)
    p_20_8_mm_new = np.roll(p_20_8_mm_np, -1)
    p_20_8_mm_new[-1] = 0.0

    # 组装日期（仅用于 UTC 日聚合）
    date_local = pd.to_datetime(
        dict(
            year=_coerce_numeric(df[cols["year"]]).astype(int),
            month=_coerce_numeric(df[cols["month"]]).astype(int),
            day=_coerce_numeric(df[cols["day"]]).astype(int),
        ),
        errors="coerce",
    )
    if date_local.isna().any():
        bad = df[date_local.isna()].index[:5].tolist()
        raise ValueError("存在无法解析的日期，示例行索引：{}".format(bad))

    # 半日（UTC）映射
    utc_0_12 = pd.DataFrame({
        "StationID": _coerce_numeric(df[cols["station"]]).astype(int),
        "DATE_UTC": date_local.dt.date,
        "HOUR_UTC": 0,
        "VALUE": p_8_20_mm,          # BJT 8-20
    })
    utc_12_24 = pd.DataFrame({
        "StationID": _coerce_numeric(df[cols["station"]]).astype(int),
        "DATE_UTC": date_local.dt.date,
        "HOUR_UTC": 12,
        "VALUE": p_20_8_mm_new,      # BJT 20 - 次日 8
    })
    half_df = pd.concat([utc_0_12, utc_12_24], ignore_index=True)

    # ---- 关键补丁 A：VALUE 置空为 0（覆盖 None / NaN / '' / 空白）----
    half_df["VALUE"] = (
        pd.to_numeric(
            half_df["VALUE"]
                .replace(r"^\s*$", np.nan, regex=True),  # '' 或全空白 -> NaN
            errors="coerce"
        )
        .fillna(0.0)  # NaN/None -> 0
    )

    # 日尺度聚合（UTC 当天总降水 = 两个半日之和）
    daily = half_df.groupby(["StationID", "DATE_UTC"], as_index=False)["VALUE"].sum()

    # === 关键：补全所有“站点 × 日期”的组合，缺的补 0 ===
    # 全部站点
    all_stations = np.sort(daily["StationID"].unique())
    # 全部日期范围（以 half_df 的 DATE_UTC 为基准）
    date_min = pd.to_datetime(half_df["DATE_UTC"]).min()
    date_max = pd.to_datetime(half_df["DATE_UTC"]).max()
    all_dates = pd.date_range(date_min, date_max, freq="D").date  # 纯日期

    # 设为 MultiIndex，reindex 到完整网格
    daily = (daily
             .set_index(["StationID", "DATE_UTC"])
             .reindex(
        pd.MultiIndex.from_product([all_stations, all_dates],
                                   names=["StationID", "DATE_UTC"]),
        fill_value=0.0
    )
             .reset_index()
             )

    # 用真正 datetime 排序
    daily["DATETIME"] = pd.to_datetime(daily["DATE_UTC"])
    daily = daily.sort_values(["DATETIME", "StationID"]).reset_index(drop=True)

    # 一次性格式化到你要的样式（例如 "YYYY-MM-DD HH:MM"；如果要固定 0:00，这样写最稳）
    daily["DATETIME"] = daily["DATETIME"].dt.strftime("%Y-%m-%d") + " 0:00"

    # 仅保留必要列
    daily = daily[["StationID", "DATETIME", "VALUE"]].copy()

    # 构造结果字典（每个 DATETIME 下：{station_id: value}），值里再保险补 0
    result = {}
    for dt, group in daily.groupby("DATETIME", sort=False):
        vals = pd.to_numeric(group["VALUE"], errors="coerce").fillna(0.0)
        result[dt] = dict(zip(group["StationID"], vals.astype(float)))

    return result


def convert_cma_precip_to_utc_per_station(in_path, out_file, cols=None, type_label="P", tz_local="Asia/Shanghai"):
    """
    与 convert_cma_precip_to_utc_csv 相同的清洗/换算规则，但按“站点”分别导出：
    输出文件名：observed_<TYPE>_<StationID>.csv（例如 observed_P_322.csv）。
    返回 {station_id: filepath, ...}
    """
    if cols is None:
        cols = DEFAULT_COLS.copy()
    df = _read_any(in_path)
    # 必要列检查
    required = [cols["year"], cols["month"], cols["day"], cols["station"],
                cols["p_20_8"], cols["p_8_20"]]
    for c in required:
        if c not in df.columns:
            raise KeyError("输入缺少必要列：{}（现有列：{}）".format(c, list(df.columns)))

    daily = _to_daily_utc_dataframe(df, cols, type_label=type_label)

    if not os.path.exists(os.path.dirname(out_file)):
        os.makedirs(os.path.dirname(out_file))
    # 获取所有唯一的stationid并按字母顺序排序，确保列顺序一致
    all_station_ids = sorted(set(
        station_id
        for station_dict in daily.values()
        for station_id in station_dict.keys()
    ))

    # 创建DataFrame
    rows = []
    for datetime, station_dict in daily.items():
        row = {'DATETIME': datetime}
        # 为每个stationid添加对应的值，如果没有值则为空
        for station_id in all_station_ids:
            row[station_id] = station_dict.get(station_id, '')
        rows.append(row)

    df = pd.DataFrame(rows)

    # 确保列的顺序：DATETIME + 按字母排序的stationid
    columns = ['DATETIME'] + all_station_ids
    df = df[columns]

    # 写入文件
    with open(out_file, "w", encoding="utf-8", newline="") as f:
        f.write("#UTCTIME\n")

    df.to_csv(out_file, index=False, encoding="utf-8", mode="a")

    return {type_label: out_file}


# 把国家地球系统科学中心的国家气象站数据转为WISE需要的csv数据
if __name__ == "__main__":


    # 如果你的表头和示例不完全一样，也可以传自定义映射：
    custom_cols = {
        "year": "年", "month": "月", "day": "日",
        "station": "区站号",
        "p_20_8": "20-8时降水量",
        "p_8_20": "8-20时降水量",
        # "qc_20_20": "20-20时降水量质量控制码",
        "qc_20_8": "20-8时降水量质量控制码",
        "qc_8_20": "8-20时降水量质量控制码",
    }
    china_prcp_csv = r'J:\G\data\鄱阳湖数据\鄱阳湖流域国家气象站气象观测数据集（2010-2019年）\2010-2019\降水.csv'
    seims_prcp_dir = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\data_prepare\climate\pcp_daily_NESSDC.csv'
    convert_cma_precip_to_utc_per_station(china_prcp_csv, seims_prcp_dir, cols=custom_cols, type_label="P")
