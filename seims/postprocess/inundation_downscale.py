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
from pathlib import Path
import re

def read_hand_index_txt_with_meta(index_dir, read_elevmin=True):
    """
    不再读取 hand_index.txt，改为根据目录中的 HAND_<hid>_cells.txt 文件名构建映射。

    返回：
      hid_to_file:   {hid: "HAND_<hid>_cells.txt"}
      hid_to_elevmin:{hid: elev_min}  # 如果 read_elevmin=False，则不填或填 None

    说明：
      - 文件名解析 hid：HAND_82_cells.txt -> 82
      - elev_min 从 cells 文件第一条数据行的第 3 列（elevation）读取
        （你的 cells 文件已按 elevation 升序写，所以第一条数据就是 elev_min）
    """
    index_dir = Path(index_dir)
    if not index_dir.exists():
        raise FileNotFoundError(f"index_dir not found: {index_dir}")

    pattern = re.compile(r"^HAND_(\d+)_cells\.txt$", re.IGNORECASE)

    hid_to_file = {}
    hid_to_elevmin = {}

    for p in index_dir.iterdir():
        if not p.is_file():
            continue

        m = pattern.match(p.name)
        if not m:
            continue

        hid = int(m.group(1))
        hid_to_file[hid] = p.name

        if read_elevmin:
            elev_min = None
            try:
                with open(p, "r", encoding="utf-8", errors="ignore") as f:
                    for line in f:
                        s = line.strip()
                        if (not s) or s.startswith("#"):
                            continue
                        parts = s.split()
                        # 0:rank 1:flat_index 2:elevation ...
                        if len(parts) >= 3:
                            elev_min = float(parts[2])
                        break
            except Exception:
                elev_min = None

            hid_to_elevmin[hid] = elev_min
        else:
            hid_to_elevmin[hid] = None

    if not hid_to_file:
        raise FileNotFoundError(
            f"No HAND_<hid>_cells.txt files found in: {index_dir}"
        )

    # 保持可预测顺序（可选）
    hid_to_file = dict(sorted(hid_to_file.items()))
    hid_to_elevmin = dict(sorted(hid_to_elevmin.items()))

    return hid_to_file, hid_to_elevmin




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



# =========================================================
# 8) build 预处理：每个 HAND 输出 txt
#    prefix_sum 改为“叠加上一层 SumArea 的常数项”，用于体积反演
# =========================================================

import pandas as pd
import geopandas as gpd
from rasterio.features import rasterize


