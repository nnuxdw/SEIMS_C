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
        # ✅ 全局字体：Times New Roman
        "font.family": "Times New Roman",
        "font.serif": ["Times New Roman", "Times", "DejaVu Serif"],  # 备用
        "mathtext.fontset": "custom",
        "mathtext.rm": "Times New Roman",
        "mathtext.it": "Times New Roman:italic",
        "mathtext.bf": "Times New Roman:bold",

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
        "savefig.dpi": 1000,
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


def _add_summary_and_points(
    ax,
    data,
    pos,
    side: str,
    jitter: float,
    point_size: float = 3,          # ✅ 样本点更小（原来 10）
    point_color: str = "black"      # ✅ 全部黑色
):
    """
    在半小提琴上叠加：散点（轻微抖动）、中位数点、IQR 线
    - 样本点：更小 & 黑色
    - 中位数点：黑色
    - IQR线：黑色
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

    # ✅ 样本散点：小一点 + 全黑
    ax.scatter(
        xs, data,
        s=point_size,
        c=point_color,
        linewidths=0,
        alpha=0.55,
        zorder=3
    )

    # 中位数与IQR
    q1, med, q3 = np.percentile(data, [25, 50, 75])
    x_med = pos - jitter * 0.55 if side == "left" else pos + jitter * 0.55

    # ✅ 中位数点：黑色（也可略大一点突出）
    ax.scatter(
        [x_med], [med],
        s=18,               # 原来 24，稍微收敛一点
        c=point_color,
        linewidths=0,
        zorder=4
    )

    # ✅ IQR 竖线：黑色
    ax.plot(
        [x_med, x_med], [q1, q3],
        color=point_color,
        linewidth=1.2,
        zorder=4
    )


def plot_monthly_split_half_violin(
    month_to_fi: Dict[int, List[float]],
    month_to_bi: Dict[int, List[float]],
    out_png: Union[str, Path],
    out_pdf: Optional[Union[str, Path]] = None,
    y_label: str = "Score",
    title: str = "Monthly FI (left) and BI (right)",
    y_lim: Tuple[float, float] = (-0.5, 1.0),
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

def plot_monthly_boxplots_two_panels(
    month_to_fi: Dict[int, List[float]],
    month_to_bi: Dict[int, List[float]],
    out_png: Union[str, Path],
    out_pdf: Optional[Union[str, Path]] = None,
    title: str = "Seasonal variation of FI and BI",
    fi_y_lim: Optional[Tuple[float, float]] = (0.0, 1.0),
    bi_y_lim: Optional[Tuple[float, float]] = None,   # None -> 自动
    show_points: bool = True,
    point_size: float = 8,
    point_alpha: float = 0.55,
    jitter: float = 0.18,
):
    """
    两行箱线图：上 FI，下 BI（按月份）
    - 箱体：Q1~Q3（IQR）
    - 中位数线：median
    - whisker：默认 1.5*IQR（matplotlib boxplot 默认）
    - 叠加散点：黑色小点（可关）
    """
    _set_nature_style()

    out_png = Path(out_png)
    out_png.parent.mkdir(parents=True, exist_ok=True)
    if out_pdf is not None:
        out_pdf = Path(out_pdf)
        out_pdf.parent.mkdir(parents=True, exist_ok=True)

    months = np.arange(1, 13)

    # 组织数据（缺月用空列表占位）
    fi_data = [month_to_fi.get(m, []) for m in months]
    bi_data = [month_to_bi.get(m, []) for m in months]

    # BI y 轴自动范围（确保能显示负值）
    if bi_y_lim is None:
        all_bi = np.asarray([v for lst in bi_data for v in lst], dtype=float)
        all_bi = all_bi[~np.isnan(all_bi)]
        if all_bi.size == 0:
            bi_y_lim = (-1.0, 1.0)
        else:
            ymin = float(all_bi.min())
            ymax = float(all_bi.max())
            pad = max(0.05, 0.10 * (ymax - ymin))
            ymin = max(-1.0, ymin - pad)  # BI 理论下限 -1
            ymax = min(1.0, ymax + pad) if ymax <= 1.0 else (ymax + pad)
            bi_y_lim = (ymin, ymax)

    # 颜色（箱体填充）
    c_fi = "#1f9e89"   # teal
    c_bi = "#f18f01"   # orange

    fig, (ax1, ax2) = plt.subplots(
        2, 1, figsize=(6.8, 4.6), sharex=True,
        gridspec_kw={"height_ratios": [1, 1], "hspace": 0.12}
    )

    def _boxplot(ax, data, facecolor, ylab, y_lim):
        # boxplot 样式：Nature 风格简洁
        bp = ax.boxplot(
            data,
            positions=months,
            widths=0.55,
            patch_artist=True,
            showfliers=False,   # 离群点我们用散点自己画
            whis=1.5,
        )

        for box in bp["boxes"]:
            box.set(facecolor=facecolor, edgecolor="black", linewidth=0.9, alpha=0.85)
        for med in bp["medians"]:
            med.set(color="black", linewidth=1.2)
        for w in bp["whiskers"]:
            w.set(color="black", linewidth=0.9)
        for c in bp["caps"]:
            c.set(color="black", linewidth=0.9)

        # 叠加散点
        if show_points:
            rng = np.random.default_rng(42)
            for m, vals in zip(months, data):
                if vals is None or len(vals) == 0:
                    continue
                vals = np.asarray(vals, dtype=float)
                vals = vals[~np.isnan(vals)]
                if vals.size == 0:
                    continue
                xs = m + (rng.random(vals.size) - 0.5) * 2 * jitter
                ax.scatter(xs, vals, s=point_size, c="black", alpha=point_alpha, linewidths=0, zorder=3)

        # 轴样式
        ax.set_ylabel(ylab)
        if y_lim is not None:
            ax.set_ylim(*y_lim)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)

        # 轻微纵向参考线（可选：你也可以注释掉）
        for m in months:
            ax.vlines(m, ax.get_ylim()[0], ax.get_ylim()[1], linewidth=0.5, alpha=0.15)

    _boxplot(ax1, fi_data, c_fi, "FI", fi_y_lim)
    _boxplot(ax2, bi_data, c_bi, "BI", bi_y_lim)

    ax1.set_title(title, pad=6)

    ax2.set_xlim(0.4, 12.6)
    ax2.set_xticks(months)
    ax2.set_xticklabels([str(i) for i in months])
    ax2.set_xlabel("Month")

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
    # if debug:
    #     n = len(records)
    #     print(f"\n[done] 配对并计算完成，共 {n} 条记录。")
    #     for m in range(1, 13):
    #         print(f"  Month {m:02d}: FI n={len(month_to_fi.get(m, []))}, BI n={len(month_to_bi.get(m, []))}")

    # print(f"fi: {month_to_fi}")
    # print(f"bi: {month_to_bi}")

    # plot_monthly_split_half_violin(
    #     month_to_fi=month_to_fi,
    #     month_to_bi=month_to_bi,
    #     out_png=out_png,
    #     out_pdf=out_pdf,
    #     y_label="FI / BI",
    #     title="Monthly FI (left) and BI (right)",
    #     y_lim=(-0.5, 1.0),
    # )
    month_to_fi = {10: [0.5245526159500677, 0.5527009254166766, 0.4638400419937487, 0.4528471843713245, 0.36916547993644244, 0.5813598241345038, 0.4874489172477243, 0.46110041476287633, 0.40590015460356793, 0.4302170905608173, 0.44257030087668603, 0.5897655345502469, 0.576745662453614, 0.5527136259989002, 0.5565258448498932, 0.5739379660440601, 0.4459317251428175, 0.3759060196082699, 0.40588474733965224, 0.3804960441970805, 0.4120907499429143, 0.37204651794901666, 0.3973842498567208, 0.3242268408293678, 0.3368932597193467], 11: [0.44228219978010114, 0.5306106040682731, 0.5205828290824198, 0.39901731301733084, 0.5844103822915392, 0.44548238017451547, 0.4863652596623829, 0.463773259925106, 0.4848145182812917, 0.5017361196860661, 0.5939394252182463, 0.56670362004151, 0.5172904840332294, 0.4660220955989913, 0.41364659474575693, 0.3282708376603665, 0.4045431127347665, 0.45481941906919104, 0.49026479653437255, 0.46736837757106825, 0.34683791758397353, 0.328791552600704, 0.31934248941422133, 0.30369647064220484, 0.2877111011218259], 12: [0.4275569227980126, 0.4161154664559528, 0.4003170695200887, 0.3813212858727662, 0.37751906417809444, 0.558606314313049, 0.54822940468582, 0.5096955847374374, 0.537657924013594, 0.43468893148034626, 0.4878612070537979, 0.5351979243125139, 0.43816424576600527, 0.41926655560908505, 0.49832851219060376, 0.4074716299989643, 0.3876620492885344, 0.49750818774525213, 0.5025896189076106, 0.4652756762908303, 0.4827704203843777, 0.46334965690426905, 0.31652671903416785, 0.29092708631493275, 0.3592599883086981, 0.2895463856395882, 0.2907302320839988], 4: [0.5423083048205505, 0.5509411947563071, 0.6243319322467954, 0.5848605263048511, 0.5905542085836288, 0.5831525735106918, 0.5955335832070352, 0.5750440976176122, 0.48789972979570534, 0.44105550519096715, 0.46190536915604535, 0.48693170601273966, 0.5357947083168852, 0.5434989003331664, 0.5324343892548882, 0.5177182764562137, 0.5271626335059252, 0.5436223830587459], 5: [0.5833344899444232, 0.6322486788405653, 0.6242916775188123, 0.623029854807538, 0.5679678152941284, 0.5702344928421383, 0.5520095894108421, 0.5704150932925123, 0.5444880652152847, 0.5342724478583835, 0.5162674728308416, 0.5323747427919776, 0.5541226474230587, 0.498264724509183, 0.5511711262242995, 0.5468215918179503, 0.5530718681323412, 0.5854873373605948, 0.5933611790751494], 6: [0.5899180993695592, 0.6062530942273436, 0.6050198350611202, 0.6128397068518667, 0.626534590122732, 0.5562817472216921, 0.5825867689860418, 0.5664500455068588, 0.6170121994575761, 0.6043639825397427, 0.5946988983094621, 0.581101995922076, 0.582729244186867, 0.5584145373334846, 0.5934512605111091, 0.5679061579013919, 0.6097022090711479, 0.5795364692238759, 0.6083001644025267], 7: [0.6160783139954575, 0.5866868114079408, 0.6197679306042343, 0.6014273349442869, 0.6168327966845892, 0.6174140846330224, 0.5648059208794162, 0.5836784988460936, 0.5680712219847545, 0.5591357017175581, 0.5533152123270425, 0.5764759880663733, 0.5850888385978429, 0.5768793721063837, 0.5716855349822568, 0.5698189459450115, 0.5959532841360995, 0.6126644951540253, 0.6131764299191957, 0.6084741009364125], 8: [0.549815415844124, 0.5372210745724229, 0.5497966151007533, 0.5347350993377483, 0.5961789526507616, 0.5406022398510864, 0.6020335421903725, 0.5983729968588617, 0.5941525986056893, 0.5873541586512833, 0.5700004791818313, 0.5919987910585557, 0.5809215242717709, 0.5919998829393919, 0.6024671967540329, 0.5762657541367739, 0.5852688855429538, 0.5794398865099039, 0.5835010280894196, 0.5762713087833692], 9: [0.5574053859450329, 0.5738364954941073, 0.5460846496595144, 0.4001776834847179, 0.5905607208849522, 0.5451000616950721, 0.5879102960078271, 0.5729460625099835, 0.5832583419403, 0.5709372411232039, 0.49051860883735576, 0.4191597430705858, 0.4173375702141893, 0.546317203110934, 0.40977184836614394, 0.38244488420655354, 0.42504383884949215, 0.41518159887049616], 1: [0.5099073786693239, 0.5173987193709921, 0.5115140085862854, 0.43492672714584785, 0.5336554649660266, 0.489734584197624, 0.47120986316217317, 0.4224922717425699, 0.37015072728667536, 0.390408801936741, 0.4911788372272827, 0.4359760092437075, 0.4296393327565239, 0.5391699557469202, 0.48008238773801626, 0.5228296651383902, 0.5272227189894623, 0.48141428488044047, 0.42937534137221123], 2: [0.527672495375236, 0.49359293558904704, 0.37684293885319375, 0.36120321959273527, 0.3503202595349998, 0.376879147287345, 0.46217245150054675, 0.4248060986536263, 0.3629982823957374, 0.3638800065930443, 0.40416110756617746, 0.39037538935469324, 0.5183986676056023, 0.5047278288827365, 0.5213036655716237, 0.5741579206414364], 3: [0.556918989716961, 0.3463083860546507, 0.37606715125496987, 0.4660198844506312, 0.524217694098213, 0.5809927198920746, 0.5035109506507577, 0.5083801513690812, 0.4263892256485992, 0.4723158826445723, 0.5862840407206648, 0.5967160989242973, 0.6005720012916158, 0.5512509878852753, 0.5537811449607853]}
    month_to_bi = {10: [-0.3392705869141174, -0.28782828815525785, -0.3599205146940885, -0.259222086260707, -0.19386408291212331, -0.006572998861297363, 0.06772771436314007, 0.07954328561690516, 0.06989264670272011, 0.007910777781097833, 0.18383863954492652, -0.22573688836800465, -0.28561215321909106, -0.33788017615937105, -0.3613519295526868, -0.33299204577026376, 0.11337130687580288, -0.09966974305236442, 0.21192464982662873, 0.273534653035008, 0.023571712386933008, 0.13230286467986785, 0.0950039207834894, -0.05319933364284346, 0.2949620400627895], 11: [-0.25925079399734596, -0.13595704358812377, -0.08733866177026517, -0.15292819501928046, -0.02528411117040763, 0.006536791768428252, -0.0904428154199608, -0.0003984138617955102, 0.14862956260758264, 0.1300462724575544, -0.27688990183306317, -0.1487161303666933, -0.0950776540397944, -0.029541079627590672, 0.08121655376710746, 0.01306078568107849, -0.13638642267362133, 0.04950456379351431, 0.17101717382121162, 0.24054889346168706, -0.11693608470383887, -0.040648081083948795, 0.02296096093128508, 0.08443222003929263, 0.11653831219259647], 12: [-0.2513991679591405, -0.18283179657000503, -0.11939367833774328, -0.07488436592907777, -0.15854441781412032, 0.15386295043327136, 0.1323569241639191, 0.2311399740359108, 0.1074546745759879, 0.303585296670529, 0.07933277415959483, -0.06687946006362311, 0.07448645091579698, 0.11206174257802037, -0.03367218706521247, 0.18289291319791823, 0.2937207443198102, 0.14839530866442407, 0.14649880457492404, 0.24756407204633812, 0.1886152384829365, 0.16344686022557164, 0.02896274798234799, 0.23397627175288238, -0.16936659744453864, 0.17625754069559196, 0.1947328583892347], 4: [0.008777608769437784, 0.15651265168852002, 0.008572952925130162, -0.0015540361699952765, 0.007662332101003422, -0.004966402989594942, -0.02165864911244264, -0.01843039653022016, -0.05133999933854416, 0.26530061794247306, 0.1397845941246394, 0.1992932940554646, 0.07461465194585681, 0.18188058497785242, 0.09896378449955767, 0.21521734972938855, 0.20276735298437276, 0.17449771186648078], 5: [-0.026644195459959552, 0.042874230275107506, 0.008372989710013323, 0.013258788707527724, 0.02159584333004605, 0.046681824212672085, 0.14612140537522222, 0.03439196232790542, 0.11974814449576998, 0.07121300672962239, 0.09131648717768237, 0.04605082406215133, 0.06502476730918327, -0.02010042774781473, 0.1494795188467848, 0.2247538686956969, 0.1609762379677382, 0.025009207224823138, -0.09055048034136226], 6: [0.07227947496597675, -0.020803651228256448, -0.016843877646471883, -0.014439603807331669, 0.01218480725870097, 0.06973863495602628, -0.06579161715025839, -0.04707471422496812, -0.0038913189314129237, -0.18973567125829272, -0.03431495905429205, -0.05859445897613835, 0.05491290645689295, 0.1731440058635152, -0.04736183403716321, -0.00912454735427648, 0.06079396812703486, -0.10204167810316067, 0.0838119439727758], 7: [-0.00157194705891317, -0.07934645521546124, -0.05466762592215724, -0.042469675754763014, -0.06319629422294948, -0.05769119542585188, -0.2062347923643879, -0.15625377590407497, -0.21982643167086302, 0.13367913779134888, 0.13975693021985536, -0.029749048519281507, -0.05348092923330128, -0.11767449600129087, -0.17438654144279164, -0.10716279155945374, 0.06627798589493317, 0.16003052549516505, 0.14209937988515753, 0.0024505659797322554], 8: [0.11758430990079405, 0.1298754879114008, -0.24860524108883797, -0.2961179325049629, -0.06321928309430003, 0.029735498088391044, -0.05070496309395667, -0.14327733483080474, -0.09798710161653912, -0.12481552972743581, -0.13968679702370657, -0.11059891046047621, -0.2553186136100597, -0.02558192992644215, 0.06474506717634498, -0.08933376921181801, -0.04422073264241011, -0.10086486927521299, -0.062486253936068104, -0.04690723493566429], 9: [0.09327299177696369, 0.03008979492390984, -0.056281212292028204, 0.0037040129336844974, -0.03458350641349106, -0.033391818605678414, -0.092145106501903, -0.20120076235013307, 0.03765024068121181, -0.07885277113063527, 0.044486982851209333, 0.19042808265308864, 0.15423284597314257, 0.0335364670263556, 0.3092739968101288, 0.34390984217328513, 0.07276402223345113, 0.1069644844808395], 1: [0.21626824414354862, 0.22263529900907764, 0.21502116337753163, 0.19059383208125924, -0.043605953141405474, 0.06730969738848835, 0.08273153321436322, 0.24489580332276972, -0.001833809215742499, -0.08691707739803933, 0.0003649284548161713, 0.13859309096876338, 0.18408659201094424, -0.09661015755407343, 0.06443166711786885, 0.14087323087032955, 0.0945271314941416, 0.3160708446452065, 0.3836338078743744], 2: [0.19455052226723413, 0.182056465616512, -0.017178590253486004, 0.07325813499028144, 0.10805151236404953, 0.022583939868710656, 0.05816122096621812, 0.1879166338532523, -0.004520415977076753, -0.04923899513409524, -0.12712931720002574, 0.49617641070320895, -0.0435532479503401, -0.005128243077241801, 0.005509404085471736, 0.036627094326261656], 3: [0.11001586222524362, 0.04927135356639778, 0.3859891199187959, 0.11484114804414558, 0.037792257088989256, -0.0028019805780524765, 0.009179469408186147, -0.06322720174724061, -0.00024722260850751354, 0.20581143932676893, 0.13501198108301216, 0.14554642353283764, 0.0008807200935241344, 0.08491900438860678, 0.11299174321245631]}

    plot_monthly_boxplots_two_panels(
        month_to_fi=month_to_fi,
        month_to_bi=month_to_bi,
        out_png=out_png,
        out_pdf=out_pdf,
        title="Seasonal variation of FI and BI",
        fi_y_lim=(0.3, 0.7),
        bi_y_lim=None,  # ✅ 自动包含负值
        show_points=True,
        point_size=6,
        point_alpha=0.55,
        jitter=0.18
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
