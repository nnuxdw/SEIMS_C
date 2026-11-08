#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import glob
from rasterio.mask import mask
import numpy as np
import rasterio.plot
import matplotlib.colors as mcolors
from matplotlib.colors import ListedColormap, BoundaryNorm
from matplotlib.patches import Patch
from pathlib import Path
from typing import List, Optional

import rasterio
from rasterio.mask import mask
from rasterio.warp import transform_geom
import fiona
# ======================
# 一些通用小工具
# ======================


# =========================================================
# 方法一：根据观测时间筛选模拟路径，并按月只保留一个
# =========================================================

import shutil
from pathlib import Path
from collections import defaultdict

import shutil
from pathlib import Path
from collections import defaultdict

def select_simulation_files_by_obs_time_monthly(
    obs_dir: str,
    sim_dir: str,
    selected_obs_out_dir: str,
    selected_sim_out_dir: str,
    keep_one_per_month: bool = True,  # 新增参数：是否每月只保留一张
):
    """
    参数
    ----
    obs_dir : 观测淹没范围 tif 目录（图1）
    sim_dir : 模拟值沿模范 tif 目录（图2）
    selected_obs_out_dir : 按筛选后要保存的观测 tif 目录
    selected_sim_out_dir : 按筛选后要保存的模拟 tif 目录
    keep_one_per_month : 是否每月只保留一张（默认 True）
        - True  : 每月只保留第一张（原逻辑不变）
        - False : 保留所有时间匹配上的图（按天匹配后的所有 common_dates）

    返回
    ----
    selected_sim_paths : list[str]
    selected_obs_paths : list[str]
    """

    obs_dir = Path(obs_dir)
    sim_dir = Path(sim_dir)
    selected_obs_out_dir = Path(selected_obs_out_dir)
    selected_sim_out_dir = Path(selected_sim_out_dir)

    selected_obs_out_dir.mkdir(parents=True, exist_ok=True)
    selected_sim_out_dir.mkdir(parents=True, exist_ok=True)

    # 1. 扫描观测 & 模拟文件
    obs_files = [p for p in obs_dir.glob("*.tif")]
    sim_files = [p for p in sim_dir.glob("*.tif")]

    # 2. 建立日期映射（按 date 粒度）
    obs_by_date = defaultdict(list)
    for p in obs_files:
        d = _parse_date_from_obs_name(p.name)
        obs_by_date[d.date()].append(p)

    sim_by_date = defaultdict(list)
    for p in sim_files:
        d = _parse_date_from_sim_name(p.name)
        sim_by_date[d.date()].append(p)

    # 3. 找出同时存在的日期，并做“按天匹配”
    common_dates = sorted(set(obs_by_date.keys()) & set(sim_by_date.keys()))

    matched_sim_paths = []
    matched_obs_paths = []

    for d in common_dates:
        # 若同一天有多张，取最小排序的那张（保持你原逻辑）
        sim_p = sorted(sim_by_date[d])[0]
        obs_p = sorted(obs_by_date[d])[0]
        matched_sim_paths.append(sim_p)
        matched_obs_paths.append(obs_p)

    # 4. 根据 keep_one_per_month 决定输出集
    selected_sim_paths = []
    selected_obs_paths = []

    if keep_one_per_month:
        # ---- 原逻辑：按月分组，每月保留第一张 ----
        ym_to_indices = defaultdict(list)
        for idx, sim_path in enumerate(matched_sim_paths):
            d = _parse_date_from_sim_name(sim_path.name)
            ym_to_indices[(d.year, d.month)].append(idx)

        for (year, month) in sorted(ym_to_indices.keys()):
            first_idx = ym_to_indices[(year, month)][0]

            sim_p = matched_sim_paths[first_idx]
            obs_p = matched_obs_paths[first_idx]

            selected_sim_paths.append(str(sim_p))
            selected_obs_paths.append(str(obs_p))

            shutil.copy(obs_p, selected_obs_out_dir / obs_p.name)
            shutil.copy(sim_p, selected_sim_out_dir / sim_p.name)

    else:
        # ---- 新逻辑：保留所有按时间匹配上的图（所有 common_dates）----
        for sim_p, obs_p in zip(matched_sim_paths, matched_obs_paths):
            selected_sim_paths.append(str(sim_p))
            selected_obs_paths.append(str(obs_p))

            shutil.copy(obs_p, selected_obs_out_dir / obs_p.name)
            shutil.copy(sim_p, selected_sim_out_dir / sim_p.name)

    return selected_sim_paths, selected_obs_paths


# =========================================================
# 方法二：tif -> 面 shp；Mollweide 投影筛选面积，再投影回 WGS84
# =========================================================

from pathlib import Path
from concurrent.futures import ProcessPoolExecutor, as_completed

import rasterio
from rasterio.features import shapes
import geopandas as gpd
from shapely.geometry import shape


