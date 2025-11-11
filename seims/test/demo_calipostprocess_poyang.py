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
def main():
    NN = 1  #可调，需要前多少组参数集
    watershed_num = 225
    model_name = f'poyang_lake1_longterm_model_{watershed_num}'
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
    cali_cfg_file = model_paths.cfg_dir + os.path.sep + f'calibration_{watershed_num}.ini'
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
    for param, values in result.items():
        temp2 = [1 - values[i] for i in range(len(values))]
        temp = np.sqrt([x*y for x,y in zip(temp2,temp2)])
        new  = [new[i] + temp[i] for i in range(len(temp))]

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
    # var_name = ['Q_322']
    for nn,kk in enumerate(var_name):
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
                        value.append(float(v[0]))
                    for k,v in d.items():
                        Date.append(k)
                        value.append(float(v[0]))
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
        pcp_date_value = readData.Precipitation(1, start, end)
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

        for index,name in enumerate(["NSE","logNSE","KGE","Rsquare","pbias"]):
            ax.text(0.16, 0.75-index*0.04, '%s=%0.2f(%0.2f,%0.2f)'%(name,np.mean(cali_dict[name]),np.min(cali_dict[name]),np.max(cali_dict[name])), fontsize=16,transform = ax.transAxes)
            ax.text(0.66, 0.75-index*0.04, '%s=%0.2f(%0.2f,%0.2f)'%(name,np.mean(vali_dict[name]),np.min(vali_dict[name]),np.max(vali_dict[name])), fontsize=16,transform = ax.transAxes)
        maxy = np.max([np.max(np.max(sims,1))*1.8,np.max(np.max(obssim['Obs']))*1.8])
        ax.set_ylim(0,maxy)

        plt.axvline(etime, c='#000000', ls='--', lw=1)
        ax.legend(frameon=False, fontsize=14, bbox_to_anchor=(0., 1.02, 1., 0.102),borderaxespad=0.,ncol=3,loc='lower left', fancybox=True)
        ax2.legend(frameon=False, fontsize=14, bbox_to_anchor=(0., 1.02, 1., 0.102),borderaxespad=0.,ncol=1,loc='lower right', fancybox=True)

        ax.set_ylabel('%s'%kk, fontsize=15)
        ax.tick_params('both', length=5, width=2, which='major',labelsize=15)
        plt.tight_layout()
        plt.savefig(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,kk), dpi=300)
        print(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,kk))
        print("///////查看结果///////")


if __name__ == "__main__":
    main()
