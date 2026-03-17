from __future__ import absolute_import, unicode_literals

import os
import sys

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from pygeoc.utils import UtilClass
import pandas as pd
import numpy as np
import json
import pickle
from pygeoc.utils import StringClass
from test.demo_config import ModelPaths
from test.demo_config import DEMO_MODELS, get_watershed_name
from pymongo import MongoClient
from configparser import ConfigParser
from utility import read_data_items_from_txt
from datetime import datetime
from preprocess.db_mongodb import ConnectMongoDB
import matplotlib.pyplot as plt
from spotpy.objectivefunctions import nashsutcliffe, rsquared, rmse, pbias, kge, lognashsutcliffe, rsr, mae
from preprocess.db_read_model import ReadModelData
import matplotlib as mpl
import re


def main():
    # ========= 新增：选择参数集的方式 =========
    # 方式1：False -> 使用原来的“加权排序 + 取前 NN 组”
    # 方式2：True  -> 直接指定某一代、某个 ID 的个体
    USE_SPECIFIC_PARAMSET = False  # 这里改 True/False 来切换

    # 只有在 USE_SPECIFIC_PARAMSET = True 时才有效：
    SPECIFIC_GENERATION = 85   # 例如第 10 代
    SPECIFIC_ID = 8            # 例如 id = 5 的个体
    # 需要前多少组参数集,只有当USE_SPECIFIC_PARAMSET = False才有效
    NN = 1
    # tar = ['QG','QI','QS','SBGS']
    tar = ['Q']
    plot_tar_map = {'F':'Inundation Area(km²)','Q':'Discharge(m³/s)'}
    if os.name == 'nt':  # Windows
        base_path = r'G:\program\seims\SEIMS_HAND\data'
    else:  # Linux/Unix
        base_path = '/data/user/xiaodw/software/WISE_V20160219/data'
    subbasin_id = 2
    wtsd_name = "US_2"
    conn = MongoClient('127.0.0.1', 27017)
    longterm_modelDB = wtsd_name + '_longterm_model'
    db = conn[longterm_modelDB]   #需要自己修改数据库名字
    calibration_ini_file = os.path.join(base_path,wtsd_name,'model_configs','calibration.ini')
    HydroClimateDB = wtsd_name + '_HydroClimate'
    plot_legent = False
    label_font_size = 24
    title_font_size = 28
    nse_font_size = 28

    if wtsd_name not in list(DEMO_MODELS.keys()):
        print('%s is not one of the available demo watershed: %s' %
              (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
        exit(-1)

    cur_path = UtilClass.current_path(lambda: 0)
    SEIMS_path = os.path.abspath(cur_path + '../../..')
    model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
    cf = ConfigParser()
    cali_cfg_file = model_paths.cfg_dir + os.path.sep + f'calibration.ini'
    cf.read(cali_cfg_file)

    # 读取率定结果
    ngens = cf.getint('NSGA2', 'generationsnum') if \
        cf.has_option('NSGA2', 'generationsnum') else 1
    npop = cf.getint('NSGA2', 'populationsize') if \
        cf.has_option('NSGA2', 'populationsize') else 1
    filename = model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s/runtime.log' % (ngens, npop)
    df = pd.read_table(filename, sep='\t', skiprows=2)
    df.drop_duplicates(keep='first', inplace=True)
    df = pd.DataFrame(df)
    df = df.drop(df[df['generation'] == 'generation'].index)
    df = df[~df['generation'].astype(str).str.contains('#')]  # 保留数值型 generation
    Ntar = int((len(df.columns) - 3) / 2)
    index_ini = 2
    index_end = int(1 + (len(df.columns) - 3) / 2)
    print("率定目标：", Ntar, (df.columns[index_ini]), (df.columns[index_end]))
    print("       ")

    # ========= 按不同模式选择参数集 =========
    if USE_SPECIFIC_PARAMSET:
        # ---- 模式 2：直接按 generation + id 选出目标个体 ----
        # 注意：runtime.log 一般有 'generation' 和 'id' 两列
        # 如果你的列名不是 'id'，这里记得改成实际列名
        df['generation_int'] = df['generation'].astype(int)
        df['id_int'] = df['calibrationID'].astype(int)

        # mask = (df['generation_int'] == SPECIFIC_GENERATION) & (df['id_int'] == SPECIFIC_ID)
        mask = (df['generation_int'] == SPECIFIC_GENERATION)
        selected_df = df[mask]

        if selected_df.empty:
            print(f"未在 runtime.log 中找到 generation={SPECIFIC_GENERATION} 对应的参数集，请检查配置。")
            return

        newdf = selected_df.reset_index(drop=True)
        print("///// 指定代数 + ID 后的参数集 /////")
        print(newdf)
        print("       ")

    else:
        # ---- 模式 1：原有逻辑——加权排序 + 取前 NN 组 ----
        result = dict()
        for i in range(0, Ntar):
            result[df.columns[index_ini + i]] = list(df[df.columns[index_ini + i]].astype(float))

        new = [0] * len(df)
        temp = []
        for param, values in result.items():
            # 这里相当于计算每个目标的 (1 - value) 的“距离”，再累加
            temp2 = [1 - values[i] for i in range(len(values))]
            temp = np.sqrt([x * y for x, y in zip(temp2, temp2)])
            new = [new[i] + temp[i] for i in range(len(temp))]

        stac = pd.DataFrame(new, columns=['statics'])
        stac['FID'] = range(0, len(df))
        stac['rank'] = stac['statics'].rank(ascending=True)
        newdata = stac['statics'].tolist()
        df['weight'] = newdata
        select = stac.sort_values(by='rank')[0:NN].FID.tolist()

        newdf = pd.DataFrame()
        for i in select:
            newdf = newdf.append(df[i:i + 1])
        newdf = newdf.reset_index(drop=True)
        print("///// 筛选后参数集 /////")
        print(newdf)
        print("       ")

    # ========= 后面读取参数集 + 写入 MongoDB + 跑模型部分保持不变 =========
    # 读取参数集
    # ========= 构建 calibrationID → parameters行号 的映射 =========
    id2row = {}
    for row_idx in range(len(newdf)):
        cid = int(newdf.loc[row_idx, 'calibrationID'])
        id2row[cid] = row_idx
        SPECIFIC_ID = cid
        SPECIFIC_GENERATION = int(newdf.loc[row_idx, 'generation'])

    print("ID → row 映射：", id2row)

    parameters = newdf.gene_values.to_frame()
    parameters = parameters.gene_values.str.split(',', expand=True)
    parameters[1] = parameters[1].astype(str).str.replace(r'\[|\]|,', '')
    parameters[parameters.shape[1] - 1] = parameters[parameters.shape[1] - 1].astype(str).str.replace(r'\[|\]|\ |\)|,', '')
    parameters = parameters.drop([0], axis=1)

    param_range_def = cf.get('CALI_Settings', 'paramrngdef')
    items = read_data_items_from_txt(model_paths.model_dir + os.path.sep + param_range_def)
    names = []
    for item in items:
        if len(item) < 3:
            continue
        names.append(item[0])
    print("率定参数：", names)
    out = dict()

    # parameters 维度：行 = 选中的参数集数 M；列 = 参数个数 P
    # names           长度 = P，对应每一列的参数名
    assert parameters.shape[1] == len(names), "参数个数与 names 数量不一致，请检查"
    # 更新impact
    for j in range(parameters.shape[1]):  # 按“列”循环：每一列是一个参数
        # 这个参数在所有选中参数集中的取值：长度 M
        data = parameters.iloc[:, j].astype(float)
        # 拼成字符串："v1,v2,...,vM"
        s = ",".join(str(v) for v in data.tolist())
        param_name = names[j]
        print(f"import calibration values to mongoDB for parameter {param_name}")
        # 只更新这一条参数记录
        db["PARAMETERS"].find_one_and_update(
            {'NAME': param_name},
            {'$set': {'CALI_VALUES': s}}
        )

    # os.system('python demo_runmodel.py -name %s' % (wtsd_name))
    ### xiaodw,这里要仿照率定的方式，使用指定第n代 第id组参数，重新“率定”模型（仿照率定模型才会读取caliparam_sub.csv和caliparam.csv）
    from calibration.calibrate import Calibration, initialize_calibrations
    from calibration.config import CaliConfig
    # cf, method = get_optimization_config()
    cf = ConfigParser()
    cf.read(calibration_ini_file)
    psa_mtd = 'nsga2'
    cali_cfg = CaliConfig(cf, method=psa_mtd)
    cali_obj = Calibration(cali_cfg)
    df_gen = df[df['generation'].astype(int) == SPECIFIC_GENERATION].copy()

    # 排序（保持 calibrationID 顺序）
    if 'calibrationID' in df_gen.columns:
        df_gen['calibrationID'] = df_gen['calibrationID'].astype(int)
        df_gen = df_gen.sort_values('calibrationID')

    # 仿照率定的方式跑模型
    from run_seims import MainSEIMS
    # 这里要用新的ID了
    cali_obj.ID = id2row[SPECIFIC_ID]
    model_args = cali_obj.model.ConfigDict
    model_args['calibration_id'] = id2row[SPECIFIC_ID]

    model_obj = MainSEIMS(args_dict=model_args)

    # Execute model
    model_obj.SetMongoClient()
    # model_obj.run()

    # 获取模拟数据
    path = model_paths.model_dir + os.path.sep + f'OUTPUT0-{int(id2row[SPECIFIC_ID])}'
    for index, name in enumerate(tar):
        filename = os.path.join(path, f"{name}.txt")

        if name == 'F':
            # ===== F：保持现在的按日期求和逻辑 =====
            temp = pd.read_table(
                filename,
                sep=r"\s+",
                header=None,
                names=['DATE', 'TIME', 'value'],
                engine="python",
                dtype=str
            )

            mask = temp['DATE'].str.match(r'^\d{4}-\d{2}-\d{2}$')
            temp = temp[mask].copy()
            temp['value'] = temp['value'].astype(float)

            # 多 subbasin 时，同一天求和
            temp_group = temp.groupby('DATE', as_index=False)['value'].sum()

            out.setdefault(name, []).append(list(temp_group['value']))

            if index == 0:
                out.setdefault('Date', []).append(list(temp_group['DATE']))

        elif name == 'Q':
            # ===== Q：只读取指定 Subbasin =====
            target_sub = int(subbasin_id)  # 比如 141

            records = []
            current_sub = None

            with open(filename, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue

                    parts = line.split()

                    # 识别 "Subbasin: 141" 行
                    if parts[0] == 'Subbasin:' and len(parts) >= 2:
                        try:
                            current_sub = int(parts[1])
                        except ValueError:
                            current_sub = None
                        continue

                    # 只在当前块为目标 subbasin 时，读日期行
                    if current_sub == target_sub and re.match(r'^\d{4}-\d{2}-\d{2}$', parts[0]):
                        date_str = parts[0]
                        time_str = parts[1] if len(parts) > 1 else "00:00:00"
                        val_str = parts[2] if len(parts) > 2 else "nan"
                        try:
                            val = float(val_str)
                        except ValueError:
                            continue
                        records.append((date_str, time_str, val))

            if not records:
                raise RuntimeError(f"在 {filename} 中没有找到 Subbasin: {target_sub} 的数据")

            temp = pd.DataFrame(records, columns=['DATE', 'TIME', 'value'])

            # Q 不需要再 groupby（每个 date 只有一个值）
            out.setdefault(name, []).append(list(temp['value']))

            if index == 0:
                out.setdefault('Date', []).append(list(temp['DATE']))

        else:
            # 其它变量类型的话，你可以按需要扩展
            raise ValueError(f"暂时不支持的变量类型: {name}")

    # ================== 降雨读取：循环外初始化一次 ==================
    HOSTNAME = cf.get('SEIMS_Model', 'HOSTNAME')
    PORT = int(cf.get('SEIMS_Model', 'PORT'))
    mongoclient = ConnectMongoDB(HOSTNAME, PORT).get_conn()
    # 这里假设你前面已经有 wtsd_name（流域名），如 'Poyang' 之类
    readData = ReadModelData(mongoclient, DEMO_MODELS[wtsd_name])

    # ========= 画图：F 全时段指标；Q 保持率定/验证分开 =========
    for index, name in enumerate(tar):
        # ---------- 1. 构造模拟序列 DataFrame ----------
        newdf = pd.DataFrame()
        newdf['Date'] = [datetime.strptime(x, '%Y-%m-%d') for x in out['Date'][0]]
        for jj in range(len(out[name])):
            newdf[f'{name}_{jj}'] = out[name][jj]
        newdf.set_index('Date', inplace=True)

        # 只跑了一个参数集的话，这里通常只有一列 name_0
        sim_col = f'{name}_0'

        # ============ 根据模拟时间段读取降雨 ============
        start = newdf.index[0]
        end = newdf.index[-1]
        pcp_date_value = readData.Precipitation(0, start, end)
        pcp_date = [v[0] for v in pcp_date_value]
        preci = [v[1] for v in pcp_date_value]

        # ---------- 2. 从 cali/vali Obs JSON 里取观测 ----------
        var_full_name = f'{name}_{subbasin_id}'  # 比如 F_1171 / Q_1171

        cali_obs_data = None
        vali_obs_data = None

        with open(
            model_paths.model_dir + os.path.sep
            + r'CALI_NSGA2_Gen_%s_Pop_%s/simulated_data/gen%s_caliObsData.json'
            % (ngens, npop, SPECIFIC_GENERATION),
            'r', encoding='utf-8'
        ) as f:
            cali_list = json.load(f)

        with open(
            model_paths.model_dir + os.path.sep
            + r'CALI_NSGA2_Gen_%s_Pop_%s/simulated_data/gen%s_valiObsData.json'
            % (ngens, npop, SPECIFIC_GENERATION),
            'r', encoding='utf-8'
        ) as f:
            vali_list = json.load(f)

        for item in cali_list:
            if int(item['Gen']) == SPECIFIC_GENERATION and int(item['ID']) == SPECIFIC_ID:
                cali_obs_data = item.get(var_full_name, None)
                break

        for item in vali_list:
            if int(item['Gen']) == SPECIFIC_GENERATION and int(item['ID']) == SPECIFIC_ID:
                vali_obs_data = item.get(var_full_name, None)
                break

        # ---------- 2.1 先尝试用 JSON 组装 obs_series ----------
        # ---------- 2. 从 cali/vali Obs JSON 里取观测 ----------
        var_full_name = f'{name}_{subbasin_id}'  # 比如 F_1171 / Q_1171

        cali_obs_data = None
        vali_obs_data = None

        with open(
            model_paths.model_dir + os.path.sep
            + r'CALI_NSGA2_Gen_%s_Pop_%s/simulated_data/gen%s_caliObsData.json'
            % (ngens, npop, SPECIFIC_GENERATION),
            'r', encoding='utf-8'
        ) as f:
            cali_list = json.load(f)

        with open(
            model_paths.model_dir + os.path.sep
            + r'CALI_NSGA2_Gen_%s_Pop_%s/simulated_data/gen%s_valiObsData.json'
            % (ngens, npop, SPECIFIC_GENERATION),
            'r', encoding='utf-8'
        ) as f:
            vali_list = json.load(f)

        for item in cali_list:
            if int(item['Gen']) == SPECIFIC_GENERATION and int(item['ID']) == SPECIFIC_ID:
                cali_obs_data = item.get(var_full_name, None)
                break

        for item in vali_list:
            if int(item['Gen']) == SPECIFIC_GENERATION and int(item['ID']) == SPECIFIC_ID:
                vali_obs_data = item.get(var_full_name, None)
                break

        # ========== 先尝试 JSON，没有再从 MongoDB.MEASUREMENT 读取 ==========
        obs_series = None

        if cali_obs_data is not None and vali_obs_data is not None:
            # --- 用 JSON ---
            obs_dates = cali_obs_data['UTCDATETIME'][:] + vali_obs_data['UTCDATETIME'][:]
            obs_vals = cali_obs_data['Obs'][:] + vali_obs_data['Obs'][:]
            obs_dt = [StringClass.get_datetime(s) for s in obs_dates]
            obs_series = pd.Series(obs_vals, index=obs_dt, dtype='float64', name='Obs')
        else:
            print(f"JSON 中没有找到 {var_full_name} 的观测数据，尝试从 MongoDB.MEASUREMENT 读取……")

            # start / end 前面已经有：start = newdf.index[0]; end = newdf.index[-1]
            # 根据你之前的结构：mongoclient = ConnectMongoDB(...).get_conn()
            # DEMO_MODELS[wtsd_name] 是当前模型所在的数据库名
            db = mongoclient[HydroClimateDB]
            coll = db['MEASUREMENT']

            # start / end 原来就是 pandas 的 datetime（无时区），按 UTC 处理就行
            from datetime import timezone
            start_utc = start.replace(tzinfo=timezone.utc)
            end_utc = end.replace(tzinfo=timezone.utc)

            query = {
                "STATIONID": int(subbasin_id),  # 1171 之类
                "TYPE": name,  # 'Q' 或 'F'
                "UTCDATETIME": {"$gte": start_utc, "$lte": end_utc}
            }
            proj = {"_id": 0, "UTCDATETIME": 1, "VALUE": 1}

            docs = list(coll.find(query, proj).sort("UTCDATETIME", 1))

            if docs:
                obs_dt_raw = [d["UTCDATETIME"] for d in docs]

                # 如果你希望跟 newdf.index 一样是“无时区”的 datetime，可以去掉 tzinfo
                obs_dt = [
                    dt.replace(tzinfo=None) if isinstance(dt, datetime) else StringClass.get_datetime(str(dt))
                    for dt in obs_dt_raw
                ]

                obs_vals = [float(d["VALUE"]) for d in docs]
                obs_series = pd.Series(obs_vals, index=obs_dt, dtype='float64', name='Obs')
                print(f"从 MongoDB.MEASUREMENT 读取到 {len(obs_series)} 条 {var_full_name} 观测数据。")
            else:
                print(f"MongoDB.MEASUREMENT 中也没有 {var_full_name} 的观测数据，将只画模拟曲线。")
                obs_series = None

        # ---------- 3. 合并 Obs 和模拟，算指标 ----------
        etime = StringClass.get_datetime(cf.get('CALI_Settings', 'cali_time_end'))

        metrics_cali = None
        metrics_vali = None
        metrics_whole = None

        if obs_series is not None:
            merged = pd.concat(
                [obs_series, newdf[sim_col].rename('Sim')],
                axis=1
            ).dropna()

            if name == 'Q':
                # === Q：率定/验证分开 ===
                cali_vals = merged[merged.index <= etime]
                vali_vals = merged[merged.index > etime]

                metrics_cali = {}
                metrics_vali = {}

                metrics_cali["NSE"] = nashsutcliffe(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["NSE"] = nashsutcliffe(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["logNSE"] = lognashsutcliffe(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["logNSE"] = lognashsutcliffe(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["KGE"] = kge(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["KGE"] = kge(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["Rsquare"] = rsquared(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["Rsquare"] = rsquared(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["pbias"] = pbias(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["pbias"] = pbias(vali_vals['Obs'], vali_vals['Sim'])
            else:
                # === F：全时段一个指标 ===
                metrics_whole = {}
                metrics_whole["NSE"] = nashsutcliffe(merged['Obs'], merged['Sim'])
                metrics_whole["logNSE"] = lognashsutcliffe(merged['Obs'], merged['Sim'])
                metrics_whole["KGE"] = kge(merged['Obs'], merged['Sim'])
                metrics_whole["Rsquare"] = rsquared(merged['Obs'], merged['Sim'])
                metrics_whole["pbias"] = pbias(merged['Obs'], merged['Sim'])

        # ---------- 3. 合并 Obs 和模拟，算指标 ----------
        etime = StringClass.get_datetime(cf.get('CALI_Settings', 'cali_time_end'))

        metrics_cali = None
        metrics_vali = None
        metrics_whole = None

        if obs_series is not None:
            merged = pd.concat(
                [obs_series, newdf[sim_col].rename('Sim')],
                axis=1
            ).dropna()

            if name == 'Q':
                # === Q：率定/验证分开 ===
                cali_vals = merged[merged.index <= etime]
                vali_vals = merged[merged.index > etime]

                metrics_cali = {}
                metrics_vali = {}

                metrics_cali["NSE"] = nashsutcliffe(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["NSE"] = nashsutcliffe(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["logNSE"] = lognashsutcliffe(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["logNSE"] = lognashsutcliffe(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["KGE"] = kge(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["KGE"] = kge(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["Rsquare"] = rsquared(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["Rsquare"] = rsquared(vali_vals['Obs'], vali_vals['Sim'])

                metrics_cali["pbias"] = pbias(cali_vals['Obs'], cali_vals['Sim'])
                metrics_vali["pbias"] = pbias(vali_vals['Obs'], vali_vals['Sim'])
            else:
                # === F：全时段一个指标 ===
                metrics_whole = {}
                metrics_whole["NSE"] = nashsutcliffe(merged['Obs'], merged['Sim'])
                metrics_whole["logNSE"] = lognashsutcliffe(merged['Obs'], merged['Sim'])
                metrics_whole["KGE"] = kge(merged['Obs'], merged['Sim'])
                metrics_whole["Rsquare"] = rsquared(merged['Obs'], merged['Sim'])
                metrics_whole["pbias"] = pbias(merged['Obs'], merged['Sim'])

            # ---------- 4. 画图（SCI 风格） ----------
            fig, ax = plt.subplots(1, 1, figsize=(22, 8), dpi=300)

            # 统一：去掉顶部和右侧边框，更“期刊范”
            ax.spines['top'].set_visible(False)
            ax.spines['right'].set_visible(False)

            # 4.1 观测：用黑色小圆点
            if obs_series is not None:
                ax.scatter(
                    x=obs_series.index,
                    y=obs_series.values,
                    label='Observation',
                    color='k',
                    s=10,
                    alpha=0.7,
                    zorder=3
                )

            # 4.2 模拟：红色实线，略粗一点
            ax.plot(
                newdf.index,
                newdf[sim_col],
                label='Simulation',
                color='#C82423',
                linewidth=1.6,
                zorder=4
            )

            # Q 画率定/验证分界线，F 不画
            if name == 'Q':
                ax.axvline(
                    etime,
                    color='k',
                    linestyle='--',
                    linewidth=1.0,
                    alpha=0.8
                )

            # ========== 在上方画降雨（倒 Y 轴） ==========
            ax2 = ax.twinx()  # 共享 x 轴
            ax2.spines['top'].set_visible(False)

            # 降雨柱状图：淡蓝色、半透明，放在背景层
            if len(preci) > 0:
                p3 = ax2.bar(
                    pcp_date,
                    preci,
                    label='Precipitation',
                    linewidth=0,
                    align='center',
                    color='blue',
                    zorder=1
                )

                # 倒 Y 轴：上小下大
                pmax = float(max(preci))
                pmin = float(min(preci))
                ax2.set_ylim(pmax * 2.5, pmin * 0.0)

            ax2.set_ylabel('Precipitation (mm)', fontsize=title_font_size,fontweight='bold')
            ax2.tick_params('y', length=4, width=1, labelsize=title_font_size)
            ax2.grid(False)

            # 4.3 指标文字：左侧 Calibration，右侧 Validation
            if metrics_cali is not None and metrics_vali is not None:
                # metric_names = ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]
                metric_names = ["KGE"]
                # ---- 大标题：Calibration / Validation（类似图2，加粗一点）----
                ax.text(
                    0.20, 0.86, "Calibration Period",
                    fontsize=label_font_size, fontweight='bold',
                    transform=ax.transAxes,
                    ha='center'
                )
                ax.text(
                    0.78, 0.86, "Validation Period",
                    fontsize=label_font_size, fontweight='bold',
                    transform=ax.transAxes,
                    ha='center'
                )

                # ---- 下面两列是各自的评价指标（略小一号）----
                y0 = 0.75  # 起始高度
                dy = 0.06  # 行距

                for idx_m, m in enumerate(metric_names):
                    y_pos = y0 - idx_m * dy

                    # 左侧：Calibration 指标
                    ax.text(
                        0.20, y_pos,
                        "{0:<7}= {1:6.3f}".format(m, metrics_cali[m]),
                        fontsize=nse_font_size,
                        fontweight='bold',
                        transform=ax.transAxes,
                        ha='center',
                        color='black'
                    )

                    # 右侧：Validation 指标
                    ax.text(
                        0.78, y_pos,
                        "{0:<7}= {1:6.3f}".format(m, metrics_vali[m]),
                        fontsize=nse_font_size,
                        fontweight='bold',
                        transform=ax.transAxes,
                        ha='center',
                        color='black'
                    )

            elif metrics_whole is not None:
                # metric_names = ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]
                metric_names = [ "KGE"]

                # --- 定义整列的中心位置 ---
                x_center = 0.48  # 正中央

                # --- 大标题（居中） ---
                # ax.text(
                #     x_center, 0.88, "Full period",
                #     fontsize=15, fontweight='bold',
                #     transform=ax.transAxes,
                #     ha='center'
                # )

                # --- 指标组：字体与 Q 图一致（13号，居中对齐） ---
                y0 = 0.78  # 起始高度
                dy = 0.045  # 行距，与 Q 一致

                for idx_m, m in enumerate(metric_names):
                    y = y0 - idx_m * dy
                    ax.text(
                        x_center, y,
                        f"{m:<7}= {metrics_whole[m]:6.3f}",
                        fontsize=nse_font_size,
                        fontweight='bold',
                        transform=ax.transAxes,
                        ha='center',  # 小文字也严格居中
                        color='black'
                    )

            # 4.4 y 轴范围（流量 / 淹没面积）
            sim_max = np.nanmax(newdf[sim_col])
            if obs_series is not None:
                obs_max = np.nanmax(obs_series)
                maxy = max(sim_max, obs_max) * 1.3
                min_y = 0
            else:
                maxy = sim_max * 1.3
                min_y = 0
            ax.set_ylim(min_y, maxy)

            # X 轴日期格式、网格
            ax.tick_params('both', length=5, width=1.5, which='major', labelsize=title_font_size)
            ax.grid(alpha=0.25, linestyle='--', linewidth=0.8, axis='y')

            ax.set_ylabel('%s' % plot_tar_map[name], fontsize=title_font_size,fontweight='bold')

            # ========== legend：合并主轴 + 降雨 ==========
            handles1, labels1 = ax.get_legend_handles_labels()
            handles2, labels2 = ax2.get_legend_handles_labels()
            handles = handles1 + handles2
            labels = labels1 + labels2
            if plot_legent:
            # 图例放上边居中，稍微缩小一些
                ax.legend(
                    handles,
                    labels,
                    frameon=False,
                    fontsize=11,
                    bbox_to_anchor=(0., 1.02, 1., 0.102),
                    borderaxespad=0.,
                    ncol=3,
                    loc='lower left'
                )

            plt.tight_layout()
            plt.savefig(
                model_paths.model_dir + os.path.sep
                + r'CALI_NSGA2_Gen_%s_Pop_%s/%s.png' % (ngens, npop, f'{name}_{subbasin_id}'),
                dpi=300
            )
            print(
                model_paths.model_dir + os.path.sep
                + r'CALI_NSGA2_Gen_%s_Pop_%s/%s.png' % (ngens, npop, f'{name}_{subbasin_id}')
            )
            print("///////查看结果///////")



if __name__ == "__main__":
    main()
