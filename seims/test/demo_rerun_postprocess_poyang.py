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


def main():
    # ========= 新增：选择参数集的方式 =========
    # 方式1：False -> 使用原来的“加权排序 + 取前 NN 组”
    # 方式2：True  -> 直接指定某一代、某个 ID 的个体
    USE_SPECIFIC_PARAMSET = True  # 这里改 True/False 来切换

    # 只有在 USE_SPECIFIC_PARAMSET = True 时才有效：
    SPECIFIC_GENERATION = 93   # 例如第 10 代
    SPECIFIC_ID = 7            # 例如 id = 5 的个体
    # 需要前多少组参数集,只有当USE_SPECIFIC_PARAMSET = False才有效
    NN = 1
    # tar = ['QG','QI','QS','SBGS']
    tar = ['F','Q']
    watershed_num = 1171
    conn = MongoClient('127.0.0.1', 27017)
    db = conn.poyang_lake1_longterm_model_1171   #需要自己修改数据库名字

    wtsd_name = "poyang_lake1"
    if wtsd_name not in list(DEMO_MODELS.keys()):
        print('%s is not one of the available demo watershed: %s' %
              (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
        exit(-1)

    cur_path = UtilClass.current_path(lambda: 0)
    SEIMS_path = os.path.abspath(cur_path + '../../..')
    model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
    cf = ConfigParser()
    cali_cfg_file = model_paths.cfg_dir + os.path.sep + f'calibration_{watershed_num}.ini'
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
    from calibration.config import CaliConfig, get_optimization_config
    cf, method = get_optimization_config()
    cali_cfg = CaliConfig(cf, method=method)
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
    # 获取模拟数据
    path = model_paths.model_dir + os.path.sep + f'OUTPUT0-{int(id2row[SPECIFIC_ID])}'

    for index, name in enumerate(tar):
        filename = os.path.join(path, f"{name}.txt")

        # 1. 读整个文件（不再用 skiprows，让 Subbasin: 也读进来）
        temp = pd.read_table(
            filename,
            sep=r"\s+",
            header=None,
            names=['DATE', 'TIME', 'value'],
            engine="python",
            dtype=str  # 先都读成字符串，方便过滤
        )

        # 2. 只保留形如 2010-01-01 的日期行，自动丢掉 "Subbasin:" 之类的
        mask = temp['DATE'].str.match(r'^\d{4}-\d{2}-\d{2}$')
        temp = temp[mask].copy()

        # 3. 数值转成 float
        temp['value'] = temp['value'].astype(float)

        # 4. 关键：按 DATE 聚合求和（多 Subbasin 时，把同一天相加）
        temp_group = temp.groupby('DATE', as_index=False)['value'].sum()

        # 5. 把聚合后的结果塞进 out
        out.setdefault(name, []).append(list(temp_group['value']))

        # 只第一次循环时记录日期序列
        if index == 0:
            out.setdefault('Date', []).append(list(temp_group['DATE']))

    # ========= 画图：加入观测值 + 率定/验证指标 =========
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

        # ---------- 2. 从 cali/vali Obs JSON 里取观测 ----------
        var_full_name = f'{name}_{watershed_num}'  # 比如 F_1171 / Q_1171

        # 观测值与模拟（率定+验证期）
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

        if cali_obs_data is None or vali_obs_data is None:
            print(f"没有在 Obs JSON 中找到 {var_full_name} 的观测数据，{name} 只画模拟。")
            obs_series = None
        else:
            obs_dates = cali_obs_data['UTCDATETIME'][:] + vali_obs_data['UTCDATETIME'][:]
            obs_vals = cali_obs_data['Obs'][:] + vali_obs_data['Obs'][:]
            obs_dt = [StringClass.get_datetime(s) for s in obs_dates]
            obs_series = pd.Series(obs_vals, index=obs_dt, dtype='float64', name='Obs')

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

        # ---------- 4. 画图 ----------
        fig, ax = plt.subplots(1, 1, figsize=(22, 8), dpi=100)

        # 4.1 观测
        if obs_series is not None:
            ax.scatter(
                x=obs_series.index,
                y=obs_series.values,
                label='Observation',
                color='#6F6F6F',
                s=13
            )

        # 4.2 模拟
        ax.plot(
            newdf.index,
            newdf[sim_col],
            label='Simulation',
            color='#C82423'
        )

        # 分位数带（以后有多条曲线时也适用）
        low_CI_bound = newdf.quantile(0.1, axis=1)
        high_CI_bound = newdf.quantile(0.9, axis=1)
        x = newdf.index
        ax.fill_between(
            x,
            low_CI_bound,
            high_CI_bound,
            linewidth=1,
            color='#F8AC8C',
            label='10th - 90th percentile'
        )

        # Q 画率定/验证分界线，F 不画
        if name == 'Q':
            plt.axvline(etime, c='#000000', ls='--', lw=1)

        # 4.3 指标文字
        if metrics_cali is not None and metrics_vali is not None:
            # Q：左侧 Cali，右侧 Vali
            metric_names = ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]
            for idx_m, m in enumerate(metric_names):
                y_pos = 0.80 - idx_m * 0.04
                ax.text(
                    0.15, y_pos,
                    "Cali %s = %.3f" % (m, metrics_cali[m]),
                    fontsize=13,
                    transform=ax.transAxes
                )
                ax.text(
                    0.55, y_pos,
                    "Vali %s = %.3f" % (m, metrics_vali[m]),
                    fontsize=13,
                    transform=ax.transAxes
                )
        elif metrics_whole is not None:
            # F：只写一列全时段指标
            metric_names = ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]
            for idx_m, m in enumerate(metric_names):
                y_pos = 0.80 - idx_m * 0.04
                ax.text(
                    0.20, y_pos,
                    "%s = %.3f" % (m, metrics_whole[m]),
                    fontsize=13,
                    transform=ax.transAxes
                )

        # 4.4 y 轴范围
        sim_max = np.nanmax(newdf[sim_col])
        if obs_series is not None:
            obs_max = np.nanmax(obs_series)
            maxy = max(sim_max, obs_max) * 1.8
        else:
            maxy = sim_max * 1.8
        ax.set_ylim(0, maxy)

        ax.legend(
            frameon=False,
            fontsize=14,
            bbox_to_anchor=(0., 1.02, 1., 0.102),
            borderaxespad=0.,
            ncol=3,
            loc='lower left',
            fancybox=True
        )

        ax.set_ylabel('%s' % name, fontsize=15)
        ax.tick_params('both', length=5, width=2, which='major', labelsize=15)

        plt.tight_layout()
        plt.savefig(
            model_paths.model_dir + os.path.sep
            + r'CALI_NSGA2_Gen_%s_Pop_%s/%s.png' % (ngens, npop, name),
            dpi=300
        )
        print(
            model_paths.model_dir + os.path.sep
            + r'CALI_NSGA2_Gen_%s_Pop_%s/%s.png' % (ngens, npop, name)
        )
        print("///////查看结果///////")


if __name__ == "__main__":
    main()
