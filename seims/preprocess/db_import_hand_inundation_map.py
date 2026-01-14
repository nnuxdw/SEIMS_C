# -*- coding: utf-8 -*-

from pymongo import MongoClient
import gridfs
import ast
from struct import pack
import pandas as pd


def import_array_to_mongodb(gfs, array, fname):
    fname = fname.upper()
    if gfs.exists(filename=fname):
        x = gfs.get_version(filename=fname)
        gfs.delete(x._id)

    rows = len(array)
    cols = len(array[0])

    meta_dict = dict()
    if 'WEIGHT' in fname:
        meta_dict['NUM_SITES'] = rows
        meta_dict['NUM_CELLS'] = cols
        meta_dict['SUBBASIN'] = 0
    else:
        meta_dict['TYPE'] = fname
        meta_dict['ID'] = fname
        meta_dict['DESCRIPTION'] = fname
        meta_dict['SUBBASIN'] = 0
        meta_dict['CELLSIZE'] = 1
        meta_dict['NODATA_VALUE'] = -9999
        meta_dict['NCOLS'] = cols
        meta_dict['NROWS'] = 1
        meta_dict['XLLCENTER'] = 0
        meta_dict['YLLCENTER'] = 0
        meta_dict['LAYERS'] = rows
        meta_dict['CELLSNUM'] = cols
        meta_dict['SRS'] = ''

    myfile = gfs.new_file(filename=fname, metadata=meta_dict)
    for j in range(0, cols):
        cur_col = []
        for i in range(0, rows):
            cur_col.append(float(array[i][j]))
        fmt = '%df' % rows
        myfile.write(pack(fmt, *cur_col))
    myfile.close()
    print('Import %s done!' % fname)


def _parse_list_cell(x):
    if x is None:
        return []
    s = str(x).strip()
    if s == "" or s.lower() == "nan":
        return []
    try:
        v = ast.literal_eval(s)
        if isinstance(v, (list, tuple)):
            return [float(i) for i in v]
        return []
    except Exception:
        s = s.strip("[]")
        if s.strip() == "":
            return []
        return [float(t.strip()) for t in s.split(",") if t.strip()]