def load_hand_area_map_attr_only(hand_shp_equal_area, field_id, area_field):
    """
    只读等面积 shp 的属性表，得到 {hid: area}。
    目标：尽最大努力不触发几何解析（避免坏几何导致 'Shell is not a LinearRing'）。
    """
    def _to_int(x):
        try:
            return int(float(x))
        except Exception:
            return None

    def _to_float(x):
        try:
            v = float(x)
            return v if np.isfinite(v) else None
        except Exception:
            return None

    # 方案 A：pyogrio（最快，且可 read_geometry=False）
    df = None
    try:
        import pyogrio
        df = pyogrio.read_dataframe(
            hand_shp_equal_area,
            columns=[field_id, area_field],
            read_geometry=False,
        )
    except Exception as e_pyogrio:
        # 方案 B：fiona 逐条读属性（不构建 shapely geometry，最稳）
        try:
            import fiona
            rows = []
            with fiona.open(hand_shp_equal_area, "r") as src:
                for feat in src:
                    props = feat.get("properties") or {}
                    rows.append({
                        field_id: props.get(field_id),
                        area_field: props.get(area_field),
                    })
            df = pd.DataFrame(rows)
        except Exception as e_fiona:
            # 方案 C：最后兜底 geopandas（可能会触发坏几何报错）
            import geopandas as gpd
            df = gpd.read_file(hand_shp_equal_area)[[field_id, area_field]].copy()

    if df is None or df.empty:
        raise ValueError(f"Failed to read attribute table from: {hand_shp_equal_area}")

    # 字段转换
    df[field_id] = df[field_id].apply(_to_int)
    df[area_field] = df[area_field].apply(_to_float)

    df = df[df[field_id].notnull()].copy()
    df = df[df[area_field].notnull()].copy()
    df[field_id] = df[field_id].astype(int)
    df[area_field] = df[area_field].astype(float)

    # 保留正面积
    df = df[df[area_field] > 0].copy()

    # 如果同一 hid 出现多行：取最大值（更稳）
    df = df.sort_values(area_field).drop_duplicates(subset=[field_id], keep="last")

    area_map = dict(zip(df[field_id].tolist(), df[area_field].tolist()))
    print(f"[AREA] area_map size={len(area_map)}")
    return area_map

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
    step_eps=1e-6,  # ✅ 台阶量化精度：建议按 DEM 垂直精度调整（例如 1e-3, 1e-2）
):
    """
    输出：
      out_dir/hand_index.txt
      out_dir/HAND_<hid>_cells.txt

    cell 文件列：
      # rank flat_index elevation prefix_sum lon lat
      注：lon/lat 实际为 DEM CRS 下的 x/y（与你原注释一致）

    ✅ 新定义：prefix_sum[i] 表示 “水位从 z0 抬升到 z[i] 时（第 i 个格子刚开始被淹），累计需要的总水量”
       - 0..i-1 号格子在该水位下完全淹没
       - i 号格子刚开始接触水（深度为 0）

    ✅ 同高程（含浮点误差）格子合并为同一个台阶：要淹就一起淹
       - zq = round(z/eps)*eps
       - 用 unique(zq) 得到台阶 z_u，台阶计数 cnt，逐栅格台阶索引 inv
       - 台阶 j 的“受水面积”（用于跨到下一个台阶）：
           A_u[j] = A_prev + cum_count[j] * cell_area
         其中 cum_count[j] = cnt[0] + ... + cnt[j]
       - 台阶体积增量：
           dV_u[j] = A_u[j] * (z_u[j+1] - z_u[j])
       - 台阶前缀：
           prefix_u[0]=0
           prefix_u[j] = sum_{k=0..j-1} dV_u[k]
       - 每个格子的 prefix_sum 通过 prefix_u[inv] 映射回去：同台阶相同 prefix
    """

    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 读分层累计面积表（你项目已有）
    sub_level_table = load_subbasin_level_table_csv(level_csv_path)
    sub_hru_level = load_sub_hru_level_map(level_txt_path)  # FloodStep.txt 映射

    # ✅ 只读等面积 shp 的 HAND 面积（m^2），完全信任 area_field
    area_map = load_hand_area_map_attr_only(hand_shp_equal_area, field_id, area_field)

    # 预构建 hid -> (sub_id, flood_level) 映射，避免每个 hid 都扫一遍字典
    hid_to_sub_level = {}
    for sid, hid_map in sub_hru_level.items():
        sid_int = int(sid)
        for hid, lvl in hid_map.items():
            hid_to_sub_level[int(hid)] = (sid_int, int(lvl))

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
        raise ValueError(f"hand_shp_wgs84 missing field: {field_id}")

    gdf = gdf[gdf.geometry.notnull()].copy()
    gdf[field_id] = gdf[field_id].apply(lambda x: int(float(x)) if pd.notnull(x) else None)
    gdf = gdf[gdf[field_id].notnull()].copy()
    gdf[field_id] = gdf[field_id].astype(int)

    # 投影到 DEM CRS
    gdf = gdf.to_crs(crs)

    # 修复 invalid geometry（不要直接 gdf[gdf.is_valid]）
    try:
        from shapely.validation import make_valid

        def _fix_geom(geom):
            if geom is None:
                return None
            if geom.is_valid:
                return geom
            try:
                return make_valid(geom)
            except Exception:
                try:
                    return geom.buffer(0)
                except Exception:
                    return None

        gdf["geometry"] = gdf.geometry.apply(_fix_geom)
    except Exception:
        # shapely<2.0 兜底
        def _fix_geom(geom):
            if geom is None:
                return None
            if geom.is_valid:
                return geom
            try:
                return geom.buffer(0)
            except Exception:
                return None

        gdf["geometry"] = gdf.geometry.apply(_fix_geom)

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
            print(f"[BUILD] HAND {hid} ...")

            mask = (id_flat == hid) & valid_flat
            if not np.any(mask):
                continue

            # 等面积 shp 中必须存在该 hid 的面积
            if hid not in area_map:
                print(f"  [SKIP] no area for HAND {hid} in equal-area shp")
                continue

            flat_idx = np.where(mask)[0].astype(np.int64)
            elev = dem_flat[flat_idx].astype(np.float64)

            order = np.argsort(elev, kind="mergesort")
            flat_idx_sorted = flat_idx[order]
            elev_sorted = elev[order]
            n = int(len(elev_sorted))
            if n <= 0:
                continue

            # 查该 HAND 对应的 (sub_id, flood_level) -> A_prev
            if hid not in hid_to_sub_level:
                print(f"  [SKIP] HAND {hid} not found in FloodStep.txt mapping")
                continue

            sub_id, flood_level = hid_to_sub_level[hid]

            if sub_id not in sub_level_table:
                print(f"  [SKIP] sub {sub_id} not found in InundationMap.csv")
                continue
            if flood_level not in sub_level_table[sub_id]:
                print(f"  [SKIP] level {flood_level} not found in InundationMap.csv for sub {sub_id}")
                continue

            A_prev = float(sub_level_table[sub_id][flood_level]["SumAreaPrev"])

            # cell_area：用等面积 shp 的总面积 / 像元数 得到平均格子面积
            area_hand = float(area_map[hid])
            cell_area = area_hand / float(n)

            # =========================
            # ✅ 台阶法：同高程（含浮点误差）归并为同一台阶
            # =========================
            z = elev_sorted.astype(np.float64)

            eps = float(step_eps)
            if eps <= 0:
                raise ValueError("step_eps must be > 0")

            zq = np.round(z / eps) * eps  # 量化后的“台阶高程”

            # 一次 unique 拿到：台阶高程 z_u、逐栅格台阶索引 inv、每台阶格子数 cnt
            z_u, inv, cnt = np.unique(zq, return_inverse=True, return_counts=True)
            n_steps = int(len(z_u))

            # cum_count：到台阶 j 为止累计格子数
            cum = np.cumsum(cnt).astype(np.float64)  # len=n_steps

            # 台阶间高差
            dz_u = z_u[1:] - z_u[:-1]                # len=n_steps-1

            # 台阶受水面积 A_u[j]（用于从 z_u[j] 抬升到 z_u[j+1]）
            A_u = A_prev + cum * cell_area           # len=n_steps

            # 台阶体积增量
            dV_u = A_u[:-1] * dz_u                   # len=n_steps-1

            # 台阶前缀
            prefix_u = np.zeros(n_steps, dtype=np.float64)
            if n_steps > 1:
                prefix_u[1:] = np.cumsum(dV_u, dtype=np.float64)

            # 映射回每个格子：同台阶同 prefix
            prefix = prefix_u[inv]                   # len=n

            # cell center (实际是 DEM CRS 的 x/y)
            rows = flat_idx_sorted // width
            cols = flat_idx_sorted % width
            xs, ys = rasterio.transform.xy(transform, rows, cols, offset="center")
            xs = np.asarray(xs, dtype=np.float64)
            ys = np.asarray(ys, dtype=np.float64)

            cell_file = f"HAND_{hid}_cells.txt"
            with open(str(out_dir / cell_file), "w", encoding="utf-8") as f:
                f.write("# rank  flat_index  elevation  prefix_sum  lon  lat\n")
                for i in range(n):
                    f.write(
                        f"{i} {int(flat_idx_sorted[i])} {float(zq[i]):.6f} {float(prefix[i]):.6f} "
                        f"{float(xs[i]):.8f} {float(ys[i]):.8f}\n"
                    )

            # elev_min/max：用台阶后的高程更一致（也可改回原 z[0], z[-1]）
            idx_f.write(
                f"{hid} {n} {float(z_u[0]):.6f} {float(z_u[-1]):.6f} {cell_file}\n"
            )

    print(f"[OK] TXT index written to: {str(out_dir)}")






