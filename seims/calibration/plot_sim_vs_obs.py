import re
from typing import Dict, List, Optional
from dataclasses import dataclass
import pandas as pd
from pymongo import MongoClient
import matplotlib.pyplot as plt

SHOW = {
    "ol":  True,
    "gw":  True,
    "upS": True,
    "upI": True,
    "upG": True,
    "gwSto": True
}
# ===========================================


@dataclass
class SimRecord:
    t: pd.Timestamp
    qout: float
    qin: float
    level: str
    ol: float = float("nan")
    ifl: float = float("nan")
    gw: float = float("nan")
    upS: float = float("nan")
    upI: float = float("nan")
    upG: float = float("nan")
    gwSto: float = float("nan")


TIME_LINE_RE = re.compile(r"^\s*(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})\s*$")
NUM = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"

# 抓 Level、qIn、Qout（同一行）
RESBUDGET_RE = re.compile(
    rf"\[ResBudget\].*?[Ll]evel\s*=\s*(?P<level>.*?)\s+Stage=.*?"
    rf"\bqIn\s*=\s*(?P<qin>{NUM})\s*m3/s.*?"
    rf"\bQout\s*=\s*(?P<qout>{NUM})\s*m3/s",
    re.IGNORECASE
)

PARTS_RE = re.compile(
    rf"parts:\s*ol=(?P<ol>{NUM})\s*if=(?P<ifl>{NUM})\s*gw=(?P<gw>{NUM})\s*"
    rf"upS=(?P<upS>{NUM})\s*upI=(?P<upI>{NUM})\s*upG=(?P<upG>{NUM})"
    rf"(?:.*?gwSto=(?P<gwSto>{NUM}))?",   # gwSto 可选，允许中间夹 precip=... / dt=...
    re.IGNORECASE
)


def normalize_level(level_raw: str) -> str:
    level_raw = level_raw.strip()
    level_clean = re.sub(r"\s*\(.*?\)\s*$", "", level_raw).strip()
    substitutes = {
        "≤2Lc": "≤2Lc",
        "<=2Lc": "≤2Lc",
        "(2Lc, Ln]": "(2Lc, Ln]",
        "(Ln, Normal_Flood]": "(Ln, Normal_Flood]",
        "(Normal_Flood, Lf]": "(Normal_Flood, Lf]",
        ">Lf": ">Lf"
    }
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
    return substitutes.get(level_clean, level_clean)


def parse_resbudget_log(log_path: str, tz: Optional[str] = None) -> pd.DataFrame:
    sim_records: List[SimRecord] = []
    current_time: Optional[pd.Timestamp] = None

    with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            m_t = TIME_LINE_RE.match(line)
            if m_t:
                try:
                    ts = pd.to_datetime(m_t.group(1))
                    if tz:
                        ts = ts.tz_localize(tz) if ts.tzinfo is None else ts.tz_convert(tz)
                    current_time = ts
                except Exception:
                    pass
                continue

            m = RESBUDGET_RE.search(line)
            if m and current_time is not None:
                qout = float(m.group("qout"))
                qin = float(m.group("qin"))
                level = m.group("level")

                # 可选的 parts
                ol = ifl = gw = upS = upI = upG = gwSto = float("nan")
                p = PARTS_RE.search(line)
                if p:
                    ol  = float(p.group("ol"))
                    ifl = float(p.group("ifl"))
                    gw  = float(p.group("gw"))
                    upS = float(p.group("upS"))
                    upI = float(p.group("upI"))
                    upG = float(p.group("upG"))
                    gwSto = float(p.group("gwSto"))

                sim_records.append(SimRecord(current_time, qout, qin, level, ol, ifl, gw, upS, upI, upG,gwSto))

    if not sim_records:
        raise ValueError("未在日志中解析到任何 [ResBudget] 记录，请检查文件格式。")

    df = pd.DataFrame([r.__dict__ for r in sim_records]).sort_values("t").reset_index(drop=True)
    df = df.rename(columns={"t": "time"})
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
    client = MongoClient(mongo_uri)
    col = client[db_name][collection_name]
    rows = list(col.find({"STATIONID": station_id, "TYPE": type_value}, {time_field: 1, value_field: 1, "_id": 0}))
    if not rows:
        raise ValueError("未查询到任何实测数据，请检查 STATIONID/TYPE/字段名。")
    obs = pd.DataFrame(rows)
    obs["time"] = pd.to_datetime(obs[time_field])
    obs = obs.sort_values("time").rename(columns={value_field: "Qobs"})
    return obs[["time", "Qobs"]].reset_index(drop=True)


def _color_map() -> Dict[str, str]:
    # 保持原有颜色
    return {
        "≤2Lc": "#2ca02c",              # 绿
        "(2Lc, Ln]": "#1f77b4",         # 蓝
        "(Ln, Normal_Flood]": "#ff7f0e",# 橙
        "(Normal_Flood, Lf]": "#d62728",# 红
        ">Lf": "#9467bd"                # 紫
    }


import numpy as np

