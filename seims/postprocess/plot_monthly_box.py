"""
    ------------------------------绘制半小提琴图-----------------------------------
"""
import re
from pathlib import Path
from datetime import date
from collections import defaultdict
from typing import Union, Tuple, Dict, List, Optional

import numpy as np
import rasterio
from rasterio.warp import reproject, Resampling

import matplotlib as mpl
import matplotlib.pyplot as plt
import os

def compute_fi_bi(
    sim_arr: np.ndarray,
    obs_arr: np.ndarray,
    sim_nodata: float = None,
    obs_nodata: float = None,
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
    *,
    auto_crop_to_common: bool = True,
    debug: bool = True
):
    """
    计算 FI 和 BI（鲁棒版）

    解决点：
    - 若 sim/obs shape 不一致，默认裁剪到共同最小窗口（避免 boolean index mismatch）
    - 模拟 nodata 位置 → 同时屏蔽模拟与观测（不参与比较）
    - 观测值 0 是有效像元；仅当 obs_nodata != 0 才屏蔽 obs_nodata

    返回：
    - FI, BI
    """

    sim = sim_arr.astype("float32", copy=False)
    obs = obs_arr.astype("float32", copy=False)

    if sim.ndim != 2 or obs.ndim != 2:
        raise ValueError(f"sim/obs 必须是二维数组，当前 sim.ndim={sim.ndim}, obs.ndim={obs.ndim}")

    # 0) shape 不一致：自动裁剪到共同窗口
    if sim.shape != obs.shape:
        if not auto_crop_to_common:
            raise ValueError(f"sim.shape={sim.shape} 与 obs.shape={obs.shape} 不一致，无法比较。")

        min_rows = min(sim.shape[0], obs.shape[0])
        min_cols = min(sim.shape[1], obs.shape[1])

        if debug:
            print(f"[WARN] sim/obs shape 不一致：sim={sim.shape}, obs={obs.shape} -> crop 到 ({min_rows}, {min_cols})")

        sim = sim[:min_rows, :min_cols].copy()
        obs = obs[:min_rows, :min_cols].copy()
    else:
        sim = sim.copy()
        obs = obs.copy()

    # 1) 模拟 nodata 位置 → 同时屏蔽模拟与观测
    if sim_nodata is not None:
        invalid_mask = (sim == sim_nodata)
    else:
        invalid_mask = np.zeros(sim.shape, dtype=bool)

    sim[invalid_mask] = np.nan
    obs[invalid_mask] = np.nan

    # 2) 观测 nodata（若不为 0）屏蔽
    if obs_nodata is not None and obs_nodata != 0:
        obs[obs == obs_nodata] = np.nan

    # 3) 有效比较区域
    valid_mask = (~np.isnan(sim)) & (~np.isnan(obs))
    if not np.any(valid_mask):
        if debug:
            print("[WARN] valid_mask 全 False：没有可比较像元（可能全是 nodata/NaN）")
        return np.nan, np.nan

    sim_valid = sim[valid_mask]
    obs_valid = obs[valid_mask]

    # 4) 阈值判断
    sim_bin = sim_valid > sim_thresh
    obs_bin = obs_valid > obs_thresh

    # 5) 分类
    hit = sim_bin & obs_bin
    miss = (~sim_bin) & obs_bin
    false_alarm = sim_bin & (~obs_bin)

    H = int(hit.sum())
    M = int(miss.sum())
    F = int(false_alarm.sum())

    if debug:
        print(f"[DEBUG] shape={sim.shape}, valid_pixels={int(valid_mask.sum())}, H={H}, M={M}, F={F}")

    # FI: 交并比
    FI = np.nan if (H + M + F) == 0 else H / (H + M + F)

    # BI: (Sim/Obs) - 1
    BI = np.nan if (H + M) == 0 else (H + F) / (H + M) - 1

    return FI, BI

# ----------------------------
# 1) 日期解析：尽量“宽容”
# ----------------------------
def parse_date_from_name(fname: str):
    """
    从文件名中解析日期，支持：
    - YYYY-MM-DD
    - YYYY_MM_DD / YYYY_M_D
    - YYYYMMDD
    解析不到就抛 ValueError
    """
    s = Path(fname).stem

    # 1) YYYY-MM-DD / YYYY_MM_DD / YYYY.M.D
    m = re.search(r"(20\d{2})[-_.](\d{1,2})[-_.](\d{1,2})", s)
    if m:
        y, mo, d = map(int, m.groups())
        return date(y, mo, d)

    # 2) YYYYMMDD
    m = re.search(r"(20\d{2})(\d{2})(\d{2})", s)
    if m:
        y, mo, d = map(int, m.groups())
        return date(y, mo, d)

    raise ValueError(f"无法从文件名解析日期: {fname}")


