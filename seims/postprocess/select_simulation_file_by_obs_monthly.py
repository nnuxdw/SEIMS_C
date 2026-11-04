import shutil
from pathlib import Path
from collections import defaultdict
import os
import re
from datetime import datetime

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

def _parse_date_from_sim_name(name: str):
    import re
    from datetime import datetime
    stem = Path(name).stem
    m = re.search(r"TS_(\d{4})_(\d{2})_(\d{2})_", stem)
    if not m:
        raise ValueError(f"无法从模拟文件名解析日期: {name}")
    y, mth, d = map(int, m.groups())
    return datetime(y, mth, d)


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
        print(f"匹配到日期: {d}")
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

if __name__ == "__main__":
    if os.name == 'nt':  # Windows
        base_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171'
        obs_dir = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\2014-2023年鄱阳湖水域面积栅格数据"
    else:  # Linux/Unix
        base_path = '/data/user/xiaodw/software/WISE/data/poyang_lake1/poyang_lake1_longterm_model_1171'
        obs_dir = '/data/user/xiaodw/software/WISE/data/poyang_lake1/鄱阳湖全天候面积逐日数据集/2014-2023年鄱阳湖水域面积栅格数据'
    inundation_base_path = os.path.join(base_path,'淹没范围绘图')
    # 1. 根据观测时间筛选模拟文件
    sim_dir = os.path.join(base_path,'OUTPUT0-0')
    selected_obs_out_dir = os.path.join(inundation_base_path,'selected_obs_tif')
    selected_sim_out_dir = os.path.join(inundation_base_path, 'selected_sim_tif')
    monthly_sim, monthly_obs = select_simulation_files_by_obs_time_monthly(
        obs_dir=obs_dir,
        sim_dir=sim_dir,
        selected_obs_out_dir=selected_obs_out_dir,
        selected_sim_out_dir=selected_sim_out_dir,
        keep_one_per_month=False
    )