def _convert_single_obs_tif(
    tif_path_str: str,
    out_dir_str: str,
    value_threshold: float,
    area_threshold_km2: float,
    moll_crs: str,
):
    """
    单个 tif 的处理函数（给多进程用）
    """
    tif_path = Path(tif_path_str)
    out_dir = Path(out_dir_str)

    try:
        print(f"[PID] 处理观测 tif: {tif_path.name}")

        with rasterio.open(tif_path) as src:
            data = src.read(1)

            # 按阈值掩膜
            mask = data > value_threshold

            geom_list = []
            for geom, val in shapes(data.astype("int16"),
                                    mask=mask,
                                    transform=src.transform):
                if val > value_threshold:
                    geom_list.append(shape(geom))

            if not geom_list:
                print(f"  -> 无满足阈值的像元，跳过: {tif_path.name}")
                return None

            gdf = gpd.GeoDataFrame(geometry=geom_list, crs=src.crs)

        # 转为 Mollweide（单位：m）
        gdf_moll = gdf.to_crs(moll_crs)

        # 计算面积（km²）
        gdf_moll["area_km2"] = gdf_moll.geometry.area / 1e6

        # 按面积筛选
        gdf_large = gdf_moll[gdf_moll["area_km2"] > area_threshold_km2]
        if gdf_large.empty:
            print(f"  -> 没有面积 > {area_threshold_km2} km² 的面，跳过: {tif_path.name}")
            return None

        # 投影回 WGS84
        gdf_wgs84 = gdf_large.to_crs("EPSG:4326")

        out_shp = out_dir / f"{tif_path.stem}_filtered.shp"
        gdf_wgs84.to_file(out_shp)

        print(f"  -> 已保存: {out_shp}  (最小面积阈值: {area_threshold_km2} km²)")
        return str(out_shp)

    except Exception as e:
        print(f"[错误] 处理 {tif_path.name} 时出错: {e}")
        return None


def convert_obs_tifs_to_filtered_polygons(
    in_dir: str,
    out_dir: str,
    value_threshold: float = 0.1,
    area_threshold_km2: float = 1.0,   # 用 km² 做筛选阈值
    moll_crs: str = "+proj=moll +lon_0=0 +x_0=0 +y_0=0 +datum=WGS84 +units=m +no_defs",
    n_workers: int = 4,
):
    """
    多进程版：
    in_dir 目录下所有 tif 并行处理，生成 *_filtered.shp 到 out_dir。

    参数
    ----
    in_dir : 原始观测淹没范围 tif 目录
    out_dir : 输出筛选后 shp 的目录（WGS84）
    value_threshold : 栅格值 > value_threshold 即认为是淹没区域
    area_threshold_km2 : 面积阈值（km²），小于该值的淹没面将被剔除
    moll_crs : 用于面积计算的等面积投影（Mollweide）
    n_workers : 并行进程数（建议不超过 CPU 核心数）
    """

    in_dir = Path(in_dir)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    tif_paths = sorted(in_dir.glob("*.tif"))
    if not tif_paths:
        print(f"[提示] 目录中没有 tif 文件：{in_dir}")
        return []

    results = []

    # 如果只给 1 个 worker，就退化为串行（方便调试）
    if n_workers <= 1:
        for tif_path in tif_paths:
            res = _convert_single_obs_tif(
                tif_path_str=str(tif_path),
                out_dir_str=str(out_dir),
                value_threshold=value_threshold,
                area_threshold_km2=area_threshold_km2,
                moll_crs=moll_crs,
            )
            if res is not None:
                results.append(res)
        return results

    # 多进程并行
    with ProcessPoolExecutor(max_workers=n_workers) as executor:
        future_to_tif = {
            executor.submit(
                _convert_single_obs_tif,
                str(tif_path),
                str(out_dir),
                value_threshold,
                area_threshold_km2,
                moll_crs,
            ): tif_path
            for tif_path in tif_paths
        }

        for fut in as_completed(future_to_tif):
            tif_path = future_to_tif[fut]
            try:
                res = fut.result()
                if res is not None:
                    results.append(res)
            except Exception as e:
                print(f"[并行错误] 文件 {tif_path.name} 处理失败: {e}")

    return results




# =========================================================
# 方法三：叠加绘图 + 计算 FI / BI
# =========================================================

import numpy as np


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



from pathlib import Path
import numpy as np
import rasterio
import rasterio.plot
import geopandas as gpd
import matplotlib.pyplot as plt

# ===== 已有的日期解析函数（前面你已经有） =====
# S1A_2014_10_8.tif
def _parse_date_from_obs_name(name: str):
    import re
    from datetime import datetime
    stem = Path(name).stem
    m = re.search(r"S1[A-Z]_(\d{4})_(\d{1,2})_(\d{1,2})", stem)
    if not m:
        raise ValueError(f"无法从观测文件名解析日期: {name}")
    y, mth, d = map(int, m.groups())
    from datetime import datetime
    return datetime(y, mth, d)

# OL_Hand_WTRDEP_TS_2010_01_01_000000.tif
def _parse_date_from_sim_name(name: str):
    import re
    from datetime import datetime
    stem = Path(name).stem
    m = re.search(r"TS_(\d{4})_(\d{2})_(\d{2})_", stem)
    if not m:
        raise ValueError(f"无法从模拟文件名解析日期: {name}")
    y, mth, d = map(int, m.groups())
    return datetime(y, mth, d)


# ===== 你原来的单图绘制函数：保持不变 =====