def plot_sim_vs_obs(sim_df: pd.DataFrame, obs_df: pd.DataFrame,
                    title: str = "", out_png: Optional[str] = None):
    """
    只绘制“模拟值和观测值同时存在”的时间点，并计算 NSE。
    返回: fig, ax1, nse
    """

    # === 1. 先对齐时间，只保留两边都有值的行 ===
    # 需要保留的模拟列
    sim_cols = ["time", "qout", "level"]
    if "qin" in sim_df.columns:
        sim_cols.append("qin")

    sim_sub = sim_df[sim_cols].copy()
    obs_sub = obs_df[["time", "Qobs"]].copy()

    merged = pd.merge(obs_sub, sim_sub, on="time", how="inner").sort_values("time")

    if merged.empty:
        raise ValueError("sim_df 和 obs_df 没有任何重叠时间点，无法绘图和计算 NSE。")

    # === 2. 计算 NSE ===
    qobs = merged["Qobs"].to_numpy(dtype=float)
    qsim = merged["qout"].to_numpy(dtype=float)

    if len(qobs) < 2 or np.allclose(qobs, qobs.mean()):
        nse = float("nan")
    else:
        nse = 1.0 - np.sum((qsim - qobs) ** 2) / np.sum((qobs - qobs.mean()) ** 2)

    # === 3. 绘图（只用合并后的 merged） ===
    tmin = merged["time"].min()
    tmax = merged["time"].max()

    fig, ax1 = plt.subplots(figsize=(12, 5))

    # 观测
    ax1.plot(
        merged["time"], merged["Qobs"],
        label="Observed Q",
        linewidth=1.5, color="black", alpha=0.85
    )

    # 入流（如果存在且不是全 NaN）
    if "qin" in merged.columns and not merged["qin"].isna().all():
        ax1.plot(
            merged["time"], merged["qin"],
            label="Inflow qIn",
            linewidth=1.0, color="#8a8a8a", alpha=0.85
        )

    # 模拟 Qout 按 Level 着色的散点
    cmap = _color_map()
    for level, grp in merged.groupby("level"):
        ax1.scatter(
            grp["time"], grp["qout"], s=12, alpha=0.9,
            color=cmap.get(level, None), label=f"Sim Qout [{level}]"
        )

    ax1.set_xlim([tmin, tmax])
    ax1.set_ylabel("Discharge (m³/s)")
    if title:
        ax1.set_title(title)
    ax1.grid(True, alpha=0.25)
    ax1.set_xlabel("Time")

    # 图例去重
    handles, labels = ax1.get_legend_handles_labels()
    seen = set()
    uniq_handles, uniq_labels = [], []
    for h, lab in zip(handles, labels):
        if lab not in seen:
            seen.add(lab)
            uniq_handles.append(h)
            uniq_labels.append(lab)
    ax1.legend(uniq_handles, uniq_labels, ncol=2, fontsize=9)

    # 在图上标注 NSE
    if not np.isnan(nse):
        ax1.text(
            0.02, 0.95,
            f"NSE = {nse:.3f}",
            transform=ax1.transAxes,
            ha="left", va="top",
            fontsize=10,
            bbox=dict(boxstyle="round", facecolor="white", alpha=0.7)
        )

    fig.autofmt_xdate()

    if out_png:
        plt.tight_layout()
        plt.savefig(out_png, dpi=160)

    plt.tight_layout()
    return fig, ax1, nse



def load_sim_from_qtxt(qtxt_path: str,
                       target_subbasin: int,
                       level_label: str = None) -> pd.DataFrame:
    """
    从多子流域的 Q.txt 中，提取指定 Subbasin 的模拟流量时间序列。

    文件格式示例：
        Subbasin: 141
        2010-01-01 00:00:00   86.03697968
        2010-01-02 00:00:00   60.70171356
        ...
        Subbasin: 322
        2010-01-01 00:00:00   3779.98
        ...

    只返回包含列: time, qout, qin(全 NaN), level, 以及其它分量(全 NaN)。
    """

    records = []
    in_block = False  # 当前是否在 target_subbasin 的数据块里

    with open(qtxt_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            # 子流域头行：Subbasin: 141
            if line.startswith("Subbasin"):
                # 解析子流域 id
                try:
                    sid = int(line.split(":", 1)[1])
                except Exception:
                    continue

                if sid == target_subbasin:
                    in_block = True
                    records = []   # 重新开始收集这一块的数据
                else:
                    # 如果刚刚结束了目标子流域的块，直接退出
                    if in_block:
                        break
                    in_block = False
                continue

            # 普通数据行，只在目标子流域块里收集
            if in_block:
                parts = line.split()
                if len(parts) < 3:
                    continue
                date_str, time_str, q_str = parts[0], parts[1], parts[2]
                records.append((date_str + " " + time_str, float(q_str)))

    if not records:
        raise ValueError(f"在 {qtxt_path} 中没有找到 Subbasin: {target_subbasin} 的数据。")

    df_raw = pd.DataFrame(records, columns=["time", "qout"])
    df_raw["time"] = pd.to_datetime(df_raw["time"])

    sim_df = pd.DataFrame()
    sim_df["time"] = df_raw["time"]
    sim_df["qout"] = df_raw["qout"]

    # 没有入流，就设为 NaN
    sim_df["qin"] = float("nan")

    # level 就写成子流域号
    if level_label is None:
        level_label = f"Subbasin {target_subbasin}"
    sim_df["level"] = level_label

    # 其它分量全部 NaN
    for col in ["ol", "ifl", "gw", "upS", "upI", "upG", "gwSto"]:
        sim_df[col] = float("nan")

    return sim_df


if __name__ == "__main__":
    # === 配置区 ===
    QTXT_PATH = r"C:\Users\David\Desktop\鄱阳\Q.txt"
    MONGO_URI = "mongodb://172.21.124.127:27019"
    STATION_ID = 141

    # 1) 从 Q.txt 读模拟流量
    sim = load_sim_from_qtxt(QTXT_PATH,STATION_ID, level_label="Q_sim")

    # 2) 从 MongoDB 读实测
    obs = fetch_observed_timeseries(MONGO_URI, station_id=STATION_ID)

    # 3) 画图（保持这两行不变）
    plot_sim_vs_obs(sim, obs, title=f"Station {STATION_ID} Sim vs Obs", out_png="sim_vs_obs.png")
    plt.show()
