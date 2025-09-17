import pandas as pd

# === 1. 读取 def 文件，提取有效参数名 ===
def read_def_vars(def_file):
    vars = []
    with open(def_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith("#") or not line:
                continue
            var = line.split(",")[0].strip()
            vars.append(var)
    return vars

# === 2. 读取参数表，并提取目标参数值（跳过 -9999）===
def read_parameters(param_file, vars):
    df = pd.read_csv(param_file)
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

def get_nsubbasin(csv_file):
    df = pd.read_csv(csv_file)
    return int(df['subbasin'].max())

# === 4. 生成 DataFrame ===
def generate_parameter_df(param_values, num):
    data = {f"{k}_1d": [v]*num for k, v in param_values.items()}
    df = pd.DataFrame(data)
    df.insert(0, 'FID', range(num))
    return df

# === 5. 主逻辑 ===
def_file = r"D:\SEIMS\data\817\817_test_longterm_model\\cali_param_rng_DOC.def"
param_file = "parameters.csv"
subbasin_file = "subbasin.csv"

# Step 1: 读取所有参数名
vars_all = read_def_vars(def_file)

# Step 2: 设置你想放到 group2 的变量名，剩下的自动归入 group1
vars_group2 = ['Base_ex', 'Kg','gw_delay','ch_n','ep_ch','hlife_docgw','krp','kd_rp','sv_rp','krd'] 
vars_group1 = [var for var in vars_all if var not in vars_group2]

# Step 3: 获取两个子集的 count
num = get_row_count(subbasin_file)

num1 = get_nsubbasin(subbasin_file)


# Step 4: 提取参数值
param_values_all = read_parameters(param_file, vars_all)

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

# 保存为 CSV
df_group1.to_csv("param_group1.csv", index=False)
df_group2.to_csv("param_group2.csv", index=False)