# =========================================================
# 10) ✅ 主函数：子流域 + 边界 HAND + SumArea 累计面积体积
# =========================================================
from pathlib import Path
import numpy as np
import rasterio


def load_hand_cells_txt_with_prefix(cell_path):
    """
    读取 build_hand_dem_rank_index_all_txt 输出的 HAND_<hid>_cells.txt
    期望列：rank flat_index elevation prefix_sum lon lat
    返回：elev_sorted(n,), prefix_sorted(n,)
    """
    elev = []
    prefix = []
    with open(cell_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if (not line) or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 4:
                continue
            elev.append(float(parts[2]))
            prefix.append(float(parts[3]))
    if len(elev) == 0:
        return np.array([], dtype=np.float64), np.array([], dtype=np.float64)
    return np.asarray(elev, dtype=np.float64), np.asarray(prefix, dtype=np.float64)


def invert_water_level_with_prefix(
    elev_sorted,         # (n,) 升序
    prefix_sorted,       # (n,) 同台阶相同 prefix（累计体积到该台阶）
    cell_area,           # m^2
    A_prev,              # m^2
    V_target,            # m^3
):
    """
    用 cells.txt 里的 prefix_sum 语义直接反演水位 h。

    你的 prefix_sum 定义：
      prefix_sum 对应“水位抬升到该 elevation（该台阶刚开始接触水）时的累计体积”
      且构造 prefix 时已经包含了 A_prev 的“顶起”效应

    反演策略（台阶层面）：
      1) 把 elev_sorted 压缩为台阶 z_u，每台阶格子数 cnt
      2) prefix_u[j] = 该台阶的累计体积（取该台阶第一条的 prefix）
      3) A_u[j] = A_prev + cum_count[j]*cell_area
      4) 找到 j 使 prefix_u[j] <= V < prefix_u[j+1]
         h = z_u[j] + (V - prefix_u[j]) / A_u[j]

    返回：
      h, step_id
    """
    n = int(len(elev_sorted))
    if n == 0:
        return None, None

    if (not np.isfinite(V_target)) or V_target < 0:
        return None, None

    z = elev_sorted
    p = prefix_sorted

    # 压缩到台阶（你的 build 已把 elevation 写成量化后的 zq，因此 unique 是稳定的）
    z_u, first_idx, cnt = np.unique(z, return_index=True, return_counts=True)
    prefix_u = p[first_idx].astype(np.float64)  # 每台阶的累计体积
    n_steps = int(len(z_u))

    # cum_count：到台阶 j 为止累计格子数
    cum = np.cumsum(cnt).astype(np.float64)

    # 台阶受水面积（用于从 z_u[j] 抬升到下一个台阶）
    A_u = A_prev + cum * cell_area  # len=n_steps

    # V 在第一个台阶之前
    if V_target <= prefix_u[0]:
        return float(z_u[0]), 0

    # 找到 j：prefix_u[j] <= V < prefix_u[j+1]
    j = int(np.searchsorted(prefix_u, V_target, side="right") - 1)
    if j < 0:
        j = 0

    # 超过最后一个台阶：允许全淹后继续加深
    if j >= n_steps - 1:
        extra = float(V_target - prefix_u[-1])
        denom = float(A_u[-1])
        if denom <= 0:
            return None, None
        h = float(z_u[-1] + extra / denom)
        return h, n_steps - 1

    extra = float(V_target - prefix_u[j])
    denom = float(A_u[j])
    if denom <= 0:
        return None, None

    h = float(z_u[j] + extra / denom)

    # 保险：不超过下一台阶太多（浮点误差）
    z_next = float(z_u[j + 1])
    if h > z_next:
        h = z_next

    return h, j


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
    """
    ✅ 已适配 build_hand_dem_rank_index_all_txt 的 cells.txt 输出含义：
    - 直接读取 HAND_<hid>_cells.txt 的 elevation + prefix_sum
    - 用 prefix_sum 反演水位 h（台阶累计体积语义）
    """

    # ---- load tables ----
    sub_level_table = load_subbasin_level_table_csv(level_csv_path)
    sub_hru_level = load_sub_hru_level_map(level_txt_path)  # sub -> {hid: flood_level}

    # ⚠️ 你之前说面积字段已算好：建议这里用“只读属性表”的版本更快更稳
    area_map = load_hand_area_map_attr_only(hand_shp_equal_area, field_id, area_field)

    hid_to_file, hid_to_elevmin = read_hand_index_txt_with_meta(index_dir)  # elevmin 可不用，但保留返回
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
    out[dem_valid] = 0.0  # 默认：DEM 有效像元都“未淹=0”

    # ---- unit conversion ----
    if depth_unit not in ("m", "mm"):
        raise ValueError("depth_unit must be 'm' or 'mm'")
    depth_scale = 1.0 if depth_unit == "m" else 0.001  # mm -> m

    # ---- flatten ----
    dem_flat = dem.ravel()
    dem_valid_flat = dem_valid.ravel()
    dep_flat = coarse.ravel()
    dep_valid_flat = dep_valid.ravel()
    hand_flat = hand_id_raster.ravel()
    sub_flat = sub_id_raster.ravel()
    out_flat = out.ravel()

    # ---- subbasin list ----
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

        # 候选 HAND：子流域内 depth>0 的 HAND
        cand_hids = np.unique(hand_flat[m_wet])
        cand_hids = cand_hids[cand_hids != 0]
        if cand_hids.size == 0:
            continue

        # 过滤没 index/area 的
        valid_cands = []
        for hid in cand_hids:
            hid = int(hid)
            if (hid in hid_to_file) and (hid in area_map):
                valid_cands.append(hid)
        if not valid_cands:
            continue

        # 子流域的层级表（图1 InundationMap.csv）
        if sub_id not in sub_level_table:
            continue
        sub_levels = sub_level_table[sub_id]  # {level: {...}}

        # FloodStep.txt：sub -> {hid: flood_level}
        if sub_id not in sub_hru_level:
            continue
        sub_map = sub_hru_level[sub_id]

        # 在 valid_cands 中找 flood_level 最大的 boundary_hid
        max_level = None
        boundary_hid = None
        for hid in valid_cands:
            if hid in sub_map:
                lvl = int(sub_map[hid])
                if (max_level is None) or (lvl > max_level):
                    max_level = lvl
                    boundary_hid = hid

        if boundary_hid is None:
            continue

        flood_level = int(max_level)

        # Flood_Level 必须在 InundationMap.csv 里
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
        # -----------------------
        V_target = depth_hand_m * A_cum  # m^3

        # -----------------------
        # 4) ✅ 用 cells.txt 的 prefix_sum（累计体积）反演水位 h
        # -----------------------
        cell_file = hid_to_file[boundary_hid]
        cell_path = index_dir / cell_file
        if not cell_path.exists():
            continue

        elev_sorted, prefix_sorted = load_hand_cells_txt_with_prefix(cell_path)
        n = int(len(elev_sorted))
        if n == 0:
            continue

        # cell_area：boundary HAND 的平均像元面积
        area_hand = float(area_map[boundary_hid])
        cell_area = area_hand / float(n)

        # 可选 sanity check：prefix[0] 应该是 0
        # if abs(prefix_sorted[0]) > 1e-6:
        #     print(f"[WARN] prefix[0]!=0 for hid={boundary_hid}, got {prefix_sorted[0]}")

        h, _step_id = invert_water_level_with_prefix(
            elev_sorted=elev_sorted,
            prefix_sorted=prefix_sorted,
            cell_area=cell_area,
            A_prev=A_prev,
            V_target=V_target,
        )
        if h is None:
            continue

        # -----------------------
        # 5) 写出：子流域内 <= 当前 Flood_Level 的 HAND 都用同一水位 h
        # -----------------------
        idx_sub = np.where(m_sub)[0]
        sub_hand_ids = hand_flat[idx_sub]

        low_mask = np.zeros(sub_hand_ids.size, dtype=np.bool_)
        # 这里 sub_map 已经是 sub_id 对应的 {hid: level}
        for i in range(sub_hand_ids.size):
            hid = int(sub_hand_ids[i])
            if hid == 0:
                low_mask[i] = False
                continue
            if hid in sub_map:
                lvl = int(sub_map[hid])
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

    # ---- write ----
    profile.update(dtype=rasterio.float32, nodata=out_nodata, count=1, compress="lzw")
    Path(out_depth_tif).parent.mkdir(parents=True, exist_ok=True)
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
