import math
from collections import defaultdict

import numpy as np
import rasterio
from pymongo import MongoClient

# ================= 配置区域 =================
SUBBASIN_TIF = r"D:\SEIMS_LP\data\SLC_648\workspace\spatial_raster\subbasin.tif"
DEM_TIF = r"D:\SEIMS_LP\data\SLC_648\workspace\spatial_raster\dem.tif"
STREAM_TIF = r"D:\SEIMS_LP\data\SLC_648\workspace\spatial_raster\stream_link.tif"
HRU_TIF = r"D:\SEIMS_LP\data\SLC_648\workspace\rundata\HRU.tif"
HAND_TIF = r"D:\SEIMS_LP\data\SLC_648\workspace\rundata\hand.tif"

MONGO_URI = "mongodb://localhost:27017/"
DB_NAME = "SLC_648_longterm_model"
COLLECTION_NAME = "REACHES"

WRITE_TO_DB = True

# 上游 CH_BED_ELEV 必须严格大于下游
BED_ELEV_EPS = 0.001

# 出口河网像元周边搜索 HAND 带的半径（像元）
SEARCH_RADII = [1, 2, 4, 6]

# 是否要求出口对应 HAND 带必须在同一子流域内
REQUIRE_SAME_SUBBASIN_FOR_HAND = True
# ============================================


def is_valid_value(val, nodata):
    if np.isnan(val):
        return False
    if nodata is None:
        return True
    if np.isnan(nodata):
        return not np.isnan(val)
    return val != nodata


def open_rasters():
    with rasterio.open(SUBBASIN_TIF) as ds_sub, \
         rasterio.open(DEM_TIF) as ds_dem, \
         rasterio.open(STREAM_TIF) as ds_str, \
         rasterio.open(HRU_TIF) as ds_hru, \
         rasterio.open(HAND_TIF) as ds_hand:

        sub_arr = ds_sub.read(1)
        dem_arr = ds_dem.read(1)
        str_arr = ds_str.read(1)
        hru_arr = ds_hru.read(1)
        hand_arr = ds_hand.read(1)

        shape = sub_arr.shape
        if dem_arr.shape != shape or str_arr.shape != shape or hru_arr.shape != shape or hand_arr.shape != shape:
            raise ValueError("所有栅格尺寸必须一致。")

        transform = ds_dem.transform
        crs = ds_dem.crs

        meta = {
            "sub_nodata": ds_sub.nodata,
            "dem_nodata": ds_dem.nodata,
            "str_nodata": ds_str.nodata,
            "hru_nodata": ds_hru.nodata,
            "hand_nodata": ds_hand.nodata,
            "transform": transform,
            "crs": crs,
        }

    return sub_arr, dem_arr, str_arr, hru_arr, hand_arr, meta


def rc_to_xy(transform, row, col):
    x, y = rasterio.transform.xy(transform, row, col, offset="center")
    return float(x), float(y)


def build_docs():
    client = MongoClient(MONGO_URI)
    db = client[DB_NAME]
    col = db[COLLECTION_NAME]

    docs = list(col.find(
        {},
        {
            "_id": 1,
            "SUBBASINID": 1,
            "CH_DEPTH": 1,
            "DOWNSTREAM": 1,
            "DOWN_UP_ORDER": 1,
        }
    ))

    data_map = {}
    for d in docs:
        sid = int(d["SUBBASINID"])
        ch_depth = float(d.get("CH_DEPTH", 0.0) or 0.0)
        data_map[sid] = {
            "_id": d["_id"],
            "sid": sid,
            "dsid": int(d.get("DOWNSTREAM", -1) or -1),
            "order": int(d.get("DOWN_UP_ORDER", 0) or 0),
            "ch_depth": ch_depth,
        }

    return client, col, data_map


def get_neighbors8(row, col, nrows, ncols):
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            if dr == 0 and dc == 0:
                continue
            rr = row + dr
            cc = col + dc
            if 0 <= rr < nrows and 0 <= cc < ncols:
                yield rr, cc


def find_boundary_outlet_candidates(sid, dsid, sub_arr, dem_arr, str_arr, meta):
    nrows, ncols = sub_arr.shape
    dem_nodata = meta["dem_nodata"]
    str_nodata = meta["str_nodata"]

    sid_mask = (sub_arr == sid)
    if not np.any(sid_mask):
        return []

    candidates = []
    rows, cols = np.where(sid_mask & (str_arr > 0))

    for row, col in zip(rows, cols):
        dem_val = dem_arr[row, col]
        str_val = str_arr[row, col]
        if not is_valid_value(dem_val, dem_nodata):
            continue
        if not is_valid_value(str_val, str_nodata) or str_val <= 0:
            continue

        has_boundary = False

        # 优先：与下游子流域直接接壤
        if dsid > 0:
            for rr, cc in get_neighbors8(row, col, nrows, ncols):
                if sub_arr[rr, cc] == dsid:
                    has_boundary = True
                    break
        else:
            # 出口子流域：只要贴着本流域外边界即可
            for rr, cc in get_neighbors8(row, col, nrows, ncols):
                nbr_sid = sub_arr[rr, cc]
                if nbr_sid != sid:
                    has_boundary = True
                    break

        if has_boundary:
            candidates.append((float(dem_val), int(row), int(col)))

    candidates.sort(key=lambda x: (x[0], x[1], x[2]))
    return candidates