# ----------------------------
# 2) 读栅格 + 自动对齐（更稳）
# ----------------------------
def read_and_align_rasters(
    sim_tif: Union[str, Path],
    obs_tif: Union[str, Path],
    debug: bool = False
):
    """
    读取 sim/obs 的 band1，并把 obs 对齐到 sim 的网格（shape/transform/crs）。
    - 若二者 CRS/transform/shape 不同：将 obs 重投影/重采样到 sim 网格（nearest）
    - 若只 shape 不同但 transform/crs 一样：也会走重投影流程确保严格一致

    返回：
    sim_arr, obs_arr_aligned, sim_nodata, obs_nodata
    """
    sim_tif = Path(sim_tif)
    obs_tif = Path(obs_tif)

    with rasterio.open(sim_tif) as ssrc:
        sim_arr = ssrc.read(1)
        sim_meta = ssrc.meta.copy()
        sim_crs = ssrc.crs
        sim_transform = ssrc.transform
        sim_nodata = ssrc.nodata
        sim_shape = sim_arr.shape

    with rasterio.open(obs_tif) as osrc:
        obs_arr = osrc.read(1)
        obs_crs = osrc.crs
        obs_transform = osrc.transform
        obs_nodata = osrc.nodata

    need_reproject = (
        (sim_crs != obs_crs) or
        (sim_transform != obs_transform) or
        (obs_arr.shape != sim_shape)
    )

    if not need_reproject:
        if debug:
            print("[align] sim/obs 已在同一网格，无需对齐。")
        return sim_arr, obs_arr, sim_nodata, obs_nodata

    # 将 obs 对齐到 sim 网格
    obs_aligned = np.full(sim_shape, obs_nodata if obs_nodata is not None else 0, dtype=obs_arr.dtype)

    reproject(
        source=obs_arr,
        destination=obs_aligned,
        src_transform=obs_transform,
        src_crs=obs_crs,
        dst_transform=sim_transform,
        dst_crs=sim_crs,
        resampling=Resampling.nearest,  # 淹没范围一般用 nearest
        src_nodata=obs_nodata,
        dst_nodata=obs_nodata,
    )

    if debug:
        print("[align] 已将 obs 重投影/对齐到 sim 网格。")
        print(f"        sim shape: {sim_shape}, obs shape: {obs_arr.shape} -> {obs_aligned.shape}")

    return sim_arr, obs_aligned, sim_nodata, obs_nodata


# ----------------------------
# 3) 批量计算：按日期配对 -> 月份汇总
# ----------------------------
def collect_fi_bi_by_month(
    sim_dir: Union[str, Path],
    obs_dir: Union[str, Path],
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
    debug: bool = True
):
    """
    读取两个目录下所有 tif：
    - 用文件名解析日期
    - 按日期一一匹配
    - 计算 FI/BI
    - 汇总到 month -> list

    返回：
    month_to_fi, month_to_bi, records[(date, FI, BI)]
    """
    sim_dir = Path(sim_dir)
    obs_dir = Path(obs_dir)

    # 建 obs 映射：date -> path
    obs_map: dict[date, Path] = {}
    for p in obs_dir.glob("*.tif"):
        try:
            d = parse_date_from_name(p.name)
            obs_map[d] = p
        except ValueError:
            if debug:
                print(f"[obs] 跳过无法解析日期的文件: {p.name}")

    month_to_fi = defaultdict(list)
    month_to_bi = defaultdict(list)
    records: list[tuple[date, float, float]] = []

    # 遍历 sim
    for sim_p in sorted(sim_dir.glob("*.tif")):
        try:
            d = parse_date_from_name(sim_p.name)
        except ValueError:
            if debug:
                print(f"[sim] 跳过无法解析日期的文件: {sim_p.name}")
            continue

        obs_p = obs_map.get(d)
        if obs_p is None:
            if debug:
                print(f"[pair] {d} 找不到匹配 obs，跳过。sim={sim_p.name}")
            continue

        # 读 & 对齐
        sim_arr, obs_arr, sim_nodata, obs_nodata = read_and_align_rasters(sim_p, obs_p, debug=False)

        # 计算 FI/BI（你已有的函数）
        FI, BI = compute_fi_bi(
            sim_arr, obs_arr,
            sim_nodata=sim_nodata,
            obs_nodata=obs_nodata,
            sim_thresh=sim_thresh,
            obs_thresh=obs_thresh,
        )

        if debug:
            print(f"[fi/bi] {d}  FI={FI:.4f}  BI={BI:.4f}   sim={sim_p.name}  obs={obs_p.name}")

        if FI is not None and not np.isnan(FI):
            month_to_fi[d.month].append(float(FI))
        if BI is not None and not np.isnan(BI):
            month_to_bi[d.month].append(float(BI))

        records.append((d, float(FI) if FI is not None else np.nan, float(BI) if BI is not None else np.nan))

    return dict(month_to_fi), dict(month_to_bi), records