def batch_plot_overlap_difference(
    sim_dir: str,
    obs_tif_dir: str,
    out_dir: str,
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
    bbox: tuple = None,
):
    """
    批量绘制 Sim vs Obs 三分类差异图（不再使用 shp）：

    参数
    ----
    sim_dir : 模拟水深 tif 目录（通常是按月筛选后的目录）
              命名需能被 _parse_date_from_sim_name 解析出日期
    obs_tif_dir : 观测栅格 tif 目录（S1A_YYYY_MM_D.tif 等）
                  命名需能被 _parse_date_from_obs_name 解析出日期
    out_dir : 输出差异图目录
    sim_thresh : 模拟淹没阈值
    obs_thresh : 观测淹没阈值
    """

    sim_dir = Path(sim_dir)
    obs_tif_dir = Path(obs_tif_dir)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 1. 建立 观测 tif 的 日期 -> 路径 映射
    obs_tif_by_date = {}
    for p in obs_tif_dir.glob("*.tif"):
        try:
            d = _parse_date_from_obs_name(p.name).date()
            obs_tif_by_date[d] = p
        except ValueError:
            print(f"[obs tif] 跳过无法解析日期的文件: {p.name}")
    # 用于统计平均 FI / BI
    FI_list = []
    BI_list = []
    # 2. 遍历每个模拟 tif，根据日期去匹配 obs tif
    for sim_p in sorted(sim_dir.glob("*.tif")):
        try:
            d = _parse_date_from_sim_name(sim_p.name).date()
        except ValueError:
            print(f"[sim] 跳过无法解析日期的文件: {sim_p.name}")
            continue

        obs_tif_p = obs_tif_by_date.get(d)
        if obs_tif_p is None:
            print(f"日期 {d} 缺少匹配的 obs_tif，跳过。")
            continue

        time_str = d.strftime("%Y-%m-%d")
        out_png_path = out_dir / f"{time_str}.png"

        print(f"绘制 {time_str}:")
        print(f"  sim : {sim_p.name}")
        print(f"  obs : {obs_tif_p.name}")

        FI, BI = plot_overlap_difference(
            sim_tif_path=str(sim_p),
            obs_tif_path=str(obs_tif_p),
            out_png_path=str(out_png_path),
            sim_thresh=sim_thresh,
            obs_thresh=obs_thresh,
            time_str=time_str,
            bbox=bbox
        )
        # ---------- 收集统计 ----------
        if FI is not None and not np.isnan(FI):
            FI_list.append(FI)
        if BI is not None and not np.isnan(BI):
            BI_list.append(BI)
    # ==============================
    # 3. 输出平均 FI / BI
    # ==============================
    print("\n================ 平均 FI / BI 统计结果 ================")

    if len(FI_list) > 0:
        FI_mean = np.mean(FI_list)
        print(f"平均 FI (Hits / Obs) : {FI_mean:.4f}")
    else:
        print("平均 FI : 无有效值")

    if len(BI_list) > 0:
        BI_mean = np.mean(BI_list)
        print(f"平均 BI (Hits / Sim) : {BI_mean:.4f}")
    else:
        print("平均 BI : 无有效值")

    print("======================================================\n")

def _align_to_common_window(sim_arr: np.ndarray, obs_arr: np.ndarray, *, debug: bool = True):
    """
    将 sim/obs 裁到共同最小窗口，保证 shape 一致。
    返回：sim2, obs2
    """
    if sim_arr.shape == obs_arr.shape:
        return sim_arr, obs_arr

    min_rows = min(sim_arr.shape[0], obs_arr.shape[0])
    min_cols = min(sim_arr.shape[1], obs_arr.shape[1])

    if debug:
        print(f"[WARN] sim/obs shape 不一致: sim={sim_arr.shape}, obs={obs_arr.shape} -> crop=({min_rows},{min_cols})")

    return sim_arr[:min_rows, :min_cols], obs_arr[:min_rows, :min_cols]


