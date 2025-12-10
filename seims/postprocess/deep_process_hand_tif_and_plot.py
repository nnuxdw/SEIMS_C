#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
from pathlib import Path
from datetime import datetime
from collections import defaultdict

import numpy as np
import glob
from rasterio.warp import reproject, Resampling
from rasterio.io import MemoryFile
from rasterio.mask import mask
from pathlib import Path
import numpy as np
import rasterio
import rasterio.plot
import geopandas as gpd
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.colors import ListedColormap, BoundaryNorm
from matplotlib.patches import Patch
import imageio
from PIL import Image
# ======================
# 一些通用小工具
# ======================

def _parse_date_from_obs_name(name: str) -> datetime:
    """
    观测 S1A 文件名示例：
        S1A_2014_10_3.tif
        S1A_2015_6_29.tif
    统一解析为 datetime(YYYY, MM, DD)
    """
    # 去掉后缀
    stem = Path(name).stem
    # 用正则粗略匹配
    m = re.search(r"S1A_(\d{4})_(\d{1,2})_(\d{1,2})", stem)
    if not m:
        raise ValueError(f"无法从观测文件名解析日期: {name}")
    y, mth, d = map(int, m.groups())
    return datetime(y, mth, d)


def _parse_date_from_sim_name(name: str) -> datetime:
    """
    模拟文件名示例（图2）：
        OL_Hand_WTRDEP_TS_2010_01_01_000000.tif
    解析为 datetime(YYYY, MM, DD)
    """
    stem = Path(name).stem
    m = re.search(r"TS_(\d{4})_(\d{2})_(\d{2})_", stem)
    if not m:
        raise ValueError(f"无法从模拟文件名解析日期: {name}")
    y, mth, d = map(int, m.groups())
    return datetime(y, mth, d)


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
):
    """
    参数
    ----
    obs_dir : 观测淹没范围 tif 目录（图1）
    sim_dir : 模拟值沿模范 tif 目录（图2）
    selected_obs_out_dir : 按月筛选后要保存的观测 tif 目录
    selected_sim_out_dir : 按月筛选后要保存的模拟 tif 目录

    返回
    ----
    monthly_sim_paths : list[str]
    monthly_obs_paths : list[str]
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

    # 2. 建立日期映射
    obs_by_date = defaultdict(list)
    for p in obs_files:
        d = _parse_date_from_obs_name(p.name)
        obs_by_date[d.date()].append(p)

    sim_by_date = defaultdict(list)
    for p in sim_files:
        d = _parse_date_from_sim_name(p.name)
        sim_by_date[d.date()].append(p)

    # 3. 找出同时存在的日期
    common_dates = sorted(set(obs_by_date.keys()) & set(sim_by_date.keys()))

    matched_sim_paths = []
    matched_obs_paths = []

    for d in common_dates:
        sim_p = sorted(sim_by_date[d])[0]
        obs_p = sorted(obs_by_date[d])[0]

        matched_sim_paths.append(sim_p)
        matched_obs_paths.append(obs_p)

    # 4. 按月分组
    monthly_sim_paths = []
    monthly_obs_paths = []

    ym_to_indices = defaultdict(list)
    for idx, sim_path in enumerate(matched_sim_paths):
        d = _parse_date_from_sim_name(sim_path.name)
        ym_to_indices[(d.year, d.month)].append(idx)

    # 5. 每月保留第一个，并复制到不同输出目录
    for (year, month) in sorted(ym_to_indices.keys()):
        first_idx = ym_to_indices[(year, month)][0]

        sim_p = matched_sim_paths[first_idx]
        obs_p = matched_obs_paths[first_idx]

        monthly_sim_paths.append(str(sim_p))
        monthly_obs_paths.append(str(obs_p))

        # 复制到单独的 obs/sim 输出路径
        shutil.copy(obs_p, selected_obs_out_dir / obs_p.name)
        shutil.copy(sim_p, selected_sim_out_dir / sim_p.name)

    return monthly_sim_paths, monthly_obs_paths


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

def compute_fi_bi(sim_arr: np.ndarray,
                  obs_arr: np.ndarray,
                  sim_nodata: float = None,
                  obs_nodata: float = None,
                  sim_thresh: float = 0.0,
                  obs_thresh: float = 0.0):
    """
    计算 FI 和 BI。

    修正版逻辑：
    - 模拟值为 nodata 的位置，观测值也被屏蔽（不参与比较）
    - 观测值的 0 表示未淹没，是有效像元，不屏蔽
    """

    sim = sim_arr.astype("float32").copy()
    obs = obs_arr.astype("float32").copy()

    # 1. 模拟 nodata 位置 → 同时屏蔽模拟与观测
    if sim_nodata is not None:
        invalid_mask = (sim == sim_nodata)
    else:
        invalid_mask = np.zeros_like(sim, dtype=bool)

    sim[invalid_mask] = np.nan
    obs[invalid_mask] = np.nan

    # 2. 观测 nodata（若不为 0）也应该被屏蔽
    #    例如 obs_nodata = -9999，但 obs=0 是有效的
    if obs_nodata is not None and obs_nodata != 0:
        obs[obs == obs_nodata] = np.nan

    # 3. 有效比较区域
    valid_mask = (~np.isnan(sim)) & (~np.isnan(obs))
    if not np.any(valid_mask):
        return np.nan, np.nan

    sim_valid = sim[valid_mask]
    obs_valid = obs[valid_mask]

    # 4. 阈值判断
    sim_bin = sim_valid > sim_thresh
    obs_bin = obs_valid > obs_thresh

    # 5. 分类
    hit = np.logical_and(sim_bin, obs_bin)
    miss = np.logical_and(~sim_bin, obs_bin)
    false_alarm = np.logical_and(sim_bin, ~obs_bin)

    H = int(hit.sum())
    M = int(miss.sum())
    F = int(false_alarm.sum())

    print(f"[DEBUG] H={H}, M={M}, F={F}")

    # FI: 交并比
    if (H + M + F) == 0:
        FI = np.nan
    else:
        FI = H / (H + M + F)

    # BI: 模拟淹没面积 / 观测淹没面积
    if (H + M) == 0:
        BI = np.nan
    else:
        BI = (H + F) / (H + M) - 1

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

    # 1. 读模拟水深
    with rasterio.open(sim_tif_path) as sim_src:
        sim_arr = sim_src.read(1)
        sim_nodata = sim_src.nodata
        sim_extent = rasterio.plot.plotting_extent(sim_src)  # (xmin, xmax, ymin, ymax)

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

    # 4. 阈值掩膜
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

    # 5. 颜色
    cmap = ListedColormap([
        "#4169E1",   # 1 - Sim only (overestimate)
        "#FF4500",   # 2 - Obs only (underestimate)
        "#00C853",   # 3 - Overlap
    ])
    norm = BoundaryNorm([0.5, 1.5, 2.5, 3.5], cmap.N)

    # 6. 绘图
    plt.rcParams["font.family"] = "Times New Roman"  # ← 新罗马字体设置（关键）

    fig, ax = plt.subplots(figsize=(6, 8))

    im = ax.imshow(
        cat_ma,
        extent=sim_extent,      # 仍用整幅 extent
        origin="upper",
        cmap=cmap,
        norm=norm,
    )

    # 如果给了 bbox，则缩放到指定经纬度范围
    if bbox is not None:
        xmin, xmax, ymin, ymax = bbox
        ax.set_xlim(xmin, xmax)
        ax.set_ylim(ymin, ymax)

    # 图例
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
        fontweight='bold',
        bbox=dict(facecolor="white", alpha=0.8, edgecolor="none"),
    )

    # ax.set_xlabel("Longitude")
    # ax.set_ylabel("Latitude")
    ax.set_title("Sim vs Obs inundation difference")

    plt.tight_layout()
    fig.savefig(
        out_png_path,
        dpi=500,
        bbox_inches="tight",
        pad_inches=0.05  # 可以改 0.01–0.1 调整边距大小
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
):
    """
    方法二：对方法一的输出结果做裁剪
        读取 in_dir 下已对齐网格的 tif（通常是 *_rs.tif），
        使用 clip_shp 作为裁剪多边形（crop=True），
        输出到 out_dir。

    参数
    ----
    in_dir : 已经对齐到 ref_tif 网格的栅格目录（例如方法一的 out_dir）
    clip_shp_path : 用来裁剪的 shp（可以是流域 / 湖区等 polygon）
    out_dir : 输出裁剪后 tif 的目录

    返回
    ----
    out_paths : list[str]  裁剪结果栅格路径列表
    """
    in_dir = Path(in_dir)
    clip_shp_path = Path(clip_shp_path)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 读取裁剪 shp
    gdf = gpd.read_file(clip_shp_path)
    if gdf.crs is None:
        raise ValueError("clip_shp 没有 CRS，请先为 shp 设置坐标系。")

    out_paths = []

    # 遍历对齐后的栅格
    for tif_path in sorted(in_dir.glob("*.tif")):
        print(f"[裁剪] 处理 tif: {tif_path.name}")

        try:
            with rasterio.open(tif_path) as src:
                raster_crs = src.crs

                # 确保 polygon 与栅格在同一坐标系
                gdf_use = gdf
                if gdf.crs != raster_crs:
                    gdf_use = gdf.to_crs(raster_crs)

                shapes = list(gdf_use.geometry)
                if not shapes:
                    print("  -> 裁剪 shp 中没有几何对象，跳过。")
                    continue

                # 直接对 src 做 mask 裁剪
                out_image, out_transform = mask(
                    src,
                    shapes,
                    crop=True,
                    nodata=src.nodata,
                )

                out_meta = src.meta.copy()
                out_meta.update(
                    {
                        "height": out_image.shape[1],
                        "width": out_image.shape[2],
                        "transform": out_transform,
                        "nodata": src.nodata,
                    }
                )

                out_tif = out_dir / f"{tif_path.stem}_clip.tif"
                with rasterio.open(out_tif, "w", **out_meta) as dst:
                    dst.write(out_image)

                print(f"  -> 已保存裁剪结果: {out_tif}")
                out_paths.append(str(out_tif))

        except Exception as e:
            print(f"[警告] 裁剪 {tif_path.name} 时出错，已跳过。原因: {e}")

    return out_paths



# ============================
# 简单示例（你自己改路径）
# ============================
if __name__ == "__main__":

    base_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171'
    # 1. 根据观测时间筛选模拟文件
    obs_dir = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\2014-2023年鄱阳湖水域面积栅格数据"
    sim_dir = os.path.join(base_path,'OUTPUT0-0')
    selected_obs_out_dir = os.path.join(base_path,'selected_obs_tif')
    selected_sim_out_dir = os.path.join(base_path, 'selected_sim_tif')
    # monthly_sim, monthly_obs = select_simulation_files_by_obs_time_monthly(
    #     obs_dir=obs_dir,
    #     sim_dir=sim_dir,
    #     selected_obs_out_dir=selected_obs_out_dir,
    #     selected_sim_out_dir=selected_sim_out_dir,
    # )

    # 1.1 把观测值tif裁剪到鄱阳湖本身的范围
    clip_sim_dir = os.path.join(base_path,'cliped_sim_tif')
    shp_dir =  r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\sci_figure_study_region\upstream_subbasin_1171.shp"
    # clip_tifs_batch(selected_sim_out_dir,shp_dir,clip_sim_dir)

    # 2. 把观测 tif 转成面 shp，并用面积阈值筛
    obs_shp_dir =os.path.join(base_path,'obs_shp')
    # convert_obs_tifs_to_filtered_polygons(
    #     in_dir=selected_obs_out_dir,
    #     out_dir=obs_shp_dir,
    #     value_threshold=0.0,
    #     area_threshold_km2=1.0,
    #     n_workers=10
    # )

    # 2.1 把观测tif重采样到dem，并裁剪到鄱阳湖本身的范围，以确保行列数正确，用于后续计算FI，BI
    dem_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_raster\dem.tif"
    rsp_obs_dir = os.path.join(base_path, 'rsp_obs_tif')
    clip_obs_dir = os.path.join(base_path, 'cliped_obs_tif')

    # 先做：投影 + 重采样（对齐模拟网格）
    # resampled_paths = resample_obs_to_ref_grid(
    #     obs_dir=selected_obs_out_dir,
    #     ref_tif_path=dem_path,
    #     out_dir=rsp_obs_dir,
    # )

    # 2再做：用 shp 裁剪这些对齐后的 obs
    # clipped_paths = clip_rasters_by_polygon_batch(
    #     in_dir=rsp_obs_dir,
    #     clip_shp_path=shp_dir,
    #     out_dir=clip_obs_dir,
    # )


    # 3. 单个时刻绘图 + FI/BI（你可以用循环对 monthly_* 做）
    plot_dir = os.path.join(base_path,'plot_tif')

    # batch_plot_overlay_and_indices(
    #     sim_dir=clip_sim_dir,
    #     obs_extent_shp_dir=obs_shp_dir,
    #     obs_tif_dir=clip_obs_dir,
    #     out_dir=plot_dir,
    #     sim_thresh=0.1,
    #     obs_thresh=0.1,  # 看你 S1A 的有效值阈值
    # )
    xmin = 115.7
    xmax = 117.0
    ymin = 28.3
    ymax = 29.78
    bbox = (xmin, xmax, ymin, ymax)
    batch_plot_overlap_difference(
        sim_dir=clip_sim_dir,
        obs_tif_dir=clip_obs_dir,
        out_dir=plot_dir,
        sim_thresh=0.1,
        obs_thresh=0.1,  # 看你 S1A 的有效值阈值
        bbox=bbox
    )

    # 4. 多张叠加图合成 GIF

    imgs = sorted(Path(plot_dir).glob("*.png"))
    gif_path = os.path.join(base_path,'plot_gif','poyang.gif')
    make_gif_from_images(imgs, out_gif_path=gif_path, duration=0.5)