# ----------------------------
# 4) Nature 风格半小提琴绘图
#    左半：FI，右半：BI
# ----------------------------
def _set_nature_style():
    mpl.rcParams.update({
        # 字体：Nature 常见是无衬线；你也可改 Times New Roman
        "font.family": "DejaVu Sans",
        "font.size": 9,
        "axes.titlesize": 10,
        "axes.labelsize": 9,
        "xtick.labelsize": 8.5,
        "ytick.labelsize": 8.5,
        "axes.linewidth": 0.8,
        "xtick.major.width": 0.8,
        "ytick.major.width": 0.8,
        "xtick.major.size": 3,
        "ytick.major.size": 3,
        "figure.dpi": 150,
        "savefig.dpi": 600,
    })


def _half_violin(ax, data, pos, side: str, width: float, facecolor: str, edgecolor: str = "#111111", alpha: float = 0.9):
    """
    用 matplotlib.violinplot 画一个 violin，并把其裁成左/右半边
    side: "left" 或 "right"
    """
    if data is None or len(data) == 0:
        return

    parts = ax.violinplot(
        dataset=[data],
        positions=[pos],
        widths=width,
        showmeans=False,
        showmedians=False,
        showextrema=False,
    )
    body = parts["bodies"][0]
    body.set_facecolor(facecolor)
    body.set_edgecolor(edgecolor)
    body.set_alpha(alpha)
    body.set_linewidth(0.6)

    # 裁半边：把另一侧的 x 坐标压到中心线
    path = body.get_paths()[0]
    verts = path.vertices
    if side == "left":
        verts[:, 0] = np.minimum(verts[:, 0], pos)
    elif side == "right":
        verts[:, 0] = np.maximum(verts[:, 0], pos)
    else:
        raise ValueError("side must be 'left' or 'right'")


def _add_summary_and_points(ax, data, pos, side: str, jitter: float, point_size: float = 10):
    """
    在半小提琴上叠加：散点（轻微抖动）、中位数点、IQR 线
    """
    if data is None or len(data) == 0:
        return

    data = np.asarray(data, dtype=float)
    data = data[~np.isnan(data)]
    if len(data) == 0:
        return

    # 点：沿半边抖动
    rng = np.random.default_rng(42)
    if side == "left":
        xs = pos - rng.random(len(data)) * jitter
    else:
        xs = pos + rng.random(len(data)) * jitter

    ax.scatter(xs, data, s=point_size, linewidths=0, alpha=0.55, zorder=3)

    # 中位数与IQR
    q1, med, q3 = np.percentile(data, [25, 50, 75])
    x_med = pos - jitter * 0.55 if side == "left" else pos + jitter * 0.55
    ax.scatter([x_med], [med], s=24, zorder=4)

    # IQR 竖线（更像 Nature 常见的“简洁统计提示”）
    ax.plot([x_med, x_med], [q1, q3], linewidth=1.2, zorder=4)