def find_lowest_stream_pixel_in_subbasin(sid, sub_arr, dem_arr, str_arr, meta):
    dem_nodata = meta["dem_nodata"]
    str_nodata = meta["str_nodata"]

    mask = (sub_arr == sid) & (str_arr > 0)
    rows, cols = np.where(mask)

    best = None
    for row, col in zip(rows, cols):
        dem_val = dem_arr[row, col]
        str_val = str_arr[row, col]
        if not is_valid_value(dem_val, dem_nodata):
            continue
        if not is_valid_value(str_val, str_nodata) or str_val <= 0:
            continue
        item = (float(dem_val), int(row), int(col))
        if best is None or item < best:
            best = item

    return best


def find_outlet_control_pixel(sid, dsid, sub_arr, dem_arr, str_arr, meta):
    boundary_candidates = find_boundary_outlet_candidates(sid, dsid, sub_arr, dem_arr, str_arr, meta)
    if boundary_candidates:
        dem_val, row, col = boundary_candidates[0]
        return {
            "out_dem": dem_val,
            "row": row,
            "col": col,
            "status": "BOUNDARY_LOWEST_STREAM_CELL",
        }

    fallback = find_lowest_stream_pixel_in_subbasin(sid, sub_arr, dem_arr, str_arr, meta)
    if fallback is not None:
        dem_val, row, col = fallback
        return {
            "out_dem": dem_val,
            "row": row,
            "col": col,
            "status": "FALLBACK_LOWEST_STREAM_CELL",
        }

    return None


def find_outlet_handid_for_stream_pixel(
    sid,
    out_row,
    out_col,
    sub_arr,
    dem_arr,
    hru_arr,
    hand_arr,
    meta
):
    nrows, ncols = sub_arr.shape
    dem_nodata = meta["dem_nodata"]
    hru_nodata = meta["hru_nodata"]
    hand_nodata = meta["hand_nodata"]

    best = None
    best_status = "NO_VALID_HAND_NEAR_OUTLET"

    for radius in SEARCH_RADII:
        local_best = None

        r0 = max(0, out_row - radius)
        r1 = min(nrows - 1, out_row + radius)
        c0 = max(0, out_col - radius)
        c1 = min(ncols - 1, out_col + radius)

        for rr in range(r0, r1 + 1):
            for cc in range(c0, c1 + 1):
                if rr == out_row and cc == out_col:
                    continue

                # 只搜当前 radius 的外环，避免重复
                if max(abs(rr - out_row), abs(cc - out_col)) != radius:
                    continue

                if REQUIRE_SAME_SUBBASIN_FOR_HAND and sub_arr[rr, cc] != sid:
                    continue

                hand_val = hand_arr[rr, cc]
                hru_val = hru_arr[rr, cc]
                dem_val = dem_arr[rr, cc]

                if not is_valid_value(hand_val, hand_nodata):
                    continue
                if not is_valid_value(hru_val, hru_nodata):
                    continue
                if not is_valid_value(dem_val, dem_nodata):
                    continue

                dist2 = (rr - out_row) ** 2 + (cc - out_col) ** 2

                # 关键排序：HAND 最小优先，其次距离最近，再次 DEM 最低
                item = (
                    float(hand_val),
                    int(dist2),
                    float(dem_val),
                    int(rr),
                    int(cc),
                    int(hru_val),
                )

                if local_best is None or item < local_best:
                    local_best = item

        if local_best is not None:
            hand_val, dist2, dem_val, rr, cc, hru_val = local_best
            best = {
                "hand_val": hand_val,
                "hand_row": rr,
                "hand_col": cc,
                "hand_dem": dem_val,
                "hand_dist": math.sqrt(dist2),
                "hand_id": hru_val,
            }
            best_status = f"NEARBY_MIN_HAND_RADIUS_{radius}"
            break

    if best is None:
        return {
            "hand_id": -9999,
            "hand_val": -9999,
            "hand_row": -1,
            "hand_col": -1,
            "hand_dem": -9999,
            "hand_dist": -1,
            "hand_status": best_status,
        }

    best["hand_status"] = best_status
    return best


def enforce_upstream_bed_monotonicity(data_map):
    """
    统一约束：
    上游 CH_BED_ELEV 必须严格大于下游 CH_BED_ELEV
    不区分河流/湖泊，一律处理
    """
    # 按水流顺序，从下游到上游
    sorted_sids = sorted(data_map.keys(), key=lambda x: data_map[x]["order"])
    correction_count = 0

    for sid in sorted_sids:
        cur = data_map[sid]
        dsid = cur["dsid"]
        if dsid in data_map:
            ds_bed = data_map[dsid]["bed_final"]
            if cur["bed_final"] <= ds_bed:
                cur["bed_final"] = ds_bed + BED_ELEV_EPS
                correction_count += 1

    return correction_count