def plot_overlap_difference(
    sim_tif_path: str,
    obs_tif_path: str,
    out_png_path: str,
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
    time_str: str = "",
    bbox: tuple = None,
    # bbox = (xmin, xmax, ymin, ymax)  例如 (115.5, 116.8, 28.5, 29.9)
):
    """
    Sim vs Obs 三分类差异图（不使用 shp）

    bbox: 可选的绘图经纬度范围 (xmin, xmax, ymin, ymax)。
          若为 None，则使用整幅 tif 的范围。
    """

    sim_tif_path = Path(sim_tif_path)
    obs_tif_path = Path(obs_tif_path)
    out_png_path = Path(out_png_path)
    out_png_path.parent.mkdir(parents=True, exist_ok=True)

    # 1) 读模拟水深
    with rasterio.open(sim_tif_path) as sim_src:
        sim_arr = sim_src.read(1)
        sim_nodata = sim_src.nodata
        sim_extent = rasterio.plot.plotting_extent(sim_src)  # (xmin, xmax, ymin, ymax)

    # 2) 读观测栅格
    with rasterio.open(obs_tif_path) as obs_src:
        obs_arr = obs_src.read(1)
        obs_nodata = obs_src.nodata

    # ✅ 2.5) 强制对齐，避免后续广播/boolean index 报错
    sim_arr, obs_arr = _align_to_common_window(sim_arr, obs_arr, debug=True)

    # 3) 计算 FI / BI（建议你的 compute_fi_bi 内部也做对齐兜底；但这里已经对齐过了）
    FI, BI = compute_fi_bi(
        sim_arr, obs_arr,
        sim_nodata=sim_nodata,
        obs_nodata=obs_nodata,
        sim_thresh=sim_thresh,
        obs_thresh=obs_thresh,
    )

    # 4) 阈值掩膜（这里严格使用对齐后的 sim_arr/obs_arr）
    if sim_nodata is None:
        sim_bin = sim_arr > sim_thresh
    else:
        sim_bin = (sim_arr > sim_thresh) & (sim_arr != sim_nodata)

    if obs_nodata is None:
        obs_bin = obs_arr > obs_thresh
    else:
        obs_bin = (obs_arr > obs_thresh) & (obs_arr != obs_nodata)

    # 0=都不淹, 1=sim only, 2=obs only, 3=overlap
    cat = np.zeros_like(sim_arr, dtype=np.uint8)
    cat[(sim_bin) & (~obs_bin)] = 1
    cat[(~sim_bin) & (obs_bin)] = 2
    cat[(sim_bin) & (obs_bin)] = 3
    cat_ma = np.ma.masked_where(cat == 0, cat)

    # 5) 颜色
    cmap = ListedColormap([
        "#4169E1",   # 1 - False alarms (Sim only)
        "#FF4500",   # 2 - Misses (Obs only)
        "#00C853",   # 3 - Hits (Overlap)
    ])
    norm = BoundaryNorm([0.5, 1.5, 2.5, 3.5], cmap.N)

    # 6) 绘图
    plt.rcParams["font.family"] = "Times New Roman"  # 新罗马字体

    fig, ax = plt.subplots(figsize=(6, 8))

    im = ax.imshow(
        cat_ma,
        extent=sim_extent,      # 仍用 sim 的 extent（与你原来一致）
        origin="upper",
        cmap=cmap,
        norm=norm,
    )

    # bbox 缩放
    if bbox is not None:
        xmin, xmax, ymin, ymax = bbox
        ax.set_xlim(xmin, xmax)
        ax.set_ylim(ymin, ymax)

    # 图例（你原来的顺序）
    legend_patches = [
        Patch(facecolor="#00C853", edgecolor="none", label="Hits"),
        Patch(facecolor="#FF4500", edgecolor="none", label="Misses"),
        Patch(facecolor="#4169E1", edgecolor="none", label="False alarms"),
    ]
    ax.legend(
        handles=legend_patches,
        loc="upper right",
        frameon=True,
        framealpha=0.8,
        fontsize=11,
    )

    # 文本
    text_lines = []
    if time_str:
        text_lines.append(f"{time_str}")
    text_lines.append(f"FI = {FI:.3f}" if not np.isnan(FI) else "FI = NaN")
    text_lines.append(f"BI = {BI:.3f}" if not np.isnan(BI) else "BI = NaN")
    text = "\n".join(text_lines)

    ax.text(
        0.02, 0.98,
        text,
        transform=ax.transAxes,
        va="top",
        ha="left",
        fontsize=12,
        fontweight="bold",
        bbox=dict(facecolor="white", alpha=0.8, edgecolor="none"),
    )

    ax.set_title("Sim vs Obs inundation difference")

    plt.tight_layout()
    fig.savefig(
        out_png_path,
        dpi=500,
        bbox_inches="tight",
        pad_inches=0.05
    )
    plt.close(fig)

    print(f"已保存差异图: {out_png_path}")
    return FI, BI

def plot_overlay_and_indices(
    sim_tif_path: str,
    obs_extent_shp_path: str,
    obs_tif_path: str,
    out_png_path: str,
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
    time_str: str = "",
):
    """
    单次绘图：一个 sim tif + 一个 obs tif + 一个 shp
    模拟值用青绿色渐变（不从白色开始），未淹没区域透明，
    观测范围为高透明度灰色实心面。
    """
    sim_tif_path = Path(sim_tif_path)
    obs_extent_shp_path = Path(obs_extent_shp_path)
    obs_tif_path = Path(obs_tif_path)
    out_png_path = Path(out_png_path)
    out_png_path.parent.mkdir(parents=True, exist_ok=True)

    # 1. 读模拟水深
    with rasterio.open(sim_tif_path) as sim_src:
        sim_arr = sim_src.read(1)
        sim_nodata = sim_src.nodata
        sim_extent = rasterio.plot.plotting_extent(sim_src)

    # 2. 读观测栅格
    with rasterio.open(obs_tif_path) as obs_src:
        obs_arr = obs_src.read(1)
        obs_nodata = obs_src.nodata

    # 3. 计算 FI / BI
    FI, BI = compute_fi_bi(
        sim_arr, obs_arr,
        sim_nodata=sim_nodata,
        obs_nodata=obs_nodata,
        sim_thresh=sim_thresh,
        obs_thresh=obs_thresh,
    )

    # 4. 读观测面 shp
    gdf = gpd.read_file(obs_extent_shp_path)

    # 5. 对模拟值做阈值掩膜，只显示淹没区域（其他设为 NaN）
    if sim_nodata is None:
        sim_plot = np.where(sim_arr > sim_thresh, sim_arr, np.nan)
    else:
        sim_plot = np.where(
            (sim_arr > sim_thresh) & (sim_arr != sim_nodata),
            sim_arr,
            np.nan,
        )

    # ==== 自定义洪水色带：浅青 -> 青绿 -> 深青 ====
    flood_cmap = mcolors.LinearSegmentedColormap.from_list(
        "flood_cmap",
        [
            "#b2f7ef",  # 很浅的青色（接近图2的浅蓝绿）
            "#40e0d0",  # 亮青色（中间值）
            "#0077b6",  # 偏深的蓝青色（高水深）
        ]
    )
    # 对 NaN（未淹没区域）设置为完全透明
    flood_cmap.set_bad(color="none")

    # 6. 绘图
    fig, ax = plt.subplots(figsize=(6, 8))

    im = ax.imshow(
        sim_plot,
        extent=sim_extent,
        origin="upper",
        cmap=flood_cmap,
        vmin=sim_thresh if sim_thresh is not None else None,
        # vmax 可选：想统一色标上限可加上，比如 vmax=20
    )
    plt.colorbar(im, ax=ax, label="Simulated water depth")

    # 观测 shp：实心灰色，高透明度，起到“范围参考”的作用
    gdf.plot(
        ax=ax,
        facecolor="gray",
        edgecolor="gray",
        linewidth=0.3,
        alpha=0.25,
    )

    # 7. 标注 FI/BI 和时间
    text_lines = []
    if time_str:
        text_lines.append(f"Date: {time_str}")
    text_lines.append(f"FI = {FI:.3f}" if not np.isnan(FI) else "FI = NaN")
    text_lines.append(f"BI = {BI:.3f}" if not np.isnan(BI) else "BI = NaN")
    text = "\n".join(text_lines)

    ax.text(
        0.02, 0.98,
        text,
        transform=ax.transAxes,
        va="top",
        ha="left",
        fontsize=10,
        bbox=dict(facecolor="white", alpha=0.7, edgecolor="none"),
    )

    ax.set_xlabel("Longitude")
    ax.set_ylabel("Latitude")
    ax.set_title("Observed extent (gray) over simulated depth")

    plt.tight_layout()
    fig.savefig(out_png_path, dpi=300)
    plt.close(fig)

    print(f"已保存叠加图: {out_png_path}")




