
"""
plot_resbudget_vs_observed.py

功能：
1) 解析你调试日志中的 [ResBudget] 输出行，提取时间、Qout（m3/s）、Level。
2) 连接 MongoDB，从 poyang_lake1_HydroClimate 数据库的 MEASUREMENT 表读取 STATIONID=141、TYPE="Q" 的实测流量时间序列。
3) 将模拟与实测绘制在一张图上；模拟曲线按 Level 着色。

依赖：pandas, matplotlib, pymongo
安装：pip install pandas matplotlib pymongo
"""

import re
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
import pandas as pd
from pymongo import MongoClient
import matplotlib.pyplot as plt


@dataclass
class SimRecord:
    t: pd.Timestamp
    qout: float
    level: str


# 匹配时间戳行：例如 "2010-01-01 00:00:00"
TIME_LINE_RE = re.compile(r"^\s*(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})\s*$")

# 匹配 ResBudget 行，抓 level 和 Qout（最后一个 Qout=... m3/s）
# 示例行：
# [ResBudget] day=1 rchID=1176 Sto=... Fill=0.356809 level=(2Lc, Ln] (正常库容以下) Stage=2 qIn=479.242 m3/s Qout_raw=28.8228 m3/s Qout=28.8228 m3/s [Lc=..., Ln=..., ...]
RESBUDGET_RE = re.compile(
    r"\[ResBudget\].*?level\s*=\s*(?P<level>[^S\[]+?)\s+Stage=.*?\bQout\s*=\s*(?P<qout>[+-]?\d+(?:\.\d+)?)\s*m3/s",
    re.IGNORECASE
)

# 规范化 Level 文本到固定标签
def normalize_level(level_raw: str) -> str:
    level_raw = level_raw.strip()
    # 移除括号后的中文注释
    level_clean = re.sub(r"\s*\(.*?\)\s*$", "", level_raw).strip()

    # 可能的变体映射
    substitutes = {
        "≤2Lc": "≤2Lc",
        "<=2Lc": "≤2Lc",
        "(2Lc, Ln]": "(2Lc, Ln]",
        "(Ln, Normal_Flood]": "(Ln, Normal_Flood]",
        "(Normal_Flood, Lf]": "(Normal_Flood, Lf]",
        ">Lf": ">Lf"
    }
    # 若日志里用中文区段名，做一轮启发式替换
    if "保守库容" in level_raw:
        return "≤2Lc"
    if "正常库容以下" in level_raw or "2Lc, Ln" in level_raw:
        return "(2Lc, Ln]"
    if "正常~可调洪" in level_raw or "Ln, Normal_Flood" in level_raw:
        return "(Ln, Normal_Flood]"
    if "可调洪~防洪上限" in level_raw or "Normal_Flood, Lf" in level_raw:
        return "(Normal_Flood, Lf]"
    if "超防洪" in level_raw or ">Lf" in level_raw:
        return ">Lf"

    # 直接映射或返回清洗文本
    return substitutes.get(level_clean, level_clean)


def parse_resbudget_log(log_path: str, tz: Optional[str] = None) -> pd.DataFrame:
    """
    解析日志文件，返回包含 ['time','Qout','Level'] 的 DataFrame。
    - tz: 指定时区字符串（如 "UTC", "Asia/Shanghai"），若 None 则按 naive 时间处理。
    """
    sim_records: List[SimRecord] = []
    current_time: Optional[pd.Timestamp] = None

    with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            # 先匹配时间行
            m_t = TIME_LINE_RE.match(line)
            if m_t:
                try:
                    ts = pd.to_datetime(m_t.group(1))
                    if tz:
                        ts = ts.tz_localize(tz) if ts.tzinfo is None else ts.tz_convert(tz)
                    current_time = ts
                except Exception:
                    # 忽略解析错误的时间行
                    pass
                continue

            # 匹配 ResBudget 行
            m = RESBUDGET_RE.search(line)
            if m and current_time is not None:
                qout = float(m.group("qout"))
                level_raw = m.group("level")
                level = normalize_level(level_raw)
                sim_records.append(SimRecord(current_time, qout, level))

    if not sim_records:
        raise ValueError("未在日志中解析到任何 [ResBudget] 记录，请检查文件格式。")

    df = pd.DataFrame([{"time": r.t, "Qout": r.qout, "Level": r.level} for r in sim_records])
    # 去重并按时间排序（有些日志一天内多条的话，也许你只要第一条/最后一条，视需要可聚合）
    df = df.sort_values("time").reset_index(drop=True)
    return df