def plot_monthly_split_half_violin(
    month_to_fi: Dict[int, List[float]],
    month_to_bi: Dict[int, List[float]],
    out_png: Union[str, Path],
    out_pdf: Optional[Union[str, Path]] = None,
    y_label: str = "Score",
    title: str = "Monthly FI (left) and BI (right)",
    y_lim: Tuple[float, float] = (0.0, 1.0),
):
    _set_nature_style()

    out_png = Path(out_png)
    out_png.parent.mkdir(parents=True, exist_ok=True)
    if out_pdf is not None:
        out_pdf = Path(out_pdf)
        out_pdf.parent.mkdir(parents=True, exist_ok=True)

    # 配色（克制、对比明确）
    c_fi = "#1f9e89"   # teal
    c_bi = "#f18f01"   # orange

    fig, ax = plt.subplots(figsize=(6.6, 3.2))  # 期刊常见横向短图

    months = np.arange(1, 13)
    width = 0.78
    jitter = 0.28

    # 画 12 个月
    for m in months:
        fi = month_to_fi.get(m, [])
        bi = month_to_bi.get(m, [])

        # 左半 FI
        _half_violin(ax, fi, pos=m, side="left", width=width, facecolor=c_fi)
        _add_summary_and_points(ax, fi, pos=m, side="left", jitter=jitter)

        # 右半 BI
        _half_violin(ax, bi, pos=m, side="right", width=width, facecolor=c_bi)
        _add_summary_and_points(ax, bi, pos=m, side="right", jitter=jitter)

        # 中心线（让“半小提琴”分割更明确）
        ax.vlines(m, y_lim[0], y_lim[1], linewidth=0.6, alpha=0.35)

    # 轴样式（Nature 风：去上右边框，网格极淡或不用）
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)

    ax.set_xlim(0.3, 12.7)
    ax.set_ylim(*y_lim)
    ax.set_xticks(months)
    ax.set_xticklabels([str(i) for i in months])
    ax.set_xlabel("Month")
    ax.set_ylabel(y_label)
    ax.set_title(title, pad=6)

    # 图例（简洁）
    from matplotlib.patches import Patch
    ax.legend(
        handles=[
            Patch(facecolor=c_fi, edgecolor="none", label="FI (Hits / Obs)"),
            Patch(facecolor=c_bi, edgecolor="none", label="BI (Hits / Sim)"),
        ],
        loc="upper right",
        frameon=False,
        fontsize=8.5
    )

    plt.tight_layout()
    fig.savefig(out_png, bbox_inches="tight", pad_inches=0.02)
    if out_pdf is not None:
        fig.savefig(out_pdf, bbox_inches="tight", pad_inches=0.02)
    plt.close(fig)

    print(f"[save] {out_png}")
    if out_pdf is not None:
        print(f"[save] {out_pdf}")


# ----------------------------
# 5) 一键入口
# ----------------------------
def run_monthly_fi_bi_half_violin(
    sim_dir: Union[str, Path],
    obs_dir: Union[str, Path],
    out_png: Union[str, Path],
    out_pdf: Optional[Union[str, Path]] = None,
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
    debug: bool = True
):
    month_to_fi, month_to_bi, records = collect_fi_bi_by_month(
        sim_dir=sim_dir,
        obs_dir=obs_dir,
        sim_thresh=sim_thresh,
        obs_thresh=obs_thresh,
        debug=debug
    )

    # 你如果希望额外保存逐日记录（date, FI, BI），可以自行写 csv
    # 这里仅给个提示：
    if debug:
        n = len(records)
        print(f"\n[done] 配对并计算完成，共 {n} 条记录。")
        for m in range(1, 13):
            print(f"  Month {m:02d}: FI n={len(month_to_fi.get(m, []))}, BI n={len(month_to_bi.get(m, []))}")

    plot_monthly_split_half_violin(
        month_to_fi=month_to_fi,
        month_to_bi=month_to_bi,
        out_png=out_png,
        out_pdf=out_pdf,
        y_label="FI / BI",
        title="Monthly FI (left) and BI (right)",
        y_lim=(0.0, 1.0),
    )


if __name__ == '__main__':
    if os.name == 'nt':  # Windows
        base_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171'
        obs_dir = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\2014-2023年鄱阳湖水域面积栅格数据"
        shp_dir = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\sci_figure_study_region\upstream_subbasin_1171.shp"
        dem_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_raster\dem.tif"
    else:  # Linux/Unix
        base_path = '/data/user/xiaodw/software/WISE/data/poyang_lake1/poyang_lake1_longterm_model_1171'
        obs_dir = '/data/user/xiaodw/software/WISE/data/poyang_lake1/鄱阳湖全天候面积逐日数据集/2014-2023年鄱阳湖水域面积栅格数据'
        shp_dir = r"/data/user/xiaodw/software/WISE/data/poyang_lake1/sci_figure_study_region/upstream_subbasin_1171.shp"
        dem_path = r"/data/user/xiaodw/software/WISE/data/poyang_lake1/workspace/spatial_raster/dem.tif"
    inundation_base_path = os.path.join(base_path, '淹没范围绘图')
    clip_sim_dir = os.path.join(inundation_base_path, 'cliped_sim_tif')
    clip_obs_dir = os.path.join(inundation_base_path, 'cliped_obs_tif')
    # 5. 每个月的FI BI绘制半小提琴图
    violin_png_path = os.path.join(inundation_base_path, 'plot_violin', 'poyang_violin.png')
    violin_pdf_path = os.path.join(inundation_base_path, 'plot_violin', 'poyang_violin.pdf')
    run_monthly_fi_bi_half_violin(
        sim_dir=clip_sim_dir,
        obs_dir=clip_obs_dir,
        out_png=violin_png_path,
        out_pdf=violin_pdf_path,
        sim_thresh=0.1,
        obs_thresh=0.1,
        debug=True
    )
