# -*- coding: utf-8 -*-
from pathlib import Path

import numpy as np
import pandas as pd
import rasterio
import geopandas as gpd

from rasterio.features import rasterize
from rasterio.warp import reproject, Resampling

import warnings
warnings.filterwarnings(
    "ignore",
    message="__len__ for multi-part geometries is deprecated.*",
)
# =========================
# 1) 读取分层表（图1 csv）
# =========================
def load_subbasin_level_table_csv(csv_path):
    """
    图1 csv 需要至少包含列：
      Subbasin, Flood_Level, SumArea
    可选列：
      AccVolume

    返回：
      table[sub_id][level] = {
          "SumArea": float,
          "SumAreaPrev": float,
          "AccVolume": float or None,
          "AccVolumePrev": float or None
      }
    """
    df = pd.read_csv(csv_path)
    for c in ("Subbasin", "Flood_Level", "SumArea"):
        if c not in df.columns:
            raise ValueError("CSV missing column: {0}".format(c))

    df["Subbasin"] = df["Subbasin"].astype(int)
    df["Flood_Level"] = df["Flood_Level"].astype(int)
    df["SumArea"] = df["SumArea"].astype(float)

    has_acc = ("AccVolume" in df.columns)
    if has_acc:
        df["AccVolume"] = df["AccVolume"].astype(float)

    table = {}
    for sub_id, g in df.groupby("Subbasin"):
        g = g.sort_values("Flood_Level")
        levels = g["Flood_Level"].values
        sum_areas = g["SumArea"].values
        if has_acc:
            acc_vol = g["AccVolume"].values
        else:
            acc_vol = None

        sub_map = {}
        for i in range(len(levels)):
            lvl = int(levels[i])
            A = float(sum_areas[i])
            A_prev = float(sum_areas[i - 1]) if i > 0 else 0.0

            if has_acc:
                V = float(acc_vol[i])
                V_prev = float(acc_vol[i - 1]) if i > 0 else 0.0
            else:
                V = None
                V_prev = None

            sub_map[lvl] = {
                "SumArea": A,
                "SumAreaPrev": A_prev,
                "AccVolume": V,
                "AccVolumePrev": V_prev,
            }

        table[int(sub_id)] = sub_map

    return table


