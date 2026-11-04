import csv
import math
import os
import re
from typing import Dict, List, Optional, Sequence, Tuple

import numpy as np
import rasterio
from pymongo import MongoClient, UpdateOne
from pymongo.errors import BulkWriteError


# ================= Configuration =================
SUBBASIN_TIF = r"D:\SEIMS_LP\data\CW_2\workspace\spatial_raster\subbasin.tif"
DEM_TIF = r"D:\SEIMS_LP\data\CW_2\workspace\spatial_raster\dem.tif"
STREAM_TIF = r"D:\SEIMS_LP\data\CW_2\workspace\spatial_raster\stream_link.tif"
ACC_TIF = r"D:\SEIMS_LP\data\CW_2\workspace\spatial_raster\acc.tif"
HRU_TIF = r"D:\SEIMS_LP\data\CW_2\workspace\rundata\HRU.tif"
HAND_TIF = r"D:\SEIMS_LP\data\CW_2\workspace\rundata\hand.tif"

FLOODSTEP_TXT = r"D:\SEIMS_LP\data\CW_2\workspace\rundata\FloodStep.txt"

MONGO_URI = "mongodb://localhost:27017/"
DB_NAME = "CW_2_longterm_model"
COLLECTION_NAME = "REACHES"

# 建议先设为 False，确认预览结果正确后再改回 True
WRITE_TO_DB = True
CREATE_INDEXES = True

BED_FIELD = "CH_BED_ELEV"
HANDID_FIELD = "CH_OUTLET_HANDID"
STREAM_RELIEF_FIELD = "CH_STREAM_MAX_RELIEF"

CELL_SIZE_M = 90.0

REQUIRE_SAME_SUBBASIN_FOR_OUTLET = True

SKIP_OUTLET_CELLS = 4
TARGET_LEN_FRAC = 0.30
MIN_CONTROL_LEN_M = 1500.0
MAX_CONTROL_LEN_M = 5000.0
TARGET_RELIEF_M = 10.0
MIN_CONTROL_CELLS = 12
MAX_CONTROL_CELLS = 60
REQUIRE_BOTH_LEN_AND_RELIEF = True

CTRL_BED_Q = 0.75
USE_ACC_WEIGHTED_STREAM_MEAN = False

OUTLET_HAND_SEARCH_RADII = (1, 2, 4, 6)
CONTROL_STRIP_HAND_SEARCH_RADIUS_CELLS = 2
REQUIRE_SAME_SUBBASIN_FOR_HAND = True

BED_ELEV_EPS = 0.001
MIN_BED_SLOPE = 1.0e-4

PREVIEW_ROWS = 120

DEBUG_SIDS = {255}
DEBUG_LOG_PATH = r"D:\SEIMS_LP\log\preprocess_reach_control_bed_hand_debug.txt"
DEBUG_MAX_PATH_CELLS = 80
DEBUG_MAX_HAND_CANDIDATES = 20

WRITE_ALL_REACH_LOG = True
ALL_REACH_LOG_PATH = r"D:\SEIMS_LP\log\preprocess_reach_control_bed_hand_all.csv"
ALL_REACH_SUMMARY_PATH = r"D:\SEIMS_LP\log\preprocess_reach_control_bed_hand_summary.txt"
LAKE_HAND_VALIDATION_PATH = r"D:\SEIMS_LP\log\preprocess_reach_control_bed_hand_lake_validation.csv"

# FloodStep 目标 HAND 区间：0–0.1 m
TARGET_FLOODSTEP_LOWER = 0.0
TARGET_FLOODSTEP_UPPER = 0.1
INTERVAL_TOL = 1.0e-9
# =================================================


Cell = Tuple[int, int]


def is_valid_value(val, nodata) -> bool:
    if np.isnan(val):
        return False
    if nodata is None:
        return True
    if np.isnan(nodata):
        return not np.isnan(val)
    return val != nodata


def get_neighbors8(row: int, col: int, nrows: int, ncols: int):
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            if dr == 0 and dc == 0:
                continue
            rr = row + dr
            cc = col + dc
            if 0 <= rr < nrows and 0 <= cc < ncols:
                yield rr, cc


def cell_size_from_transform(transform) -> float:
    return CELL_SIZE_M


def path_length_m(cells: Sequence[Cell], cell_size: float) -> float:
    if not cells:
        return 0.0
    if len(cells) == 1:
        return cell_size

    total = 0.0
    for (r0, c0), (r1, c1) in zip(cells[:-1], cells[1:]):
        if abs(r1 - r0) == 1 and abs(c1 - c0) == 1:
            total += cell_size * math.sqrt(2.0)
        else:
            total += cell_size
    return float(total)


def append_debug(debug_lines: List[str], line: str = ""):
    debug_lines.append(line)


def maybe_debug_sid(sid: int) -> bool:
    return sid in DEBUG_SIDS


def get_first_present(doc: Dict, keys: Sequence[str], default=None):
    for key in keys:
        if key in doc and doc[key] is not None:
            return doc[key]
    return default


def as_binary_flag(value) -> int:
    if value is None:
        return 0
    if isinstance(value, str):
        text = value.strip().lower()
        return 1 if text in {"1", "true", "yes", "y"} else 0
    try:
        return 1 if float(value) == 1.0 else 0
    except (TypeError, ValueError):
        return 0


def parse_hand_interval(interval_text: str) -> Optional[Tuple[float, float]]:
    """
    尽量稳健地解析 HAND_Threshold_Interval：
    支持类似：
      0.00,0.10
      0,0.1
      [0.00,0.10]
      (0.00, 0.10)
      0.00 - 0.10
      0.00~0.10
    """
    if interval_text is None:
        return None

    text = str(interval_text).strip()
    if not text:
        return None

    text = text.replace("，", ",")
    text = text.replace("～", "~")
    text = text.replace("—", "-")
    text = text.replace("–", "-")
    text = text.replace("[", "").replace("]", "")
    text = text.replace("(", "").replace(")", "")
    text = text.replace(" ", "")

    # 先尝试逗号分隔
    if "," in text:
        parts = text.split(",")
        if len(parts) == 2:
            try:
                return float(parts[0]), float(parts[1])
            except ValueError:
                pass

    # 再尝试 -, ~ 等
    parts = re.split(r"[-~]", text)
    parts = [p for p in parts if p != ""]
    if len(parts) == 2:
        try:
            return float(parts[0]), float(parts[1])
        except ValueError:
            pass

    return None


