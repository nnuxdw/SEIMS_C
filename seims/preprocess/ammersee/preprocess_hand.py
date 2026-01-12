import os.path

import pandas as pd
import geopandas as gpd
from pymongo import MongoClient
from collections import defaultdict
from collections import defaultdict, deque
import copy

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
        sorted_levels = sorted(levels, key=lambda x: x[0])

        prev_hand_area_sum = 0.0

        sub_result = []
        level_depths = []
        avg_depths = []
        level_indices = []

        for idx, (level, level_df) in enumerate(sorted_levels):
            hand_area = level_df["area"].iloc[0]
            hand_volume = level_df["area"].iloc[0] * level_df["Depth"].iloc[0]

            interval = level_df["HAND_Threshold_Interval"].iloc[0]
            lower, upper = map(float, interval.split(","))
            level_depth = upper - lower

            ch = ch_map.get(int(sbid), {})
            ch_area = 0.0
            channel_volume = 0.0

            is_lake = ch.get("Is_Lake", 0)
            is_res = ch.get("Is_Res", 0)

            if is_lake != 1 and is_res != 1 and idx == 0:
                ch_area = ch.get("CH_WIDTH", 0.0) * ch.get("CH_LEN", 0.0)
                channel_volume = ch_area * level_depth

            prev_hand_volume = prev_hand_area_sum * level_depth

            sum_volume = channel_volume + prev_hand_volume + hand_volume
            sum_area = ch_area + prev_hand_area_sum + hand_area

            avg_depth = sum_volume / sum_area if sum_area > 0 else 0.0


            row = {
                "Subbasin": sbid,
                "Flood_Level": level,
                "LevelDepth": level_depth,
                "SumArea": sum_area,
                "SumVolume": sum_volume,
                "AvgDepth": avg_depth,
            }
            sub_result.append(row)

            prev_hand_area_sum += hand_area

            level_depths.append(level_depth)
            avg_depths.append(avg_depth)
            level_indices.append(level)

        # ===== LowerAccDepth 保留你的原逻辑 =====
        n = len(level_depths)
        acc_depth_matrix = [[0.0 for _ in range(n + 1)] for _ in range(n + 1)]

        for i in range(1, n + 1):
            for lev in range(i + 1, n + 1):
                if lev == i + 1:
                    acc_depth_matrix[i][lev] = avg_depths[i - 1]
                else:
                    acc_depth_matrix[i][lev] = avg_depths[i - 1] + sum(level_depths[i:lev - 1])

        for i in range(1, n + 1):
            sub_result[i - 1]["LowerAccDepth"] = [round(v, 3) for v in acc_depth_matrix[i]]

        result.extend(sub_result)

    df_result = pd.DataFrame(result)
    df_result.sort_values(by=["Subbasin", "Flood_Level"], inplace=True)

    df_result["AccVolume"] = df_result.groupby("Subbasin")["SumVolume"].cumsum()

    df_result["SumArea"] = df_result["SumArea"].round(3)
    df_result["SumVolume"] = df_result["SumVolume"].round(3)
    df_result["AvgDepth"] = df_result["AvgDepth"].round(3)
    df_result["AccVolume"] = df_result["AccVolume"].round(3)

    cols = [
               col for col in df_result.columns
               if col not in ("LowerAccDepth")
           ] + ["LowerAccDepth"]

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

"""一个临时方法，用于修复龙平产生的FloodStep.txt中,Flood_Level从0开始的问题"""
def modify_flood_level(input_path: str, output_path: str):
    with open(input_path, 'r', encoding='utf-8') as fin, open(output_path, 'w', encoding='utf-8') as fout:
        for idx, line in enumerate(fin):
            # 处理表头：不变写入
            if idx == 0:
                fout.write(line)
                continue

            parts = line.strip().split()

            if len(parts) < 5:
                fout.write(line)
                continue

            try:
                # 将第三列 Flood_Level 加 1 并转为整数
                parts[2] = str(int(float(parts[2]) + 1))
                fout.write("\t".join(parts) + "\n")
            except Exception as e:
                print(f"处理第 {idx+1} 行出错: {line.strip()} — {e}")
                fout.write(line)

"""一个临时方法，用于修复龙平产生的FloodStep.txt中,Flood_Level存在间隙的问题，例如1,2,3,5，则判定缺少4"""
def check_flood_level_gaps(file_path):
    subbasin_levels = defaultdict(set)

    with open(file_path, 'r', encoding='utf-8') as fin:
        for idx, line in enumerate(fin):
            if idx == 0:
                continue  # 跳过表头

            parts = line.strip().split()
            if len(parts) < 3:
                continue

            try:
                subbasin = parts[1]
                level = int(float(parts[2]))
                subbasin_levels[subbasin].add(level)
            except Exception as e:
                print(f"⚠️ 第 {idx+1} 行解析错误: {line.strip()} — {e}")

    # 分析缺失等级
    for subbasin, levels in subbasin_levels.items():
        sorted_levels = sorted(levels)
        missing = []

        for i in range(sorted_levels[0], sorted_levels[-1]):
            if i not in levels:
                missing.append(i)

        if missing:
            print(f"Subbasin {subbasin} 缺失的 Flood_Level: {missing}")
        # else:
        #     print(f"Subbasin {subbasin} 等级连续 ✅")

