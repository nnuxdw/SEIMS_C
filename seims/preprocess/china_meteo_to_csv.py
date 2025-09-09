
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

用法示例
--------
>>> from cma_rain_to_utc_csv import convert_cma_precip_to_utc_csv
>>> convert_cma_precip_to_utc_csv("input.xlsx", "rain_utc_daily.csv")

依赖
----
pandas>=1.4, python>=3.9（使用标准库 zoneinfo，无需 pytz）
"""
from __future__ import annotations
import os
import math
import pandas as pd
import numpy as np
from zoneinfo import ZoneInfo
from typing import Optional, Sequence, Union, Dict

# ----------------------------- 用户可修改区 ----------------------------- #
# 中文列名的默认映射（根据你的文件必要时调整）
DEFAULT_COLS = {
    "year": "年",
    "month": "月",
    "day": "日",
    "station": "区站号",
    "p_20_8": "20-8时降水量",
    "p_8_20": "8-20时降水量",
    "qc_20_20": "20-20时降水量质量控制码",
    # 可选：如果你想在缺失 qc_20_20 时退回到分段质控
    "qc_20_8": "20-8时降水量质量控制码",
    "qc_8_20": "8-20时降水量质量控制码",
}

# 一些常见的精确特殊码
SPECIAL_CODES_EXACT = {32700, 32744, 32766, 999990, 999999}

# ----------------------------- 工具函数 ----------------------------- #
def _read_any(path: str) -> pd.DataFrame:
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
        raise ValueError(f"不支持的文件类型：{ext}")

def _coerce_numeric(s: pd.Series) -> pd.Series:
    """将任意 Series 转为数值，无法解析的置 NaN。"""
    return pd.to_numeric(s, errors="coerce")

def _is_code_like(v: float) -> bool:
    """
    判断是否为应置零的“编码/异常值”。
    规则：NaN / 负数 / 精确特殊码 / [30000, ∞) 全部视为编码。
    其中 30xxx/31xxx/32xxx 将被自然包含在 (>=30000) 里。
    """
    if pd.isna(v):
        return True
    iv = int(v)
    if iv in SPECIAL_CODES_EXACT:
        return True
    if iv < 0:
        return True
    if iv >= 30000:
        return True
    return False

def _clean_precip_series(raw: pd.Series) -> pd.Series:
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

def _fmt_datetime_ymd_hm(dt: pd.Timestamp) -> str:
    """生成形如 YYYY/M/D H:MM 的字符串（与示例截图一致）。"""
    # 保证无前导零，小时这里固定为 0:00
    return f"{dt.year}/{dt.month}/{dt.day} {dt.hour}:00"

# ----------------------------- 主函数 ----------------------------- #
def convert_cma_precip_to_utc_csv(
    in_path: str,
    out_csv: str,
    *,
    cols: Dict[str, str] = DEFAULT_COLS,
    type_label: str = "P",
    tz_local: str = "Asia/Shanghai",
) -> pd.DataFrame:
    """
    读取 CMA 20-8/8-20 降水（北京时间），清洗编码，换算到 UTC，并导出日尺度 CSV。

    参数
    ----
    in_path : 输入 Excel/CSV 路径
    out_csv : 输出 CSV 路径（Figure2 风格）
    cols    : 列名映射（如与你的文件不一致可传入修改）
    type_label : 导出 Type 字段（默认 'P'）
    tz_local   : 源数据时区，默认 'Asia/Shanghai'

    返回
    ----
    返回导出的 DataFrame（便于在 notebook 里直接查看）。
    """
    df = _read_any(in_path)
    # 必要列存在性检查
    required = [cols["year"], cols["month"], cols["day"], cols["station"],
                cols["p_20_8"], cols["p_8_20"]]
    for c in required:
        if c not in df.columns:
            raise KeyError(f"输入缺少必要列：{c}（现有列：{list(df.columns)}）")

    # 仅保留 20-20 质控为 0 的记录；若缺失该列，则退回到分段质控全为 0
    if cols.get("qc_20_20") in df.columns:
        df = df[df[cols["qc_20_20"]].fillna(1).astype(int) == 0].copy()
    else:
        qc_ok = pd.Series(True, index=df.index)
        for seg in ("qc_20_8", "qc_8_20"):
            c = cols.get(seg)
            if c and (c in df.columns):
                qc_ok &= (df[c].fillna(1).astype(int) == 0)
        df = df[qc_ok].copy()

    # 清洗数值并转换单位（0.1 mm -> mm）
    p_20_8_mm = _clean_precip_series(df[cols["p_20_8"]])
    p_8_20_mm = _clean_precip_series(df[cols["p_8_20"]])

    # 组装日期（本地时区用于理解，但我们只需要 UTC 日聚合）
    date_local = pd.to_datetime(
        dict(year=_coerce_numeric(df[cols["year"]]).astype(int),
             month=_coerce_numeric(df[cols["month"]]).astype(int),
             day=_coerce_numeric(df[cols["day"]]).astype(int)),
        errors="coerce",
    )
    if date_local.isna().any():
        bad = df[date_local.isna()].index[:5].tolist()
        raise ValueError(f"存在无法解析的日期，示例行索引：{bad}")

    # 半日（UTC）记录：
    # - 8-20(BJT) -> UTC 当天 00:00-12:00，归到当日 00:00
    # - 20-8(BJT) -> UTC 当天 12:00-24:00，归到当日 12:00
    # 这里我们先构造“UTC 日期”（不带时区的日期对象），然后再聚合为日尺度。
    utc_half_1 = pd.DataFrame({
        "StationID": _coerce_numeric(df[cols["station"]]).astype(int),
        "DATE_UTC": date_local.dt.date,         # UTC 当天
        "HOUR_UTC": 0,                          # 00:00 桶（0-12）
        "VALUE": p_8_20_mm                      # 0-12 累计（mm）
    })
    utc_half_2 = pd.DataFrame({
        "StationID": _coerce_numeric(df[cols["station"]]).astype(int),
        "DATE_UTC": date_local.dt.date,         # UTC 当天
        "HOUR_UTC": 12,                         # 12:00 桶（12-24）
        "VALUE": p_20_8_mm                      # 12-24 累计（mm）
    })
    half_df = pd.concat([utc_half_1, utc_half_2], ignore_index=True)

    # 日尺度聚合（UTC 当天总降水 = 两个半日之和）
    daily = (half_df
             .groupby(["StationID", "DATE_UTC"], as_index=False)["VALUE"]
             .sum())

    # 生成 Figure2 风格列
    # DATETIME 统一设为每天 0:00（UTC），字符串格式如 "2014/1/1 0:00"
    dt_series = pd.to_datetime(daily["DATE_UTC"])  # 时间点为 UTC 日期 00:00
    daily["DATETIME"] = dt_series.dt.strftime("%Y/%-m/%-d 0:00") if os.name != "nt"             else dt_series.apply(lambda x: f"{x.year}/{x.month}/{x.day} 0:00")
    daily["Type"] = type_label
    daily = daily[["StationID", "DATETIME", "Type", "VALUE"]].copy()
    daily = daily.sort_values(["StationID", "DATETIME"]).reset_index(drop=True)

    # 导出 CSV
    daily.to_csv(out_csv, index=False, encoding="utf-8-sig")
    return daily

# 如果作为脚本直接运行，可在此放一个简单的 CLI（可选）
if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="CMA 20-8/8-20 降水(北京时间) -> UTC 日尺度 CSV")
    ap.add_argument("input", help="输入 Excel/CSV 文件路径")
    ap.add_argument("output", help="输出 CSV 文件路径")
    ap.add_argument("--type", default="P", help="输出 Type 字段（默认 P）")
    args = ap.parse_args()
    df_out = convert_cma_precip_to_utc_csv(args.input, args.output, type_label=args.type)
    print(f"已导出 {len(df_out)} 行到: {args.output}")