def fetch_observed_timeseries(
    mongo_uri: str,
    station_id: int = 141,
    db_name: str = "poyang_lake1_HydroClimate",
    collection_name: str = "MEASUREMENT",
    type_value: str = "Q",
    time_field: str = "UTCDATETIME",
    value_field: str = "VALUE"
) -> pd.DataFrame:
    """
    读取 MongoDB 中的实测流量数据，返回 ['time','Qobs'] 的 DataFrame（UTC 时间）。
    - time_field 可按你的表结构改为 LOCALDATETIME/UTCDATETIME 等；
    """
    client = MongoClient(mongo_uri)
    col = client[db_name][collection_name]
    cursor = col.find(
        {"STATIONID": station_id, "TYPE": type_value},
        {time_field: 1, value_field: 1, "_id": 0}
    )

    rows = list(cursor)
    if not rows:
        raise ValueError("未查询到任何实测数据，请检查 STATIONID/TYPE/字段名。")

    obs = pd.DataFrame(rows)
    # 解析时间
    obs["time"] = pd.to_datetime(obs[time_field])
    # 若是带 Z 的 UTC 字符串，pandas 会自动识别为 UTC
    obs = obs.sort_values("time").rename(columns={value_field: "Qobs"})
    obs = obs[["time", "Qobs"]].reset_index(drop=True)
    return obs


def _color_map() -> Dict[str, str]:
    """
    定义 Level -> 颜色 的映射。
    你也可以替换为你喜欢的配色。
    """
    return {
        "≤2Lc": "#2ca02c",                 # 绿色
        "(2Lc, Ln]": "#1f77b4",            # 蓝色
        "(Ln, Normal_Flood]": "#ff7f0e",   # 橙色
        "(Normal_Flood, Lf]": "#d62728",   # 红色
        ">Lf": "#9467bd"                   # 紫色
    }


def plot_sim_vs_obs(sim_df: pd.DataFrame, obs_df: pd.DataFrame, title: str = "", out_png: Optional[str] = None):
    """
    画图：
    - 实测(Qobs)为黑色线；
    - 模拟(Qout)按 Level 分段着色（连续折线，颜色随 Level 变化）。
    """
    # 对两组数据按时间外连接，保证对齐（绘图时不强制对齐，但便于检视范围）
    tmin = min(sim_df["time"].min(), obs_df["time"].min())
    tmax = max(sim_df["time"].max(), obs_df["time"].max())

    fig, ax = plt.subplots(figsize=(12, 5))

    # 实测
    ax.plot(obs_df["time"], obs_df["Qobs"], label="Observed Q", linewidth=1.5, color="black", alpha=0.8)

    # 模拟：按 Level 分组绘制连续线段
    cmap = _color_map()
    for level, grp in sim_df.groupby("Level"):
        ax.plot(grp["time"], grp["Qout"], label=f"Sim Qout [{level}]", linewidth=1.2, color=cmap.get(level, None))

    ax.set_xlim([tmin, tmax])
    ax.set_ylabel("Discharge (m³/s)")
    ax.set_xlabel("Time")
    if title:
        ax.set_title(title)
    ax.grid(True, alpha=0.25)
    ax.legend(ncol=2, fontsize=9)
    fig.autofmt_xdate()

    if out_png:
        plt.tight_layout()
        plt.savefig(out_png, dpi=160)
    plt.tight_layout()
    return fig, ax


if __name__ == "__main__":
    # === 使用示例 ===
    # 1) 修改日志路径和 Mongo 连接字符串
    LOG_PATH = r"C:\path\to\your\debuglog.txt"  # ← 替换成你的日志文件路径
    MONGO_URI = "mongodb://localhost:27017"     # ← 替换为实际连接字符串
    STATION_ID = 141

    # 2) 解析模拟日志
    # 若你的时间是本地时间，可设置 tz="Asia/Shanghai"；若已是UTC或无需时区，保持 None。
    # sim = parse_resbudget_log(LOG_PATH, tz=None)

    # 3) 读取实测数据
    # obs = fetch_observed_timeseries(MONGO_URI, station_id=STATION_ID)

    # 4) 画图并保存
    # plot_sim_vs_obs(sim, obs, title=f"Station {STATION_ID} Sim vs Obs", out_png="sim_vs_obs.png")
    # plt.show()
    pass
