import pandas as pd
import geopandas as gpd
from pymongo import MongoClient

# 1. 读取 SHP 文件并返回 {FIELDID: area}
def load_area_map_from_shapefile(shapefile_path):
    gdf = gpd.read_file(shapefile_path)
    return dict(zip(gdf["FIELDID"], gdf["area"]))

# 2. 从 MongoDB 加载河道宽度、长度等信息
def load_channel_properties_from_mongo(uri, db_name, collection_name):
    client = MongoClient(uri)
    collection = client[db_name][collection_name]
    projection = {
        "SUBBASINID": 1,
        "CH_WIDTH": 1,
        "CH_LEN": 1,
        "Is_Lake": 1,
        "Is_Res": 1
    }
    docs = list(collection.find({}, projection))
    client.close()
    return {
        doc["SUBBASINID"]: {
            "CH_WIDTH": doc.get("CH_WIDTH", 0),
            "CH_LEN": doc.get("CH_LEN", 0),
            "Is_Lake": doc.get("Is_Lake", 0),
            "Is_Res": doc.get("Is_Res", 0)
        }
        for doc in docs
    }

# 3. 模拟 C++ 中 m_levelHandSumArea 和 m_levelHandSumVol 的计算逻辑
def compute_hand_area_volume(hand_df, ch_map):
    result = []
    grouped = hand_df.groupby("Subbasin")

    for sbid, group in grouped:
        levels = group.groupby("Flood_Level")
        sorted_levels = sorted(levels, key=lambda x: x[0])  # 按层升序

        prev_hand_area_sum = 0.0
        sub_result = []

        level_depths = []
        avg_depths = []
        level_indices = []

        for idx, (level, level_df) in enumerate(sorted_levels):
            hand_area_sum = level_df["area"].sum()
            hand_volume = (level_df["area"] * level_df["Depth"]).sum()
            interval = level_df["HAND_Threshold_Interval"].iloc[0]
            lower, upper = map(float, interval.split(","))
            level_depth = upper - lower

            ch = ch_map.get(int(sbid), {})
            ch_area = 0
            channel_volume = 0
            is_lake = ch.get("Is_Lake", 0)
            is_res = ch.get("Is_Res", 0)

            if is_lake != 1 and is_res != 1 and idx == 0:
                ch_area = ch.get("CH_WIDTH", 0) * ch.get("CH_LEN", 0)
                channel_volume = ch_area * level_depth

            prev_hand_volume = prev_hand_area_sum * level_depth
            sum_volume = channel_volume + prev_hand_volume + hand_volume
            sum_area = ch_area + prev_hand_area_sum + hand_area_sum

            avg_depth = sum_volume / sum_area if sum_area > 0 else 0.0

            sub_result.append({
                "Subbasin": sbid,
                "Flood_Level": level,
                "LevelDepth": level_depth,
                "SumArea": sum_area,
                "SumVolume": sum_volume,
                "AvgDepth": avg_depth
            })

            level_depths.append(level_depth)
            avg_depths.append(avg_depth)
            level_indices.append(level)

            prev_hand_area_sum += hand_area_sum

        n = len(level_depths)
        acc_depth_matrix = [[0.0 for _ in range(n + 1)] for _ in range(n + 1)]

        for i in range(1, n + 1):
            for lev in range(i + 1, n + 1):
                if lev == i + 1:
                    acc_depth_matrix[i][lev] = avg_depths[i - 1]
                else:
                    acc_depth_matrix[i][lev] = avg_depths[i - 1] + sum(level_depths[i:lev - 1])

        for i in range(1, n + 1):
            acc_depth = [round(v, 3) for v in acc_depth_matrix[i]]
            sub_result[i - 1]["LowerAccDepth"] = acc_depth

        result.extend(sub_result)

    df_result = pd.DataFrame(result)
    df_result.sort_values(by=["Subbasin", "Flood_Level"], inplace=True)

    df_result["AccVolume"] = df_result.groupby("Subbasin")["SumVolume"].cumsum()

    # 保留 3 位小数
    df_result["SumArea"] = df_result["SumArea"].round(3)
    df_result["SumVolume"] = df_result["SumVolume"].round(3)
    df_result["AvgDepth"] = df_result["AvgDepth"].round(3)
    df_result["AccVolume"] = df_result["AccVolume"].round(3)

    # 确保 AccDepth 在最后一列
    cols = [col for col in df_result.columns if col != "LowerAccDepth"] + ["LowerAccDepth"]
    df_result = df_result[cols]

    return df_result



# 4. 主函数：整合流程
def build_hand_lookup_table(shp_path, hand_txt_path, mongo_uri, mongo_db, mongo_col, output_csv):
    print("📦 读取 HRU shapefile...")
    area_map = load_area_map_from_shapefile(shp_path)

    print("连接 MongoDB，获取河道属性...")
    ch_map = load_channel_properties_from_mongo(mongo_uri, mongo_db, mongo_col)

    print("读取 HAND 层文件...")
    df_hand = pd.read_csv(hand_txt_path, sep="\t")
    df_hand["area"] = df_hand["HRU_ID"].map(area_map)

    print("开始逐层计算面积和体积...")
    df_lookup = compute_hand_area_volume(df_hand, ch_map)

    print(f"保存查找表到 {output_csv}")
    df_lookup.to_csv(output_csv, index=False)
    print("完成！")




if __name__ == '__main__':
    input_shp = r"G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\workspace\HRU_file\HRU_mollwede.shp"
    input_hand_flood_step = r"G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\rundata\FloodStep.txt"
    output_map = r"G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\rundata\InundationMap.csv"
    build_hand_lookup_table(
        shp_path=input_shp,
        hand_txt_path=input_hand_flood_step,
        output_csv=output_map,
        mongo_uri="mongodb://localhost:27017",
        mongo_db="11_159084_48_120933_longterm_model",
        mongo_col="REACHES",
    )