def import_hand_level_table_csv_to_gridfs(
    gfs,
    csv_path,
    fname_prefix="0_HAND_",
    sort_by=("Subbasin", "Flood_Level"),
    nodata_value=-9999.0,
    use_hru_id_as_index=True,    # True=按 HRU_ID 做数组下标（稀疏填充）；False=按行顺序
):
    """
    读取表格（含 HRU_ID），并入库到 GridFS。
    - HRU_ID 直接从表里读取，不再自动生成
    - 如果 use_hru_id_as_index=True：所有数组长度为 max(HRU_ID)+1，按 HRU_ID 位置填值
      如果 False：按行顺序存，HRU_ID 单独存一份作为索引（更省空间）
    """
    import pandas as pd

    df = pd.read_csv(csv_path)

    required_cols = [
        "Subbasin", "HRU_ID", "Flood_Level", "LevelDepth",
        "SumArea", "SumVolume", "AvgDepth", "AccVolume",
        "LowerAccDepth"
    ]
    missing = [c for c in required_cols if c not in df.columns]
    if missing:
        raise ValueError("CSV 缺少列: %s" % missing)

    # 排序只是为了可复现（不影响 HRU_ID 下标逻辑）
    df = df.sort_values(list(sort_by)).reset_index(drop=True)

    # HRU_ID 必须是整数
    df["HRU_ID"] = df["HRU_ID"].astype(int)

    # 检查唯一性（非常建议）
    if df["HRU_ID"].duplicated().any():
        dup = df[df["HRU_ID"].duplicated(keep=False)]["HRU_ID"].tolist()[:10]
        raise ValueError("HRU_ID 存在重复，无法作为数组下标。示例重复值: %s" % dup)

    n_rows = len(df)
    print("[INFO] rows =", n_rows)

    # 解析 LowerAccDepth
    lower_lists = [_parse_list_cell(x) for x in df["LowerAccDepth"].tolist()]

    # ========== 两种存储模式 ==========
    if use_hru_id_as_index:
        max_id = int(df["HRU_ID"].max())
        size = max_id + 1
        print("[INFO] use HRU_ID as index, array size =", size)

        # 初始化为 nodata
        def make_vec():
            return [float(nodata_value)] * size

        subbasin = make_vec()
        flood_level = make_vec()
        level_depth = make_vec()
        sum_area = make_vec()
        sum_volume = make_vec()
        avg_depth = make_vec()
        acc_volume = make_vec()

        # 变长数组：len 也按下标存；flat 仍然是拼接一维
        lower_len = make_vec()
        lower_2d = [None] * size
        lower_flat = []

        # HRU_ID 自己也存一份“存在性/索引”数组：位置 i 存 i（不存在则 nodata）
        hru_id_vec = make_vec()

        for (_, row), lad in zip(df.iterrows(), lower_lists):
            hid = int(row["HRU_ID"])
            hru_id_vec[hid] = float(hid)
            subbasin[hid] = float(row["Subbasin"])
            flood_level[hid] = float(row["Flood_Level"])
            level_depth[hid] = float(row["LevelDepth"])
            sum_area[hid] = float(row["SumArea"])
            sum_volume[hid] = float(row["SumVolume"])
            avg_depth[hid] = float(row["AvgDepth"])
            acc_volume[hid] = float(row["AccVolume"])

            lower_len[hid] = float(len(lad))
            lower_2d[hid] = lad

        for hid in range(size):
            lad = lower_2d[hid]
            if lad:  # None 或 [] 都会跳过
                lower_flat.extend(lad)
        print("[INFO] LowerAccDepth total flat length =", len(lower_flat))


    # =========================
    # 入库：一维数组 => 1 layer
    # 要求：写 meta_dict，并确保 CELLSNUM = len(vec)
    # =========================
    def put(name, vec):
        # 文件名（注意 upper 由 import_array_to_mongodb 内部处理的话也可以，但你这里要 meta 用 fname）
        fname = (fname_prefix + name).upper()

        cols = len(vec)  # ✅ 最重要：CELLSNUM 必须等于 len(vec)
        rows = 1         # 这里存的是 1 行的长向量
        layers = 1       # import_array_to_mongodb 传 [vec]，所以 layers=1

        meta_dict = {}
        meta_dict["TYPE"] = fname
        meta_dict["ID"] = fname
        meta_dict["DESCRIPTION"] = fname
        meta_dict["SUBBASIN"] = 0
        meta_dict["CELLSIZE"] = 1
        meta_dict["NODATA_VALUE"] = float(nodata_value)
        meta_dict["NCOLS"] = cols
        meta_dict["NROWS"] = rows
        meta_dict["XLLCENTER"] = 0
        meta_dict["YLLCENTER"] = 0
        meta_dict["LAYERS"] = layers
        meta_dict["CELLSNUM"] = cols   # ✅ 关键字段：等于列数组长度
        meta_dict["SRS"] = ""

        # 你的入库函数：仍然按 [vec]（1 layer）写入
        import_array_to_mongodb(gfs, [vec], fname)

    put("HRU_ID", hru_id_vec)
    put("SUBBASIN", subbasin)
    put("FLOOD_LEVEL", flood_level)
    put("LEVELDEPTH", level_depth)
    put("SUMAREA", sum_area)
    put("SUMVOLUME", sum_volume)
    put("AVGDEPTH", avg_depth)
    put("ACCVOLUME", acc_volume)
    put("LOWERACCDEPTH_LEN", lower_len)
    put("LOWERACCDEPTH_FLAT", lower_flat)

    return {
        "rows": n_rows,
        "use_hru_id_as_index": use_hru_id_as_index,
        "flat_len": len(lower_flat),
        "fname_prefix": fname_prefix.upper()
    }




if __name__ == '__main__':

    client = MongoClient("mongodb://127.0.0.1:27017")
    db = client["poyang_lake1_longterm_model_1171"]         # 你自己的库名
    gfs_spatial = gridfs.GridFS(db, collection="SPATIAL")
    csv_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\InundationMap.csv'

    result = import_hand_level_table_csv_to_gridfs(
        gfs_spatial,
        csv_path=csv_path,
        fname_prefix="0_HAND_"
    )

    print(result)