# ===== 新增：批量版本 =====
def batch_plot_overlay_and_indices(
    sim_dir: str,
    obs_extent_shp_dir: str,
    obs_tif_dir: str,
    out_dir: str,
    sim_thresh: float = 0.0,
    obs_thresh: float = 0.0,
):
    """
    批量绘图：
    给出三个目录，自动按“日期”匹配 sim / obs / shp，循环绘制。

    参数
    ----
    sim_dir : 模拟水深 tif 目录（通常是按月筛选后的目录）
    obs_extent_shp_dir : 观测面 shp 目录（convert_obs_tifs_to_filtered_polygons 生成的 *_filtered.shp）
    obs_tif_dir : 观测栅格 tif 目录（S1A_YYYY_MM_D.tif）
    out_dir : 输出叠加图目录
    """

    sim_dir = Path(sim_dir)
    obs_extent_shp_dir = Path(obs_extent_shp_dir)
    obs_tif_dir = Path(obs_tif_dir)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 1. 建立 观测 tif 的 日期 -> 路径 映射
    obs_tif_by_date = {}
    for p in obs_tif_dir.glob("*.tif"):
        try:
            d = _parse_date_from_obs_name(p.name).date()
            obs_tif_by_date[d] = p
        except ValueError:
            print(f"[obs tif] 跳过无法解析日期的文件: {p.name}")

    # 2. 建立 观测 shp 的 日期 -> 路径 映射
    shp_by_date = {}
    for p in obs_extent_shp_dir.glob("*.shp"):
        stem = p.stem
        # 例如 S1A_2014_10_8_filtered.shp -> S1A_2014_10_8
        if stem.endswith("_filtered"):
            base = stem[:-9]   # 去掉 "_filtered"
        else:
            base = stem
        try:
            d = _parse_date_from_obs_name(base).date()
            shp_by_date[d] = p
        except ValueError:
            print(f"[obs shp] 跳过无法解析日期的文件: {p.name}")

    # 3. 对每个模拟 tif，根据日期去匹配 obs tif 和 shp
    for sim_p in sorted(sim_dir.glob("*.tif")):
        try:
            d = _parse_date_from_sim_name(sim_p.name).date()
        except ValueError:
            print(f"[sim] 跳过无法解析日期的文件: {sim_p.name}")
            continue

        obs_tif_p = obs_tif_by_date.get(d)
        shp_p = shp_by_date.get(d)

        if obs_tif_p is None or shp_p is None:
            print(f"日期 {d} 缺少匹配的 obs_tif 或 shp，跳过。")
            continue

        # 输出图文件名：可以用日期命名，也可以用 sim_tif 的 stem
        time_str = d.strftime("%Y-%m-%d")
        out_png_path = out_dir / f"{time_str}.png"

        print(f"绘制 {time_str}:")
        print(f"  sim : {sim_p.name}")
        print(f"  obs : {obs_tif_p.name}")
        print(f"  shp : {shp_p.name}")

        plot_overlay_and_indices(
            sim_tif_path=str(sim_p),
            obs_extent_shp_path=str(shp_p),
            obs_tif_path=str(obs_tif_p),
            out_png_path=str(out_png_path),
            sim_thresh=sim_thresh,
            obs_thresh=obs_thresh,
            time_str=time_str,
        )



# =========================================================
# 方法四：把第三步得到的图像序列生成 GIF
# =========================================================

from pathlib import Path
from PIL import Image


from pathlib import Path
from PIL import Image