def load_lake_zero_to_point_one_handid_map(path: str) -> Dict[int, int]:
    """
    Read FloodStep.txt and map each Subbasin to the HRU_ID of the 0-0.1 m HAND band.
    这里改成按数值区间匹配，而不是依赖精确字符串 '0.00,0.10'。
    """
    mapping: Dict[int, int] = {}
    if not os.path.exists(path):
        raise FileNotFoundError(f"FloodStep file not found: {path}")

    with open(path, "r", encoding="utf-8-sig", newline="") as f:
        sample = f.read(4096)
        f.seek(0)
        dialect = csv.Sniffer().sniff(sample, delimiters="\t,")
        reader = csv.DictReader(f, dialect=dialect)

        required = {"Subbasin", "HAND_Threshold_Interval", "HRU_ID"}
        if reader.fieldnames is None or not required.issubset(set(reader.fieldnames)):
            raise ValueError(
                "FloodStep file must contain columns: Subbasin, HAND_Threshold_Interval, HRU_ID"
            )

        for row in reader:
            parsed = parse_hand_interval(row["HAND_Threshold_Interval"])
            if parsed is None:
                continue

            lower, upper = parsed
            if abs(lower - TARGET_FLOODSTEP_LOWER) > INTERVAL_TOL:
                continue
            if abs(upper - TARGET_FLOODSTEP_UPPER) > INTERVAL_TOL:
                continue

            sid = int(float(row["Subbasin"]))
            hand_id = int(float(row["HRU_ID"]))
            if sid not in mapping:
                mapping[sid] = hand_id

    return mapping


def build_lake_hand_pick_from_floodstep(hand_id: int) -> Dict:
    return {
        "hand_id": int(hand_id),
        "hand_val": 0.0,
        "hand_row": -1,
        "hand_col": -1,
        "hand_dem": float("nan"),
        "local_base": float("nan"),
        "hand_status": "FLOODSTEP_0_0P1M_HAND",
    }


def relief_group(relief_m: float) -> str:
    if not np.isfinite(relief_m):
        return "unknown"
    if relief_m < 10.0:
        return "<10m"
    if relief_m < 30.0:
        return "10-30m"
    if relief_m < 60.0:
        return "30-60m"
    return ">=60m"


def delta_group(delta_m: float) -> str:
    abs_delta = abs(delta_m)
    if abs_delta < 0.1:
        return "<0.1m"
    if abs_delta < 0.5:
        return "0.1-0.5m"
    if abs_delta < 2.0:
        return "0.5-2m"
    if abs_delta < 5.0:
        return "2-5m"
    return ">=5m"


def quantile_or_nan(values: Sequence[float], q: float) -> float:
    if not values:
        return float("nan")
    return float(np.quantile(np.asarray(values, dtype=float), q))


