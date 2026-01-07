# -*- coding: utf-8 -*-
"""
Monthly FI/BI grouped boxplot (Nature-style)
- Scan sim/obs tif folders
- Parse date from filename
- Pair by date
- Align obs raster to sim grid
- Compute FI/BI
- Aggregate by month
- Plot grouped boxplot: FI & BI as two boxes per month
"""

import re
from pathlib import Path
from datetime import date
from collections import defaultdict
from typing import Dict, List, Tuple, Union, Optional

import numpy as np
import rasterio
from rasterio.warp import reproject, Resampling

import matplotlib as mpl
import matplotlib.pyplot as plt
import os


# =========================================================
# 1) 日期解析：尽量宽容（你可按自己的命名再精确化）
# =========================================================
def parse_date_from_name(fname: str) -> date:
    """
    从文件名中解析日期，支持：
    - YYYY-MM-DD
    - YYYY_MM_DD / YYYY_M_D / YYYY.D.D
    - YYYYMMDD
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

    raise ValueError("无法从文件名解析日期: %s" % fname)


# =========================================================
# 2) 栅格对齐：将 obs 对齐到 sim 网格（更稳）
# =========================================================
def read_and_align_rasters(
    sim_tif: Union[str, Path],
    obs_tif: Union[str, Path],
    debug: bool = False
) -> Tuple[np.ndarray, np.ndarray, Optional[float], Optional[float]]:
    """
    读取 sim/obs 的 band1，并把 obs 对齐到 sim 的网格（shape/transform/crs）。
    若二者 CRS/transform/shape 任一不同：将 obs 重投影/重采样到 sim 网格（nearest）。
    """
    sim_tif = Path(sim_tif)
    obs_tif = Path(obs_tif)

    with rasterio.open(sim_tif) as ssrc:
        sim_arr = ssrc.read(1)
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
            print("[align] sim/obs 已同网格，无需对齐。")
        return sim_arr, obs_arr, sim_nodata, obs_nodata

    # 将 obs 对齐到 sim 网格
    fill_value = obs_nodata if obs_nodata is not None else 0
    obs_aligned = np.full(sim_shape, fill_value, dtype=obs_arr.dtype)

    reproject(
        source=obs_arr,
        destination=obs_aligned,
        src_transform=obs_transform,
        src_crs=obs_crs,
        dst_transform=sim_transform,
        dst_crs=sim_crs,
        resampling=Resampling.nearest,  # 分类/淹没边界用 nearest 更合适
        src_nodata=obs_nodata,
        dst_nodata=obs_nodata,
    )

    if debug:
        print("[align] 已将 obs 对齐到 sim 网格。")
        print("        sim shape:", sim_shape, " obs shape:", obs_arr.shape, "->", obs_aligned.shape)

    return sim_arr, obs_aligned, sim_nodata, obs_nodata


# =========================================================
# 3) FI / BI 计算（你若已有同名函数，可直接替换这段）
# =========================================================
def compute_fi_bi(
    sim_arr: np.ndarray,
    obs_arr: np.ndarray,
    sim_nodata: Optional[float] = None,
    obs_nodata: Optional[float] = None,
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
) -> Tuple[float, float]:
    """
    根据二值淹没（> thresh）计算：
      hits = sim_inund & obs_inund
      FI = hits / obs_inund
      BI = hits / sim_inund

    若分母为 0，则返回 NaN
    """
    # 有效像元掩膜（排除 nodata）
    if sim_nodata is None:
        sim_valid = np.ones(sim_arr.shape, dtype=bool)
    else:
        sim_valid = (sim_arr != sim_nodata)

    if obs_nodata is None:
        obs_valid = np.ones(obs_arr.shape, dtype=bool)
    else:
        obs_valid = (obs_arr != obs_nodata)

    valid = sim_valid & obs_valid

    sim_bin = (sim_arr > sim_thresh) & valid
    obs_bin = (obs_arr > obs_thresh) & valid

    hits = np.sum(sim_bin & obs_bin)
    obs_inund = np.sum(obs_bin)
    sim_inund = np.sum(sim_bin)

    FI = (hits / obs_inund) if obs_inund > 0 else np.nan
    BI = (hits / sim_inund) if sim_inund > 0 else np.nan
    return float(FI), float(BI)


# =========================================================
# 4) 批量：按日期配对 -> 计算 FI/BI -> 按月份汇总
# =========================================================
def collect_fi_bi_by_month(
    sim_dir: Union[str, Path],
    obs_dir: Union[str, Path],
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
    debug: bool = True
) -> Tuple[Dict[int, List[float]], Dict[int, List[float]], List[Tuple[date, float, float]]]:
    sim_dir = Path(sim_dir)
    obs_dir = Path(obs_dir)

    # 建立 obs: date -> path
    obs_map: Dict[date, Path] = {}
    for p in obs_dir.glob("*.tif"):
        try:
            d = parse_date_from_name(p.name)
            obs_map[d] = p
        except ValueError:
            if debug:
                print("[obs] 跳过无法解析日期:", p.name)

    month_to_fi = defaultdict(list)  # type: ignore
    month_to_bi = defaultdict(list)  # type: ignore
    records: List[Tuple[date, float, float]] = []

    for sim_p in sorted(sim_dir.glob("*.tif")):
        try:
            d = parse_date_from_name(sim_p.name)
        except ValueError:
            if debug:
                print("[sim] 跳过无法解析日期:", sim_p.name)
            continue

        obs_p = obs_map.get(d, None)
        if obs_p is None:
            if debug:
                print("[pair] 缺少匹配 obs，跳过:", d, " sim=", sim_p.name)
            continue

        # 读并对齐（obs -> sim）
        sim_arr, obs_arr, sim_nodata, obs_nodata = read_and_align_rasters(sim_p, obs_p, debug=False)

        FI, BI = compute_fi_bi(
            sim_arr, obs_arr,
            sim_nodata=sim_nodata,
            obs_nodata=obs_nodata,
            sim_thresh=sim_thresh,
            obs_thresh=obs_thresh,
        )

        if debug:
            print("[fi/bi] %s  FI=%.4f  BI=%.4f  sim=%s  obs=%s"
                  % (d.isoformat(), FI, BI, sim_p.name, obs_p.name))

        if not np.isnan(FI):
            month_to_fi[d.month].append(FI)
        if not np.isnan(BI):
            month_to_bi[d.month].append(BI)

        records.append((d, FI, BI))

    return dict(month_to_fi), dict(month_to_bi), records


# =========================================================
# 5) Nature 风格分组箱线图：每月两个箱体（FI & BI）
# =========================================================
def _set_nature_style_box():
    mpl.rcParams.update({
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


def plot_monthly_grouped_boxplot(
    month_to_fi: Dict[int, List[float]],
    month_to_bi: Dict[int, List[float]],
    out_png: Union[str, Path],
    out_pdf: Optional[Union[str, Path]] = None,
    title: str = "Monthly FI and BI",
    y_label: str = "Score",
    y_lim: Tuple[float, float] = (0.0, 1.0),
    show_points: bool = True,
):
    _set_nature_style_box()

    out_png = Path(out_png)
    out_png.parent.mkdir(parents=True, exist_ok=True)
    if out_pdf is not None:
        out_pdf = Path(out_pdf)
        out_pdf.parent.mkdir(parents=True, exist_ok=True)

    months = np.arange(1, 13)

    # 每月两个箱体位置：FI 在左，BI 在右
    offset = 0.18
    pos_fi = months - offset
    pos_bi = months + offset
    box_width = 0.28

    data_fi = [month_to_fi.get(int(m), []) for m in months]
    data_bi = [month_to_bi.get(int(m), []) for m in months]

    # 克制配色
    c_fi = "#1f9e89"   # teal
    c_bi = "#f18f01"   # orange
    edge = "#111111"

    fig, ax = plt.subplots(figsize=(6.6, 3.2))

    # --- FI 箱线图 ---
    bp_fi = ax.boxplot(
        data_fi,
        positions=pos_fi,
        widths=box_width,
        patch_artist=True,
        showfliers=False,
        whis=(5, 95),  # 更稳健；想用传统 1.5*IQR 可改 whis=1.5
        medianprops=dict(color=edge, linewidth=1.2),
        boxprops=dict(linewidth=0.8, color=edge),
        whiskerprops=dict(linewidth=0.8, color=edge),
        capprops=dict(linewidth=0.8, color=edge),
    )
    for b in bp_fi["boxes"]:
        b.set_facecolor(c_fi)
        b.set_alpha(0.85)

    # --- BI 箱线图 ---
    bp_bi = ax.boxplot(
        data_bi,
        positions=pos_bi,
        widths=box_width,
        patch_artist=True,
        showfliers=False,
        whis=(5, 95),
        medianprops=dict(color=edge, linewidth=1.2),
        boxprops=dict(linewidth=0.8, color=edge),
        whiskerprops=dict(linewidth=0.8, color=edge),
        capprops=dict(linewidth=0.8, color=edge),
    )
    for b in bp_bi["boxes"]:
        b.set_facecolor(c_bi)
        b.set_alpha(0.85)

    # --- 叠加散点（显示样本量/分布） ---
    if show_points:
        rng = np.random.default_rng(42)
        jitter = 0.07

        def _scatter(points_list: List[List[float]], base_positions: np.ndarray):
            for vals, x0 in zip(points_list, base_positions):
                arr = np.asarray(vals, dtype=float)
                arr = arr[~np.isnan(arr)]
                if arr.size == 0:
                    continue
                xs = x0 + (rng.random(arr.size) - 0.5) * 2 * jitter
                ax.scatter(xs, arr, s=10, linewidths=0, alpha=0.55, zorder=3)

        _scatter(data_fi, pos_fi)
        _scatter(data_bi, pos_bi)

    # --- Nature 风格轴 ---
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)

    ax.set_xlim(0.3, 12.7)
    ax.set_ylim(y_lim[0], y_lim[1])
    ax.set_xticks(months)
    ax.set_xticklabels([str(int(m)) for m in months])
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

    print("[save]", str(out_png))
    if out_pdf is not None:
        print("[save]", str(out_pdf))


# =========================================================
# 6) 一键入口：从文件夹 -> 计算 -> 箱线图
# =========================================================
def run_monthly_fi_bi_boxplot(
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

    if debug:
        print("\n[done] 配对并计算完成，记录数 =", len(records))
        for m in range(1, 13):
            print("  Month %02d: FI n=%d, BI n=%d"
                  % (m, len(month_to_fi.get(m, [])), len(month_to_bi.get(m, []))))

    plot_monthly_grouped_boxplot(
        month_to_fi=month_to_fi,
        month_to_bi=month_to_bi,
        out_png=out_png,
        out_pdf=out_pdf,
        title="Monthly FI and BI",
        y_label="FI / BI",
        y_lim=(0.0, 1.0),
        show_points=True,
    )

""" 画箱线图 """
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
    # 5. 每个月的FI BI绘制箱线图
    violin_png_path = os.path.join(inundation_base_path, 'plot_box', 'poyang_box.png')
    violin_pdf_path = os.path.join(inundation_base_path, 'plot_box', 'poyang_box.pdf')
    run_monthly_fi_bi_boxplot(
        sim_dir=clip_sim_dir,
        obs_dir=clip_obs_dir,
        out_png=violin_png_path,
        out_pdf=violin_pdf_path,
        sim_thresh=0.1,
        obs_thresh=0.1,
        debug=True
    )