def make_gif_from_images(
    image_paths,
    out_gif_path: str,
    duration: float = 0.5,
    loop: int = 0,
):
    """
    使用 Pillow 生成 GIF，所有帧共享第一帧的调色板，
    并正确按照该调色板重新量化每一帧，避免后续帧变灰 / 变色。

    参数
    ----
    image_paths : list[str] 或 list[Path]
        按时间顺序排好的 PNG/JPG 等图片路径
    out_gif_path : 输出 GIF 路径
    duration : 每帧停留时间（秒）
    loop : 循环次数，0 表示无限循环
    """
    out_gif_path = Path(out_gif_path)
    out_gif_path.parent.mkdir(parents=True, exist_ok=True)

    # 1. 读入所有帧，统一为 RGB
    rgb_frames = []
    for p in image_paths:
        p = Path(p)
        if not p.exists():
            print(f"警告：找不到图像 {p}，跳过。")
            continue
        img = Image.open(str(p)).convert("RGB")
        rgb_frames.append(img)

    if not rgb_frames:
        raise RuntimeError("没有可用帧，无法生成 GIF。")

    # 2. 用第一帧生成“基准调色板”
    # 注意：这里得到的是一个带调色板的 P 图像
    palette_img = rgb_frames[0].convert(
        "P",
        palette=Image.ADAPTIVE,
        colors=256,
        dither=Image.NONE,
    )

    # 3. 所有帧都按照这份调色板重新量化（quantize）
    paletted_frames = []
    for im in rgb_frames:
        # 关键：palette=palette_img，会用 palette_img 的调色板算“最近颜色”的索引
        p = im.quantize(palette=palette_img, dither=Image.NONE)
        paletted_frames.append(p)

    first_frame, *other_frames = paletted_frames

    # 4. 保存 GIF
    first_frame.save(
        str(out_gif_path),
        save_all=True,
        append_images=other_frames,
        format="GIF",
        duration=int(duration * 1000),  # ms
        loop=loop,
        disposal=2,     # 每帧刷新
        optimize=False, # 不再让 Pillow 自己乱优化调色板
    )

    print(f"GIF 已保存: {out_gif_path}")



def clip_tifs_batch(input_dir,  shp_path, output_dir):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    tif_files = glob.glob(os.path.join(input_dir, "*.tif"))
    for tif_file in tif_files:
        output_tif = os.path.join(output_dir, os.path.basename(tif_file))
        clip_raster_by_polygon(tif_file, shp_path, output_tif)
        print(f"Cliped {tif_file} to {output_tif}")

def clip_raster_by_polygon(dem_tif_path, polygon_shp_path, output_tif_path):
    # 加载矢量数据
    gdf = gpd.read_file(polygon_shp_path)

    # 读取 DEM 数据
    with rasterio.open(dem_tif_path) as src:
        # 使用多边形作为掩模裁剪 DEM
        out_image, out_transform = mask(src, gdf.geometry, crop=True)  # crop=True 确保输出裁剪区域
        out_meta = src.meta.copy()

        # 更新元数据信息，保持与原文件一致
        out_meta.update({
            "driver": "GTiff",
            "height": out_image.shape[1],
            "width": out_image.shape[2],
            "transform": out_transform,
            "crs": src.crs,  # 投影保持一致
            "nodata": src.nodata  # NoData 值保持一致
        })

        # 保存裁剪后的 TIF 文件
        with rasterio.open(output_tif_path, "w", **out_meta) as dest:
            dest.write(out_image)
            print(f"HRU裁剪DEM已完成，保存到: {output_tif_path}")




from pathlib import Path
import numpy as np
import rasterio
from rasterio.warp import reproject, Resampling
import geopandas as gpd  # 虽然这里不一定用到，但和整体工程保持一致


def resample_obs_to_ref_grid(
    obs_dir: str,
    ref_tif_path: str,
    out_dir: str,
    resampling: Resampling = Resampling.nearest,
):
    """
    方法一：将 obs_dir 下的所有观测 tif
        直接重投影 + 重采样到 ref_tif 的网格（CRS、transform、分辨率统一），
        不裁剪，只把“对齐后的”栅格写到 out_dir。

    参数
    ----
    obs_dir : 观测值 tif 目录
    ref_tif_path : 参考栅格（例如模拟水深 tif），用于提供 CRS、transform、分辨率
    out_dir : 输出重采样并对齐后的观测 tif 目录
    resampling : 重采样方法，默认最近邻（0/1淹没栅格推荐）

    返回
    ----
    out_paths : list[str]  写出的栅格路径列表
    """
    obs_dir = Path(obs_dir)
    ref_tif_path = Path(ref_tif_path)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 读取参考栅格的网格信息
    with rasterio.open(ref_tif_path) as ref_src:
        ref_crs = ref_src.crs
        ref_transform = ref_src.transform
        ref_width = ref_src.width
        ref_height = ref_src.height

    out_paths = []

    for obs_tif in sorted(obs_dir.glob("*.tif")):
        print(f"[重采样] 处理观测 tif: {obs_tif.name}")

        try:
            with rasterio.open(obs_tif) as src:
                src_arr = src.read(1)
                src_transform = src.transform
                src_crs = src.crs
                src_nodata = src.nodata

                dst_nodata = src_nodata if src_nodata is not None else -9999

                # 目标网格：大小 = ref_tif 的宽高
                dst_arr = np.full(
                    (ref_height, ref_width),
                    dst_nodata,
                    dtype=src_arr.dtype,
                )

                # 一步完成：从 src_crs → ref_crs，且对齐 ref_transform 网格
                reproject(
                    source=src_arr,
                    destination=dst_arr,
                    src_transform=src_transform,
                    src_crs=src_crs,
                    dst_transform=ref_transform,
                    dst_crs=ref_crs,
                    src_nodata=src_nodata,
                    dst_nodata=dst_nodata,
                    resampling=resampling,
                )

                # 更新 profile
                profile = src.profile.copy()
                profile.update(
                    {
                        "crs": ref_crs,
                        "transform": ref_transform,
                        "width": ref_width,
                        "height": ref_height,
                        "nodata": dst_nodata,
                        "count": 1,
                        "dtype": dst_arr.dtype,
                    }
                )

                out_tif = out_dir / f"{obs_tif.stem}_rs.tif"
                with rasterio.open(out_tif, "w", **profile) as dst:
                    dst.write(dst_arr, 1)

                print(f"  -> 已保存重采样结果: {out_tif}")
                out_paths.append(str(out_tif))

        except Exception as e:
            print(f"[警告] 跳过 {obs_tif.name}，原因: {e}")

    return out_paths


