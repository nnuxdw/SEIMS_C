import os.path
import pandas as pd
import geopandas as gpd
from pathlib import Path

# === 1. 读取 def 文件，提取有效参数名 ===
# def read_def_vars(def_file):
#     vars = []
#     with open(def_file, 'r') as f:
#         for line in f:
#             line = line.strip()
#             if line.startswith("#") or not line:
#                 continue
#             var = line.split(",")[0].strip()
#             vars.append(var)
#     return vars

def read_def_vars(def_file):
    vars = []
    with open(def_file, 'r', encoding='utf-8-sig') as f:
        for line in f:
            line = line.strip()
            if line.startswith("#") or not line:
                continue
            var = line.split(",")[0].strip()
            vars.append(var)
    return vars

# === 2. 读取参数表，并提取目标参数值（跳过 -9999）===
def read_parameters(param_file, vars):
    # df = pd.read_csv(param_file)
    df = pd.read_csv(param_file, comment='#', header=0, encoding='utf-8-sig')
    param_values = {}
    for var in vars:
        row = df[df['NAME'].str.lower() == var.lower()]
        if not row.empty:
            val = float(row['VALUE'].values[0])
            if val != -9999:
                param_values[var] = val
    return param_values

# === 3. 获取 csv 文件的行数 ===
def get_row_count(csv_file):
    df = pd.read_csv(csv_file)
    return len(df)

# === 4. 生成 DataFrame ===
# def generate_parameter_df(param_values, num):
#     data = {f"{k}_1d": [v]*num for k, v in param_values.items()}
#     df =pd.DataFrame(data)
#     df.insert(0, 'FID', range(num))
#     return df

def generate_parameter_df(param_values, num):
    data = {k: [v] * num for k, v in param_values.items()}
    df = pd.DataFrame(data)
    df.insert(0, 'FID', range(num))
    return df


def csv_to_subbasin_shp(
    csv_path: str,
    subbasin_shp_path: str,
    out_shp_path: str,
    shp_id_field: str = "SUBBASINID",      # shp 中的 ID 字段
    csv_id_col: str = "subbasin",          # CSV 的 ID 列
    csv_value_col: str = "caliparam_sub",  # CSV 的值列
    out_field: str = "caliparam",          # 输出到 shp 的字段名（≤10 个字符）
    keep_only_matched: bool = True,        # 仅保留匹配到 CSV 的子流域
    fill_value=None                        # 未匹配到时的填充值；默认保持 NaN
):
    """
    将图1格式 CSV（subbasin, caliparam_sub）与子流域 shp 的 SUBBASINID 对应，写出新 shp。
    """

    csv_path = Path(csv_path)
    subbasin_shp_path = Path(subbasin_shp_path)
    out_shp_path = Path(out_shp_path)

    # 确保父目录存在
    out_shp_path.parent.mkdir(parents=True, exist_ok=True)

    # 1) 读取 CSV
    df = pd.read_csv(csv_path)
    for need_col in (csv_id_col, csv_value_col):
        if need_col not in df.columns:
            raise ValueError(f"CSV 缺少列: {need_col}")

    df = (df[[csv_id_col, csv_value_col]]
            .dropna(subset=[csv_id_col])
            .drop_duplicates(subset=[csv_id_col]))
    df[csv_id_col] = pd.to_numeric(df[csv_id_col], errors="raise").astype("int64")

    # 2) 读取子流域 shp
    gdf = gpd.read_file(subbasin_shp_path)
    if shp_id_field not in gdf.columns:
        raise ValueError(f"Shapefile 缺少字段: {shp_id_field}")
    gdf[shp_id_field] = pd.to_numeric(gdf[shp_id_field], errors="raise").astype("int64")

    # 3) 合并
    how = "inner" if keep_only_matched else "left"
    merged = gdf.merge(df, left_on=shp_id_field, right_on=csv_id_col, how=how)

    # 4) 输出字段名检查
    if len(out_field) > 10:
        raise ValueError("Shapefile 字段名需 ≤10 个字符，请缩短 out_field 参数。")
    merged = merged.rename(columns={csv_value_col: out_field})

    if not keep_only_matched and fill_value is not None:
        merged[out_field] = merged[out_field].fillna(fill_value)

    try:
        merged[out_field] = pd.to_numeric(merged[out_field], errors="raise")
        if (merged[out_field] % 1 == 0).all():
            merged[out_field] = merged[out_field].astype("int64")
    except Exception:
        pass

    # 5) 写出新的 shp
    merged.to_file(out_shp_path, driver="ESRI Shapefile")

    matched_n = merged.shape[0]
    csv_unique_n = df.shape[0]
    shp_n = gdf.shape[0]
    return {
        "crs": str(gdf.crs),
        "original_shp_features": shp_n,
        "csv_unique_ids": csv_unique_n,
        "output_features": matched_n,
        "output_path": str(out_shp_path)
    }