# =========================
# 2) 读取层级映射（图2 txt）
# =========================
def load_sub_hru_level_map(level_txt_path):
    """
    固定表头：
      HRU_ID    Subbasin    Flood_Level    HAND_Threshold_Interval    Depth

    返回：
      sub_hru_level[sub_id][hru_id] = flood_level
    """
    sub_hru_level = {}
    header_seen = False

    with open(level_txt_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            if s.startswith("#"):
                continue

            parts = s.split()  # 同时支持 tab 和空格
            if not header_seen:
                # 严格检查表头
                # 你的表头是：HRU_ID Subbasin Flood_Level HAND_Threshold_Interval Depth
                if len(parts) >= 5 and parts[0] == "HRU_ID" and parts[1] == "Subbasin" and parts[2] == "Flood_Level":
                    header_seen = True
                    continue
                else:
                    raise ValueError("Unexpected header in FloodStep.txt. Got: {0}".format(s))

            # 数据行：至少 3 列
            if len(parts) < 3:
                continue

            try:
                hru_id = int(float(parts[0]))
                sub_id = int(float(parts[1]))
                level = int(float(parts[2]))
            except Exception:
                # 有脏行就跳过
                continue

            if sub_id not in sub_hru_level:
                sub_hru_level[sub_id] = {}
            sub_hru_level[sub_id][hru_id] = level

    if not header_seen:
        raise ValueError("Header line not found in file: {0}".format(level_txt_path))
    if not sub_hru_level:
        raise ValueError("No mapping parsed from file: {0}".format(level_txt_path))

    return sub_hru_level



# =========================
# 3) HAND index txt（含 elev_min）
# =========================
def read_hand_index_txt_with_meta(index_dir):
    index_dir = Path(index_dir)
    index_txt = index_dir / "hand_index.txt"
    if not index_txt.exists():
        raise FileNotFoundError("hand_index.txt not found: {0}".format(index_txt))

    hid_to_file = {}
    hid_to_elevmin = {}
    with open(str(index_txt), "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            s = line.strip()
            if (not s) or s.startswith("#"):
                continue
            parts = s.split()
            if len(parts) < 5:
                continue
            hid = int(parts[0])
            elev_min = float(parts[2])
            cell_file = parts[4]
            hid_to_file[hid] = cell_file
            hid_to_elevmin[hid] = elev_min
    return hid_to_file, hid_to_elevmin


# =========================
# 4) 等面积 shp -> 面积映射
# =========================
def load_hand_area_map(hand_shp_equal_area, field_id, area_field):
    """
    稳健读取 HAND 面积映射：
    - 不因 is_valid 直接丢 HAND（环状/带洞也保留）
    - geometry 只用于 dissolve
    - 面积严格来自 area_field（等面积投影 shp 中的字段）
    """
    import numpy as np
    import geopandas as gpd

    gdf = gpd.read_file(hand_shp_equal_area)
    if gdf.crs is None:
        raise ValueError("hand_shp_equal_area has no CRS.")
    if field_id not in gdf.columns:
        raise ValueError(f"equal-area shp missing field_id={field_id}")
    if area_field not in gdf.columns:
        raise ValueError(f"equal-area shp missing area_field={area_field}")

    # 基本清洗
    gdf = gdf[gdf.geometry.notnull()].copy()

    # FIELDID / HRU_ID → int（兼容 '1171', '1171.0'）
    def _to_int(x):
        try:
            return int(float(x))
        except Exception:
            return None

    gdf[field_id] = gdf[field_id].apply(_to_int)
    gdf = gdf[gdf[field_id].notnull()].copy()
    gdf[field_id] = gdf[field_id].astype(int)

    # area 字段 → float
    gdf[area_field] = gdf[area_field].astype(float)

    # ⚠️ 关键变化：
    # 不再用 gdf.is_valid 过滤
    # 只在 dissolve 前做最小修复，避免 rasterize / dissolve 崩
    try:
        from shapely.validation import make_valid
        gdf["geometry"] = gdf.geometry.apply(
            lambda g: make_valid(g) if (g is not None and not g.is_valid) else g
        )
    except Exception:
        # shapely<2.0 兜底
        gdf["geometry"] = gdf.geometry.apply(
            lambda g: g.buffer(0) if (g is not None and not g.is_valid) else g
        )

    # 修复失败（geometry 变 None）的才丢
    gdf = gdf[gdf.geometry.notnull()].copy()

    # dissolve：一个 HID 可能多 polygon（环状、分块）
    # 注意：area 不在这里 sum，而是 dissolve 后再从字段取
    gdf = gdf.dissolve(by=field_id, as_index=False)

    # 构建 area_map（完全信任 area_field）
    area_map = {}
    bad = 0
    for _, row in gdf.iterrows():
        hid = int(row[field_id])
        area = float(row[area_field])
        if np.isfinite(area) and area > 0:
            area_map[hid] = area
        else:
            bad += 1

    print(f"[AREA] area_map size={len(area_map)}, bad_area={bad}")
    return area_map


# =========================
# 5) 栅格化 shp 到 DEM 网格（支持 dissolve）
# =========================
def rasterize_id_shp_to_dem_grid(dem_tif, shp_path, id_field,
                                 all_touched=False, fill=0, dtype=np.int32):
    """
    把 shp 按 id_field 栅格化到 dem_tif 的网格上。
    ✅ 关键：不再直接丢弃 is_valid=False 的要素，而是先用 buffer(0) 修复，
       避免像 1171 这种带 hole 的面因为 invalid 被过滤掉。

    参数
    ----
    dem_tif : str
        参考 DEM（决定输出网格、transform、crs、shape）
    shp_path : str
        输入矢量
    id_field : str
        用来栅格化的字段名（例如 SUBBASINID / Subbasin / FIELDID）
    all_touched : bool
        rasterize 的 all_touched
    fill : int
        输出栅格填充值
    dtype : numpy dtype
        输出栅格类型（建议 int32）

    返回
    ----
    arr : np.ndarray (H, W)
        栅格化后的 ID 栅格
    """
    # 1) 读 DEM 作为参考网格
    with rasterio.open(dem_tif) as src:
        transform = src.transform
        crs = src.crs
        h, w = src.height, src.width

    if crs is None:
        raise ValueError("DEM has no CRS: {0}".format(dem_tif))

    # 2) 读矢量
    gdf = gpd.read_file(shp_path)
    if gdf.crs is None:
        raise ValueError("SHP has no CRS: {0}".format(shp_path))
    if id_field not in gdf.columns:
        raise ValueError("SHP missing id_field={0}: {1}".format(id_field, shp_path))

    # 3) 基础清理 + 类型
    gdf = gdf[gdf.geometry.notnull()].copy()
    if gdf.empty:
        return np.full((h, w), fill, dtype=dtype)

    try:
        gdf[id_field] = gdf[id_field].astype(int)
    except Exception:
        # 兼容字段是 float/str 的情况
        gdf[id_field] = gdf[id_field].apply(lambda x: int(float(x)))

    # 4) 投影到 DEM CRS
    gdf = gdf.to_crs(crs)

    # 5) ✅ 修复 invalid（不要直接丢！）
    #    Shapely 1.x 常用修复：buffer(0)
    invalid_mask = ~gdf.geometry.is_valid
    if invalid_mask.any():
        # 注意：buffer(0) 可能把 Polygon 修成 MultiPolygon，这是正常的
        gdf.loc[invalid_mask, "geometry"] = gdf.loc[invalid_mask, "geometry"].buffer(0)

    # 6) 再过滤空/无效（修复后）
    gdf = gdf[gdf.geometry.notnull()].copy()
    gdf = gdf[~gdf.geometry.is_empty].copy()
    gdf = gdf[gdf.geometry.is_valid].copy()
    if gdf.empty:
        return np.full((h, w), fill, dtype=dtype)

    # 7) dissolve（把同一个 id 的多要素合并）
    gdf = gdf.dissolve(by=id_field, as_index=False)
    gdf = gdf[gdf.geometry.notnull()].copy()
    gdf = gdf[~gdf.geometry.is_empty].copy()
    gdf = gdf[gdf.geometry.is_valid].copy()
    if gdf.empty:
        return np.full((h, w), fill, dtype=dtype)

    # 8) rasterize
    shapes = []
    for geom, val in zip(gdf.geometry, gdf[id_field]):
        if geom is None:
            continue
        shapes.append((geom, int(val)))

    arr = rasterize(
        shapes=shapes,
        out_shape=(h, w),
        transform=transform,
        fill=fill,
        all_touched=all_touched,
        dtype=dtype,
    )
    return arr


# =========================
# 6) 对齐 raster 到 DEM 网格
# =========================
def align_raster_to_reference(src_tif, ref_tif, src_band=1, resampling="nearest", dst_nodata=-9999.0):
    res_map = {
        "nearest": Resampling.nearest,
        "bilinear": Resampling.bilinear,
        "cubic": Resampling.cubic,
    }
    if resampling not in res_map:
        raise ValueError("resampling must be nearest/bilinear/cubic")

    with rasterio.open(ref_tif) as ref:
        dst_crs = ref.crs
        dst_transform = ref.transform
        dst_h, dst_w = ref.height, ref.width

    with rasterio.open(src_tif) as src:
        src_arr = src.read(src_band).astype(np.float32)
        src_crs = src.crs
        src_transform = src.transform
        src_nodata = src.nodata

    dst = np.full((dst_h, dst_w), dst_nodata, dtype=np.float32)

    reproject(
        source=src_arr,
        destination=dst,
        src_transform=src_transform,
        src_crs=src_crs,
        src_nodata=src_nodata,
        dst_transform=dst_transform,
        dst_crs=dst_crs,
        dst_nodata=dst_nodata,
        resampling=res_map[resampling],
    )
    return dst


# =========================
# 7) 健壮读取 HAND cells txt（支持 1 行）
# =========================
def load_hand_cells_txt(cell_txt_path):
    cell_txt_path = Path(cell_txt_path)

    try:
        arr = np.loadtxt(str(cell_txt_path), comments="#")
    except Exception:
        arr = np.loadtxt(str(cell_txt_path), comments="#", delimiter=",")

    if arr.size == 0:
        raise ValueError("HAND cells file has no numeric rows: {0}".format(str(cell_txt_path)))

    if arr.ndim == 1:
        if arr.shape[0] < 4:
            head = []
            with open(str(cell_txt_path), "r", encoding="utf-8", errors="ignore") as f:
                for _ in range(10):
                    line = f.readline()
                    if not line:
                        break
                    head.append(line.rstrip("\n"))
            raise ValueError(
                "HAND cells file has <4 columns (need rank,flat_index,elev,prefix...). "
                "File: {0}\nHead:\n{1}".format(str(cell_txt_path), "\n".join(head))
            )
        arr = arr.reshape(1, -1)

    if arr.shape[1] < 4:
        raise ValueError("HAND cells file has <4 columns: {0}".format(str(cell_txt_path)))

    flat_idx = arr[:, 1].astype(np.int64)
    elev = arr[:, 2].astype(np.float64)
    prefix = arr[:, 3].astype(np.float64)
    return flat_idx, elev, prefix


# =========================================================
# 8) build 预处理：每个 HAND 输出 txt
#    ✅ prefix_sum 改为“叠加上一层 SumArea 的常数项”，用于体积反演
# =========================================================
def build_hand_dem_rank_index_all_txt(
    dem_tif_wgs84,
    hand_shp_wgs84,
    field_id,
    out_dir,
    hand_shp_equal_area,
    area_field,
    level_csv_path,
    level_txt_path,
    all_touched=False,
    dem_band=1,
):
    """
    输出：
      out_dir/hand_index.txt
      out_dir/HAND_<hid>_cells.txt

    cell 文件列：
      # rank flat_index elevation prefix_sum lon lat

    这里的 prefix_sum 不再是“单纯 elev 的前缀和”，而是：
      prefix_sum[i] = sum(elev[0..i]) * cell_area + z0 * A_prev

    这样对任意水位 h（且淹没格子数 k=i+1）：
      V(h) = h*(k*cell_area + A_prev) - prefix_sum[i]

    其中：
      A_prev 来自图1 csv：该 HAND 对应 Flood_Level 的上一层 SumArea（累计面积）
      HAND -> Flood_Level 通过图2 txt（HRU_ID/FIELDID 与 Flood_Level 的对应）
    """
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 读分层累计面积表（图1）
    sub_level_table = load_subbasin_level_table_csv(level_csv_path)
    sub_hru_level = load_sub_hru_level_map(level_txt_path)  # ✅ FloodStep.txt 映射
    # 读等面积 shp 的 HAND 面积（m^2）
    area_map = load_hand_area_map(hand_shp_equal_area, field_id, area_field)

    with rasterio.open(dem_tif_wgs84) as src:
        dem = src.read(dem_band).astype(np.float32)
        dem_nodata = src.nodata
        transform = src.transform
        crs = src.crs
        height, width = src.height, src.width

    if crs is None:
        raise ValueError("DEM has no CRS.")

    dem_valid = np.isfinite(dem) if dem_nodata is None else (dem != dem_nodata)

    # HAND shp -> DEM CRS
    gdf = gpd.read_file(hand_shp_wgs84)
    if gdf.crs is None:
        raise ValueError("hand_shp_wgs84 has no CRS.")
    if field_id not in gdf.columns:
        raise ValueError("hand_shp_wgs84 missing field: {0}".format(field_id))

    gdf = gdf[gdf.geometry.notnull()].copy()
    gdf[field_id] = gdf[field_id].apply(lambda x: int(float(x)) if pd.notnull(x) else None)
    gdf = gdf[gdf[field_id].notnull()].copy()
    gdf[field_id] = gdf[field_id].astype(int)

    # 投影到 DEM CRS
    gdf = gdf.to_crs(crs)

    # ✅ 修复 invalid geometry：不要直接 gdf[gdf.is_valid]
    try:
        from shapely.validation import make_valid
        fixed = []
        for geom in gdf.geometry:
            if geom is None:
                fixed.append(None)
            elif geom.is_valid:
                fixed.append(geom)
            else:
                try:
                    fixed.append(make_valid(geom))
                except Exception:
                    try:
                        fixed.append(geom.buffer(0))
                    except Exception:
                        fixed.append(None)
        gdf["geometry"] = fixed
    except Exception:
        # shapely<2.0 兜底
        fixed = []
        for geom in gdf.geometry:
            if geom is None:
                fixed.append(None)
            elif geom.is_valid:
                fixed.append(geom)
            else:
                try:
                    fixed.append(geom.buffer(0))
                except Exception:
                    fixed.append(None)
        gdf["geometry"] = fixed

    gdf = gdf[gdf.geometry.notnull()].copy()

    # dissolve 支持 multipolygon / 环状 / 碎片
    gdf = gdf.dissolve(by=field_id, as_index=False)

    shapes = []
    for geom, fid in zip(gdf.geometry, gdf[field_id]):
        if geom is None:
            continue
        shapes.append((geom, int(fid)))

    hand_id_raster = rasterize(
        shapes=shapes,
        out_shape=(height, width),
        transform=transform,
        fill=0,
        all_touched=all_touched,
        dtype=np.int32,
    )

    dem_flat = dem.ravel()
    id_flat = hand_id_raster.ravel()
    valid_flat = dem_valid.ravel()

    index_txt = out_dir / "hand_index.txt"
    with open(str(index_txt), "w", encoding="utf-8") as idx_f:
        idx_f.write("# HAND_ID  n_cells  elev_min  elev_max  cell_file\n")

        unique_ids = np.unique(hand_id_raster)
        unique_ids = unique_ids[unique_ids != 0]
        unique_ids.sort()

        for hid in unique_ids:
            hid = int(hid)
            print("[BUILD] HAND {0} ...".format(hid))

            m = (id_flat == hid) & valid_flat
            if not np.any(m):
                continue

            # 等面积 shp 中必须存在该 hid 的面积
            if hid not in area_map:
                print("  [SKIP] no area for HAND {0} in equal-area shp".format(hid))
                continue

            flat_idx = np.where(m)[0].astype(np.int64)
            elev = dem_flat[flat_idx].astype(np.float64)

            order = np.argsort(elev, kind="mergesort")
            flat_idx_sorted = flat_idx[order]
            elev_sorted = elev[order]
            n = len(elev_sorted)

            # 查该 HAND 对应的 (sub_id, flood_level) -> A_prev
            # 注意：同一个 hid 理论上只属于一个 sub_id（你的 FloodStep.txt 提供这种对应关系）
            sub_id = None
            flood_level = None

            # 在 level txt 里找 hid 属于哪个 sub_id
            # （sub_hru_level[sub][hru]=level）
            for sid in sub_hru_level:
                if hid in sub_hru_level[sid]:
                    sub_id = int(sid)
                    flood_level = int(sub_hru_level[sid][hid])
                    break

            if sub_id is None or flood_level is None:
                print("  [SKIP] HAND {0} not found in FloodStep.txt mapping".format(hid))
                continue

            if sub_id not in sub_level_table:
                print("  [SKIP] sub {0} not found in InundationMap.csv".format(sub_id))
                continue
            if flood_level not in sub_level_table[sub_id]:
                print("  [SKIP] level {0} not found in InundationMap.csv for sub {1}".format(flood_level, sub_id))
                continue

            A_prev = float(sub_level_table[sub_id][flood_level]["SumAreaPrev"])

            # cell_area
            area_hand = float(area_map[hid])
            cell_area = area_hand / float(n)

            # prefix_sum[i] = sum(elev[0..i]) * cell_area + z0 * A_prev
            z0 = float(elev_sorted[0])
            csum = np.cumsum(elev_sorted, dtype=np.float64)
            prefix = csum * cell_area + z0 * A_prev

            # lon/lat of cell center (debug only)
            rows = flat_idx_sorted // width
            cols = flat_idx_sorted % width
            xs, ys = rasterio.transform.xy(transform, rows, cols, offset="center")
            xs = np.asarray(xs, dtype=np.float64)
            ys = np.asarray(ys, dtype=np.float64)

            cell_file = "HAND_{0}_cells.txt".format(hid)
            with open(str(out_dir / cell_file), "w", encoding="utf-8") as f:
                f.write("# rank  flat_index  elevation  prefix_sum  lon  lat\n")
                for i in range(n):
                    f.write(
                        "{0} {1} {2:.6f} {3:.6f} {4:.8f} {5:.8f}\n".format(
                            i, int(flat_idx_sorted[i]), float(elev_sorted[i]), float(prefix[i]),
                            float(xs[i]), float(ys[i])
                        )
                    )

            idx_f.write(
                "{0} {1} {2:.6f} {3:.6f} {4}\n".format(
                    hid, n, float(elev_sorted[0]), float(elev_sorted[-1]), cell_file
                )
            )

    print("[OK] TXT index written to: {0}".format(str(out_dir)))


# =========================================================
# 9) ✅ 分层累计面积参与的“边界 HAND 反演”
# =========================================================
def invert_water_level_with_prev_area(elev_sorted, cell_area, A_prev, V_target):
    """
    解 V(h) = V_target

    V(h)= (sum_{i=1..k} (h - e_i)) * cell_area  + (h - z0) * A_prev
        = h*(k*cell_area + A_prev) - (sum_e(k)*cell_area + z0*A_prev)

    返回：
      h (float) 水位
      k (int) 被淹格子数
    """
    n = len(elev_sorted)
    if n == 0:
        return None, 0

    z0 = float(elev_sorted[0])

    csum = np.cumsum(elev_sorted, dtype=np.float64)

    def sum_e(k):
        if k <= 0:
            return 0.0
        return float(csum[k - 1])

    # V_at_z(k): 当水位 z=elev[k-1] 时的体积
    V_at_z = np.empty(n, dtype=np.float64)
    for k in range(1, n + 1):
        z = float(elev_sorted[k - 1])
        V_at_z[k - 1] = (k * z - sum_e(k)) * cell_area + (z - z0) * A_prev

    k_idx = int(np.searchsorted(V_at_z, V_target, side="left"))
    if k_idx < 0:
        k = 1
    elif k_idx >= n:
        k = n
    else:
        k = k_idx + 1

    denom = (k * cell_area + A_prev)
    if denom <= 0:
        return None, 0

    h = (V_target + sum_e(k) * cell_area + z0 * A_prev) / denom
    return float(h), int(k)


# =========================================================
# 10) ✅ 主函数：子流域 + 边界 HAND + SumArea 累计面积体积
# =========================================================
def downscale_hand_depth_with_subbasin_layer_area(
    dem_tif_wgs84,
    hand_shp_wgs84,
    hand_shp_equal_area,
    field_id,
    area_field,
    index_dir,
    coarse_hand_depth_tif,
    out_depth_tif,

    subbasin_shp_wgs84,
    sub_id_field,

    level_csv_path,      # 图1 csv
    level_txt_path,      # 图2 txt（用于 HAND->Flood_Level）

    depth_unit="m",
    depth_stat="max",
    all_touched=False,
    dem_band=1,
    depth_band=1,
    out_nodata=-9999.0,
    align_if_needed=True,
):
    # ---- load tables ----
    sub_level_table = load_subbasin_level_table_csv(level_csv_path)
    sub_hru_level = load_sub_hru_level_map(level_txt_path)  # ✅ 直接 HRU(FIELDID)->Flood_Level

    area_map = load_hand_area_map(hand_shp_equal_area, field_id, area_field)
    hid_to_file, hid_to_elevmin = read_hand_index_txt_with_meta(index_dir)
    index_dir = Path(index_dir)

    # ---- read DEM ----
    with rasterio.open(dem_tif_wgs84) as dem_src:
        dem = dem_src.read(dem_band).astype(np.float32)
        dem_nodata = dem_src.nodata
        profile = dem_src.profile.copy()

    dem_valid = np.isfinite(dem) if dem_nodata is None else (dem != dem_nodata)

    # ---- read coarse depth ----
    with rasterio.open(coarse_hand_depth_tif) as dep_src:
        coarse = dep_src.read(depth_band).astype(np.float32)
        dep_nodata = dep_src.nodata

    if align_if_needed and coarse.shape != dem.shape:
        print("[WARN] coarse grid != DEM grid, aligning coarse -> DEM ...")
        coarse = align_raster_to_reference(
            src_tif=coarse_hand_depth_tif,
            ref_tif=dem_tif_wgs84,
            src_band=depth_band,
            resampling="nearest",
            dst_nodata=-9999.0,
        ).astype(np.float32)
        dep_nodata = -9999.0

    dep_valid = np.isfinite(coarse) if dep_nodata is None else (coarse != dep_nodata)

    # ---- rasterize HAND & subbasin ----
    hand_id_raster = rasterize_id_shp_to_dem_grid(
        dem_tif=dem_tif_wgs84,
        shp_path=hand_shp_wgs84,
        id_field=field_id,
        all_touched=all_touched,
        fill=0,
        dtype=np.int32,
    )
    sub_id_raster = rasterize_id_shp_to_dem_grid(
        dem_tif=dem_tif_wgs84,
        shp_path=subbasin_shp_wgs84,
        id_field=sub_id_field,
        all_touched=all_touched,
        fill=0,
        dtype=np.int32,
    )

    # ---- output ----
    out = np.full(dem.shape, out_nodata, dtype=np.float32)
    # ✅ 默认：所有 DEM 有效像元都是“未淹=0”
    out[dem_valid] = 0.0

    # unit conversion
    if depth_unit not in ("m", "mm"):
        raise ValueError("depth_unit must be 'm' or 'mm'")
    depth_scale = 1.0 if depth_unit == "m" else 0.001

    # flatten
    dem_flat = dem.ravel()
    dem_valid_flat = dem_valid.ravel()
    dep_flat = coarse.ravel()
    dep_valid_flat = dep_valid.ravel()
    hand_flat = hand_id_raster.ravel()
    sub_flat = sub_id_raster.ravel()
    out_flat = out.ravel()

    # subbasin list
    sub_ids = np.unique(sub_id_raster)
    sub_ids = sub_ids[sub_ids != 0]
    sub_ids.sort()

    for sub_id in sub_ids:
        sub_id = int(sub_id)

        # 子流域 mask
        m_sub = (sub_flat == sub_id) & dem_valid_flat
        if not np.any(m_sub):
            continue

        # 有水深的像元（粗图里 >0）
        m_wet = m_sub & dep_valid_flat & (dep_flat > 0)
        if not np.any(m_wet):
            continue

        # 候选 HAND：子流域内 depth>0 的 HAND（说明已经“全淹/部分淹”至少在粗图上出现）
        cand_hids = np.unique(hand_flat[m_wet])
        cand_hids = cand_hids[cand_hids != 0]
        if cand_hids.size == 0:
            continue

        # 过滤没 index/area 的
        valid_cands = []
        for hid in cand_hids:
            hid = int(hid)
            if hid in hid_to_file and hid in hid_to_elevmin and hid in area_map:
                valid_cands.append(hid)
        if not valid_cands:
            continue

        # 子流域的层级表（图1 InundationMap.csv）
        if sub_id not in sub_level_table:
            continue
        sub_levels = sub_level_table[sub_id]  # {level: {...}}

        # ✅ 用 FloodStep.txt 的映射：HAND_ID(=HRU_ID) -> Flood_Level
        max_level = None
        boundary_hid = None

        # valid_cands 里都是“wet”的 HAND（候选）
        if sub_id in sub_hru_level:
            sub_map = sub_hru_level[sub_id]  # {hru_id: flood_level}
        else:
            sub_map = None

        if sub_map is None:
            continue

        for hid in valid_cands:
            if hid in sub_map:
                lvl = sub_map[hid]
                if (max_level is None) or (lvl > max_level):
                    max_level = lvl
                    boundary_hid = hid

        if boundary_hid is None:
            continue

        flood_level = int(max_level)

        # 防止 Flood_Level 不在 InundationMap.csv 里
        if flood_level not in sub_levels:
            continue

        A_cum = float(sub_levels[flood_level]["SumArea"])
        A_prev = float(sub_levels[flood_level]["SumAreaPrev"])

        # -----------------------
        # 2) 取边界 HAND 的 coarse depth 统计值
        # -----------------------
        m_b = m_sub & dep_valid_flat & (dep_flat > 0) & (hand_flat == boundary_hid)
        if not np.any(m_b):
            continue

        vals = dep_flat[m_b]
        if depth_stat == "max":
            depth_hand = float(np.max(vals))
        elif depth_stat == "mean":
            depth_hand = float(np.mean(vals))
        elif depth_stat == "median":
            depth_hand = float(np.median(vals))
        else:
            raise ValueError("depth_stat must be 'max'/'mean'/'median'")

        depth_hand_m = depth_hand * depth_scale
        if (not np.isfinite(depth_hand_m)) or depth_hand_m <= 0:
            continue

        # -----------------------
        # 3) ✅ 目标体积：用该层累计面积 SumArea 作为“底面积”
        #    因为每往上淹一点，都必须先把低层的累计面积那部分一起“顶起来”
        # -----------------------
        V_target = depth_hand_m * A_cum  # m^3

        # -----------------------
        # 4) 对 boundary HAND 反演水位 h（体积公式里含 A_prev）
        # -----------------------
        cell_file = hid_to_file[boundary_hid]
        cell_path = index_dir / cell_file
        if not cell_path.exists():
            continue

        flat_idx_sorted, elev_sorted, _prefix = load_hand_cells_txt(cell_path)
        n = len(elev_sorted)
        if n == 0:
            continue

        area_hand = float(area_map[boundary_hid])
        cell_area = area_hand / float(n)

        h, kf = invert_water_level_with_prev_area(
            elev_sorted=elev_sorted,
            cell_area=cell_area,
            A_prev=A_prev,
            V_target=V_target,
        )
        if h is None:
            continue

        # -----------------------
        # 5) 写出：子流域内 <= 当前 Flood_Level 的 HAND 都用同一水位 h
        #    - 低于该层：自然全淹（深度= max(h-dem,0)）
        #    - 该层：部分淹（同样 max(h-dem,0)，会自动只淹低于 h 的格子）
        #    - 高于该层：不淹（0）
        # -----------------------
        # ✅ 这里不要再用 elev_min 排序判断层级，而是用 FloodStep.txt 的 level 映射判断
        idx_sub = np.where(m_sub)[0]
        sub_hand_ids = hand_flat[idx_sub]

        low_mask = np.zeros(sub_hand_ids.size, dtype=np.bool_)
        for i in range(sub_hand_ids.size):
            hid = int(sub_hand_ids[i])
            if hid == 0:
                low_mask[i] = False
                continue
            if hid in sub_hru_level[sub_id]:
                lvl = int(sub_hru_level[sub_id][hid])
                low_mask[i] = (lvl <= flood_level)
            else:
                low_mask[i] = False

        idx_low = idx_sub[low_mask]
        if idx_low.size > 0:
            d = (h - dem_flat[idx_low]).astype(np.float32)
            d = np.maximum(d, 0.0)
            vv = dem_valid_flat[idx_low]
            out_flat[idx_low[vv]] = d[vv]

        idx_high = idx_sub[~low_mask]
        if idx_high.size > 0:
            vv = dem_valid_flat[idx_high]
            out_flat[idx_high[vv]] = 0.0

        print(
            "[SUB] sub={0} level={1} A_prev={2:.1f} A_cum={3:.1f} boundary_hid={4} depth={5:.4f}m h={6:.3f}m".format(
                sub_id, flood_level, A_prev, A_cum, boundary_hid, depth_hand_m, h
            )
        )

    # write
    profile.update(dtype=rasterio.float32, nodata=out_nodata, count=1, compress="lzw")
    with rasterio.open(out_depth_tif, "w", **profile) as dst:
        dst.write(out.astype(np.float32), 1)

    print("[OK] Downscaled depth written: {0}".format(out_depth_tif))


# ----------------------------
# Example usage
# ----------------------------
if __name__ == "__main__":

    DEM_TIF = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_raster\dem_1171_upstream.tif"
    HAND_WGS84_SHP = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\HRU_file\HRU_dissolved_upstream_1171.shp"
    HAND_EQUAL_SHP = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\HRU_file\HRU_mollwede_dissolved_upstream_1171.shp"

    FIELDID = "FIELDID"
    AREA_FIELD = "area"

    INDEX_DIR = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171\淹没范围绘图\downscale_index\hand_rank_index_txt"

    COARSE_HAND_DEPTH_TIF = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171\淹没范围绘图\cliped_sim_tif\OL_Hand_WTRDEP_TS_2014_10_03_000000.tif"
    OUT_DEPTH_TIF = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171\淹没范围绘图\downscale_tif\depth_downscaled.tif"

    SUBBASIN_SHP = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_shp\subbasin_1171_upstream.shp"

    level_csv = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\InundationMap.csv"
    level_txt = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\FloodStep.txt"

    # Step A: build index（只做一次）
    # ✅ 这里会把 prefix_sum 按“叠加上一层 SumArea”写入每个 HAND 的 cells.txt
    build_hand_dem_rank_index_all_txt(
        dem_tif_wgs84=DEM_TIF,
        hand_shp_wgs84=HAND_WGS84_SHP,
        field_id=FIELDID,
        out_dir=INDEX_DIR,
        hand_shp_equal_area=HAND_EQUAL_SHP,
        area_field=AREA_FIELD,
        level_csv_path=level_csv,
        level_txt_path=level_txt,
        all_touched=False,
    )

    # Step B: downscale（每个时刻的 coarse 深度图都可以跑一次）
    downscale_hand_depth_with_subbasin_layer_area(
        dem_tif_wgs84=DEM_TIF,
        hand_shp_wgs84=HAND_WGS84_SHP,
        hand_shp_equal_area=HAND_EQUAL_SHP,
        field_id=FIELDID,
        area_field=AREA_FIELD,
        index_dir=INDEX_DIR,
        coarse_hand_depth_tif=COARSE_HAND_DEPTH_TIF,
        out_depth_tif=OUT_DEPTH_TIF,
        subbasin_shp_wgs84=SUBBASIN_SHP,
        sub_id_field="SUBBASINID",
        level_csv_path=level_csv,
        level_txt_path=level_txt,
        depth_unit="m",
        depth_stat="max",
        all_touched=False,
        out_nodata=-9999.0,
        align_if_needed=True,
    )