def write_all_reach_diagnostics(data_map: Dict[int, Dict]):
    rows = []
    for sid in sorted(data_map.keys()):
        d = data_map[sid]
        new_bed = d["bed_final"]
        old_bed = d.get("old_bed", float("nan"))
        bed_delta = new_bed - old_bed if np.isfinite(new_bed) and np.isfinite(old_bed) else float("nan")
        new_hand = int(d.get("hand_id", -9999))
        old_hand = int(d.get("old_hand_id", -9999))
        rows.append({
            "sid": sid,
            "dsid": d["dsid"],
            "order": d["order"],
            "ch_len_m": float(d.get("ch_len", 0.0) or 0.0),
            "ch_depth_m": float(d.get("ch_depth", 0.0) or 0.0),
            "old_bed": old_bed,
            "new_bed": new_bed,
            "bed_delta_m": bed_delta,
            "old_handid": old_hand,
            "new_handid": new_hand,
            "hand_changed": int(old_hand != new_hand),
            "stream_relief_m": float(d.get("stream_relief_m", float("nan"))),
            "relief_group": relief_group(float(d.get("stream_relief_m", float("nan")))),
            "delta_group": delta_group(bed_delta) if np.isfinite(bed_delta) else "unknown",
            "control_len_m": float(d.get("control_len_m", 0.0) or 0.0),
            "strip_n": int(d.get("strip_n", 0) or 0),
            "stop_reason": d.get("stop_reason", ""),
            "status": d.get("status", ""),
        })

    log_dir = os.path.dirname(ALL_REACH_LOG_PATH)
    if log_dir:
        os.makedirs(log_dir, exist_ok=True)

    with open(ALL_REACH_LOG_PATH, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "sid", "dsid", "order", "ch_len_m", "ch_depth_m",
                "old_bed", "new_bed", "bed_delta_m",
                "old_handid", "new_handid", "hand_changed",
                "stream_relief_m", "relief_group", "delta_group",
                "control_len_m", "strip_n", "stop_reason", "status",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    valid_deltas = [float(r["bed_delta_m"]) for r in rows if np.isfinite(r["bed_delta_m"])]
    abs_deltas = [abs(v) for v in valid_deltas]
    hand_changed_count = sum(int(r["hand_changed"]) for r in rows)

    summary_lines = []
    summary_lines.append("preprocess_reach_control_bed_hand basin summary")
    summary_lines.append(f"total_reaches={len(rows)}")
    summary_lines.append(f"valid_bed_deltas={len(valid_deltas)}")
    summary_lines.append(f"hand_changed_count={hand_changed_count}")
    if abs_deltas:
        summary_lines.append(f"abs_delta_mean={float(np.mean(abs_deltas)):.6f}")
        summary_lines.append(f"abs_delta_median={float(np.median(abs_deltas)):.6f}")
        summary_lines.append(f"abs_delta_p90={quantile_or_nan(abs_deltas, 0.90):.6f}")
        summary_lines.append(f"abs_delta_max={float(np.max(abs_deltas)):.6f}")
    summary_lines.append("")

    summary_lines.append("delta_group_counts:")
    for group in ["<0.1m", "0.1-0.5m", "0.5-2m", "2-5m", ">=5m"]:
        count = sum(1 for r in rows if r["delta_group"] == group)
        summary_lines.append(f"  {group}: {count}")
    summary_lines.append("")

    summary_lines.append("relief_group_summary:")
    for group in ["<10m", "10-30m", "30-60m", ">=60m", "unknown"]:
        group_rows = [r for r in rows if r["relief_group"] == group]
        if not group_rows:
            continue
        group_abs = [abs(float(r["bed_delta_m"])) for r in group_rows if np.isfinite(r["bed_delta_m"])]
        large_count = sum(1 for r in group_rows if np.isfinite(r["bed_delta_m"]) and abs(float(r["bed_delta_m"])) >= 2.0)
        hand_count = sum(int(r["hand_changed"]) for r in group_rows)
        summary_lines.append(
            f"  {group}: n={len(group_rows)}, "
            f"mean_abs_delta={float(np.mean(group_abs)) if group_abs else float('nan'):.6f}, "
            f"median_abs_delta={float(np.median(group_abs)) if group_abs else float('nan'):.6f}, "
            f"delta_ge_2m={large_count}, hand_changed={hand_count}"
        )
    summary_lines.append("")

    summary_lines.append("top20_abs_delta:")
    top_rows = sorted(
        [r for r in rows if np.isfinite(r["bed_delta_m"])],
        key=lambda x: abs(float(x["bed_delta_m"])),
        reverse=True,
    )[:20]
    for r in top_rows:
        summary_lines.append(
            f"  sid={r['sid']}, dsid={r['dsid']}, old_bed={float(r['old_bed']):.6f}, "
            f"new_bed={float(r['new_bed']):.6f}, delta={float(r['bed_delta_m']):.6f}, "
            f"relief={float(r['stream_relief_m']):.6f}, strip_n={r['strip_n']}, "
            f"control_len_m={float(r['control_len_m']):.3f}, stop_reason={r['stop_reason']}, "
            f"status={r['status']}"
        )

    summary_dir = os.path.dirname(ALL_REACH_SUMMARY_PATH)
    if summary_dir:
        os.makedirs(summary_dir, exist_ok=True)

    with open(ALL_REACH_SUMMARY_PATH, "w", encoding="utf-8") as f:
        f.write("\n".join(summary_lines))

    return {
        "rows": rows,
        "summary_lines": summary_lines,
        "valid_delta_count": len(valid_deltas),
        "hand_changed_count": hand_changed_count,
    }


def open_rasters():
    with rasterio.open(SUBBASIN_TIF) as ds_sub, \
         rasterio.open(DEM_TIF) as ds_dem, \
         rasterio.open(STREAM_TIF) as ds_str, \
         rasterio.open(ACC_TIF) as ds_acc, \
         rasterio.open(HRU_TIF) as ds_hru, \
         rasterio.open(HAND_TIF) as ds_hand:

        sub_arr = ds_sub.read(1)
        dem_arr = ds_dem.read(1)
        str_arr = ds_str.read(1)
        acc_arr = ds_acc.read(1)
        hru_arr = ds_hru.read(1)
        hand_arr = ds_hand.read(1)

        shape = sub_arr.shape
        for arr in (dem_arr, str_arr, acc_arr, hru_arr, hand_arr):
            if arr.shape != shape:
                raise ValueError("All rasters must have the same shape.")

        meta = {
            "sub_nodata": ds_sub.nodata,
            "dem_nodata": ds_dem.nodata,
            "str_nodata": ds_str.nodata,
            "acc_nodata": ds_acc.nodata,
            "hru_nodata": ds_hru.nodata,
            "hand_nodata": ds_hand.nodata,
            "transform": ds_dem.transform,
        }

    return sub_arr, dem_arr, str_arr, acc_arr, hru_arr, hand_arr, meta


def build_docs():
    client = MongoClient(MONGO_URI)
    collection = client[DB_NAME][COLLECTION_NAME]

    docs = list(collection.find(
        {},
        {
            "_id": 1,
            "SUBBASINID": 1,
            "DOWNSTREAM": 1,
            "DOWN_UP_ORDER": 1,
            "CH_DEPTH": 1,
            "CH_LEN": 1,
            "Is_Lake": 1,
            "Is_Res": 1,
            "ISLAKE": 1,
            "ISRES": 1,
            "is_lake": 1,
            "is_res": 1,
            BED_FIELD: 1,
            HANDID_FIELD: 1,
        }
    ))

    data_map: Dict[int, Dict] = {}
    for doc in docs:
        sid = int(doc["SUBBASINID"])
        is_lake = as_binary_flag(get_first_present(doc, ("Is_Lake", "ISLAKE", "is_lake"), 0))
        is_res = as_binary_flag(get_first_present(doc, ("Is_Res", "ISRES", "is_res"), 0))

        old_bed_raw = doc.get(BED_FIELD, None)
        old_hand_raw = doc.get(HANDID_FIELD, None)

        data_map[sid] = {
            "_id": doc["_id"],
            "sid": sid,
            "dsid": int(doc.get("DOWNSTREAM", -1) or -1),
            "order": int(doc.get("DOWN_UP_ORDER", 0) or 0),
            "ch_depth": float(doc.get("CH_DEPTH", 0.0) or 0.0),
            "ch_len": float(doc.get("CH_LEN", 0.0) or 0.0),
            "is_lake": is_lake,
            "is_res": is_res,
            "is_lake_like": int(is_lake == 1 or is_res == 1),
            "old_bed": float(old_bed_raw) if old_bed_raw is not None else float("nan"),
            "old_hand_id": int(old_hand_raw) if old_hand_raw is not None else -9999,
        }
    return client, collection, data_map


def is_stream_cell(sid: int, row: int, col: int, sub_arr, str_arr, meta) -> bool:
    str_nodata = meta["str_nodata"]
    if REQUIRE_SAME_SUBBASIN_FOR_OUTLET and sub_arr[row, col] != sid:
        return False
    stream_val = str_arr[row, col]
    return is_valid_value(stream_val, str_nodata) and stream_val > 0


def find_outlet_pixel(sid: int, dsid: int, sub_arr, dem_arr, str_arr, acc_arr, meta) -> Optional[Dict]:
    nrows, ncols = sub_arr.shape
    dem_nodata = meta["dem_nodata"]
    acc_nodata = meta["acc_nodata"]

    mask = (sub_arr == sid) & (str_arr > 0)
    rows, cols = np.where(mask)

    candidates = []
    for row, col in zip(rows, cols):
        dem_val = dem_arr[row, col]
        acc_val = acc_arr[row, col]
        if not is_valid_value(dem_val, dem_nodata):
            continue
        if not is_valid_value(acc_val, acc_nodata):
            continue

        touches_boundary = False
        for rr, cc in get_neighbors8(int(row), int(col), nrows, ncols):
            nbr_sid = int(sub_arr[rr, cc])
            if dsid > 0:
                if nbr_sid == dsid:
                    touches_boundary = True
                    break
            else:
                if nbr_sid != sid:
                    touches_boundary = True
                    break

        if touches_boundary:
            candidates.append({
                "row": int(row),
                "col": int(col),
                "dem": float(dem_val),
                "acc": float(acc_val),
            })

    if not candidates:
        return None

    best = sorted(candidates, key=lambda x: (-x["acc"], x["dem"], x["row"], x["col"]))[0]
    best["status"] = "BOUNDARY_MAXACC"
    return best


def find_lowest_boundary_outlet_pixel(sid: int, dsid: int, sub_arr, dem_arr, str_arr, acc_arr, meta) -> Optional[Dict]:
    nrows, ncols = sub_arr.shape
    dem_nodata = meta["dem_nodata"]
    acc_nodata = meta["acc_nodata"]

    mask = (sub_arr == sid) & (str_arr > 0)
    rows, cols = np.where(mask)

    candidates = []
    for row, col in zip(rows, cols):
        dem_val = dem_arr[row, col]
        acc_val = acc_arr[row, col]
        if not is_valid_value(dem_val, dem_nodata):
            continue
        if not is_valid_value(acc_val, acc_nodata):
            continue

        touches_boundary = False
        for rr, cc in get_neighbors8(int(row), int(col), nrows, ncols):
            nbr_sid = int(sub_arr[rr, cc])
            if dsid > 0:
                if nbr_sid == dsid:
                    touches_boundary = True
                    break
            else:
                if nbr_sid != sid:
                    touches_boundary = True
                    break

        if touches_boundary:
            candidates.append({
                "row": int(row),
                "col": int(col),
                "dem": float(dem_val),
                "acc": float(acc_val),
            })

    if not candidates:
        return None

    best = sorted(candidates, key=lambda x: (x["dem"], -x["acc"], x["row"], x["col"]))[0]
    best["status"] = "BOUNDARY_LOWEST_DEM"
    return best


def trace_unique_reach_upstream(
    sid: int,
    outlet_row: int,
    outlet_col: int,
    sub_arr,
    dem_arr,
    str_arr,
    acc_arr,
    meta,
) -> List[Cell]:
    nrows, ncols = sub_arr.shape
    acc_nodata = meta["acc_nodata"]
    dem_nodata = meta["dem_nodata"]

    path = [(outlet_row, outlet_col)]
    visited = {(outlet_row, outlet_col)}

    cur_row, cur_col = outlet_row, outlet_col
    cur_acc = float(acc_arr[cur_row, cur_col])

    while len(path) < MAX_CONTROL_CELLS + SKIP_OUTLET_CELLS + 20:
        upstream_candidates = []
        lateral_candidates = []

        for rr, cc in get_neighbors8(cur_row, cur_col, nrows, ncols):
            if (rr, cc) in visited:
                continue
            if sub_arr[rr, cc] != sid:
                continue
            if not is_stream_cell(sid, rr, cc, sub_arr, str_arr, meta):
                continue

            acc_val = acc_arr[rr, cc]
            dem_val = dem_arr[rr, cc]
            if not is_valid_value(acc_val, acc_nodata):
                continue
            if not is_valid_value(dem_val, dem_nodata):
                continue

            item = (float(acc_val), float(dem_val), int(rr), int(cc))
            if acc_val < cur_acc - 1.0e-9:
                upstream_candidates.append(item)
            else:
                lateral_candidates.append(item)

        if upstream_candidates:
            next_acc, _, rr, cc = max(upstream_candidates, key=lambda x: (x[0], -x[1]))
        elif lateral_candidates:
            next_acc, _, rr, cc = max(lateral_candidates, key=lambda x: (x[0], -x[1]))
        else:
            break

        path.append((rr, cc))
        visited.add((rr, cc))
        cur_row, cur_col = rr, cc
        cur_acc = float(next_acc)

    return path


def build_control_strip(
    full_path: Sequence[Cell],
    ch_len_m: float,
    cell_size: float,
    dem_arr,
) -> Tuple[List[Cell], Dict]:
    if not full_path:
        return [], {
            "target_len_m": 0.0,
            "len_m": 0.0,
            "relief_m": 0.0,
            "stop_reason": "EMPTY_PATH",
            "stripped_n": 0,
        }

    stripped = list(full_path[SKIP_OUTLET_CELLS:]) if len(full_path) > SKIP_OUTLET_CELLS else list(full_path)
    if not stripped:
        stripped = list(full_path)

    target_len = ch_len_m * TARGET_LEN_FRAC if ch_len_m > 0 else 0.0
    target_len = max(target_len, MIN_CONTROL_LEN_M)
    target_len = min(target_len, MAX_CONTROL_LEN_M)

    selected: List[Cell] = []
    len_m = 0.0
    relief_m = 0.0
    first_dem = float(dem_arr[stripped[0][0], stripped[0][1]])
    stop_reason = "END_OF_PATH"

    for idx, cell in enumerate(stripped):
        selected.append(cell)
        if idx > 0:
            len_m = path_length_m(selected, cell_size)

        cur_dem = float(dem_arr[cell[0], cell[1]])
        relief_m = max(relief_m, abs(cur_dem - first_dem))

        enough_cells = len(selected) >= MIN_CONTROL_CELLS
        enough_len = len_m >= target_len
        enough_relief = relief_m >= TARGET_RELIEF_M
        enough_to_stop = (enough_len and enough_relief) if REQUIRE_BOTH_LEN_AND_RELIEF else (enough_len or enough_relief)
        too_many = len(selected) >= MAX_CONTROL_CELLS

        if too_many:
            stop_reason = "MAX_CONTROL_CELLS"
            break

        if enough_cells and enough_to_stop:
            if enough_len and enough_relief:
                stop_reason = "TARGET_LEN_AND_RELIEF"
            elif enough_len:
                stop_reason = "TARGET_LEN"
            else:
                stop_reason = "TARGET_RELIEF"
            break

    return selected, {
        "target_len_m": float(target_len),
        "len_m": float(len_m),
        "relief_m": float(relief_m),
        "stop_reason": stop_reason,
        "stripped_n": len(stripped),
    }


def compute_stream_relief(sid: int, sub_arr, dem_arr, str_arr, meta) -> float:
    dem_nodata = meta["dem_nodata"]
    str_nodata = meta["str_nodata"]

    mask = (sub_arr == sid) & (str_arr > 0)
    rows, cols = np.where(mask)
    if rows.size == 0:
        return float("nan")

    vals = []
    for row, col in zip(rows, cols):
        stream_val = str_arr[row, col]
        dem_val = dem_arr[row, col]
        if not is_valid_value(stream_val, str_nodata) or stream_val <= 0:
            continue
        if not is_valid_value(dem_val, dem_nodata):
            continue
        vals.append(float(dem_val))

    if not vals:
        return float("nan")
    return float(max(vals) - min(vals))


def representative_bed_from_stream_network(
    sid: int,
    sub_arr,
    dem_arr,
    str_arr,
    acc_arr,
    meta,
    ch_depth: float,
) -> Tuple[Optional[float], Dict]:
    dem_nodata = meta["dem_nodata"]
    str_nodata = meta["str_nodata"]
    acc_nodata = meta["acc_nodata"]

    mask = (sub_arr == sid) & (str_arr > 0)
    rows, cols = np.where(mask)
    if rows.size == 0:
        return None, {
            "values": [],
            "dem_values": [],
            "weights": [],
            "acc_values": [],
            "method": "STREAM_MEAN",
        }

    values = []
    weights = []
    dem_values = []
    acc_values = []

    for row, col in zip(rows, cols):
        stream_val = str_arr[row, col]
        dem_val = dem_arr[row, col]
        acc_val = acc_arr[row, col]

        if not is_valid_value(stream_val, str_nodata) or stream_val <= 0:
            continue
        if not is_valid_value(dem_val, dem_nodata):
            continue

        dem_f = float(dem_val)
        dem_values.append(dem_f)
        values.append(dem_f - ch_depth)

        if USE_ACC_WEIGHTED_STREAM_MEAN and is_valid_value(acc_val, acc_nodata):
            weight = max(math.log1p(max(float(acc_val), 0.0)), 1.0e-6)
        else:
            weight = 1.0

        weights.append(weight)
        acc_values.append(float(acc_val) if is_valid_value(acc_val, acc_nodata) else float("nan"))

    if not values:
        return None, {
            "values": [],
            "dem_values": [],
            "weights": [],
            "acc_values": [],
            "method": "STREAM_MEAN",
        }

    bed = float(np.average(np.asarray(values, dtype=float), weights=np.asarray(weights, dtype=float)))
    return bed, {
        "values": values,
        "dem_values": dem_values,
        "weights": weights,
        "acc_values": acc_values,
        "method": "ACC_WEIGHTED_STREAM_MEAN" if USE_ACC_WEIGHTED_STREAM_MEAN else "SIMPLE_STREAM_MEAN",
    }


def find_first_hand_band_candidate(
    sid: int,
    sub_arr,
    dem_arr,
    hru_arr,
    hand_arr,
    meta,
) -> Tuple[Dict, List[Dict]]:
    hand_nodata = meta["hand_nodata"]
    hru_nodata = meta["hru_nodata"]
    dem_nodata = meta["dem_nodata"]

    mask = (sub_arr == sid)
    rows, cols = np.where(mask)

    candidates: List[Dict] = []
    for row, col in zip(rows, cols):
        hand_val = hand_arr[row, col]
        if not is_valid_value(hand_val, hand_nodata):
            continue

        hru_val = hru_arr[row, col]
        dem_val = dem_arr[row, col]
        if not is_valid_value(hru_val, hru_nodata):
            continue
        if not is_valid_value(dem_val, dem_nodata):
            continue

        candidates.append({
            "hand_val": float(hand_val),
            "hand_id": int(hru_val),
            "row": int(row),
            "col": int(col),
            "dem": float(dem_val),
            "local_base": float(dem_val) - float(hand_val),
            # 给 debug 统一字段，避免 KeyError
            "radius": 0,
            "dist_cells": 0.0,
        })

    if not candidates:
        return {
            "hand_id": -9999,
            "hand_val": -9999.0,
            "hand_row": -1,
            "hand_col": -1,
            "hand_dem": -9999.0,
            "local_base": float("nan"),
            "hand_status": "NO_HAND_BAND",
        }, []

    candidates.sort(key=lambda x: (x["hand_val"], x["dem"], x["hand_id"], x["row"], x["col"]))
    best_candidate = candidates[0]

    return {
        "hand_id": best_candidate["hand_id"],
        "hand_val": best_candidate["hand_val"],
        "hand_row": best_candidate["row"],
        "hand_col": best_candidate["col"],
        "hand_dem": best_candidate["dem"],
        "local_base": best_candidate["local_base"],
        "hand_status": "FIRST_HAND_BAND",
    }, candidates


def enforce_min_bed_slope(data_map: Dict[int, Dict], cell_size: float) -> int:
    sorted_sids = sorted(data_map.keys(), key=lambda x: data_map[x]["order"])
    correction_count = 0

    for sid in sorted_sids:
        cur = data_map[sid]
        if int(cur.get("is_lake_like", 0)) == 1:
            continue

        dsid = cur["dsid"]
        if dsid not in data_map:
            continue
        if not np.isfinite(cur["bed_final"]) or not np.isfinite(data_map[dsid]["bed_final"]):
            continue

        ch_len = cur["ch_len"] if cur["ch_len"] > 0 else cur["control_len_m"]
        ch_len = max(ch_len, cell_size)
        min_drop = max(BED_ELEV_EPS, MIN_BED_SLOPE * ch_len)
        min_up_bed = data_map[dsid]["bed_final"] + min_drop

        if cur["bed_final"] <= min_up_bed:
            cur["bed_final"] = min_up_bed
            correction_count += 1

    return correction_count


def write_lake_handid_validation(data_map: Dict[int, Dict], path: str):
    rows = []
    for sid in sorted(data_map.keys()):
        d = data_map[sid]
        if int(d.get("is_lake_like", 0)) != 1:
            continue

        rows.append({
            "sid": sid,
            "old_handid": int(d.get("old_hand_id", -9999)),
            "new_handid": int(d.get("hand_id", -9999)),
            "bed_init": float(d.get("bed_init", float("nan"))),
            "bed_final": float(d.get("bed_final", float("nan"))),
            "status": d.get("status", ""),
        })

    if not rows:
        return None

    out_dir = os.path.dirname(path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["sid", "old_handid", "new_handid", "bed_init", "bed_final", "status"])
        writer.writeheader()
        writer.writerows(rows)

    return path


def preview_results(data_map: Dict[int, Dict], max_rows: int = PREVIEW_ROWS):
    print("\nPreview:")
    print("-" * 220)
    print(
        f"{'SID':<8} {'DSID':<8} {'ORDER':<8} {'DEPTH':<9} {'OUT_ACC':<12} "
        f"{'BED_INIT':<12} {'BED_FINAL':<12} {'HANDID':<10} {'STREAMREL':<12} "
        f"{'CTRL_LEN_M':<12} {'STRIP_N':<8} {'STATUS':<50}"
    )
    print("-" * 220)

    shown = 0
    for sid in sorted(data_map.keys()):
        d = data_map[sid]
        stream_relief = d["stream_relief_m"] if np.isfinite(d["stream_relief_m"]) else -9999.0
        out_acc = d["out_acc"] if np.isfinite(d["out_acc"]) else -9999.0
        print(
            f"{sid:<8} {d['dsid']:<8} {d['order']:<8} {d['ch_depth']:<9.3f} "
            f"{out_acc:<12.1f} {d['bed_init']:<12.3f} {d['bed_final']:<12.3f} "
            f"{d['hand_id']:<10} {stream_relief:<12.3f} {d['control_len_m']:<12.1f} "
            f"{d['strip_n']:<8} {d['status']:<50}"
        )
        shown += 1
        if shown >= max_rows:
            remaining = len(data_map) - shown
            if remaining > 0:
                print(f"... {remaining} more rows not shown")
            break
    print("-" * 220)


def update_to_mongo(collection, data_map: Dict[int, Dict]):
    ops = []
    for _, d in data_map.items():
        if not np.isfinite(d["bed_final"]):
            continue

        ops.append(
            UpdateOne(
                {"_id": d["_id"]},
                {"$set": {
                    BED_FIELD: float(d["bed_final"]),
                    HANDID_FIELD: int(d["hand_id"]),
                    STREAM_RELIEF_FIELD: float(d["stream_relief_m"]) if np.isfinite(d["stream_relief_m"]) else -9999.0,
                }}
            )
        )

    if not ops:
        print("No valid updates to write.")
        return

    try:
        result = collection.bulk_write(ops, ordered=False)
        print(
            f"MongoDB updated: matched={result.matched_count}, "
            f"modified={result.modified_count}, ops={len(ops)}"
        )
    except BulkWriteError as exc:
        print("Bulk write failed:")
        print(exc.details)


def main():
    print("1. Reading rasters ...")
    sub_arr, dem_arr, str_arr, acc_arr, hru_arr, hand_arr, meta = open_rasters()
    cell_size = cell_size_from_transform(meta["transform"])

    print("2. Reading MongoDB REACHES ...")
    client, collection, data_map = build_docs()

    print("2b. Reading FloodStep 0-0.1m HAND mapping ...")
    lake_zero_to_point_one_handid = load_lake_zero_to_point_one_handid_map(FLOODSTEP_TXT)
    print(f"   FloodStep 0-0.1m entries: {len(lake_zero_to_point_one_handid)}")
    print(f"   MongoDB target: {DB_NAME}.{COLLECTION_NAME}")
    print(f"   WRITE_TO_DB={WRITE_TO_DB}")

    if CREATE_INDEXES:
        collection.create_index("SUBBASINID")
        collection.create_index("DOWNSTREAM")
        collection.create_index("DOWN_UP_ORDER")

    debug_lines: List[str] = []
    if DEBUG_SIDS:
        append_debug(debug_lines, "preprocess_reach_control_bed_hand debug")
        append_debug(debug_lines, f"DEBUG_SIDS={sorted(DEBUG_SIDS)}")
        append_debug(debug_lines, f"CELL_SIZE_M={CELL_SIZE_M}")
        append_debug(debug_lines, f"SKIP_OUTLET_CELLS={SKIP_OUTLET_CELLS}")
        append_debug(debug_lines, f"TARGET_LEN_FRAC={TARGET_LEN_FRAC}")
        append_debug(debug_lines, f"MIN_CONTROL_LEN_M={MIN_CONTROL_LEN_M}")
        append_debug(debug_lines, f"MAX_CONTROL_LEN_M={MAX_CONTROL_LEN_M}")
        append_debug(debug_lines, f"TARGET_RELIEF_M={TARGET_RELIEF_M}")
        append_debug(debug_lines, f"REQUIRE_BOTH_LEN_AND_RELIEF={REQUIRE_BOTH_LEN_AND_RELIEF}")
        append_debug(debug_lines, f"CTRL_BED_Q={CTRL_BED_Q}")
        append_debug(debug_lines, f"USE_ACC_WEIGHTED_STREAM_MEAN={USE_ACC_WEIGHTED_STREAM_MEAN}")
        append_debug(debug_lines, f"WRITE_TO_DB={WRITE_TO_DB}")
        append_debug(debug_lines)

    print("3. Computing representative control bed and HANDID ...")
    success_count = 0

    for sid, d in data_map.items():
        d["stream_relief_m"] = compute_stream_relief(sid, sub_arr, dem_arr, str_arr, meta)

        if int(d.get("is_lake_like", 0)) == 1:
            outlet = find_lowest_boundary_outlet_pixel(sid, d["dsid"], sub_arr, dem_arr, str_arr, acc_arr, meta)
        else:
            outlet = find_outlet_pixel(sid, d["dsid"], sub_arr, dem_arr, str_arr, acc_arr, meta)

        if outlet is None:
            d["out_acc"] = float("nan")
            d["bed_init"] = float("nan")
            d["bed_final"] = float("nan")
            d["hand_id"] = -9999
            d["control_len_m"] = 0.0
            d["strip_n"] = 0
            d["stop_reason"] = "NO_VALID_OUTLET"
            d["status"] = "NO_VALID_OUTLET"
            if maybe_debug_sid(sid):
                append_debug(debug_lines, f"SID {sid}: NO_VALID_OUTLET")
                append_debug(debug_lines)
            continue

        success_count += 1
        d["out_acc"] = float(outlet["acc"])

        if int(d.get("is_lake_like", 0)) == 1:
            full_path = [(outlet["row"], outlet["col"])]
            control_strip = [(outlet["row"], outlet["col"])]
            strip_info = {
                "target_len_m": 0.0,
                "len_m": 0.0,
                "relief_m": 0.0,
                "stop_reason": "LAKE_OUTLET_ONLY",
                "stripped_n": 1,
            }
            d["strip_n"] = 1
            d["control_len_m"] = 0.0
            d["stop_reason"] = strip_info["stop_reason"]

            bed = float(outlet["dem"])
            bed_info = {
                "values": [float(outlet["dem"])],
                "dem_values": [float(outlet["dem"])],
                "quantile": float("nan"),
                "method": "LAKE_OUTLET_DEM",
            }
        else:
            full_path = trace_unique_reach_upstream(
                sid=sid,
                outlet_row=outlet["row"],
                outlet_col=outlet["col"],
                sub_arr=sub_arr,
                dem_arr=dem_arr,
                str_arr=str_arr,
                acc_arr=acc_arr,
                meta=meta,
            )
            control_strip, strip_info = build_control_strip(
                full_path=full_path,
                ch_len_m=d["ch_len"],
                cell_size=cell_size,
                dem_arr=dem_arr,
            )

            d["strip_n"] = len(control_strip)
            d["control_len_m"] = path_length_m(control_strip, cell_size)
            d["stop_reason"] = strip_info["stop_reason"]

            bed, bed_info = representative_bed_from_stream_network(
                sid=sid,
                sub_arr=sub_arr,
                dem_arr=dem_arr,
                str_arr=str_arr,
                acc_arr=acc_arr,
                meta=meta,
                ch_depth=max(d["ch_depth"], 0.1),
            )

        if bed is None or not np.isfinite(bed):
            d["bed_init"] = float("nan")
            d["bed_final"] = float("nan")
            d["hand_id"] = -9999
            d["stop_reason"] = strip_info["stop_reason"]
            d["status"] = "NO_VALID_CONTROL_STRIP"
            if maybe_debug_sid(sid):
                append_debug(debug_lines, f"SID {sid}: NO_VALID_CONTROL_STRIP")
                append_debug(debug_lines)
            continue

        if int(d.get("is_lake_like", 0)) == 1:
            floodstep_hand_id = lake_zero_to_point_one_handid.get(sid, -9999)
            if floodstep_hand_id >= 0:
                hand_pick = build_lake_hand_pick_from_floodstep(floodstep_hand_id)
                hand_candidates = []
            else:
                hand_pick = {
                    "hand_id": -9999,
                    "hand_val": -9999.0,
                    "hand_row": -1,
                    "hand_col": -1,
                    "hand_dem": float("nan"),
                    "local_base": float("nan"),
                    "hand_status": "NO_FLOODSTEP_0_0P1M_HAND",
                }
                hand_candidates = []

            bed = float(outlet["dem"])
            bed_info = {
                "values": [float(outlet["dem"])],
                "dem_values": [float(outlet["dem"])],
                "quantile": float("nan"),
                "method": "LAKE_OUTLET_DEM",
            }
        else:
            hand_pick, hand_candidates = find_first_hand_band_candidate(
                sid=sid,
                sub_arr=sub_arr,
                dem_arr=dem_arr,
                hru_arr=hru_arr,
                hand_arr=hand_arr,
                meta=meta,
            )

        d["bed_init"] = float(bed)
        d["bed_final"] = float(bed)
        d["hand_id"] = int(hand_pick["hand_id"])
        d["status"] = f"{outlet['status']}|{hand_pick['hand_status']}"

        if maybe_debug_sid(sid):
            append_debug(debug_lines, f"SID {sid}")
            append_debug(debug_lines, f"  dsid={d['dsid']}, order={d['order']}, ch_depth={d['ch_depth']:.6f}, ch_len={d['ch_len']:.3f}")
            append_debug(debug_lines, f"  old_bed={d.get('old_bed', float('nan')):.6f}, old_hand_id={d.get('old_hand_id', -9999)}")
            append_debug(debug_lines, f"  stream_relief_m={d['stream_relief_m']:.6f}")
            append_debug(debug_lines, f"  outlet: row={outlet['row']}, col={outlet['col']}, dem={outlet['dem']:.6f}, acc={outlet['acc']:.6f}, status={outlet['status']}")
            append_debug(debug_lines, f"  full_path_n={len(full_path)}")
            append_debug(
                debug_lines,
                f"  control_strip_n={len(control_strip)}, control_len_m={d['control_len_m']:.3f}, "
                f"target_len_m={strip_info['target_len_m']:.3f}, strip_relief_m={strip_info['relief_m']:.6f}, "
                f"stop_reason={strip_info['stop_reason']}, stripped_n={strip_info['stripped_n']}"
            )

            append_debug(debug_lines, "  full_path cells (row,col,dem,acc):")
            for idx, (row, pixel_col) in enumerate(full_path[:DEBUG_MAX_PATH_CELLS]):
                append_debug(
                    debug_lines,
                    f"    [{idx:02d}] ({row},{pixel_col}) dem={float(dem_arr[row, pixel_col]):.6f} "
                    f"acc={float(acc_arr[row, pixel_col]):.6f}"
                )
            if len(full_path) > DEBUG_MAX_PATH_CELLS:
                append_debug(debug_lines, f"    ... {len(full_path) - DEBUG_MAX_PATH_CELLS} more path cells")

            append_debug(debug_lines, "  control_strip values (row,col,dem,bed_candidate):")
            for idx, (row, pixel_col) in enumerate(control_strip):
                dem_val = float(dem_arr[row, pixel_col])
                append_debug(
                    debug_lines,
                    f"    [{idx:02d}] ({row},{pixel_col}) dem={dem_val:.6f} "
                    f"cand_bed={dem_val - max(d['ch_depth'], 0.1):.6f}"
                )

            sorted_candidates = sorted(bed_info.get("values", []))
            append_debug(debug_lines, f"  bed_method={bed_info.get('method', 'DIRECT')}")
            append_debug(debug_lines, f"  bed_candidates_sorted={','.join(f'{v:.6f}' for v in sorted_candidates)}")
            if "weights" in bed_info and bed_info.get("weights"):
                append_debug(debug_lines, f"  bed_weights={','.join(f'{w:.6f}' for w in bed_info['weights'])}")
            if np.isfinite(bed_info.get("quantile", float('nan'))):
                append_debug(debug_lines, f"  ctrl_bed_q={bed_info['quantile']:.3f}, bed_init={d['bed_init']:.6f}")
            else:
                append_debug(debug_lines, f"  direct_bed_pick={d['bed_init']:.6f}")

            append_debug(debug_lines, "  hand_candidates_top:")
            for cand in hand_candidates[:DEBUG_MAX_HAND_CANDIDATES]:
                append_debug(
                    debug_lines,
                    f"    hand_id={cand['hand_id']}, hand={cand['hand_val']:.6f}, "
                    f"radius={cand.get('radius', -1)}, dist_cells={cand.get('dist_cells', float('nan')):.3f}, "
                    f"dem={cand['dem']:.6f}, local_base={cand['local_base']:.6f}"
                )
            if len(hand_candidates) > DEBUG_MAX_HAND_CANDIDATES:
                append_debug(debug_lines, f"    ... {len(hand_candidates) - DEBUG_MAX_HAND_CANDIDATES} more hand candidates")

            append_debug(
                debug_lines,
                f"  selected_hand: hand_id={hand_pick['hand_id']}, hand={hand_pick['hand_val']:.6f}, "
                f"row={hand_pick['hand_row']}, col={hand_pick['hand_col']}, "
                f"dem={hand_pick['hand_dem']:.6f}, local_base={hand_pick['local_base']:.6f}"
            )
            append_debug(debug_lines)

    print("4. Enforcing a minimum upstream-to-downstream bed drop ...")
    correction_count = enforce_min_bed_slope(data_map, cell_size)
    print(f"   corrected reaches: {correction_count}")

    diagnostics = None
    if WRITE_ALL_REACH_LOG:
        diagnostics = write_all_reach_diagnostics(data_map)
        print(f"   all-reach log written to: {ALL_REACH_LOG_PATH}")
        print(f"   summary written to: {ALL_REACH_SUMMARY_PATH}")
        if diagnostics["valid_delta_count"] > 0:
            print(f"   valid bed deltas: {diagnostics['valid_delta_count']}")
            print(f"   hand changes: {diagnostics['hand_changed_count']}")

    if DEBUG_SIDS:
        append_debug(debug_lines, "Post-slope-correction summary:")
        for sid in sorted(DEBUG_SIDS):
            if sid not in data_map:
                continue
            d = data_map[sid]
            bed_delta = (
                d["bed_final"] - d.get("old_bed", float("nan"))
                if np.isfinite(d["bed_final"]) and np.isfinite(d.get("old_bed", float("nan")))
                else float("nan")
            )
            append_debug(
                debug_lines,
                f"  SID {sid}: old_bed={d.get('old_bed', float('nan')):.6f}, "
                f"bed_final={d['bed_final']:.6f}, bed_delta={bed_delta:.6f}, "
                f"old_hand_id={d.get('old_hand_id', -9999)}, hand_id={d['hand_id']}, status={d['status']}"
            )
        append_debug(debug_lines)

        debug_dir = os.path.dirname(DEBUG_LOG_PATH)
        if debug_dir:
            os.makedirs(debug_dir, exist_ok=True)
        with open(DEBUG_LOG_PATH, "w", encoding="utf-8") as f:
            f.write("\n".join(debug_lines))
        print(f"   debug log written to: {DEBUG_LOG_PATH}")

    lake_validation_path = write_lake_handid_validation(data_map, LAKE_HAND_VALIDATION_PATH)
    if lake_validation_path:
        print(f"   lake HAND validation written to: {lake_validation_path}")

    preview_results(data_map)
    print(f"\nSuccessful outlets: {success_count} / {len(data_map)}")

    pending_updates = sum(1 for d in data_map.values() if np.isfinite(d["bed_final"]))
    print(f"Pending MongoDB updates: {pending_updates}")

    if WRITE_TO_DB:
        confirm = input("\nWrite updates to MongoDB? (y/n): ").strip().lower()
        if confirm == "y":
            update_to_mongo(collection, data_map)
        else:
            print("Cancelled.")
    else:
        print("\nNo MongoDB write because WRITE_TO_DB=False.")

    client.close()


if __name__ == "__main__":
    main()