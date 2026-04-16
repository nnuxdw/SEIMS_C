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
from spotpy.objectivefunctions import nashsutcliffe, rsquared, rmse, pbias,kge, lognashsutcliffe,rsr,mae
from preprocess.db_read_model import ReadModelData
import matplotlib as mpl
import matplotlib.dates as mdates
from matplotlib.ticker import NullFormatter, FuncFormatter

def main(watershed_num):
    NN = 1  #可调，需要前多少组参数集
    plot_mode = 'combined'  # combined/separate
    plot_percentile = False
    plot_legent = False
    save_legend_as_png = False
    model_name = f'US_15_longterm_model'
    y_label_map={
        "Q_1":"Discharge(m³/s)"
    }
    label_font_size = 24
    title_font_size = 28
    nse_font_size = 30
    wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')
    if wtsd_name not in list(DEMO_MODELS.keys()):
        print('%s is not one of the available demo watershed: %s' %
              (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
        exit(-1)

    cur_path = UtilClass.current_path(lambda: 0)
    SEIMS_path = os.path.abspath(cur_path + '../../..')
    # model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
    model_paths = ModelPaths(SEIMS_path, wtsd_name, model_name)
    cf = ConfigParser()
    cali_cfg_file = model_paths.cfg_dir + os.path.sep + f'calibration.ini'
    cf.read(cali_cfg_file)

    #读取率定结果
    ngens = cf.getint('NSGA2', 'generationsnum') if \
            cf.has_option('NSGA2', 'generationsnum') else 1
    npop = cf.getint('NSGA2', 'populationsize') if \
            cf.has_option('NSGA2', 'populationsize') else 1
    filename = model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\runtime.log'%(ngens,npop)
    df = pd.read_table(filename,sep='	', skiprows=2)
    df.drop_duplicates(keep='first',inplace=True)
    df = pd.DataFrame(df)
    df = df.drop(df[df['generation']=='generation'].index)
    Ntar = int((len(df.columns) - 3)/2)
    index_ini = 2
    index_end = int(1 + (len(df.columns) - 3)/2)
    df = df[~df['generation'].str.contains(('#'))]
    print("率定目标：",Ntar,(df.columns[index_ini]),(df.columns[index_end]))
    print("       ")

    #计算加权排序
    result = dict()
    for i in range(0,Ntar):
        result[df.columns[index_ini+i]] = list(df[df.columns[index_ini+i]].astype(float))

    new = [0] *len(df)
    temp = []
    weights = {
        "Cali-Q_1-NSE": 0.5,  # 默认
        "Vali-Q_1-NSE": 0.5  # 默认
    }
    for param, values in result.items():
        w = weights.get(param, 1.0)  # 没设置的默认1.0
        err = [(1 - v) for v in values]  # 目标越接近1越好
        weighted_err = [abs(e) * w for e in err]
        # temp = np.sqrt([x*y for x,y in zip(temp2,temp2)])
        new = [new[i] + weighted_err[i] for i in range(len(err))]

    stac = pd.DataFrame(new,columns=['statics'])
    stac['FID'] = range(0,len(df))
    stac['rank'] = stac['statics'].rank(ascending=True)
    newdata = stac['statics'].tolist()
    df['weight'] = newdata
    select = stac.sort_values(by='rank')[0:NN].FID.tolist()

    newdf = pd.DataFrame()
    temp  = pd.DataFrame()
    for i in (select):
        newdf = newdf.append(df[i:i+1])
    newdf = newdf.reset_index(drop=True)
    print("/////筛选后参数集/////")
    print(newdf)
    print("       ")
    newdf.to_csv(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\result_%s.csv'%(ngens,npop,NN),index=False)
    with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen0_caliObsData.json', 'r', encoding='utf-8') as f:
            sim_obs_data = json.load(f)
    gen_selec = newdf['generation'].astype(int).tolist()
    ID_selec = newdf['calibrationID'].astype(int).tolist()

    var_name = sim_obs_data[0]['var_name']
    obssim_dict = {}  # 用来保存每个变量对应的 obssim（Obs + 多个 Obssim_*_*）
    sims_dict = {}  # 新增：保存每个变量对应的 sims（从 pickle 读出来的完整模拟）
    for nn, kk in enumerate(var_name):
        obssim = pd.DataFrame()
        sims = pd.DataFrame()
        for index,genID in enumerate(gen_selec):
            gens = gen_selec[index]
            IDs = ID_selec[index]
            cali_obs_data = list()
            vali_obs_data = list()

            with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_caliObsData.json'%gens, 'r', encoding='utf-8') as f:
                sim_obs_data = json.load(f)
            with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_caliSimData.pickle'%gens, 'rb') as f:
                sim_data = pickle.load(f)
            # validation data
            with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_valiObsData.json'%gens, 'r', encoding='utf-8') as f:
                vali_sim_obs_data = json.load(f)
            with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_valiSimData.pickle'%gens, 'rb') as f:
                vali_sim_data = pickle.load(f)

            for a, b, c, d in zip(sim_obs_data, sim_data, vali_sim_obs_data, vali_sim_data):
                if(a['Gen'] in [gens]):
                    if a['ID'] in [IDs]:
                        cali_obs_data.append(a)
                        vali_obs_data.append(c)
                    Date = []
                    value = []
                    for k,v in b.items():
                        Date.append(k)
                        value.append(float(v[nn]))
                    for k,v in d.items():
                        Date.append(k)
                        value.append(float(v[nn]))
                    sims['Date'] =list(Date)
                    sims['sim_%s_%s'%(nn,index)]  = value
            if(index==0):
                obssim['Date'] =list(cali_obs_data[0]['%s'%kk]['UTCDATETIME'][:]) + list(vali_obs_data[0]['%s'%kk]['UTCDATETIME'][:])
                obssim['Obs'] =list(cali_obs_data[0]['%s'%kk]['Obs'][:]) + list(vali_obs_data[0]['%s'%kk]['Obs'][:])
            obssim['Obssim_%s_%s'%(nn,index)] = list(cali_obs_data[0]['%s'%kk]['Sim'][:]) + list(vali_obs_data[0]['%s'%kk]['Sim'][:])

        obssim['Date'] = [StringClass.get_datetime(s) for s in obssim['Date']]
        obssim = obssim.set_index('Date')
        sims = sims.set_index('Date')
        print()
        print("///%s 模拟结果提取结束///"%kk)
        output = pd.concat([sims,obssim['Obs']], axis=1)
        output.to_csv(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.csv'%(ngens,npop,kk))

        #画图
        fig, ax =plt.subplots(1,1, figsize=(22,8), dpi=100)
        ax = plt.subplot(1,1,1)
        p1 = ax.scatter(x=obssim.index,y=obssim.Obs, label='Observation', color='#6F6F6F', s=13)
        p2 = ax.plot(sims.index,np.median(sims,1), label='Simulation', color='#C82423')
        if plot_percentile:
            low_CI_bound= sims.quantile(0.1,axis=1)
            high_CI_bound= sims.quantile(0.9,axis=1)
            print( low_CI_bound, high_CI_bound)
            x=sims.index
            ax.fill_between(x, low_CI_bound, high_CI_bound, linewidth=1,color='#F8AC8C',label='10th - 90th percentile')

        HOSTNAME = cf.get('SEIMS_Model', 'HOSTNAME')
        PORT = int(cf.get('SEIMS_Model', 'PORT'))
        mongoclient = ConnectMongoDB(HOSTNAME, PORT).get_conn()
        readData = ReadModelData(mongoclient, DEMO_MODELS[wtsd_name])
        start = sims.index[0]
        end = sims.index[-1]
        pcp_date_value = readData.Precipitation(watershed_num, start, end)
        pcp_date = [v[0] for v in pcp_date_value]
        preci = [v[1] for v in pcp_date_value]
        ax2 = ax.twinx()
        p3 = ax2.bar(pcp_date, preci, label='Precipitation', color='blue', linewidth=0,
                         align='center')
        ax2.set_ylim(float(max(preci)) * 4, float(min(preci)) * 0.8)

        #计算评价系数
        etime = StringClass.get_datetime(cf.get('CALI_Settings', 'cali_time_end'))
        sim_Cali = obssim[obssim.index<=etime]
        sim_Vali = obssim[obssim.index>etime]
        cali_dict = {}
        vali_dict = {}

        for i in range(0,obssim.shape[1]-1):
            for index,name in enumerate(["NSE","logNSE","KGE","Rsquare","pbias"]):
                if(index ==0):
                    cali_dict.setdefault(name, []).append(nashsutcliffe(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
                    vali_dict.setdefault(name, []).append(nashsutcliffe(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
                if(index ==1):
                    cali_dict.setdefault(name, []).append(lognashsutcliffe(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
                    vali_dict.setdefault(name, []).append(lognashsutcliffe(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
                if(index ==2):
                    cali_dict.setdefault(name, []).append(kge(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
                    vali_dict.setdefault(name, []).append(kge(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
                if(index ==3):
                    cali_dict.setdefault(name, []).append(rsquared(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
                    vali_dict.setdefault(name, []).append(rsquared(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
                if(index ==4):
                    cali_dict.setdefault(name, []).append(pbias(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
                    vali_dict.setdefault(name, []).append(pbias(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))

        for index,name in enumerate(["KGE"]):
        # for index, name in enumerate(["NSE", "logNSE", "KGE", "Rsquare", "pbias"]):
            ax.text(0.16, 0.75-index*0.04, '%s=%0.2f(%0.2f,%0.2f)'%(name,np.mean(cali_dict[name]),np.min(cali_dict[name]),np.max(cali_dict[name])), fontsize=16,transform = ax.transAxes)
            ax.text(0.66, 0.75-index*0.04, '%s=%0.2f(%0.2f,%0.2f)'%(name,np.mean(vali_dict[name]),np.min(vali_dict[name]),np.max(vali_dict[name])), fontsize=16,transform = ax.transAxes)
        maxy = np.max([np.max(np.max(sims,1))*1.8,np.max(np.max(obssim['Obs']))*1.8])
        ax.set_ylim(0,maxy)

        plt.axvline(etime, c='#000000', ls='--', lw=1)
        ax.legend(frameon=False, fontsize=14, bbox_to_anchor=(0., 1.02, 1., 0.102),borderaxespad=0.,ncol=3,loc='lower left', fancybox=True)
        ax2.legend(frameon=False, fontsize=14, bbox_to_anchor=(0., 1.02, 1., 0.102),borderaxespad=0.,ncol=1,loc='lower right', fancybox=True)

        ax.set_ylabel('%s'%kk, fontsize=label_font_size)
        ax.tick_params('both', length=5, width=2, which='major',labelsize=label_font_size)
        # major：按年打刻度，并显示年份标签
        ax.xaxis.set_major_locator(mdates.YearLocator())
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%Y'))

        # minor：按月打刻度，但不显示标签
        ax.xaxis.set_minor_locator(mdates.MonthLocator())

        def june_only(x, pos):
            dt = mdates.num2date(x)
            return '6' if dt.month == 6 else ''

        ax.xaxis.set_minor_formatter(FuncFormatter(june_only))
        # 让所有月份的小刻度线更粗、更长
        ax.tick_params(
            axis='x',
            which='minor',
            length=6,  # 默认 4，可调大
            width=1.5,  # 默认 0.8，可调粗
        )

        plt.tight_layout()

        plt.savefig(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,kk), dpi=300)
        print(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,kk))
        print("///////查看结果///////")
        # 把这个变量的 Obs + 所有参数集模拟保存起来，后面多子图要用
        sims_dict[kk] = sims.copy()
        obssim_dict[kk] = obssim.copy()
        #### xiaodw add, 把前NN组的图都画出来
        # ========= 新增：为每一组入选的参数单独画一张图（F和Q分别画） =========
        if plot_mode == 'separate':
            for i in range(0, obssim.shape[1] - 1):  # 去掉 Obs 列，剩下的每一列是一个参数集
                sim_col_name = 'Obssim_%s_%s' % (nn, i)

                # ====== 划分率定期 / 验证期 ======
                cali_vals = obssim[obssim.index <= etime]
                vali_vals = obssim[obssim.index > etime]

                # ----- 计算该参数集的五个评价指标：Cali & Vali 各一套 -----
                metrics_cali = {}
                metrics_vali = {}

                if len(cali_vals) > 0:
                    metrics_cali["NSE"] = nashsutcliffe(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_cali["logNSE"] = lognashsutcliffe(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_cali["KGE"] = kge(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_cali["Rsquare"] = rsquared(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_cali["pbias"] = pbias(cali_vals['Obs'], cali_vals[sim_col_name])
                else:
                    metrics_cali = {k: np.nan for k in ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]}

                if len(vali_vals) > 0:
                    metrics_vali["NSE"] = nashsutcliffe(vali_vals['Obs'], vali_vals[sim_col_name])
                    metrics_vali["logNSE"] = lognashsutcliffe(vali_vals['Obs'], vali_vals[sim_col_name])
                    metrics_vali["KGE"] = kge(vali_vals['Obs'], vali_vals[sim_col_name])
                    metrics_vali["Rsquare"] = rsquared(vali_vals['Obs'], vali_vals[sim_col_name])
                    metrics_vali["pbias"] = pbias(vali_vals['Obs'], vali_vals[sim_col_name])
                else:
                    metrics_vali = {k: np.nan for k in ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]}

                # ---------- 开始绘图（SCI 风格） ----------
                fig_i, ax_i = plt.subplots(1, 1, figsize=(22, 8), dpi=300)

                # 去掉顶部和右侧边框
                ax_i.spines['top'].set_visible(False)
                ax_i.spines['right'].set_visible(False)

                # 观测：黑色小圆点
                ax_i.scatter(
                    x=obssim.index,
                    y=obssim['Obs'],
                    label='Observation',
                    color='k',
                    s=10,
                    alpha=0.7,
                    zorder=3
                )

                # 当前第 i 组参数的模拟曲线：红色实线
                ax_i.plot(
                    obssim.index,
                    obssim[sim_col_name],
                    label='Simulation',
                    color='#C82423',
                    linewidth=1.6,
                    zorder=4
                )

                # 降雨柱（复用前面算好的 pcp_date / preci，倒 y 轴）
                ax_i2 = ax_i.twinx()
                ax_i2.spines['top'].set_visible(False)

                if len(preci) > 0:
                    ax_i2.bar(
                        pcp_date,
                        preci,
                        label='Precipitation',
                        color='blue',
                        # alpha=0.35,
                        linewidth=0,
                        align='center',
                        zorder=1
                    )
                    pmax = float(max(preci))
                    pmin = float(min(preci))
                    ax_i2.set_ylim(pmax * 2.5, pmin * 0.0)

                ax_i2.set_ylabel('Precipitation (mm)', fontsize=label_font_size, fontweight='bold')
                ax_i2.tick_params('y', length=4, width=1, labelsize=label_font_size)
                ax_i2.grid(False)

                # 率定/验证分界线
                ax_i.axvline(etime, c='k', ls='--', lw=1.0, alpha=0.8)

                # -------- 上方 Calibration / Validation 大字 --------
                ax_i.text(
                    0.20, 0.86, "Calibration Period",
                    fontsize=title_font_size, fontweight='bold',
                    transform=ax_i.transAxes,fontstyle='italic',
                    ha='center'
                )
                ax_i.text(
                    0.78, 0.86, "Validation Period",
                    fontsize=title_font_size, fontweight='bold',
                    transform=ax_i.transAxes, fontstyle='italic',
                    ha='center'
                )

                # -------- 在图上标注两列指标：左 Cali，右 Vali --------
                # metric_names = ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]
                metric_names = [ "KGE"]
                y0 = 0.75
                dy = 0.06

                for idx, m in enumerate(metric_names):
                    y_pos = y0 - idx * dy

                    # 左：Calibration
                    ax_i.text(
                        0.20,
                        y_pos,
                        "%s = %.3f" % (m, metrics_cali[m]),
                        fontsize=nse_font_size,
                        fontweight='bold',
                        transform=ax_i.transAxes,
                        ha='center',
                        color='black'
                    )

                    # 右：Validation
                    ax_i.text(
                        0.78,
                        y_pos,
                        "%s = %.3f" % (m, metrics_vali[m]),
                        fontsize=nse_font_size,
                        fontweight='bold',
                        transform=ax_i.transAxes,
                        ha='center',
                        color='black'
                    )

                # y 轴范围
                maxy_i = np.max([
                    np.nanmax(obssim[sim_col_name]) * 1.15,
                    np.nanmax(obssim['Obs']) * 1.15
                ])
                ax_i.set_ylim(0, maxy_i)

                # 坐标轴格式
                ax_i.set_ylabel('%s' % y_label_map[kk], fontsize=label_font_size, fontweight='bold')
                ax_i.tick_params('both', length=5, width=1.5, which='major', labelsize=label_font_size)
                ax_i.xaxis.set_major_locator(mdates.YearLocator())
                ax_i.xaxis.set_major_formatter(mdates.DateFormatter('%Y'))
                ax_i.xaxis.set_minor_locator(mdates.MonthLocator())

                def june_only(x, pos):
                    dt = mdates.num2date(x)
                    if dt.month == 6:
                        return '6'
                    return ''

                ax_i.xaxis.set_minor_formatter(FuncFormatter(june_only))

                # 网格（只画 y 方向的虚线）
                ax_i.grid(alpha=0.25, linestyle='--', linewidth=0.8, axis='y')

                # 图例：主轴 + 降雨一起
                h1, l1 = ax_i.get_legend_handles_labels()
                h2, l2 = ax_i2.get_legend_handles_labels()
                handles = h1 + h2
                labels = l1 + l2
                if plot_legent:
                    ax_i.legend(
                        handles,
                        labels,
                        frameon=False,
                        fontsize=11,
                        bbox_to_anchor=(0., 1.02, 1., 0.102),
                        borderaxespad=0.,
                        ncol=3,
                        loc='lower left'
                    )
                if save_legend_as_png:
                    legend = ax_i.legend(
                        handles,
                        labels,
                        frameon=False,
                        fontsize=11,
                        bbox_to_anchor=(0., 1.02, 1., 0.102),
                        borderaxespad=0.,
                        ncol=3,
                        loc='lower left'
                    )
                    # ====== 额外：把 legend 单独保存成一张 PNG ======
                    fig_legend = legend.figure

                    # 新建一个 figure 仅放 legend
                    fig_leg = plt.figure(figsize=(6, 1.2), dpi=300)
                    ax_leg = fig_leg.add_subplot(111)
                    ax_leg.axis('off')  # 不需要坐标轴

                    # 将 legend 添加到新的 figure
                    new_leg = ax_leg.legend(
                        handles,
                        labels,
                        frameon=False,
                        fontsize=11,
                        loc='center',
                        ncol=3
                    )

                    # 保存文件
                    leg_path = (
                        model_paths.model_dir + os.path.sep +
                        r'CALI_NSGA2_Gen_%s_Pop_%s/%s_legend.png'
                        % (ngens, npop, f'{kk}')
                    )

                    fig_leg.savefig(leg_path, dpi=300, bbox_inches='tight')
                    plt.close(fig_leg)

                    print("单独保存图例：", leg_path)

                plt.tight_layout()

                # 保存文件：加上 generation 及 calibrationID
                gen_i = gen_selec[i] if i < len(gen_selec) else 'NA'
                id_i = ID_selec[i] if i < len(ID_selec) else 'NA'
                single_png = (
                    model_paths.model_dir
                    + os.path.sep
                    + r'CALI_NSGA2_Gen_%s_Pop_%s\%s_gen%s_ID%s.png'
                    % (ngens, npop, kk, gen_i, id_i)
                )
                single_png = (
                    r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171'
                    + os.path.sep
                    + r'CALI_NSGA2_Gen_%s_Pop_%s\%s_gen%s_ID%s.png'
                    % (ngens, npop, kk, gen_i, id_i)
                )

                plt.savefig(single_png, dpi=300)
                plt.close(fig_i)
                print(single_png)

        # ========= 新增：为每一组入选的参数单独画一张图（F和Q画到一张图上，作为上下子图） =========
    if plot_mode == 'combined':
        # ========= 新增：同一参数集，把多个 var_name 画在一张图上（上下子图） =========
        n_var = len(var_name)
        # 支持自定义 y 轴别名
        var_alias = {
            'Q_1171': 'Q(m3/s)',
            'F_1171': 'Inundation Area(km2)',
            # 可扩展更多
        }
        sample_obssim = obssim_dict[var_name[0]]
        n_param = sample_obssim.shape[1] - 1  # 去掉 Obs 列，剩下都是 Obssim_*_*
        vars_with_cali_vali = [0]  # 比如 Q 的索引

        for i in range(0, n_param):  # i 表示第几组参数集
            fig, axes = plt.subplots(n_var, 1, figsize=(22, 5 * n_var), sharex=True, dpi=100)
            if n_var == 1:
                axes = [axes]

            for nn, kk in enumerate(var_name):
                ax = axes[nn]
                obssim = obssim_dict[kk]
                sims_full = sims_dict[kk]  # 新增：该变量的完整模拟序列
                sim_col_name = 'Obssim_%s_%s' % (nn, i)
                sim_full_col_name = 'sim_%s_%s' % (nn, i)  # 在 sims 里对应的列名

                # ---------- 当前变量 kk 的子图绘图 ----------
                # 有 Obs 就画，没有就算了
                if 'Obs' in obssim.columns and not obssim['Obs'].isna().all():
                    ax.scatter(
                        x=obssim.index,
                        y=obssim['Obs'],
                        label='Observation' if nn == 0 else None,
                        color='#6F6F6F',
                        s=13
                    )

                # ===== 模拟曲线：按是否在 vars_with_cali_vali 分两种取法 =====
                if nn in vars_with_cali_vali:
                    # 像 Q 一样，用 obssim 中与观测对齐的模拟值
                    ax.plot(
                        obssim.index,
                        obssim[sim_col_name],
                        label='Simulation (set %d)' % (i + 1) if nn == 0 else None,
                        color='#C82423'
                    )
                else:
                    # 比如 F：用 sims_full 中的完整模拟序列（不裁到 Obs 时间）
                    ax.plot(
                        sims_full.index,
                        sims_full[sim_full_col_name],
                        label='Simulation (set %d)' % (i + 1) if nn == 0 else None,
                        color='#C82423'
                    )

                # ===== nn 在 vars_with_cali_vali：完整风格（率定期指标 + etime + 降雨）=====
                if nn in vars_with_cali_vali:
                    ax2 = ax.twinx()
                    ax2.bar(
                        pcp_date,
                        preci,
                        label='Precipitation',
                        color='blue',
                        linewidth=0,
                        align='center'
                    )
                    ax2.set_ylim(float(max(preci)) * 4, float(min(preci)) * 0.8)

                    # 率定/验证分界线
                    ax.axvline(etime, c='#000000', ls='--', lw=1)

                    # ----- 评价指标：只用率定期 -----
                    # 率定期、验证期分开计算指标
                    cali_vals = obssim[obssim.index <= etime][['Obs', sim_col_name]].dropna()
                    vali_vals = obssim[obssim.index > etime][['Obs', sim_col_name]].dropna()

                    metrics_cali = {}
                    metrics_vali = {}

                    metrics_cali["NSE"] = nashsutcliffe(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_vali["NSE"] = nashsutcliffe(vali_vals['Obs'], vali_vals[sim_col_name])

                    metrics_cali["logNSE"] = lognashsutcliffe(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_vali["logNSE"] = lognashsutcliffe(vali_vals['Obs'], vali_vals[sim_col_name])

                    metrics_cali["KGE"] = kge(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_vali["KGE"] = kge(vali_vals['Obs'], vali_vals[sim_col_name])

                    metrics_cali["Rsquare"] = rsquared(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_vali["Rsquare"] = rsquared(vali_vals['Obs'], vali_vals[sim_col_name])

                    metrics_cali["pbias"] = pbias(cali_vals['Obs'], cali_vals[sim_col_name])
                    metrics_vali["pbias"] = pbias(vali_vals['Obs'], vali_vals[sim_col_name])

                    metric_names = ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]
                    for idx_m, m in enumerate(metric_names):
                        y_pos = 0.80 - idx_m * 0.04

                        # 左边：率定期
                        ax.text(
                            0.15,
                            y_pos,
                            "Cali %s = %.3f" % (m, metrics_cali[m]),
                            fontsize=13,
                            transform=ax.transAxes
                        )

                        # 右边：验证期
                        ax.text(
                            0.55,  # 往右放一点
                            y_pos,
                            "Vali %s = %.3f" % (m, metrics_vali[m]),
                            fontsize=13,
                            transform=ax.transAxes
                        )

                    ax2.legend(
                        frameon=False,
                        fontsize=14,
                        bbox_to_anchor=(0., 1.02, 1., 0.102),
                        borderaxespad=0.,
                        ncol=1,
                        loc='lower right',
                        fancybox=True
                    )

                # ===== nn 不在 vars_with_cali_vali：纯查看曲线，但算全时段指标 =====
                else:
                    if 'Obs' in obssim.columns and not obssim['Obs'].isna().all():
                        whole_vals = obssim[['Obs', sim_col_name]].dropna()

                        metrics = {}
                        metrics["NSE"] = nashsutcliffe(whole_vals['Obs'], whole_vals[sim_col_name])
                        metrics["logNSE"] = lognashsutcliffe(whole_vals['Obs'], whole_vals[sim_col_name])
                        metrics["KGE"] = kge(whole_vals['Obs'], whole_vals[sim_col_name])
                        metrics["Rsquare"] = rsquared(whole_vals['Obs'], whole_vals[sim_col_name])
                        metrics["pbias"] = pbias(whole_vals['Obs'], whole_vals[sim_col_name])

                        metric_names = ["NSE", "logNSE", "KGE", "Rsquare", "pbias"]
                        for idx_m, m in enumerate(metric_names):
                            ax.text(
                                0.15,
                                0.80 - idx_m * 0.04,
                                "%s = %.3f" % (m, metrics[m]),
                                fontsize=13,
                                transform=ax.transAxes
                            )
                    # 不画 etime，不画降雨

                # ===== y 轴范围：以模拟为主，Obs 有就一起考虑 =====
                if nn in vars_with_cali_vali:
                    sim_series = obssim[sim_col_name]
                else:
                    sim_series = sims_full[sim_full_col_name]

                sim_max = np.nanmax(sim_series)
                if 'Obs' in obssim.columns and not obssim['Obs'].isna().all():
                    obs_max = np.nanmax(obssim['Obs'])
                    maxy_i = np.max([sim_max, obs_max]) * 1.8
                else:
                    maxy_i = sim_max * 1.8
                ax.set_ylim(0, maxy_i)

                # 坐标轴格式（保持不变）
                y_label = var_alias.get(kk, kk)  # 若别名不存在则用原始名
                ax.set_ylabel(y_label, fontsize=label_font_size)
                ax.tick_params('both', length=5, width=2, which='major', labelsize=label_font_size)
                ax.xaxis.set_major_locator(mdates.YearLocator())
                ax.xaxis.set_major_formatter(mdates.DateFormatter('%Y'))
                ax.xaxis.set_minor_locator(mdates.MonthLocator())

                def june_only(x, pos):
                    dt = mdates.num2date(x)
                    if dt.month == 6:
                        return '6'
                    return ''

                ax.xaxis.set_minor_formatter(FuncFormatter(june_only))

            axes[0].legend(
                frameon=False,
                fontsize=14,
                bbox_to_anchor=(0., 1.02, 1., 0.102),
                borderaxespad=0.,
                ncol=2,
                loc='lower left',
                fancybox=True
            )

            plt.tight_layout()

            gen_i = gen_selec[i] if i < len(gen_selec) else 'NA'
            id_i = ID_selec[i] if i < len(ID_selec) else 'NA'
            multi_png = (
                model_paths.model_dir
                + os.path.sep
                + r'CALI_NSGA2_Gen_%s_Pop_%s\multiVar_gen%s_ID%s.png'
                % (ngens, npop, gen_i, id_i)
            )
            plt.savefig(multi_png, dpi=300)
            print(multi_png)


if __name__ == "__main__":
    # watershed_nums = [123,141,214,225,322,347,457]
    watershed_nums = [1]
    for watershed_num in watershed_nums:
        main(watershed_num)
