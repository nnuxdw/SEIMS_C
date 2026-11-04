import os.path

import pandas as pd
from pymongo import MongoClient
import csv

def export_subbasin_downstream(
    mongo_uri: str,
    db_name: str,
    coll_name: str,
    out_csv: str,
    query: None = None,
    sort_by: str="SUBBASINID",
    batch_size: int = 10_000,
):
    """
    从 MongoDB 的集合导出 SUBBASINID、DOWNSTREAM 两列到 CSV。

    - query: 可选过滤条件；默认全量。
    - sort_by: 可选排序键；默认按 SUBBASINID 升序；传 None 则不排序。
    - batch_size: 游标批量大小，防止一次性取太多内存。
    """
    query = query or {}
    client = MongoClient(mongo_uri)
    coll = client[db_name][coll_name]

    projection = {"SUBBASINID": 1, "DOWNSTREAM": 1, "_id": 0}
    cursor = coll.find(query, projection).batch_size(batch_size)
    if sort_by:
        cursor = cursor.sort(sort_by, 1)

    with open(out_csv, "w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=["SUBBASINID", "DOWNSTREAM"])
        writer.writeheader()
        for doc in cursor:
            writer.writerow({
                "SUBBASINID": doc.get("SUBBASINID", ""),
                "DOWNSTREAM": doc.get("DOWNSTREAM", ""),
            })

    client.close()
    print(f"✅ Exported to {out_csv}")

target_subbasin = 1171  # 替换为你的目标值
base_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model'
sub_df = os.path.join(base_path,r'REACHES.csv')
hru_df = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\csv\subbasin.csv'
caliparam_sub_csv = os.path.join(base_path,f'TNH_caliparam_sub_{target_subbasin}.csv')
caliparam_csv = os.path.join(base_path,f'TNH_caliparam_{target_subbasin}.csv')
# 第0步：从数据库导出REACHES.csv
# export_subbasin_downstream(
#     "mongodb://localhost:27017",
#     "poyang_lake1_longterm_model",
#     "REACHES",
#     sub_df
# )
# 第一步：读取两个 CSV 文件
sub_df = pd.read_csv(sub_df)  # 包含 SUBBASINID, DOWNSTREAM
hru_df = pd.read_csv(hru_df)  # 包含 FID, subbasinid

# 第二步：构建下游到上游的映射
downstream_to_upstream = sub_df.groupby('DOWNSTREAM')['SUBBASINID'].apply(list).to_dict()

# 第三步：递归找所有上游子流域
def find_all_upstream(target, visited=None):
    if visited is None:
        visited = set()
    if target in downstream_to_upstream:
        for upstream in downstream_to_upstream[target]:
            if upstream not in visited:
                visited.add(upstream)
                find_all_upstream(upstream, visited)
    return visited

# 第四步：指定目标子流域

upstream_subbasins = find_all_upstream(target_subbasin)
print(upstream_subbasins)
all_target_subbasins = upstream_subbasins.union({target_subbasin})

# ✅ 新增：生成子流域级的 caliparam_sub 标记，并保存 CSV
# 提取所有唯一子流域编号
all_subbasins = pd.Series(sorted(set(sub_df['SUBBASINID']).union(set(sub_df['DOWNSTREAM']))))
sub_mark_df = pd.DataFrame({'subbasin': all_subbasins})
sub_mark_df['caliparam_sub'] = sub_mark_df['subbasin'].apply(lambda x: 1 if x in all_target_subbasins else 0)

# 去掉 subbasin == -1 的行
sub_mark_df = sub_mark_df[sub_mark_df['subbasin'] != -1]

sub_mark_df.to_csv(caliparam_sub_csv, index=False)

# ✅ 第五步：在 hru_df 中标记 caliparam（HRU 层级）
hru_df['caliparam'] = hru_df['subbasin'].apply(lambda x: 1 if x in all_target_subbasins else 0)

# 第六步：提取 FID 和 caliparam 列为 numpy 数组或写入 CSV
result_array = hru_df[['FID', 'caliparam']].to_numpy()

# 打印预览
print("子流域标记表：")
print(sub_mark_df.head())
print("\nHRU 标记表预览：")
print(result_array)

# 如果需要保存到文件
hru_df[['FID', 'caliparam']].to_csv(caliparam_csv, index=False)