def preview_results(data_map, max_rows=120):
    print("\n6. 结果预览：")
    print("-" * 240)
    print(
        f"{'SID':<8} {'DSID':<8} {'ORDER':<8} {'CH_DEPTH':<10} "
        f"{'OUT_DEM':<12} {'HANDID':<10} {'HANDVAL':<10} "
        f"{'BED_INIT':<12} {'BED_FINAL':<12} "
        f"{'OUT_ROW':<8} {'OUT_COL':<8} {'HROW':<8} {'HCOL':<8} {'STATUS':<35}"
    )
    print("-" * 240)

    shown = 0
    for sid in sorted(data_map.keys()):
        d = data_map[sid]
        print(
            f"{sid:<8} {d['dsid']:<8} {d['order']:<8} {d['ch_depth']:<10.3f} "
            f"{d['out_dem']:<12.3f} {d['hand_id']:<10} {d['hand_val']:<10.3f} "
            f"{d['bed_init']:<12.3f} {d['bed_final']:<12.3f} "
            f"{d['row']:<8} {d['col']:<8} {d['hand_row']:<8} {d['hand_col']:<8} "
            f"{d['status']:<35}"
        )
        shown += 1
        if shown >= max_rows:
            remaining = len(data_map) - shown
            if remaining > 0:
                print(f"... 其余 {remaining} 条未显示")
            break

    print("-" * 240)


def update_to_mongo(col, data_map):
    updated = 0
    for sid, d in data_map.items():
        col.update_one(
            {"_id": d["_id"]},
            {
                "$set": {
                    "CH_OUTLET_HANDID": int(d["hand_id"]),
                    "CH_BED_ELEV": float(d["bed_final"]),
                }
            }
        )
        updated += 1
    print(f"\n已成功写库 {updated} 条记录。")


def main():
    print("1. 读取栅格 ...")
    sub_arr, dem_arr, str_arr, hru_arr, hand_arr, meta = open_rasters()

    print("2. 读取数据库 REACHES ...")
    client, col, data_map = build_docs()

    print("3. 查找每个子流域的出口控制像元 ...")
    success_count = 0

    for sid, d in data_map.items():
        outlet = find_outlet_control_pixel(
            sid=sid,
            dsid=d["dsid"],
            sub_arr=sub_arr,
            dem_arr=dem_arr,
            str_arr=str_arr,
            meta=meta,
        )

        if outlet is None:
            d["out_dem"] = -9999.0
            d["row"] = -1
            d["col"] = -1
            d["bed_init"] = -9999.0
            d["bed_final"] = -9999.0
            d["hand_id"] = -9999
            d["hand_val"] = -9999.0
            d["hand_row"] = -1
            d["hand_col"] = -1
            d["status"] = "NO_VALID_STREAM_PIXEL"
            continue

        success_count += 1
        d["out_dem"] = outlet["out_dem"]
        d["row"] = outlet["row"]
        d["col"] = outlet["col"]

        # 4. 在出口河网像元周围寻找同层最小 HAND 带
        hand_pick = find_outlet_handid_for_stream_pixel(
            sid=sid,
            out_row=d["row"],
            out_col=d["col"],
            sub_arr=sub_arr,
            dem_arr=dem_arr,
            hru_arr=hru_arr,
            hand_arr=hand_arr,
            meta=meta,
        )

        d["hand_id"] = hand_pick["hand_id"]
        d["hand_val"] = hand_pick["hand_val"]
        d["hand_row"] = hand_pick["hand_row"]
        d["hand_col"] = hand_pick["hand_col"]

        # 统一初始河床高程
        d["bed_init"] = d["out_dem"] - d["ch_depth"]
        d["bed_final"] = d["bed_init"]

        # 合并状态说明
        d["status"] = f"{outlet['status']}|{hand_pick['hand_status']}"

    print("4. 查找每个子流域的出口控制像元，并读取其 HAND 带 ...")

    print("5. 统一约束：上游 CH_BED_ELEV 必须严格大于下游 ...")
    correction_count = enforce_upstream_bed_monotonicity(data_map)
    print(f"  已上抬 {correction_count} 个上游子流域的 CH_BED_ELEV。")

    preview_results(data_map, max_rows=120)
    print(f"\n统计：成功识别出口的子流域 {success_count} 个 / DB 中总子流域 {len(data_map)} 个")

    if WRITE_TO_DB:
        confirm = input("\n确认写入数据库？(y/n): ").strip().lower()
        if confirm == "y":
            update_to_mongo(col, data_map)
        else:
            print("已取消写库。")
    else:
        print("\n当前 WRITE_TO_DB=False，仅预览，不写库。确认无误后改成 True 再运行。")

    client.close()


if __name__ == "__main__":
    main()