def clip_rasters_by_polygon_batch(
    in_dir: str,
    clip_shp_path: str,
    out_dir: str,
    *,
    crop: bool = True,
    all_touched: bool = False,
    filled: bool = True,
    nodata_override: Optional[float] = None,
    strict_no_crs: bool = True,
) -> List[str]:
    """
    方法二：对方法一的输出结果做裁剪（鲁棒版：尽量不因坏几何/空几何崩溃）

    读取 in_dir 下已对齐网格的 tif（通常是 *_rs.tif），
    使用 clip_shp 作为裁剪多边形（mask + crop），
    输出到 out_dir。

    参数
    ----
    in_dir : 已经对齐到 ref_tif 网格的栅格目录（例如方法一的 out_dir）
    clip_shp_path : 用来裁剪的 shp（可以是流域 / 湖区等 polygon）
    out_dir : 输出裁剪后 tif 的目录

    可选参数
    ----
    crop : 是否裁剪到最小包围盒（True 表示输出范围会变小）
    all_touched : rasterio.mask.mask 参数，True 表示只要像元被几何触碰就视为在内（边界更“胖”）
    filled : rasterio.mask.mask 参数，True 表示 outside 区域用 nodata 填充
    nodata_override : 如果栅格 src.nodata 为 None，可强制指定 nodata 值（例如 -9999）
    strict_no_crs : 如果 shp 或 tif 缺 CRS，是否直接报错（默认 True）

    返回
    ----
    out_paths : list[str]  裁剪结果栅格路径列表
    """
    in_dir = Path(in_dir)
    clip_shp_path = Path(clip_shp_path)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    if not clip_shp_path.exists():
        raise FileNotFoundError(f"clip_shp 不存在: {clip_shp_path}")
    if not in_dir.exists():
        raise FileNotFoundError(f"in_dir 不存在: {in_dir}")

    # 用 fiona 只读取一次 shp 的 CRS，真正的几何读取/转换在每个 tif 内进行（更稳）
    with fiona.open(str(clip_shp_path)) as shp_src:
        shp_crs = shp_src.crs_wkt or shp_src.crs
        feat_count = len(shp_src)

    if not shp_crs and strict_no_crs:
        raise ValueError("clip_shp 没有 CRS（prj 丢失或未定义），请先为 shp 设置坐标系。")

    out_paths: List[str] = []

    tif_list = sorted(in_dir.glob("*.tif"))
    if not tif_list:
        print(f"[提示] {in_dir} 下没有 tif 文件。")
        return out_paths

    print(f"[信息] 开始批量裁剪：tif 数量={len(tif_list)}，shp 要素数={feat_count}")

    for tif_path in tif_list:
        print(f"\n[裁剪] 处理 tif: {tif_path.name}")

        try:
            with rasterio.open(tif_path) as src:
                raster_crs = src.crs
                if raster_crs is None and strict_no_crs:
                    raise ValueError("栅格没有 CRS（src.crs=None），无法进行投影统一裁剪。")

                # nodata 处理：优先 src.nodata，其次 nodata_override
                nodata_value = src.nodata
                if nodata_value is None and nodata_override is not None:
                    nodata_value = nodata_override

                # 收集可用 shapes（每个 tif 单独构建，避免 CRS 不同导致问题）
                shapes = []
                bad_feat = 0
                skipped_empty = 0
                skipped_nonpoly = 0
                skipped_transform = 0

                with fiona.open(str(clip_shp_path)) as shp:
                    for feat in shp:
                        geom = feat.get("geometry")
                        if not geom:
                            skipped_empty += 1
                            continue

                        # 尝试 CRS 转换到栅格 CRS（比 geopandas.to_crs 更稳）
                        try:
                            if shp_crs and raster_crs:
                                geom2 = transform_geom(
                                    shp_crs,
                                    raster_crs.to_string(),
                                    geom,
                                    antimeridian_cutting=True,
                                    precision=7,
                                )
                            else:
                                # shp 或 tif 缺 CRS：只能“原样”使用（可能会错位）
                                geom2 = geom
                        except Exception:
                            skipped_transform += 1
                            continue

                        # 过滤非面几何（Point/Line/GeometryCollection 等）
                        gtype = (geom2.get("type") or "").lower()
                        if gtype not in ("polygon", "multipolygon"):
                            skipped_nonpoly += 1
                            continue

                        # 有些坏几何会出现 coordinates 为空
                        coords = geom2.get("coordinates", None)
                        if not coords:
                            skipped_empty += 1
                            continue

                        shapes.append(geom2)

                if not shapes:
                    print("  -> 没有可用的裁剪几何（可能都为空/非面/投影转换失败），跳过该 tif。")
                    print(f"     统计：empty={skipped_empty} nonpoly={skipped_nonpoly} transform_fail={skipped_transform}")
                    continue

                # 执行裁剪
                out_image, out_transform = mask(
                    src,
                    shapes,
                    crop=crop,
                    nodata=nodata_value,
                    all_touched=all_touched,
                    filled=filled,
                )

                # 更新元数据
                out_meta = src.meta.copy()
                out_meta.update(
                    {
                        "height": out_image.shape[1],
                        "width": out_image.shape[2],
                        "transform": out_transform,
                    }
                )
                if nodata_value is not None:
                    out_meta["nodata"] = nodata_value

                out_tif = out_dir / f"{tif_path.stem}_clip.tif"
                with rasterio.open(out_tif, "w", **out_meta) as dst:
                    dst.write(out_image)

                print(f"  -> 已保存裁剪结果: {out_tif}")
                print(f"     shapes_used={len(shapes)} | empty={skipped_empty} nonpoly={skipped_nonpoly} transform_fail={skipped_transform}")
                out_paths.append(str(out_tif))

        except Exception as e:
            print(f"[警告] 裁剪 {tif_path.name} 时出错，已跳过。原因: {e}")

    print(f"\n[完成] 裁剪结束：成功输出 {len(out_paths)} 个 tif。")
    return out_paths


