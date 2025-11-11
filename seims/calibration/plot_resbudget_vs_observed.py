"""
在同一图上显示：
- 实测 Qobs（黑色折线，必画）
- 模拟 Qout（按 Level 着色的散点，必画）
- 可选：qin、ol、gw、upS、upI、upG（均来自 [ResBudget] 行末尾的 "parts:" 片段）

依赖：pandas, matplotlib, pymongo
"""
import re
from typing import Dict, List, Optional
from dataclasses import dataclass
import pandas as pd
from pymongo import MongoClient
import matplotlib.pyplot as plt


# ===== 你要显示哪些可选项（True/False） =====
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


def plot_sim_vs_obs(sim_df: pd.DataFrame, obs_df: pd.DataFrame,
                    title: str = "", out_png: Optional[str] = None):
    tmin = min(sim_df["time"].min(), obs_df["time"].min())
    tmax = max(sim_df["time"].max(), obs_df["time"].max())

    optional_keys = ["ol", "gw", "upS", "upI", "upG","gwSto"]
    need_sub_ax = any(SHOW.get(k, False) for k in optional_keys)

    # —— 这里把画布和高度比例加大 ——
    MAIN_SUB_RATIO = (3, 2)        # 主图:副图 = 3:2 ；可改 (2,2)、(5,3) 等
    FIGSIZE = (12, 7.8)            # 画布更高一些

    if need_sub_ax:
        fig, (ax1, ax2) = plt.subplots(
            2, 1, figsize=FIGSIZE, sharex=True,
            gridspec_kw={"height_ratios": list(MAIN_SUB_RATIO)}
        )
    else:
        fig, ax1 = plt.subplots(figsize=(12, 5))
        ax2 = None

    # === 主图：Qobs + qIn + qOut（按 Level 的散点） ===
    ax1.plot(obs_df["time"], obs_df["Qobs"], label="Observed Q",
             linewidth=1.5, color="black", alpha=0.85)
    ax1.plot(sim_df["time"], sim_df["qin"], label="Inflow qIn",
             linewidth=1.0, color="#8a8a8a", alpha=0.85)

    cmap = _color_map()
    for level, grp in sim_df.groupby("level"):
        ax1.scatter(grp["time"], grp["qout"], s=12, alpha=0.9,
                    color=cmap.get(level, None), label=f"Sim Qout [{level}]")

    ax1.set_xlim([tmin, tmax])
    ax1.set_ylabel("Discharge (m³/s)")
    if title:
        ax1.set_title(title)
    ax1.grid(True, alpha=0.25)
    ax1.legend(ncol=2, fontsize=9)

    # === 副图：可选分量（散点） ===
    if need_sub_ax and ax2 is not None:
        sub_colors = {
            "ol": "#1f78b4",
            "gw": "#33a02c",
            "upS": "#ff7f00",
            "upI": "#6a3d9a",
            "upG": "#e31a1c",
            "gwSto": "#17becf",  # 青色，易区分
        }
        ms = 10; alpha = 0.85
        for k in optional_keys:
            if SHOW.get(k, False) and k in sim_df.columns:
                sub = sim_df[["time", k]].dropna()
                ax2.scatter(sub["time"], sub[k], s=ms, alpha=alpha,
                            label=k, color=sub_colors.get(k, None))

        ax2.set_xlim([tmin, tmax])
        ax2.set_ylabel("m³/s")
        ax2.set_xlabel("Time")
        ax2.grid(True, alpha=0.25)
        ax2.legend(ncol=5, fontsize=9)
    else:
        ax1.set_xlabel("Time")

    fig.autofmt_xdate()
    if out_png:
        plt.tight_layout()
        plt.savefig(out_png, dpi=160)
    plt.tight_layout()
    return (fig, (ax1, ax2)) if need_sub_ax else (fig, ax1)




if __name__ == "__main__":
    LOG_PATH = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_141\debuglog5.txt"
    MONGO_URI = "mongodb://localhost:27017"
    STATION_ID = 141

    sim = parse_resbudget_log(LOG_PATH, tz=None)
    obs = fetch_observed_timeseries(MONGO_URI, station_id=STATION_ID)
    plot_sim_vs_obs(sim, obs, title=f"Station {STATION_ID} Sim vs Obs", out_png="sim_vs_obs.png")
    plt.show()
