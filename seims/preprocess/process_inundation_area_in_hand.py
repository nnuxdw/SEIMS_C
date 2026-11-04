import os
from typing import List

import geopandas as gpd
import numpy as np
import pandas as pd
import rasterio
from rasterio.features import geometry_mask


def build_hand_storage_table(
    hand_shp_path: str,
    dem_tif_path: str,
    out_csv_path: str,
    id_field: str = "HAND_ID",
    n_levels: int = 10,
):
    """
    根据 HAND 面 shp 与 DEM，按 DEM 高程将每个 HAND 内部划分为 n_levels 个等高差 level，
    并为每个 level 计算：
        - 对应像元行列号列表（只包含该 level 范围内的像元）
        - level 面积（该 level 范围内像元面积之和）
        - level 的起始库容和终止库容（从最低处蓄到该 level 上界的准确体积）

    体积计算方式：
        对于每个 level 的上界高度 upper：
            Volume(upper) = Σ(upper - z_i) * pixel_area   对所有满足 z_i <= upper 的像元累加
        然后：
            Storage_start = Volume(上一 level 的 upper)
            Storage_end   = Volume(当前 level 的 upper)
            Level_volume  = Storage_end - Storage_start

    输出 CSV 字段：
        HAND_ID, Level, Storage_start, Storage_end, Level_volume, DEMs, Level_area,
        Level_lower_elev, Level_upper_elev
    """

    # 0. 确保输出目录存在
    out_dir = os.path.dirname(out_csv_path)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir, exist_ok=True)

    # 1. 读取 DEM
    with rasterio.open(dem_tif_path) as dem_src:
        dem = dem_src.read(1)  # 假设 1 波段 DEM
        dem_transform = dem_src.transform
        dem_crs = dem_src.crs
        dem_nodata = dem_src.nodata

    # 计算像元面积（假设投影坐标，单位 m）：
    # transform.a 为像元宽度，transform.e 为像元高度（一般为负）
    pixel_width = dem_transform.a
    pixel_height = dem_transform.e
    pixel_area = abs(pixel_width * pixel_height)

    # 2. 读取 HAND 面数据
    gdf = gpd.read_file(hand_shp_path)

    # 与 DEM 对齐坐标系
    if gdf.crs != dem_crs:
        gdf = gdf.to_crs(dem_crs)

    # 校验 id 字段
    if id_field not in gdf.columns:
        raise ValueError(f"shp 中未找到字段 '{id_field}'，请检查字段名或修改 id_field 参数。")

    results = []

    # 3. 对每一个 HAND 多边形进行处理
    for idx, row in gdf.iterrows():
        hand_id = row[id_field]
        geom = row.geometry

        if geom is None or geom.is_empty:
            continue

        # 3.1 生成该 HAND 在整个 DEM 网格上的掩膜（几何范围）
        geom_mask = geometry_mask(
            [geom],
            out_shape=dem.shape,
            transform=dem_transform,
            invert=True,  # True = HAND 内为 True
        )

        # 3.2 再叠加 nodata / NaN 掩膜，得到真正有效的 HAND 像元
        valid_mask = geom_mask.copy()

        if dem_nodata is not None:
            valid_mask &= (dem != dem_nodata)
        # 去掉 NaN
        valid_mask &= ~np.isnan(dem)

        # 提取 HAND 内部有效 DEM 值
        dem_vals = dem[valid_mask]

        if dem_vals.size == 0:
            # 该 HAND 内没有有效 DEM 像元
            continue

        dem_min = float(np.min(dem_vals))
        dem_max = float(np.max(dem_vals))

        # 按高程范围等分 n_levels
        level_edges = np.linspace(dem_min, dem_max, n_levels + 1)

        # 记录“上一 level 的累计体积”，初始为 0
        prev_cum_volume = 0.0

        # 3.3 遍历每一个 level
        for level_idx in range(n_levels):
            lower = level_edges[level_idx]
            upper = level_edges[level_idx + 1]

            # --- 3.3.1 当前 level 的像元范围（用于 DEMs 列表和 Level_area） ---
            # 条件：像元在 HAND 内且 DEM 在 [lower, upper)
            # 对最后一个 level，将上界设为 <= upper，避免边界遗漏
            if level_idx < n_levels - 1:
                in_level_mask = (dem >= lower) & (dem < upper) & valid_mask
            else:
                in_level_mask = (dem >= lower) & (dem <= upper) & valid_mask

            level_rows, level_cols = np.where(in_level_mask)

            if level_rows.size == 0:
                level_area = 0.0
                dems_str = ""
            else:
                level_area = float(level_rows.size * pixel_area)
                dems_pairs = [f"{r},{c}" for r, c in zip(level_rows, level_cols)]
                dems_str = ";".join(dems_pairs)

            # --- 3.3.2 精确计算“到当前 upper 水位为止”的累计体积 ---
            # 所有在 HAND 内且高程 <= upper 的像元都被淹没
            inundated_mask = (dem <= upper) & valid_mask
            dem_inundated = dem[inundated_mask]

            if dem_inundated.size == 0:
                cum_volume = 0.0
            else:
                # 对每个像元：水深 = upper - dem_i
                depth_array = upper - dem_inundated
                # 体积 = Σ depth_i * pixel_area
                cum_volume = float(np.sum(depth_array * pixel_area))

            storage_start = prev_cum_volume
            storage_end = cum_volume
            level_volume = storage_end - storage_start  # 该 level 新增体积
            prev_cum_volume = cum_volume

            results.append({
                "HAND_ID": hand_id,
                "Level": level_idx + 1,  # 从 1 开始编号
                "Storage_start": storage_start,
                "Storage_end": storage_end,
                "Level_volume": level_volume,
                "DEMs": dems_str,
                "Level_area": level_area,
                "Level_lower_elev": lower,
                "Level_upper_elev": upper,
            })

    # 4. 保存为 CSV
    df = pd.DataFrame(results)
    df.to_csv(out_csv_path, index=False, encoding="utf-8")
    print(f"LEVEL 查找表已保存到: {out_csv_path}")


if __name__ == '__main__':
    hand_shp_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\HRU_file\HRU_dissolved.shp"
    dem_tif_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_raster\dem.tif"
    out_csv_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\inundation_in_hand.csv'

    build_hand_storage_table(
        hand_shp_path=hand_shp_path,
        dem_tif_path=dem_tif_path,
        out_csv_path=out_csv_path,
        id_field="FIELDID",  # 根据你 shp 里的字段名改
        n_levels=10
    )