import rasterio
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from pathlib import Path
import os


def tif_to_png_with_colormap(tif_path, png_path, colormap='Blues'):
    """
    将 TIF 文件转换为 PNG，并应用指定的色带渲染
    :param tif_path: 输入的 TIF 文件路径
    :param png_path: 输出的 PNG 文件路径
    :param colormap: 渲染使用的色带，默认为蓝色系 ('Blues')
    """
    # 读取TIF文件
    with rasterio.open(tif_path) as src:
        # 读取数据
        data = src.read(1)

        # Mask NaN值
        data = np.ma.masked_equal(data, src.nodata)

        # 创建一个绘图
        fig, ax = plt.subplots(figsize=(10, 10))

        # 使用蓝色系色带渲染数据
        cax = ax.imshow(data, cmap=colormap)
        fig.colorbar(cax, ax=ax, orientation='vertical', label='Water Depth (m)')

        # 设置坐标轴标签为空
        ax.set_xticks([])
        ax.set_yticks([])

        # 保存为 PNG
        plt.savefig(png_path, dpi=300, bbox_inches='tight', pad_inches=0.1)
        plt.close(fig)


def convert_tif_to_pngs(input_dir, output_dir, colormap='Blues'):
    """
    批量转换TIF文件为PNG
    :param input_dir: 输入目录，包含.tif文件
    :param output_dir: 输出目录，用于保存PNG文件
    :param colormap: 渲染使用的色带
    """
    input_dir = Path(input_dir)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # 获取所有TIF文件
    tif_files = sorted(input_dir.glob("*.tif"))

    # 遍历并转换每个TIF文件
    for tif_file in tif_files:
        png_file = output_dir / (tif_file.stem + '.png')
        print(f"Converting {tif_file} to {png_file}")
        tif_to_png_with_colormap(tif_file, png_file, colormap)


# ============================
# 简单示例（你自己改路径）
# ============================
if __name__ == "__main__":

    if os.name == 'nt':  # Windows
        base_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171'
        obs_dir = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\2014-2023年鄱阳湖水域面积栅格数据"
        # shp_dir = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\sci_figure_study_region\upstream_subbasin_1171.shp"
        ## 缩小统计范围
        shp_dir = "G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_shp\subbasin_1171_upstream_only_inundation_disolved3.shp"
        dem_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_raster\dem.tif"
    else:  # Linux/Unix
        base_path = '/data/user/xiaodw/software/WISE/data/poyang_lake1/poyang_lake1_longterm_model_1171'
        obs_dir = '/data/user/xiaodw/software/WISE/data/poyang_lake1/鄱阳湖全天候面积逐日数据集/2014-2023年鄱阳湖水域面积栅格数据'
        shp_dir = r"/data/user/xiaodw/software/WISE/data/poyang_lake1/sci_figure_study_region/upstream_subbasin_1171.shp"
        dem_path = r"/data/user/xiaodw/software/WISE/data/poyang_lake1/workspace/spatial_raster/dem.tif"
    inundation_base_path = os.path.join(base_path,'淹没范围绘图')
    # 1. 根据观测时间筛选模拟文件
    sim_dir = os.path.join(base_path,'OUTPUT0-0')
    selected_obs_out_dir = os.path.join(inundation_base_path,'selected_obs_tif')
    selected_sim_out_dir = os.path.join(inundation_base_path, 'selected_sim_tif')
    ## 这一步处理的图像太多，通常在服务器上做，然后把selected_obs_tif和selected_sim_tif下载到本地
    # monthly_sim, monthly_obs = select_simulation_files_by_obs_time_monthly(
    #     obs_dir=obs_dir,
    #     sim_dir=sim_dir,
    #     selected_obs_out_dir=selected_obs_out_dir,
    #     selected_sim_out_dir=selected_sim_out_dir,
    #     keep_one_per_month=False
    # )

    # 1.1 把观测值tif裁剪到鄱阳湖本身的范围



    input_tif_dir = r"G:\program\seims\SEIMS_HAND\data\TH_4\TH_4_longterm_model\OUTPUT0-0-2 - 副本"  # TIF文件的输入目录
    output_png_dir = "G:\program\seims\SEIMS_HAND\data\TH_4\TH_4_longterm_model\png"  # PNG文件的输出目录
    convert_tif_to_pngs(input_tif_dir, output_png_dir, colormap='Blues')

    # 4. 多张叠加图合成 GIF
    imgs = sorted(Path(output_png_dir).glob("*.png"))
    gif_path = os.path.join(inundation_base_path,'plot_gif','poyang.gif')
    make_gif_from_images(imgs, out_gif_path=gif_path, duration=0.3)