def workflow(target_subbasin):
    # === 5. 主逻辑 ===
    # target_subbasin = 123  # 替换为你的目标值
    def_file = r"D:\SEIMS_C\data\CW_2\CW_2_longterm_model\cali_param_rng-Q.def"
    base_path = r"D:\SEIMS_C\data\CW_2\CW_2_longterm_model"
    param_file = os.path.join(base_path,"model_param_initi.csv")
    subbasin_file = r"D:\SEIMS_C\data\CW_2\workspace\csv\subbasin.csv"
    another_file = os.path.join(base_path,"REACHES.csv")  # 用于生成 num1
    # 新增：定义掩码文件的路径
    cali_mask_file = os.path.join(base_path,f"TNH_caliparam_{target_subbasin}.csv")
    cali_sub_mask_file = os.path.join(base_path,f"TNH_caliparam_sub_{target_subbasin}.csv")
    caliparam_path = os.path.join(base_path,f"caliparam_{target_subbasin}.csv")
    caliparam_sub_path = os.path.join(base_path,f"caliparam_sub_{target_subbasin}.csv")
    subbasin_shp_path = r"D:\SEIMS_C\data\CW_2\workspace\spatial_shp\subbasin.shp"
    hru_shp_path = r"D:\SEIMS_C\data\CW_2\workspace\HRU_file\HRU_mollwede_dissolved.shp"
    check_shp_path = os.path.join(base_path + f"\check_sub_{target_subbasin}",f"check_hru_{target_subbasin}.shp")
    check_sub_shp_path = os.path.join(base_path + f"\check_sub_{target_subbasin}",f"check_sub_{target_subbasin}.shp")
    # Step 1: 读取所有参数名
    vars_all = read_def_vars(def_file)

    # Step 2: 设置你想放到 group2 的变量名(子流域层级)，剩下的自动归入 group1(hru层级)
    # vars_group2 = ['Base_ex', 'Kg','gw_delay','ch_n','ep_ch','hlife_docgw','krp','kd_rp','sv_rp','krd']  # <<< 请在此处填入你要走 another_file 的变量名
    # vars_group2 = ['Base_ex', 'Kg','gw_delay','ch_n','ep_ch','LAKEB','LAKE_ALPHA']  # <<< 请在此处填入你要走 another_file 的变量名
    vars_group2 = ['Base_ex_1d', 'Kg_1d', 'gw_delay_1d', 'ch_n', 'ep_ch_1d', 'LAKE_ALPHA', 'LAKEB_1D']
    vars_group1 = [var for var in vars_all if var not in vars_group2]

    # Step 3: 获取两个子集的 count
    num = get_row_count(subbasin_file)
    num1 = get_row_count(another_file)

    fixed_vars = ['Conductivity', 'ch_n', 'Runoff_co','LAKE_ALPHA']

    param_values_all = {}
    for var in vars_all:
        if var in fixed_vars:
            # 如果是固定参数，从文件中获取其初值
            param_values_all[var] = 1
        else:
            # 如果是 group1 中的非固定参数，设为 0
            param_values_all[var] = 0
    # Step 4: 提取参数值
    # param_values_all = read_parameters(param_file, vars_all)  千万不能打开这个
    param_values_group1 = {k: v for k, v in param_values_all.items() if k in vars_group1}
    param_values_group2 = {k: v for k, v in param_values_all.items() if k in vars_group2}

    # Step 5: 生成 DataFrame 并导出
    df_group1 = generate_parameter_df(param_values_group1, num)
    df_group2 = generate_parameter_df(param_values_group2, num1)

    # 修改 df_group2 的 FID 从 1 开始
    df_group2['FID'] = range(1, num1 + 1)
    # ✅ 复制第一行，插入为 FID=0
    first_row = df_group2.iloc[0].copy()
    first_row['FID'] = 0
    df_group2 = pd.concat([pd.DataFrame([first_row]), df_group2], ignore_index=True)


    # ===== 新增逻辑 =====
    # Step 6: 读取掩码文件并修改 df_group1 和 df_group2
    try:
        # 读取 caliparam_mask.csv 并根据 'FID' 匹配
        mask_df1 = pd.read_csv(cali_mask_file)
        # 找到 caliparam 值为 1 的 FID
        fids_to_mask = mask_df1[mask_df1['caliparam'] == 1]['FID'].tolist()
        # 将 df_group1 中对应行所有值设为 -9999
        # 这里假设 df_group1 没有 FID 列，需要根据索引匹配
        df_group1.loc[df_group1["FID"].isin(fids_to_mask), df_group1.columns != "FID"] = -9999

        # 读取 caliparam_sub_mask.csv 并根据 'subbasin' 匹配
        mask_df2 = pd.read_csv(cali_sub_mask_file)
        # 找到 caliparam_sub 值为 1 的 subbasin
        subbasins_to_mask = mask_df2[mask_df2['caliparam_sub'] == 1]['subbasin'].tolist()
        # 将 df_group2 中对应行所有值设为 -9999
        # 这里假设 df_group2 的 'FID' 列与 subbasin 值对应
        df_group2.loc[df_group2["FID"].isin(subbasins_to_mask), df_group2.columns != "FID"] = -9999


    except FileNotFoundError as e:
        print(f"警告：找不到掩码文件 {e.filename}，跳过修改。")

    # Step 7: 转换参数名为大写并保存为 CSV
    # 1. 将 df_group1 的所有列名转换为大写
    df_group1.columns = [col.upper() for col in df_group1.columns]
    # 2. 将 df_group2 的第一列名从 'FID' 改为 'subbasin' 并将所有列名转为大写

    df_group2.columns = [col.upper() for col in df_group2.columns]
    df_group2 = df_group2.rename(columns={'FID': 'subbasin'})

    # === 最后一步：处理 df_group2 列名特殊情况, 把本来就是空间属性的，后面的1D去掉 ===
    rename_map = {}
    for col in df_group2.columns:
        if col in ["LAKE_ALPHA_1D", "CH_N_1D"]:
            rename_map[col] = col.replace("_1D", "")
    if rename_map:
        df_group2 = df_group2.rename(columns=rename_map)

    df_group1.to_csv(caliparam_path, index=False)
    df_group2.to_csv(caliparam_sub_path, index=False)

    # Step 8: 将cali_sub_mask CSV转为shp，方便检查站点上游是否正确
    # info = csv_to_subbasin_shp(
    #     csv_path=cali_sub_mask_file,            # 你的 CSV
    #     subbasin_shp_path=subbasin_shp_path,       # 你的子流域 shp
    #     out_shp_path=check_sub_shp_path,
    #     shp_id_field="SUBBASINID",               # shp 里的字段
    #     csv_id_col="subbasin",                   # CSV 里的字段
    #     csv_value_col="caliparam_sub",           # CSV 里的值列
    #     out_field="cali_sub",                    # 输出字段名（≤10）
    #     keep_only_matched=True,                  # 只导出匹配到的子流域
    #     fill_value=None                          # 不匹配的不填
    # )
    # print(info)
    # Step 9: 将cali hru CSV转为shp，方便检查站点上游是否正确
    # info = csv_to_subbasin_shp(
    #     csv_path=cali_mask_file,            # 你的 CSV
    #     subbasin_shp_path=hru_shp_path,       # 你的hru shp
    #     out_shp_path=check_shp_path,
    #     shp_id_field="FIELDID",               # shp 里的字段
    #     csv_id_col="FID",                   # CSV 里的字段
    #     csv_value_col="caliparam",           # CSV 里的值列
    #     out_field="cali_hru",                    # 输出字段名（≤10）
    #     keep_only_matched=True,                  # 只导出匹配到的子流域
    #     fill_value=None                          # 不匹配的不填
    # )

if __name__ == '__main__':
    tar_subbasin_ids = [19]
    for tar_id in tar_subbasin_ids:
        workflow(tar_id)
