# -*- coding: utf-8 -*-
"""
把“最佳基因参数”写回 caliparam_sub.csv / caliparam.csv
- 解析 best_csv 里的 gene_values（array('d', [...])）
- 按 def 文件顺序映射到参数名 → 最佳值
- 读取两个“上游标记两列CSV”（第二列为 -9999 的取第一列ID）
- 批量更新 caliparam_sub.csv 与 caliparam.csv 中对应ID的参数列
"""

import ast
import os
import re
from typing import Dict, List, Tuple
import pandas as pd


def _normalize(s: str) -> str:
    return re.sub(r'[^0-9A-Za-z]+', '', s or '').upper()


# 常见别名与规范形态
ALIASES = {
    'K_PET_1D': 'K_PET_1D',
    'RUNOFF_CO': 'RUNOFF_CO',
    'C_SNOW12_1D': 'C_SNOW12_1D',
    'C_SNOW6_1D': 'C_SNOW6_1D',
    'LAG_SNOW_1D': 'LAG_SNOW_1D',
    'T0_1D': 'T0_1D',
    'T_SNOW_1D': 'T_SNOW_1D',
    'KI_1D': 'KI_1D',
    'BASE_EX_1D': 'BASE_EX_1D',   # caliparam_sub 里也常见 BASE_EX_1
    'BASE_EX_1': 'BASE_EX_1',
    'KG_1D': 'KG_1D',
    'GW_DELAY_1D': 'GW_DELAY_1D',
    'LAKEB_1D': 'LAKEB_1D',
    'LAKE_ALPHA': 'LAKE_ALPHA',
    'CH_N': 'CH_N',
    'EP_CH_1D': 'EP_CH_1D',
    'SW_CAP': 'SW_CAP',           # 也可能写成 SW_CAP_1
    'SW_CAP_1': 'SW_CAP_1',
    'SURLAG_1D': 'SURLAG_1D',
}

CANON = {
    'K_PET_1D': ['K_PET_1D', 'K_PET', 'KPET1D', 'KPET'],
    'RUNOFF_CO': ['RUNOFF_CO', 'RUNOFF_COE', 'RUNOFFCO'],
    'C_SNOW12_1D': ['C_SNOW12_1D', 'CSNOW12', 'CSNOW12_1D'],
    'C_SNOW6_1D': ['C_SNOW6_1D', 'CSNOW6', 'CSNOW6_1D'],
    'LAG_SNOW_1D': ['LAG_SNOW_1D', 'LAGSNOW', 'LAGSNOW_1D'],
    'T0_1D': ['T0_1D', 'T0'],
    'T_SNOW_1D': ['T_SNOW_1D', 'TSNOW', 'TSNOW_1D'],
    'KI_1D': ['KI_1D', 'KI'],
    'BASE_EX_1D': ['BASE_EX_1D', 'BASE_EX', 'BASEEX1D'],
    'KG_1D': ['KG_1D', 'KG'],
    'GW_DELAY_1D': ['GW_DELAY_1D', 'GWDELAY', 'GWDELAY_1D'],
    'LAKEB_1D': ['LAKEB_1D', 'LAKEB'],
    'LAKE_ALPHA': ['LAKE_ALPHA', 'LAKEALPHA'],
    'CH_N': ['CH_N', 'CHN'],
    'EP_CH_1D': ['EP_CH_1D', 'EPCH', 'EPCH_1D'],
    'SW_CAP': ['SW_CAP', 'SWCAP', 'SW_CAP_1', 'SWCAP1'],
    'SURLAG_1D': ['SURLAG_1D', 'SURLAG'],
}


def _build_col_matcher(columns: List[str]) -> Dict[str, str]:
    return {_normalize(c): c for c in columns}


def _find_best_column(param_name: str, columns: List[str]):
    """
    简化版：param_name 即为准确列名。
    若列存在则直接返回；否则报错。
    """
    if param_name in columns:
        return param_name
    else:
        return None


def parts_to_canon(name: str) -> str:
    key = _normalize(name)
    for canon, names in CANON.items():
        if key in [_normalize(x) for x in names]:
            return canon
    return name.replace('-', '_').replace('.', '_')


