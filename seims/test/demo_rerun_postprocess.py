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
    # tar = ['QG','QI','QS','SBGS']
    tar = ['F']
    watershed_num = 1171
    conn = MongoClient('172.21.124.127', 27019)
    db = conn.poyang_lake1_longterm_model   #需要自己修改数据库名字

    wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')
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

    #读取参数集
    parameters = newdf.gene_values.to_frame()
    parameters = parameters.gene_values.str.split(',', expand=True)
    parameters[1] = parameters[1] .astype(str).str.replace(r'\[|\]|,', '')
    parameters[parameters.shape[1]-1] = parameters[parameters.shape[1]-1] .astype(str).str.replace(r'\[|\]|\ |\)|,', '')
    parameters = parameters.drop([0],axis=1)
    #print(parameters)
    param_range_def = cf.get('CALI_Settings', 'paramrngdef')
    items = read_data_items_from_txt(model_paths.model_dir + os.path.sep + param_range_def)
    names = []
    for item in items:
        if len(item) < 3:
            continue
        names.append(item[0])
    print("率定参数：",names)
    out = dict()
    for kk in range(0,len(newdf)):
        data = parameters.iloc[kk,:].astype(float)
        newdata = pd.DataFrame()
        newdata['Name'] =names
        newdata.index = np.arange(1, len(newdata)+1)
        newdata['value'] = data
        print("import calibration parameters to mongoDB for parameters ", kk)
        for index, i in enumerate(newdata.Name):
            #print(i,newdata.value[index+1])
            db["PARAMETERS"].find_one_and_update({'NAME': i},{'$set': {'IMPACT': newdata.value[index+1]}})
        os.system('python demo_runmodel.py -name %s'%(wtsd_name))

        #获取模拟数据
        path =  model_paths.model_dir + os.path.sep + r'OUTPUT0'
        for index,name in enumerate(tar):
            filename =path + '\%s.txt'%name
            temp = pd.DataFrame()
            temp = pd.read_table(filename, skiprows=1, parse_dates=True,sep='\s+', header=None, names=['DATE','TIME','value'])
            out.setdefault(name, []).append(list(temp.value))
            print((temp))
            if(index==0): out.setdefault('Date', []).append(list(temp.DATE))

    for index,name in enumerate(tar):
        newdf = pd.DataFrame()
        # for x in out['Date'][0]:
            # print(x)
            # print(datetime.strptime(x,'%Y-%m-%d'))
        newdf['Date'] = [datetime.strptime(x,'%Y-%m-%d') for x in out['Date'][0]]
        for jj in range(0,len(out['Q'])):
            newdf['%s_%s'%(name,jj)] = list(out['%s'%name][jj])
        newdf.set_index('Date',inplace=True)
        newdf.to_csv(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.csv'%(ngens,npop,name))
        print("///%s 模拟结果提取结束///"%name)

        fig, ax =plt.subplots(1,1, figsize=(22,8), dpi=100)
        ax = plt.subplot(1,1,1)
        ax.plot(newdf.index,np.median(newdf,1), label='Simulation', color='#C82423')
        low_CI_bound= newdf.quantile(0.1,axis=1)
        high_CI_bound= newdf.quantile(0.9,axis=1)
        print( low_CI_bound, high_CI_bound)
        x=newdf.index
        ax.fill_between(x, low_CI_bound, high_CI_bound, linewidth=1,color='#F8AC8C',label='10th - 90th percentile')

        etime = StringClass.get_datetime(cf.get('CALI_Settings', 'cali_time_end'))
        plt.axvline(etime, c='#000000', ls='--', lw=1)
        ax.legend(frameon=False, fontsize=14, bbox_to_anchor=(0., 1.02, 1., 0.102),borderaxespad=0.,ncol=3,loc='lower left', fancybox=True)

        ax.set_ylabel('%s'%name, fontsize=15)
        ax.tick_params('both', length=5, width=2, which='major',labelsize=15)
        plt.tight_layout()
        plt.savefig(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,name), dpi=300)
        print(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,name))
        print("///////查看结果///////")






if __name__ == "__main__":
    main()