def repair_flood_levels(input_path: str, output_path: str):
    with open(input_path, 'r', encoding='utf-8') as fin:
        lines = fin.readlines()

    header = lines[0]
    data_lines = lines[1:]

    # 第一步：解析每行内容
    subbasin_data = defaultdict(list)

    for idx, line in enumerate(data_lines):
        parts = line.strip().split()
        if len(parts) < 5:
            continue
        subbasin = parts[1]
        flood_level = int(float(parts[2]))
        subbasin_data[subbasin].append({
            "line_index": idx,
            "original_line": line,
            "parts": parts,
            "flood_level": flood_level
        })

    modified_lines = copy.deepcopy(data_lines)

    for subbasin, entries in subbasin_data.items():
        # 构建层级映射表
        level_counts = defaultdict(list)
        for e in entries:
            level_counts[e['flood_level']].append(e)

        levels_present = sorted(level_counts.keys())
        min_lv, max_lv = levels_present[0], levels_present[-1]
        total_needed = max_lv - min_lv + 1
        existing_levels = set(level_counts.keys())

        expected_levels = list(range(min_lv, min_lv + len(entries)))
        missing_levels = [lvl for lvl in expected_levels if lvl not in existing_levels]

        if not missing_levels:
            continue

        print(f"🔧 Subbasin {subbasin} 缺失层级: {missing_levels}")

        # 第一步：优先从重复层中抽冗余条目来补缺
        pool = []
        for lvl, items in level_counts.items():
            if len(items) > 1:
                pool.extend(items[1:])  # 留一条，其余为冗余

        pool = deque(pool)
        used_indices = set()

        for missing in missing_levels[:]:  # 留出空间做后续处理
            if pool:
                donor = pool.popleft()
                donor_idx = donor['line_index']
                donor['parts'][2] = str(missing)
                modified_line = "\t".join(donor["parts"]) + "\n"
                modified_lines[donor_idx] = modified_line
                used_indices.add(donor_idx)
                print(f"✅ 使用多余层 {donor['flood_level']} → 补 {missing}")
                missing_levels.remove(missing)

        # 第二步：仍有缺口 → 从高层号条目中前移（不能是已用的，也不能覆盖已有低层号）
        if missing_levels:
            # 排除已使用过的 entry，按 flood_level 降序，从大到小依次前移
            available = sorted(
                [e for e in entries if e['line_index'] not in used_indices and e['flood_level'] not in missing_levels],
                key=lambda x: x['flood_level'],
                reverse=True
            )

            assigned = set()
            for new_lv in sorted(missing_levels):
                for entry in available:
                    old_lv = entry['flood_level']
                    idx = entry['line_index']

                    # 只允许将“大于 new_lv 的层号”前移，避免覆盖低层号
                    if old_lv > new_lv and idx not in assigned:
                        entry['parts'][2] = str(new_lv)
                        modified_lines[idx] = "\t".join(entry['parts']) + "\n"
                        assigned.add(idx)
                        print(f"🔁 将 {old_lv} 前移为 {new_lv} 以补 Subbasin {subbasin}")
                        break
                else:
                    print(f"❗ Subbasin {subbasin} 无法补 {new_lv}（没有比它大的层号可用）")

    # 写回修改后的文件
    with open(output_path, 'w', encoding='utf-8') as fout:
        fout.write(header)
        fout.writelines(modified_lines)


if __name__ == '__main__':
    base_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1"
    # base_path = r"G:\program\seims\SEIMS_HAND\data\Cottonwood"
    ### 注意input_shp一定要是按照FIELDID合并之后且投影到等面积的
    input_shp = os.path.join(base_path,"workspace\spatial_shp\subbasin_mollwede_dissolved.shp")
    input_hand_flood_step = os.path.join(base_path,"rundata\FloodStep.txt")
    output_map = os.path.join(base_path,"rundata\InundationMap.csv")

    # 检查缺失层级
    # check_flood_level_gaps(input_hand_flood_step_old)
    # 修复层号从0开始的情况，改为从1开始
    # modify_flood_level(input_hand_flood_step,modifyed_flood_level)

    # 修复缺失层级
    # repair_flood_levels(input_hand_flood_step,repaired_flood_level)

    build_hand_lookup_table(
        shp_path=input_shp,
        hand_txt_path=input_hand_flood_step,
        output_csv=output_map,
        mongo_uri="mongodb://localhost:27017",
        mongo_db="poyang_lake1_longterm_model_1171",
        mongo_col="REACHES",
    )