def parse_gene_values_from_csv(best_csv_path: str, gene_col: str = 'gene_values') -> List[float]:
    df = pd.read_csv(best_csv_path)
    if df.empty:
        raise ValueError(f"{best_csv_path} 没有有效数据")
    s = str(df.iloc[0][gene_col])
    m = re.search(r'\[(.*?)\]', s)
    if not m:
        raise ValueError(f"gene_values 字段格式异常：{s}")
    arr_str = '[' + m.group(1) + ']'
    vals = ast.literal_eval(arr_str)
    return [float(x) for x in vals]


def read_param_order_from_def(def_path: str) -> List[str]:
    names = []
    with open(def_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = [x.strip() for x in line.split(',')]
            names.append(parts[0])
    return [parts_to_canon(n) for n in names]


import pandas as pd

def read_upstream_ids(two_col_csv_path):
    """
    读取“两列CSV”，返回第二列为 -9999 的行的第一列 ID 列表（int）。
    兼容点：
      - 自动探测分隔符（逗号/制表符/空格等）
      - 兼容有表头 / 无表头
      - 兼容第二列为字符串形式的 -9999（含空格/引号）
      - 若 header=None 导致首行是表头被当成数据，会尝试自动跳过首行
    """
    def _load(header):
        # sep=None + engine='python' 自动嗅探分隔符；dtype=str 保留原始字符串方便清洗
        df = pd.read_csv(
            two_col_csv_path,
            header=header,           # 0 或 None
            engine='python',
            sep=None,
            comment='#',
            dtype=str
        )
        # 去掉全空列，确保至少两列
        df = df.dropna(axis=1, how='all')
        return df

    # 先尝试“有表头”，再尝试“无表头”
    for header in (0, None):
        try:
            df = _load(header)
        except Exception:
            continue
        if df.shape[1] < 2:
            continue

        # 第一列作为 id，第二列作为 flag，先做清洗
        id_s   = df.iloc[:, 0].astype(str).str.strip().str.replace('"','').str.replace("'",'')
        flag_s = df.iloc[:, 1].astype(str).str.strip().str.replace('"','').str.replace("'",'')

        # 转成数值；无法转的会是 NaN
        flag_num = pd.to_numeric(flag_s, errors='coerce')

        # 直接匹配一遍
        mask = flag_num.eq(-9999)
        if mask.any():
            ids = pd.to_numeric(id_s[mask], errors='coerce').dropna().astype(int).tolist()
            if ids:
                return ids

        # 如果还是没匹配到，可能首行其实是表头被当作数据了（常见于 header=None）
        if header is None and len(df) > 1:
            flag_num2 = pd.to_numeric(flag_s.iloc[1:], errors='coerce')
            mask2 = flag_num2.eq(-9999)
            if mask2.any():
                ids = pd.to_numeric(id_s.iloc[1:][mask2], errors='coerce').dropna().astype(int).tolist()
                if ids:
                    return ids

    # 兜底：给一点可视化提示，方便定位文件格式问题
    preview = None
    try:
        preview = pd.read_csv(two_col_csv_path, engine='python', sep=None, nrows=5).to_string(index=False)
    except Exception:
        pass
    raise ValueError(
        f"未在 {two_col_csv_path} 找到第二列为 -9999 的行。"
        + (f"\n文件前5行预览：\n{preview}" if preview is not None else "")
    )



def apply_best_params(
    best_csv_path: str,
    def_path: str,
    sub_upstream_flags_path: str,
    hand_upstream_flags_path: str,
    target_sub_csv: str,
    target_hand_csv: str,
    subbasin_id: int,
    sub_id_col_in_target: str = 'subbasin',
    hand_id_col_in_target: str = 'FID',
    save_sub_as: str = None,
    save_hand_as: str = None,
) -> Tuple[str, str]:
    """核心函数：返回 (caliparam_sub 输出路径, caliparam 输出路径)"""
    # 1) 基因与参数顺序
    best_vals = parse_gene_values_from_csv(best_csv_path)
    param_order = read_param_order_from_def(def_path)
    if len(best_vals) != len(param_order):
        raise ValueError(f"基因数量({len(best_vals)}) 与 def 参数数量({len(param_order)}) 不一致。")
    param2best = dict(zip(param_order, best_vals))

    # 2) 上游 ID
    sub_up_ids = read_upstream_ids(sub_upstream_flags_path)
    hand_up_ids = read_upstream_ids(hand_upstream_flags_path)

    # 3) 读目标表
    df_sub = pd.read_csv(target_sub_csv)
    df_hand = pd.read_csv(target_hand_csv)

    # 4) 列映射
    sub_col_map, hand_col_map = {}, {}
    for p in param_order:
        # sub
        sub_col = _find_best_column(p, list(df_sub.columns))
        if None is not sub_col:
            sub_col_map[p] = sub_col
        hand_col = _find_best_column(p, list(df_hand.columns))

        if None is not hand_col:
            hand_col_map[p] = hand_col
        if hand_col is None and sub_col is None:
            print(f"{p} is not found in caliparam.csv and caliparam_sub.csv")

    # 5) 更新
    if sub_up_ids:
        mask_sub = df_sub[sub_id_col_in_target].astype(int).isin(sub_up_ids)
        for p, v in param2best.items():
            col = sub_col_map.get(p)
            if col in df_sub.columns:
                df_sub.loc[mask_sub, col] = v

    if hand_up_ids:
        mask_hand = df_hand[hand_id_col_in_target].astype(int).isin(hand_up_ids)
        for p, v in param2best.items():
            col = hand_col_map.get(p)
            if col in df_hand.columns:
                df_hand.loc[mask_hand, col] = v

    # 6) 保存
    sub_out = save_sub_as or target_sub_csv
    hand_out = save_hand_as or target_hand_csv
    df_sub.to_csv(sub_out, index=False)
    df_hand.to_csv(hand_out, index=False)
    return sub_out, hand_out


# ========= 示例调用（按需修改路径后直接运行） =========
if __name__ == '__main__':
    base_path = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1'
    model_name = r'poyang_lake1_longterm_model'
    # cali_name = r'CALI_NSGA2_Gen_100_Pop_20'
    #
    SUBBASIN_IDs = [123,141,214,225,322,347,457]
    cali_name_map = {123:'CALI_NSGA2_Gen_100_Pop_20',141:'CALI_NSGA2_Gen_100_Pop_20',214:'CALI_NSGA2_Gen_100_Pop_20',
                 225:'CALI_NSGA2_Gen_100_Pop_20',322:'CALI_NSGA2_Gen_100_Pop_20',347:'CALI_NSGA2_Gen_100_Pop_20',
                 457:'CALI_NSGA2_Gen_100_Pop_20'}
    TARGET_SUBBASIN_ID = 1171
    for SUBBASIN_ID in SUBBASIN_IDs:
        # 把下面这些变量改成你的实际文件路径
        BEST_CSV = os.path.join(base_path,f'{model_name}_{SUBBASIN_ID}',cali_name_map[SUBBASIN_ID],'result_1.csv')  # 含 gene_values 的单行CSV
        DEF_FILE = os.path.join(base_path,f'{model_name}_{SUBBASIN_ID}',r'cali_param_rng-Q.def')  # 参数顺序文件
        UPSTREAM_SUBBSIN_CSV = os.path.join(base_path,f'{model_name}_{SUBBASIN_ID}',r'caliparam_sub.csv')     # 两列：第一列ID、第二列flag（取 -9999）
        UPSREAM_HAND_CSV = os.path.join(base_path,f'{model_name}_{SUBBASIN_ID}',r'caliparam.csv')   # 两列：第一列ID、第二列flag（取 -9999）
        TARGET_SUB = os.path.join(base_path,f'{model_name}_{TARGET_SUBBASIN_ID}',r'caliparam_sub.csv')            # 将被更新
        TARGET_HAND = os.path.join(base_path,f'{model_name}_{TARGET_SUBBASIN_ID}',r'caliparam.csv')               # 将被更新


        # 如需另存为，填写新路径；否则留 None 直接覆盖
        SAVE_SUB_AS = None
        SAVE_HAND_AS = None

        out_sub, out_hand = apply_best_params(
            best_csv_path=BEST_CSV,
            def_path=DEF_FILE,
            sub_upstream_flags_path=UPSTREAM_SUBBSIN_CSV,
            hand_upstream_flags_path=UPSREAM_HAND_CSV,
            target_sub_csv=TARGET_SUB,
            target_hand_csv=TARGET_HAND,
            subbasin_id=SUBBASIN_ID,
            sub_id_col_in_target='subbasin',  # 你的 caliparam_sub 主键列名
            hand_id_col_in_target='FID',      # 你的 caliparam 主键列名
            save_sub_as=SAVE_SUB_AS,
            save_hand_as=SAVE_HAND_AS,
        )
        print(f'写回完成：\n  caliparam_sub → {out_sub}\n  caliparam     → {out_hand}')
