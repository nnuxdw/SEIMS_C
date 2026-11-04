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


# def main():
#     NN = 5  #可调，需要前多少组参数集
#
#     wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')
#     if wtsd_name not in list(DEMO_MODELS.keys()):
#         print('%s is not one of the available demo watershed: %s' %
#               (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
#         exit(-1)
#
#     cur_path = UtilClass.current_path(lambda: 0)
#     SEIMS_path = os.path.abspath(cur_path + '../../..')
#     model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
#     cf = ConfigParser()
#     cali_cfg_file = model_paths.workspace + os.path.sep + 'calibration.ini'
#     cf.read(cali_cfg_file)
#
#     #读取率定结果
#     ngens = cf.getint('NSGA2', 'generationsnum') if \
#             cf.has_option('NSGA2', 'generationsnum') else 1
#     npop = cf.getint('NSGA2', 'populationsize') if \
#             cf.has_option('NSGA2', 'populationsize') else 1
#     filename = model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\runtime.log'%(ngens,npop)
#     df = pd.read_table(filename,sep='	', skiprows=2)
#     df.drop_duplicates(keep='first',inplace=True)
#     df = pd.DataFrame(df)
#     df = df.drop(df[df['generation']=='generation'].index)
#     Ntar = int((len(df.columns) - 3)/2)
#     index_ini = 2
#     index_end = int(1 + (len(df.columns) - 3)/2)
#     df = df[~df['generation'].str.contains(('#'))]
#     print("率定目标：",Ntar,(df.columns[index_ini]),(df.columns[index_end]))
#     print("       ")
#
#     #计算加权排序
#     result = dict()
#     for i in range(0,Ntar):
#         result[df.columns[index_ini+i]] = list(df[df.columns[index_ini+i]].astype(float))
#
#     new = [0] *len(df)
#     temp = []
#     for param, values in result.items():
#         temp2 = [1 - values[i] for i in range(len(values))]
#         temp = np.sqrt([x*y for x,y in zip(temp2,temp2)])
#         new  = [new[i] + temp[i] for i in range(len(temp))]
#
#     stac = pd.DataFrame(new,columns=['statics'])
#     stac['FID'] = range(0,len(df))
#     stac['rank'] = stac['statics'].rank(ascending=True)
#     newdata = stac['statics'].tolist()
#     df['weight'] = newdata
#     select = stac.sort_values(by='rank')[0:NN].FID.tolist()
#
#     newdf = pd.DataFrame()
#     temp  = pd.DataFrame()
#     for i in (select):
#         newdf = newdf.append(df[i:i+1])
#     newdf = newdf.reset_index(drop=True)
#     print("/////筛选后参数集/////")
#     print(newdf)
#     print("       ")
#     newdf.to_csv(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\result_%s.csv'%(ngens,npop,NN),index=False)
#     with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen0_caliObsData.json', 'r', encoding='utf-8') as f:
#             sim_obs_data = json.load(f)
#     gen_selec = newdf['generation'].astype(int).tolist()
#     ID_selec = newdf['calibrationID'].astype(int).tolist()
#
#
#     var_name = sim_obs_data[0]['var_name']
#     # var_name = 'Q'
#     for nn,kk in enumerate(var_name):
#         obssim = pd.DataFrame()
#         sims = pd.DataFrame()
#         for index,genID in enumerate(gen_selec):
#             gens = gen_selec[index]
#             IDs = ID_selec[index]
#             cali_obs_data = list()
#             vali_obs_data = list()
#
#             with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_caliObsData.json'%gens, 'r', encoding='utf-8') as f:
#                 sim_obs_data = json.load(f)
#             with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_caliSimData.pickle'%gens, 'rb') as f:
#                 sim_data = pickle.load(f)
#             # validation data
#             with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_valiObsData.json'%gens, 'r', encoding='utf-8') as f:
#                 vali_sim_obs_data = json.load(f)
#             with open(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s'%(ngens,npop) + os.path.sep + '\simulated_data\gen%s_valiSimData.pickle'%gens, 'rb') as f:
#                 vali_sim_data = pickle.load(f)
#
#             for a, b, c, d in zip(sim_obs_data, sim_data, vali_sim_obs_data, vali_sim_data):
#                 if(a['Gen'] in [gens]):
#                     if a['ID'] in [IDs]:
#                         cali_obs_data.append(a)
#                         vali_obs_data.append(c)
#                     Date = []
#                     value = []
#                     for k,v in b.items():
#                         Date.append(k)
#                         value.append(float(v[0]))
#                     for k,v in d.items():
#                         Date.append(k)
#                         value.append(float(v[0]))
#                     sims['Date'] =list(Date)
#                     sims['sim_%s_%s'%(nn,index)]  = value
#             if(index==0):
#                 obssim['Date'] =list(cali_obs_data[0]['%s'%kk]['UTCDATETIME'][:]) + list(vali_obs_data[0]['%s'%kk]['UTCDATETIME'][:])
#                 obssim['Obs'] =list(cali_obs_data[0]['%s'%kk]['Obs'][:]) + list(vali_obs_data[0]['%s'%kk]['Obs'][:])
#             obssim['Obssim_%s_%s'%(nn,index)] = list(cali_obs_data[0]['%s'%kk]['Sim'][:]) + list(vali_obs_data[0]['%s'%kk]['Sim'][:])
#
#         obssim['Date'] = [StringClass.get_datetime(s) for s in obssim['Date']]
#         obssim = obssim.set_index('Date')
#         sims = sims.set_index('Date')
#         print()
#         print("///%s 模拟结果提取结束///"%kk)
#         output = pd.concat([sims,obssim['Obs']], axis=1)
#         output.to_csv(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.csv'%(ngens,npop,kk))
#
#         #画图
#         fig, ax =plt.subplots(1,1, figsize=(22,8), dpi=100)
#         ax = plt.subplot(1,1,1)
#         p1 = ax.scatter(x=obssim.index,y=obssim.Obs, label='Observation', color='#6F6F6F', s=13)
#         p2 = ax.plot(sims.index,np.median(sims,1), label='Simulation', color='#C82423')
#         low_CI_bound= sims.quantile(0.1,axis=1)
#         high_CI_bound= sims.quantile(0.9,axis=1)
#         print( low_CI_bound, high_CI_bound)
#         x=sims.index
#         ax.fill_between(x, low_CI_bound, high_CI_bound, linewidth=1,color='#F8AC8C',label='10th - 90th percentile')
#
#         HOSTNAME = cf.get('SEIMS_Model', 'HOSTNAME')
#         PORT = int(cf.get('SEIMS_Model', 'PORT'))
#         mongoclient = ConnectMongoDB(HOSTNAME, PORT).get_conn()
#         readData = ReadModelData(mongoclient, DEMO_MODELS[wtsd_name])
#         start = sims.index[0]
#         end = sims.index[-1]
#         pcp_date_value = readData.Precipitation(1, start, end)
#         pcp_date = [v[0] for v in pcp_date_value]
#         preci = [v[1] for v in pcp_date_value]
#         ax2 = ax.twinx()
#         p3 = ax2.bar(pcp_date, preci, label='Precipitation', color='blue', linewidth=0,
#                          align='center')
#         ax2.set_ylim(float(max(preci)) * 4, float(min(preci)) * 0.8)
#
#         #计算评价系数
#         etime = StringClass.get_datetime(cf.get('CALI_Settings', 'cali_time_end'))
#         sim_Cali = obssim[obssim.index<=etime]
#         sim_Vali = obssim[obssim.index>etime]
#         cali_dict = {}
#         vali_dict = {}
#
#         for i in range(0,obssim.shape[1]-1):
#             for index,name in enumerate(["NSE","logNSE","KGE","Rsquare","pbias"]):
#             # for index, name in enumerate(["NSE", "KGE", "Rsquare", "pbias"]):
#                 if(index ==0):
#                     cali_dict.setdefault(name, []).append(nashsutcliffe(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
#                     vali_dict.setdefault(name, []).append(nashsutcliffe(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
#                 if(index ==1):
#                     cali_dict.setdefault(name, []).append(lognashsutcliffe(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
#                     vali_dict.setdefault(name, []).append(lognashsutcliffe(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
#                 if(index ==2):
#                     cali_dict.setdefault(name, []).append(kge(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
#                     vali_dict.setdefault(name, []).append(kge(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
#                 if(index ==3):
#                     cali_dict.setdefault(name, []).append(rsquared(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
#                     vali_dict.setdefault(name, []).append(rsquared(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
#                 if(index ==4):
#                     cali_dict.setdefault(name, []).append(pbias(sim_Cali['Obs'],sim_Cali['Obssim_%s_%s'%(nn,i)]))
#                     vali_dict.setdefault(name, []).append(pbias(sim_Vali['Obs'],sim_Vali['Obssim_%s_%s'%(nn,i)]))
#
#
#         for index,name in enumerate(["NSE","logNSE","KGE","Rsquare","pbias"]):
#         # for index, name in enumerate(["NSE", "KGE", "Rsquare", "pbias"]):
#             ax.text(0.16, 0.75-index*0.04, '%s=%0.2f(%0.2f,%0.2f)'%(name,np.mean(cali_dict[name]),np.min(cali_dict[name]),np.max(cali_dict[name])), fontsize=16,transform = ax.transAxes)
#             ax.text(0.66, 0.75-index*0.04, '%s=%0.2f(%0.2f,%0.2f)'%(name,np.mean(vali_dict[name]),np.min(vali_dict[name]),np.max(vali_dict[name])), fontsize=16,transform = ax.transAxes)
#         maxy = np.max([np.max(np.max(sims,1))*1.8,np.max(np.max(obssim['Obs']))*1.8])
#         ax.set_ylim(0,maxy)
#
#         plt.axvline(etime, c='#000000', ls='--', lw=1)
#         ax.legend(frameon=False, fontsize=14, bbox_to_anchor=(0., 1.02, 1., 0.102),borderaxespad=0.,ncol=3,loc='lower left', fancybox=True)
#         ax2.legend(frameon=False, fontsize=14, bbox_to_anchor=(0., 1.02, 1., 0.102),borderaxespad=0.,ncol=1,loc='lower right', fancybox=True)
#
#         ax.set_ylabel('%s'%kk, fontsize=15)
#         ax.tick_params('both', length=5, width=2, which='major',labelsize=15)
#         plt.tight_layout()
#         plt.savefig(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,kk), dpi=300)
#         print(model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s\%s.png'%(ngens,npop,kk))
#         print("///////查看结果///////")
#
#
# if __name__ == "__main__":
#     main()




# # 只画图
# from __future__ import absolute_import, unicode_literals
#
# import os
# import sys
# import json
# import pickle
# from configparser import ConfigParser
#
# import pandas as pd
# import numpy as np
# import matplotlib.pyplot as plt
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from pygeoc.utils import UtilClass, StringClass
# from test.demo_config import ModelPaths, DEMO_MODELS, get_watershed_name
# from preprocess.db_mongodb import ConnectMongoDB
# from preprocess.db_read_model import ReadModelData
#
#
# def main():
#     NN = 5  # 取前多少组参数集
#
#     wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')
#     if wtsd_name not in list(DEMO_MODELS.keys()):
#         print('%s is not one of the available demo watershed: %s' %
#               (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
#         sys.exit(-1)
#
#     cur_path = UtilClass.current_path(lambda: 0)
#     SEIMS_path = os.path.abspath(cur_path + '../../..')
#     model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
#
#     cf = ConfigParser()
#     cali_cfg_file = model_paths.workspace + os.path.sep + 'calibration.ini'
#     cf.read(cali_cfg_file)
#
#     # 读取率定结果
#     ngens = cf.getint('NSGA2', 'generationsnum') if cf.has_option('NSGA2', 'generationsnum') else 1
#     npop = cf.getint('NSGA2', 'populationsize') if cf.has_option('NSGA2', 'populationsize') else 1
#
#     result_dir = model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s' % (ngens, npop)
#     runtime_log = result_dir + os.path.sep + 'runtime.log'
#
#     df = pd.read_table(runtime_log, sep='\t', skiprows=2)
#     df.drop_duplicates(keep='first', inplace=True)
#     df = pd.DataFrame(df)
#     df = df.drop(df[df['generation'] == 'generation'].index)
#     df = df[~df['generation'].astype(str).str.contains('#')]
#
#     Ntar = int((len(df.columns) - 3) / 2)
#     index_ini = 2
#     index_end = int(1 + (len(df.columns) - 3) / 2)
#
#     print("率定目标：", Ntar, df.columns[index_ini], df.columns[index_end])
#     print("       ")
#
#     # 计算加权排序
#     result = {}
#     for i in range(Ntar):
#         result[df.columns[index_ini + i]] = list(df[df.columns[index_ini + i]].astype(float))
#
#     new = [0] * len(df)
#     for _, values in result.items():
#         temp2 = [1 - values[i] for i in range(len(values))]
#         temp = np.sqrt([x * y for x, y in zip(temp2, temp2)])
#         new = [new[i] + temp[i] for i in range(len(temp))]
#
#     stac = pd.DataFrame(new, columns=['statics'])
#     stac['FID'] = range(0, len(df))
#     stac['rank'] = stac['statics'].rank(ascending=True)
#     df['weight'] = stac['statics'].tolist()
#
#     select = stac.sort_values(by='rank').iloc[0:NN]['FID'].tolist()
#
#     newdf = pd.DataFrame()
#     for i in select:
#         newdf = pd.concat([newdf, df.iloc[i:i + 1]], axis=0)
#
#     newdf = newdf.reset_index(drop=True)
#
#     print("/////筛选后参数集/////")
#     print(newdf)
#     print("       ")
#
#     newdf.to_csv(result_dir + os.path.sep + ('result_%s.csv' % NN), index=False)
#
#     # 读取变量名
#     with open(result_dir + os.path.sep + r'simulated_data\gen0_caliObsData.json', 'r', encoding='utf-8') as f:
#         sim_obs_data = json.load(f)
#
#     var_name = sim_obs_data[0]['var_name']
#     gen_selec = newdf['generation'].astype(int).tolist()
#     ID_selec = newdf['calibrationID'].astype(int).tolist()
#
#     for nn, kk in enumerate(var_name):
#         obssim = pd.DataFrame()
#         sims = pd.DataFrame()
#
#         for index, _ in enumerate(gen_selec):
#             gens = gen_selec[index]
#             IDs = ID_selec[index]
#
#             cali_obs_data = []
#             vali_obs_data = []
#
#             with open(result_dir + os.path.sep + (r'simulated_data\gen%s_caliObsData.json' % gens), 'r', encoding='utf-8') as f:
#                 sim_obs_data = json.load(f)
#             with open(result_dir + os.path.sep + (r'simulated_data\gen%s_caliSimData.pickle' % gens), 'rb') as f:
#                 sim_data = pickle.load(f)
#             with open(result_dir + os.path.sep + (r'simulated_data\gen%s_valiObsData.json' % gens), 'r', encoding='utf-8') as f:
#                 vali_sim_obs_data = json.load(f)
#             with open(result_dir + os.path.sep + (r'simulated_data\gen%s_valiSimData.pickle' % gens), 'rb') as f:
#                 vali_sim_data = pickle.load(f)
#
#             for a, b, c, d in zip(sim_obs_data, sim_data, vali_sim_obs_data, vali_sim_data):
#                 if a['Gen'] in [gens]:
#                     if a['ID'] in [IDs]:
#                         cali_obs_data.append(a)
#                         vali_obs_data.append(c)
#
#                     Date = []
#                     value = []
#                     for k, v in b.items():
#                         Date.append(k)
#                         value.append(float(v[0]))
#                     for k, v in d.items():
#                         Date.append(k)
#                         value.append(float(v[0]))
#
#                     sims['Date'] = list(Date)
#                     sims['sim_%s_%s' % (nn, index)] = value
#
#             if index == 0:
#                 obssim['Date'] = list(cali_obs_data[0][kk]['UTCDATETIME'][:]) + list(vali_obs_data[0][kk]['UTCDATETIME'][:])
#                 obssim['Obs'] = list(cali_obs_data[0][kk]['Obs'][:]) + list(vali_obs_data[0][kk]['Obs'][:])
#
#             obssim['Obssim_%s_%s' % (nn, index)] = list(cali_obs_data[0][kk]['Sim'][:]) + list(vali_obs_data[0][kk]['Sim'][:])
#
#         obssim['Date'] = [StringClass.get_datetime(s) for s in obssim['Date']]
#         obssim = obssim.set_index('Date')
#         sims = sims.set_index('Date')
#
#         print()
#         print("///%s 模拟结果提取结束///" % kk)
#
#         output = pd.concat([sims, obssim['Obs']], axis=1)
#         output.to_csv(result_dir + os.path.sep + ('%s.csv' % kk))
#
#         # 画图
#         fig, ax = plt.subplots(1, 1, figsize=(22, 8), dpi=100)
#
#         ax.scatter(x=obssim.index, y=obssim.Obs, label='Observation', color='#6F6F6F', s=13)
#         ax.plot(sims.index, np.median(sims, axis=1), label='Simulation', color='#C82423')
#
#         low_CI_bound = sims.quantile(0.1, axis=1)
#         high_CI_bound = sims.quantile(0.9, axis=1)
#         ax.fill_between(sims.index, low_CI_bound, high_CI_bound,
#                         linewidth=1, color='#F8AC8C', label='10th - 90th percentile')
#
#         # 读降雨并画右轴
#         HOSTNAME = cf.get('SEIMS_Model', 'HOSTNAME')
#         PORT = int(cf.get('SEIMS_Model', 'PORT'))
#         mongoclient = ConnectMongoDB(HOSTNAME, PORT).get_conn()
#         readData = ReadModelData(mongoclient, DEMO_MODELS[wtsd_name])
#
#         start = sims.index[0]
#         end = sims.index[-1]
#         pcp_date_value = readData.Precipitation(1, start, end)
#         pcp_date = [v[0] for v in pcp_date_value]
#         preci = [v[1] for v in pcp_date_value]
#
#         ax2 = ax.twinx()
#         ax2.bar(pcp_date, preci, label='Precipitation', color='blue', linewidth=0, align='center')
#
#         if len(preci) > 0:
#             ax2.set_ylim(float(max(preci)) * 4, float(min(preci)) * 0.8)
#
#         # 率定/验证分界线，只画线，不算指标
#         etime = StringClass.get_datetime(cf.get('CALI_Settings', 'cali_time_end'))
#         ax.axvline(etime, c='#000000', ls='--', lw=1)
#
#         maxy = np.max([np.max(np.max(sims, axis=1)) * 1.8,
#                        np.max(obssim['Obs']) * 1.8])
#         ax.set_ylim(0, maxy)
#
#         ax.legend(frameon=False, fontsize=14,
#                   bbox_to_anchor=(0., 1.02, 1., 0.102),
#                   borderaxespad=0., ncol=3, loc='lower left', fancybox=True)
#         ax2.legend(frameon=False, fontsize=14,
#                    bbox_to_anchor=(0., 1.02, 1., 0.102),
#                    borderaxespad=0., ncol=1, loc='lower right', fancybox=True)
#
#         ax.set_ylabel('%s' % kk, fontsize=15)
#         ax.tick_params('both', length=5, width=2, which='major', labelsize=15)
#
#         plt.tight_layout()
#         out_png = result_dir + os.path.sep + ('%s.png' % kk)
#         plt.savefig(out_png, dpi=300)
#         print(out_png)
#         print("///////查看结果///////")
#
#
# if __name__ == "__main__":
#     main()




# # 只画率定期的图
# def main():
#     NN = 5
#
#     wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')
#
#     if wtsd_name not in list(DEMO_MODELS.keys()):
#         print('%s is not one of the available demo watershed: %s' %
#               (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
#         exit(-1)
#
#     cur_path = UtilClass.current_path(lambda: 0)
#     SEIMS_path = os.path.abspath(cur_path + '../../..')
#
#     model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
#
#     cf = ConfigParser()
#     cali_cfg_file = model_paths.cfg_dir + os.path.sep + 'calibration.ini'
#     cf.read(cali_cfg_file)
#
#     ngens = cf.getint('NSGA2', 'generationsnum')
#     npop = cf.getint('NSGA2', 'populationsize')
#
#     result_dir = model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s' % (ngens, npop)
#     runtime_file = result_dir + os.path.sep + 'runtime.log'
#
#     print("读取 runtime.log:", runtime_file)
#
#     df = pd.read_table(runtime_file, sep='\t', skiprows=2)
#     df.drop_duplicates(keep='first', inplace=True)
#     df = pd.DataFrame(df)
#
#     df = df.drop(df[df['generation'] == 'generation'].index)
#     df = df[~df['generation'].astype(str).str.contains('#')]
#
#     Ntar = int((len(df.columns) - 3) / 2)
#     index_ini = 2
#     index_end = int(1 + (len(df.columns) - 3) / 2)
#
#     print("率定目标：", Ntar, df.columns[index_ini], df.columns[index_end])
#     print(" ")
#
#     result = dict()
#
#     for i in range(0, Ntar):
#         result[df.columns[index_ini + i]] = list(df[df.columns[index_ini + i]].astype(float))
#
#     new = [0] * len(df)
#
#     for param, values in result.items():
#         temp2 = [1 - values[i] for i in range(len(values))]
#         temp = np.sqrt([x * y for x, y in zip(temp2, temp2)])
#         new = [new[i] + temp[i] for i in range(len(temp))]
#
#     stac = pd.DataFrame(new, columns=['statics'])
#     stac['FID'] = range(0, len(df))
#     stac['rank'] = stac['statics'].rank(ascending=True)
#
#     df['weight'] = stac['statics'].tolist()
#
#     select = stac.sort_values(by='rank')[0:NN].FID.tolist()
#
#     newdf = pd.DataFrame()
#     for i in select:
#         newdf = newdf.append(df[i:i + 1])
#
#     newdf = newdf.reset_index(drop=True)
#
#     print("/////筛选后参数集/////")
#     print(newdf)
#     print(" ")
#
#     newdf.to_csv(result_dir + os.path.sep + r'result_%s.csv' % NN, index=False)
#
#     gen_selec = newdf['generation'].astype(int).tolist()
#     ID_selec = newdf['calibrationID'].astype(int).tolist()
#
#     with open(result_dir + os.path.sep + r'simulated_data\gen0_caliObsData.json',
#               'r', encoding='utf-8') as f:
#         sim_obs_data0 = json.load(f)
#
#     var_name = sim_obs_data0[0]['var_name']
#
#     for nn, kk in enumerate(var_name):
#
#         obssim = pd.DataFrame()
#         sims = pd.DataFrame()
#
#         for index, genID in enumerate(gen_selec):
#
#             gens = gen_selec[index]
#             IDs = ID_selec[index]
#
#             cali_obs_data = []
#             selected_sim_dict = None
#
#             with open(result_dir + os.path.sep + r'simulated_data\gen%s_caliObsData.json' % gens,
#                       'r', encoding='utf-8') as f:
#                 sim_obs_data = json.load(f)
#
#             with open(result_dir + os.path.sep + r'simulated_data\gen%s_caliSimData.pickle' % gens,
#                       'rb') as f:
#                 sim_data = pickle.load(f)
#
#             for a, b in zip(sim_obs_data, sim_data):
#                 if a['Gen'] in [gens] and a['ID'] in [IDs]:
#                     cali_obs_data.append(a)
#                     selected_sim_dict = b
#                     break
#
#             if len(cali_obs_data) == 0 or selected_sim_dict is None:
#                 print("没有找到对应 gen/id 的数据:", gens, IDs)
#                 continue
#
#             if index == 0:
#                 obssim['Date'] = list(cali_obs_data[0][kk]['UTCDATETIME'][:])
#                 obssim['Obs'] = list(cali_obs_data[0][kk]['Obs'][:])
#
#             obssim['Obssim_%s_%s' % (nn, index)] = list(cali_obs_data[0][kk]['Sim'][:])
#
#             Date = []
#             value = []
#
#             for k, v in selected_sim_dict.items():
#                 Date.append(k)
#                 value.append(float(v[0]))
#
#             if index == 0:
#                 sims['Date'] = list(Date)
#
#             sims['sim_%s_%s' % (nn, index)] = value
#
#         obssim['Date'] = pd.to_datetime(obssim['Date'])
#         obssim = obssim.set_index('Date')
#
#         sims['Date'] = pd.to_datetime(sims['Date'])
#         sims = sims.set_index('Date')
#
#         print()
#         print("///%s 率定期模拟结果提取结束///" % kk)
#
#         output = pd.concat([sims, obssim[['Obs']]], axis=1)
#
#         output_csv = result_dir + os.path.sep + r'%s_cali_continuous.csv' % kk
#         output.to_csv(output_csv)
#
#         print("输出 CSV:", output_csv)
#
#         fig, ax = plt.subplots(1, 1, figsize=(22, 8), dpi=100)
#
#         ax.scatter(
#             x=obssim.index,
#             y=obssim['Obs'],
#             label='Observation',
#             color='#6F6F6F',
#             s=13,
#             zorder=5
#         )
#
#         ax.plot(
#             sims.index,
#             np.median(sims, axis=1),
#             label='Simulation',
#             color='#C82423',
#             linewidth=2,
#             zorder=4
#         )
#
#         low_CI_bound = sims.quantile(0.1, axis=1)
#         high_CI_bound = sims.quantile(0.9, axis=1)
#
#         ax.fill_between(
#             sims.index,
#             low_CI_bound,
#             high_CI_bound,
#             linewidth=1,
#             color='#F8AC8C',
#             label='10th - 90th percentile',
#             alpha=0.7,
#             zorder=3
#         )
#
#         try:
#             HOSTNAME = cf.get('SEIMS_Model', 'HOSTNAME')
#             PORT = int(cf.get('SEIMS_Model', 'PORT'))
#
#             mongoclient = ConnectMongoDB(HOSTNAME, PORT).get_conn()
#             readData = ReadModelData(mongoclient, DEMO_MODELS[wtsd_name])
#
#             start = sims.index[0]
#             end = sims.index[-1]
#
#             pcp_date_value = readData.Precipitation(1, start, end)
#
#             pcp_date = [v[0] for v in pcp_date_value]
#             preci = [v[1] for v in pcp_date_value]
#
#             ax2 = ax.twinx()
#
#             ax2.bar(
#                 pcp_date,
#                 preci,
#                 label='Precipitation',
#                 color='blue',
#                 linewidth=0,
#                 align='center',
#                 alpha=0.6
#             )
#
#             if len(preci) > 0:
#                 ax2.set_ylim(float(max(preci)) * 4, float(min(preci)) * 0.8)
#
#             ax2.legend(
#                 frameon=False,
#                 fontsize=14,
#                 bbox_to_anchor=(0., 1.02, 1., 0.102),
#                 borderaxespad=0.,
#                 ncol=1,
#                 loc='lower right',
#                 fancybox=True
#             )
#
#         except Exception as e:
#             print("降雨读取失败，但主图继续输出:", e)
#
#         cali_dict = {}
#
#         sim_cols = [c for c in obssim.columns if c.startswith("Obssim_")]
#
#         for sim_col in sim_cols:
#
#             obs = obssim['Obs'].astype(float)
#             sim = obssim[sim_col].astype(float)
#
#             mask = np.isfinite(obs) & np.isfinite(sim)
#
#             obs = obs[mask]
#             sim = sim[mask]
#
#             cali_dict.setdefault("NSE", []).append(nashsutcliffe(obs, sim))
#             cali_dict.setdefault("KGE", []).append(kge(obs, sim))
#             cali_dict.setdefault("Rsquare", []).append(rsquared(obs, sim))
#             cali_dict.setdefault("pbias", []).append(pbias(obs, sim))
#
#         for index, name in enumerate(["NSE", "KGE", "Rsquare", "pbias"]):
#             ax.text(
#                 0.16,
#                 0.75 - index * 0.04,
#                 '%s=%0.2f(%0.2f,%0.2f)' % (
#                     name,
#                     np.nanmean(cali_dict[name]),
#                     np.nanmin(cali_dict[name]),
#                     np.nanmax(cali_dict[name])
#                 ),
#                 fontsize=16,
#                 transform=ax.transAxes
#             )
#
#         maxy = np.nanmax([
#             np.nanmax(np.nanmax(sims, axis=1)) * 1.8,
#             np.nanmax(obssim['Obs']) * 1.8
#         ])
#
#         ax.set_ylim(0, maxy)
#
#         ax.legend(
#             frameon=False,
#             fontsize=14,
#             bbox_to_anchor=(0., 1.02, 1., 0.102),
#             borderaxespad=0.,
#             ncol=3,
#             loc='lower left',
#             fancybox=True
#         )
#
#         ax.set_ylabel('%s' % kk, fontsize=15)
#         ax.tick_params('both', length=5, width=2, which='major', labelsize=15)
#
#         plt.tight_layout()
#
#         fig_path = result_dir + os.path.sep + r'%s_cali_continuous.png' % kk
#
#         plt.savefig(fig_path, dpi=300)
#         plt.close()
#
#         print("输出图片:", fig_path)
#         print("///////查看结果///////")
#
#
# if __name__ == "__main__":
#     main()






# # 只画率定期的图
# def main():
#     NN = 5
#
#     wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')
#
#     if wtsd_name not in list(DEMO_MODELS.keys()):
#         print('%s is not one of the available demo watershed: %s' %
#               (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
#         exit(-1)
#
#     cur_path = UtilClass.current_path(lambda: 0)
#     SEIMS_path = os.path.abspath(cur_path + '../../..')
#
#     model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
#
#     cf = ConfigParser()
#     cali_cfg_file = model_paths.cfg_dir + os.path.sep + 'calibration.ini'
#     cf.read(cali_cfg_file)
#
#     ngens = cf.getint('NSGA2', 'generationsnum')
#     npop = cf.getint('NSGA2', 'populationsize')
#
#     result_dir = model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s' % (ngens, npop)
#     runtime_file = result_dir + os.path.sep + 'runtime.log'
#
#     print("读取 runtime.log:", runtime_file)
#
#     df = pd.read_table(runtime_file, sep='\t', skiprows=2)
#     df.drop_duplicates(keep='first', inplace=True)
#     df = pd.DataFrame(df)
#
#     df = df.drop(df[df['generation'] == 'generation'].index)
#     df = df[~df['generation'].astype(str).str.contains('#')]
#
#     Ntar = int((len(df.columns) - 3) / 2)
#     index_ini = 2
#     index_end = int(1 + (len(df.columns) - 3) / 2)
#
#     print("率定目标：", Ntar, df.columns[index_ini], df.columns[index_end])
#     print(" ")
#
#     result = dict()
#
#     for i in range(0, Ntar):
#         result[df.columns[index_ini + i]] = list(df[df.columns[index_ini + i]].astype(float))
#
#     new = [0] * len(df)
#
#     for param, values in result.items():
#         temp2 = [1 - values[i] for i in range(len(values))]
#         temp = np.sqrt([x * y for x, y in zip(temp2, temp2)])
#         new = [new[i] + temp[i] for i in range(len(temp))]
#
#     stac = pd.DataFrame(new, columns=['statics'])
#     stac['FID'] = range(0, len(df))
#     stac['rank'] = stac['statics'].rank(ascending=True)
#
#     df['weight'] = stac['statics'].tolist()
#
#     select = stac.sort_values(by='rank')[0:NN].FID.tolist()
#
#     newdf = pd.DataFrame()
#     for i in select:
#         newdf = newdf.append(df[i:i + 1])
#
#     newdf = newdf.reset_index(drop=True)
#
#     print("/////筛选后参数集/////")
#     print(newdf)
#     print(" ")
#
#     newdf.to_csv(result_dir + os.path.sep + r'result_%s.csv' % NN, index=False)
#
#     gen_selec = newdf['generation'].astype(int).tolist()
#     ID_selec = newdf['calibrationID'].astype(int).tolist()
#
#     with open(result_dir + os.path.sep + r'simulated_data\gen0_caliObsData.json',
#               'r', encoding='utf-8') as f:
#         sim_obs_data0 = json.load(f)
#
#     var_name = sim_obs_data0[0]['var_name']
#
#     for nn, kk in enumerate(var_name):
#
#         obssim = pd.DataFrame()
#         sims = pd.DataFrame()
#
#         for index, genID in enumerate(gen_selec):
#
#             gens = gen_selec[index]
#             IDs = ID_selec[index]
#
#             cali_obs_data = []
#             selected_sim_dict = None
#
#             with open(result_dir + os.path.sep + r'simulated_data\gen%s_caliObsData.json' % gens,
#                       'r', encoding='utf-8') as f:
#                 sim_obs_data = json.load(f)
#
#             with open(result_dir + os.path.sep + r'simulated_data\gen%s_caliSimData.pickle' % gens,
#                       'rb') as f:
#                 sim_data = pickle.load(f)
#
#             for a, b in zip(sim_obs_data, sim_data):
#                 if a['Gen'] in [gens] and a['ID'] in [IDs]:
#                     cali_obs_data.append(a)
#                     selected_sim_dict = b
#                     break
#
#             if len(cali_obs_data) == 0 or selected_sim_dict is None:
#                 print("没有找到对应 gen/id 的数据:", gens, IDs)
#                 continue
#
#             if index == 0:
#                 obssim['Date'] = list(cali_obs_data[0][kk]['UTCDATETIME'][:])
#                 obssim['Obs'] = list(cali_obs_data[0][kk]['Obs'][:])
#
#             obssim['Obssim_%s_%s' % (nn, index)] = list(cali_obs_data[0][kk]['Sim'][:])
#
#             Date = []
#             value = []
#
#             for k, v in selected_sim_dict.items():
#                 Date.append(k)
#                 value.append(float(v[0]))
#
#             if index == 0:
#                 sims['Date'] = list(Date)
#
#             sims['sim_%s_%s' % (nn, index)] = value
#
#         obssim['Date'] = pd.to_datetime(obssim['Date'])
#         obssim = obssim.set_index('Date')
#
#         sims['Date'] = pd.to_datetime(sims['Date'])
#         sims = sims.set_index('Date')
#
#         # =====================================================
#         # 删除 2010-01-15 到 2010-02-18 之间的数据
#         # 包括连续模拟数据 sims 和观测配对数据 obssim
#         # =====================================================
#         remove_start = pd.to_datetime("2010-01-15")
#         remove_end = pd.to_datetime("2010-02-18")
#
#         sims = sims.loc[~((sims.index >= remove_start) & (sims.index <= remove_end))]
#         obssim = obssim.loc[~((obssim.index >= remove_start) & (obssim.index <= remove_end))]
#
#         print()
#         print("///%s 率定期模拟结果提取结束///" % kk)
#
#         output = pd.concat([sims, obssim[['Obs']]], axis=1)
#
#         output_csv = result_dir + os.path.sep + r'%s_cali_continuous_remove_20100115_20100218.csv' % kk
#         output.to_csv(output_csv)
#
#         print("输出 CSV:", output_csv)
#
#         fig, ax = plt.subplots(1, 1, figsize=(22, 8), dpi=100)
#
#         ax.scatter(
#             x=obssim.index,
#             y=obssim['Obs'],
#             label='Observation',
#             color='#6F6F6F',
#             s=13,
#             zorder=5
#         )
#
#         ax.plot(
#             sims.index,
#             np.median(sims, axis=1),
#             label='Simulation',
#             color='#C82423',
#             linewidth=2,
#             zorder=4
#         )
#
#         low_CI_bound = sims.quantile(0.1, axis=1)
#         high_CI_bound = sims.quantile(0.9, axis=1)
#
#         ax.fill_between(
#             sims.index,
#             low_CI_bound,
#             high_CI_bound,
#             linewidth=1,
#             color='#F8AC8C',
#             label='10th - 90th percentile',
#             alpha=0.7,
#             zorder=3
#         )
#
#         try:
#             HOSTNAME = cf.get('SEIMS_Model', 'HOSTNAME')
#             PORT = int(cf.get('SEIMS_Model', 'PORT'))
#
#             mongoclient = ConnectMongoDB(HOSTNAME, PORT).get_conn()
#             readData = ReadModelData(mongoclient, DEMO_MODELS[wtsd_name])
#
#             start = sims.index[0]
#             end = sims.index[-1]
#
#             pcp_date_value = readData.Precipitation(1, start, end)
#
#             pcp_date = [v[0] for v in pcp_date_value]
#             preci = [v[1] for v in pcp_date_value]
#
#             ax2 = ax.twinx()
#
#             ax2.bar(
#                 pcp_date,
#                 preci,
#                 label='Precipitation',
#                 color='blue',
#                 linewidth=0,
#                 align='center',
#                 alpha=0.6
#             )
#
#             if len(preci) > 0:
#                 ax2.set_ylim(float(max(preci)) * 4, float(min(preci)) * 0.8)
#
#             ax2.legend(
#                 frameon=False,
#                 fontsize=14,
#                 bbox_to_anchor=(0., 1.02, 1., 0.102),
#                 borderaxespad=0.,
#                 ncol=1,
#                 loc='lower right',
#                 fancybox=True
#             )
#
#         except Exception as e:
#             print("降雨读取失败，但主图继续输出:", e)
#
#         cali_dict = {}
#
#         sim_cols = [c for c in obssim.columns if c.startswith("Obssim_")]
#
#         for sim_col in sim_cols:
#
#             obs = obssim['Obs'].astype(float)
#             sim = obssim[sim_col].astype(float)
#
#             mask = np.isfinite(obs) & np.isfinite(sim)
#
#             obs = obs[mask]
#             sim = sim[mask]
#
#             cali_dict.setdefault("NSE", []).append(nashsutcliffe(obs, sim))
#             cali_dict.setdefault("KGE", []).append(kge(obs, sim))
#             cali_dict.setdefault("Rsquare", []).append(rsquared(obs, sim))
#             cali_dict.setdefault("pbias", []).append(pbias(obs, sim))
#
#         for index, name in enumerate(["NSE", "KGE", "Rsquare", "pbias"]):
#             ax.text(
#                 0.16,
#                 0.75 - index * 0.04,
#                 '%s=%0.2f(%0.2f,%0.2f)' % (
#                     name,
#                     np.nanmean(cali_dict[name]),
#                     np.nanmin(cali_dict[name]),
#                     np.nanmax(cali_dict[name])
#                 ),
#                 fontsize=16,
#                 transform=ax.transAxes
#             )
#
#         maxy = np.nanmax([
#             np.nanmax(np.nanmax(sims, axis=1)) * 1.8,
#             np.nanmax(obssim['Obs']) * 1.8
#         ])
#
#         ax.set_ylim(0, maxy)
#
#         ax.legend(
#             frameon=False,
#             fontsize=14,
#             bbox_to_anchor=(0., 1.02, 1., 0.102),
#             borderaxespad=0.,
#             ncol=3,
#             loc='lower left',
#             fancybox=True
#         )
#
#         ax.set_ylabel('%s' % kk, fontsize=15)
#         ax.tick_params('both', length=5, width=2, which='major', labelsize=15)
#
#         plt.tight_layout()
#
#         fig_path = result_dir + os.path.sep + r'%s_cali_continuous_remove_20100115_20100218.png' % kk
#
#         plt.savefig(fig_path, dpi=300)
#         plt.close()
#
#         print("输出图片:", fig_path)
#         print("///////查看结果///////")
#
#
# if __name__ == "__main__":
#     main()








# # ============================================================
# # 只画率定期的图
# # 修改内容：
# # 1. NN = 1，只选最优 1 组参数
# # 2. 删除 2010-01-15 到 2010-02-18 之间的数据
# # 3. 图上指标只显示一个数，例如 NSE=0.53
# #    不再显示 NSE=0.53(0.53,0.53)
# # ============================================================
#
# def main():
#     NN = 5
#
#     wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')
#
#     if wtsd_name not in list(DEMO_MODELS.keys()):
#         print('%s is not one of the available demo watershed: %s' %
#               (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
#         exit(-1)
#
#     cur_path = UtilClass.current_path(lambda: 0)
#     SEIMS_path = os.path.abspath(cur_path + '../../..')
#
#     model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
#
#     cf = ConfigParser()
#     cali_cfg_file = model_paths.cfg_dir + os.path.sep + 'calibration.ini'
#     cf.read(cali_cfg_file)
#
#     ngens = cf.getint('NSGA2', 'generationsnum')
#     npop = cf.getint('NSGA2', 'populationsize')
#
#     result_dir = model_paths.model_dir + os.path.sep + r'CALI_NSGA2_Gen_%s_Pop_%s' % (ngens, npop)
#     runtime_file = result_dir + os.path.sep + 'runtime.log'
#
#     print("读取 runtime.log:", runtime_file)
#
#     # ============================================================
#     # 读取 runtime.log
#     # ============================================================
#     df = pd.read_table(runtime_file, sep='\t', skiprows=2)
#     df.drop_duplicates(keep='first', inplace=True)
#     df = pd.DataFrame(df)
#
#     df = df.drop(df[df['generation'] == 'generation'].index)
#     df = df[~df['generation'].astype(str).str.contains('#')]
#
#     Ntar = int((len(df.columns) - 3) / 2)
#     index_ini = 2
#     index_end = int(1 + (len(df.columns) - 3) / 2)
#
#     print("率定目标：", Ntar, df.columns[index_ini], df.columns[index_end])
#     print(" ")
#
#     # ============================================================
#     # 计算加权排序，筛选最优参数集
#     # ============================================================
#     result = dict()
#
#     for i in range(0, Ntar):
#         result[df.columns[index_ini + i]] = list(df[df.columns[index_ini + i]].astype(float))
#
#     new = [0] * len(df)
#
#     for param, values in result.items():
#         temp2 = [1 - values[i] for i in range(len(values))]
#         temp = np.sqrt([x * y for x, y in zip(temp2, temp2)])
#         new = [new[i] + temp[i] for i in range(len(temp))]
#
#     stac = pd.DataFrame(new, columns=['statics'])
#     stac['FID'] = range(0, len(df))
#     stac['rank'] = stac['statics'].rank(ascending=True)
#
#     df['weight'] = stac['statics'].tolist()
#
#     select = stac.sort_values(by='rank')[0:NN].FID.tolist()
#
#     newdf = pd.DataFrame()
#     for i in select:
#         newdf = newdf.append(df[i:i + 1])
#
#     newdf = newdf.reset_index(drop=True)
#
#     print("/////筛选后参数集/////")
#     print(newdf)
#     print(" ")
#
#     result_csv = result_dir + os.path.sep + r'result_%s.csv' % NN
#     newdf.to_csv(result_csv, index=False)
#     print("输出最优参数集:", result_csv)
#
#     gen_selec = newdf['generation'].astype(int).tolist()
#     ID_selec = newdf['calibrationID'].astype(int).tolist()
#
#     # ============================================================
#     # 读取变量名
#     # ============================================================
#     with open(result_dir + os.path.sep + r'simulated_data\gen0_caliObsData.json',
#               'r', encoding='utf-8') as f:
#         sim_obs_data0 = json.load(f)
#
#     var_name = sim_obs_data0[0]['var_name']
#
#     # ============================================================
#     # 对每个输出变量画图
#     # ============================================================
#     for nn, kk in enumerate(var_name):
#
#         obssim = pd.DataFrame()
#         sims = pd.DataFrame()
#
#         for index, genID in enumerate(gen_selec):
#
#             gens = gen_selec[index]
#             IDs = ID_selec[index]
#
#             cali_obs_data = []
#             selected_sim_dict = None
#
#             # 读取率定期观测-模拟配对数据
#             with open(result_dir + os.path.sep + r'simulated_data\gen%s_caliObsData.json' % gens,
#                       'r', encoding='utf-8') as f:
#                 sim_obs_data = json.load(f)
#
#             # 读取率定期连续模拟数据
#             with open(result_dir + os.path.sep + r'simulated_data\gen%s_caliSimData.pickle' % gens,
#                       'rb') as f:
#                 sim_data = pickle.load(f)
#
#             # 找到当前最优 gen/id 对应的数据
#             for a, b in zip(sim_obs_data, sim_data):
#                 if a['Gen'] in [gens] and a['ID'] in [IDs]:
#                     cali_obs_data.append(a)
#                     selected_sim_dict = b
#                     break
#
#             if len(cali_obs_data) == 0 or selected_sim_dict is None:
#                 print("没有找到对应 gen/id 的数据:", gens, IDs)
#                 continue
#
#             # 观测值和配对模拟值
#             if index == 0:
#                 obssim['Date'] = list(cali_obs_data[0][kk]['UTCDATETIME'][:])
#                 obssim['Obs'] = list(cali_obs_data[0][kk]['Obs'][:])
#
#             obssim['Obssim_%s_%s' % (nn, index)] = list(cali_obs_data[0][kk]['Sim'][:])
#
#             # 连续模拟值
#             Date = []
#             value = []
#
#             for k, v in selected_sim_dict.items():
#                 Date.append(k)
#                 value.append(float(v[0]))
#
#             if index == 0:
#                 sims['Date'] = list(Date)
#
#             sims['sim_%s_%s' % (nn, index)] = value
#
#         # ============================================================
#         # 时间格式转换
#         # ============================================================
#         obssim['Date'] = pd.to_datetime(obssim['Date'])
#         obssim = obssim.set_index('Date')
#
#         sims['Date'] = pd.to_datetime(sims['Date'])
#         sims = sims.set_index('Date')
#
#         # ============================================================
#         # 删除 2010-01-15 到 2010-02-18 之间的数据
#         # 包括连续模拟数据 sims 和观测配对数据 obssim
#         # ============================================================
#         # remove_start = pd.to_datetime("2010-01-15")
#         # remove_end = pd.to_datetime("2010-02-18")
#         #
#         # sims = sims.loc[~((sims.index >= remove_start) & (sims.index <= remove_end))]
#         # obssim = obssim.loc[~((obssim.index >= remove_start) & (obssim.index <= remove_end))]
#
#         # remove_dates = pd.to_datetime([
#         #     "2010-04-19",
#         #     "2010-04-20"
#         # ])
#         #
#         # sims = sims.loc[~sims.index.normalize().isin(remove_dates)]
#         # obssim = obssim.loc[~obssim.index.normalize().isin(remove_dates)]
#
#         print()
#         print("///%s 率定期模拟结果提取结束///" % kk)
#
#         # ============================================================
#         # 输出 CSV
#         # ============================================================
#         output = pd.concat([sims, obssim[['Obs']]], axis=1)
#
#         output_csv = result_dir + os.path.sep + r'%s_cali_continuous_remove_20100115_20100218.csv' % kk
#         output.to_csv(output_csv)
#
#         print("输出 CSV:", output_csv)
#
#         # ============================================================
#         # 开始画图
#         # ============================================================
#         fig, ax = plt.subplots(1, 1, figsize=(22, 8), dpi=100)
#
#         # 观测值
#         ax.scatter(
#             x=obssim.index,
#             y=obssim['Obs'],
#             label='Observation',
#             color='#6F6F6F',
#             s=13,
#             zorder=5
#         )
#
#         # 模拟值
#         ax.plot(
#             sims.index,
#             np.median(sims, axis=1),
#             label='Simulation',
#             color='#C82423',
#             linewidth=2,
#             zorder=4
#         )
#
#         # 置信区间
#         low_CI_bound = sims.quantile(0.1, axis=1)
#         high_CI_bound = sims.quantile(0.9, axis=1)
#
#         ax.fill_between(
#             sims.index,
#             low_CI_bound,
#             high_CI_bound,
#             linewidth=1,
#             color='#F8AC8C',
#             label='10th - 90th percentile',
#             alpha=0.7,
#             zorder=3
#         )
#
#         # ============================================================
#         # 读取降雨并画右轴
#         # ============================================================
#         try:
#             HOSTNAME = cf.get('SEIMS_Model', 'HOSTNAME')
#             PORT = int(cf.get('SEIMS_Model', 'PORT'))
#
#             mongoclient = ConnectMongoDB(HOSTNAME, PORT).get_conn()
#             readData = ReadModelData(mongoclient, DEMO_MODELS[wtsd_name])
#
#             start = sims.index[0]
#             end = sims.index[-1]
#
#             pcp_date_value = readData.Precipitation(1, start, end)
#
#             pcp_date = [v[0] for v in pcp_date_value]
#             preci = [v[1] for v in pcp_date_value]
#
#             ax2 = ax.twinx()
#
#             ax2.bar(
#                 pcp_date,
#                 preci,
#                 label='Precipitation',
#                 color='blue',
#                 linewidth=0,
#                 align='center',
#                 alpha=0.6
#             )
#
#             if len(preci) > 0:
#                 ax2.set_ylim(float(max(preci)) * 4, float(min(preci)) * 0.8)
#
#             ax2.legend(
#                 frameon=False,
#                 fontsize=14,
#                 bbox_to_anchor=(0., 1.02, 1., 0.102),
#                 borderaxespad=0.,
#                 ncol=1,
#                 loc='lower right',
#                 fancybox=True
#             )
#
#         except Exception as e:
#             print("降雨读取失败，但主图继续输出:", e)
#
#         # ============================================================
#         # 计算评价指标
#         # ============================================================
#         cali_dict = {}
#
#         sim_cols = [c for c in obssim.columns if c.startswith("Obssim_")]
#
#         for sim_col in sim_cols:
#
#             obs = obssim['Obs'].astype(float)
#             sim = obssim[sim_col].astype(float)
#
#             mask = np.isfinite(obs) & np.isfinite(sim)
#
#             obs = obs[mask]
#             sim = sim[mask]
#
#             cali_dict.setdefault("NSE", []).append(nashsutcliffe(obs, sim))
#             cali_dict.setdefault("KGE", []).append(kge(obs, sim))
#             cali_dict.setdefault("Rsquare", []).append(rsquared(obs, sim))
#             cali_dict.setdefault("pbias", []).append(pbias(obs, sim))
#
#         # ============================================================
#         # 图上写评价指标
#         # 这里已经改成只显示一个数
#         # 原来是：NSE=0.53(0.53,0.53)
#         # 现在是：NSE=0.53
#         # ============================================================
#         for index, name in enumerate(["NSE", "KGE", "Rsquare", "pbias"]):
#
#             metric_value = np.nanmean(cali_dict[name])
#
#             ax.text(
#                 0.16,
#                 0.75 - index * 0.04,
#                 '%s=%0.2f' % (name, metric_value),
#                 fontsize=16,
#                 transform=ax.transAxes
#             )
#
#         # ============================================================
#         # 设置 y 轴范围
#         # ============================================================
#         maxy = np.nanmax([
#             np.nanmax(np.nanmax(sims, axis=1)) * 1.8,
#             np.nanmax(obssim['Obs']) * 1.8
#         ])
#
#         ax.set_ylim(0, maxy)
#
#         # ============================================================
#         # 图例和坐标轴
#         # ============================================================
#         ax.legend(
#             frameon=False,
#             fontsize=14,
#             bbox_to_anchor=(0., 1.02, 1., 0.102),
#             borderaxespad=0.,
#             ncol=3,
#             loc='lower left',
#             fancybox=True
#         )
#
#         ax.set_ylabel('%s' % kk, fontsize=15)
#         ax.tick_params('both', length=5, width=2, which='major', labelsize=15)
#
#         plt.tight_layout()
#
#         # ============================================================
#         # 保存图片
#         # ============================================================
#         fig_path = result_dir + os.path.sep + r'%s_cali_continuous_remove_20100115_20100218.png' % kk
#
#         plt.savefig(fig_path, dpi=300)
#         plt.close()
#
#         print("输出图片:", fig_path)
#         print("///////查看结果///////")
#
#
# if __name__ == "__main__":
#     main()




# ============================================================
# 只画率定期的图
# 修改内容：
# 1. NN = 5，仍然选择前 5 组参数，保持原始 NSE 计算口径不变
# 2. 修复连续模拟数据读取错误：
#    原来所有变量都读取 v[0]，导致 D 图实际画成 F 图
#    现在改为 v[nn]，每个变量读取自身对应的模拟结果
# 3. F_ 图纵轴改为 Inundation area (km²)
# 4. D_ 图纵轴改为 Water depth (m)
# 5. 评价指标计算方式保持原代码不变：
#    对每组参数分别计算指标，再求平均值
# 6. 当前不删除 2010-01-15 到 2010-02-18 之间的数据
# ============================================================

def main():

    # ============================================================
    # 选择最优参数组数量
    # 保持原来的 NN = 5，不改变评价指标计算口径
    # ============================================================
    NN = 1

    wtsd_name = get_watershed_name('Specify watershed name to run postprocess.')

    if wtsd_name not in list(DEMO_MODELS.keys()):
        print('%s is not one of the available demo watershed: %s' %
              (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
        exit(-1)

    cur_path = UtilClass.current_path(lambda: 0)
    SEIMS_path = os.path.abspath(cur_path + '../../..')

    model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])

    cf = ConfigParser()
    cali_cfg_file = model_paths.cfg_dir + os.path.sep + 'calibration.ini'
    cf.read(cali_cfg_file)

    ngens = cf.getint('NSGA2', 'generationsnum')
    npop = cf.getint('NSGA2', 'populationsize')

    result_dir = model_paths.model_dir + os.path.sep + \
                 r'CALI_NSGA2_Gen_%s_Pop_%s' % (ngens, npop)

    runtime_file = result_dir + os.path.sep + 'runtime.log'

    print("读取 runtime.log:", runtime_file)

    # ============================================================
    # 读取 runtime.log
    # ============================================================
    df = pd.read_table(runtime_file, sep='\t', skiprows=2)
    df.drop_duplicates(keep='first', inplace=True)
    df = pd.DataFrame(df)

    df = df.drop(df[df['generation'] == 'generation'].index)
    df = df[~df['generation'].astype(str).str.contains('#')]

    Ntar = int((len(df.columns) - 3) / 2)
    index_ini = 2
    index_end = int(1 + (len(df.columns) - 3) / 2)

    print("率定目标：", Ntar, df.columns[index_ini], df.columns[index_end])
    print(" ")

    # ============================================================
    # 计算加权排序，筛选最优参数集
    # ============================================================
    result = dict()

    for i in range(0, Ntar):
        result[df.columns[index_ini + i]] = \
            list(df[df.columns[index_ini + i]].astype(float))

    new = [0] * len(df)

    for param, values in result.items():

        temp2 = [1 - values[i] for i in range(len(values))]
        temp = np.sqrt([x * y for x, y in zip(temp2, temp2)])

        new = [new[i] + temp[i] for i in range(len(temp))]

    stac = pd.DataFrame(new, columns=['statics'])
    stac['FID'] = range(0, len(df))
    stac['rank'] = stac['statics'].rank(ascending=True)

    df['weight'] = stac['statics'].tolist()

    select = stac.sort_values(by='rank')[0:NN].FID.tolist()

    newdf = pd.DataFrame()

    for i in select:
        newdf = newdf.append(df[i:i + 1])

    newdf = newdf.reset_index(drop=True)

    print("/////筛选后参数集/////")
    print(newdf)
    print(" ")

    result_csv = result_dir + os.path.sep + r'result_%s.csv' % NN
    newdf.to_csv(result_csv, index=False)

    print("输出最优参数集:", result_csv)

    gen_selec = newdf['generation'].astype(int).tolist()
    ID_selec = newdf['calibrationID'].astype(int).tolist()

    # ============================================================
    # 读取变量名
    # ============================================================
    with open(result_dir + os.path.sep +
              r'simulated_data\gen0_caliObsData.json',
              'r', encoding='utf-8') as f:

        sim_obs_data0 = json.load(f)

    var_name = sim_obs_data0[0]['var_name']

    print("输出变量顺序：", var_name)
    print(" ")

    # ============================================================
    # 对每个输出变量分别画图
    # nn 表示变量在连续模拟数组中的位置
    # kk 表示当前变量名称，例如 F_19、D_1312 等
    # ============================================================
    for nn, kk in enumerate(var_name):

        print("============================================================")
        print("当前处理变量：", kk)
        print("当前变量索引：", nn)

        obssim = pd.DataFrame()
        sims = pd.DataFrame()

        # ========================================================
        # 读取每一组优选参数对应的数据
        # ========================================================
        for index, genID in enumerate(gen_selec):

            gens = gen_selec[index]
            IDs = ID_selec[index]

            cali_obs_data = []
            selected_sim_dict = None

            # ----------------------------------------------------
            # 读取率定期观测-模拟配对数据
            # ----------------------------------------------------
            with open(result_dir + os.path.sep +
                      r'simulated_data\gen%s_caliObsData.json' % gens,
                      'r', encoding='utf-8') as f:

                sim_obs_data = json.load(f)

            # ----------------------------------------------------
            # 读取率定期连续模拟数据
            # ----------------------------------------------------
            with open(result_dir + os.path.sep +
                      r'simulated_data\gen%s_caliSimData.pickle' % gens,
                      'rb') as f:

                sim_data = pickle.load(f)

            # ----------------------------------------------------
            # 找到当前最优 generation / calibrationID 对应的数据
            # ----------------------------------------------------
            for a, b in zip(sim_obs_data, sim_data):

                if a['Gen'] in [gens] and a['ID'] in [IDs]:
                    cali_obs_data.append(a)
                    selected_sim_dict = b
                    break

            if len(cali_obs_data) == 0 or selected_sim_dict is None:
                print("没有找到对应 gen/id 的数据:", gens, IDs)
                continue

            # ----------------------------------------------------
            # 读取观测值和与观测日期匹配的模拟值
            # 这一部分原代码就是正确的，因为使用的是当前变量 kk
            # ----------------------------------------------------
            if index == 0:
                obssim['Date'] = list(cali_obs_data[0][kk]['UTCDATETIME'][:])
                obssim['Obs'] = list(cali_obs_data[0][kk]['Obs'][:])

            obssim['Obssim_%s_%s' % (nn, index)] = \
                list(cali_obs_data[0][kk]['Sim'][:])

            # ----------------------------------------------------
            # 读取连续模拟值
            #
            # 关键修改位置：
            # 原来是 value.append(float(v[0]))
            # 这样所有变量都读取第一个变量，例如 F_19。
            #
            # 现在改成 value.append(float(v[nn]))
            # F_19   读取 v[0]
            # D_1312 读取 v[1]
            # D_1313 读取 v[2]
            # 依次类推。
            # ----------------------------------------------------
            Date = []
            value = []

            for k, v in selected_sim_dict.items():

                Date.append(k)

                if nn >= len(v):
                    raise IndexError(
                        "当前变量 %s 的索引 nn=%s 超过连续模拟数据长度 %s。"
                        "请检查 var_name 与 pickle 文件中的变量顺序是否一致。"
                        % (kk, nn, len(v))
                    )

                value.append(float(v[nn]))

            if index == 0:
                sims['Date'] = list(Date)

            sims['sim_%s_%s' % (nn, index)] = value

        # ============================================================
        # 如果当前变量没有成功提取数据，则跳过
        # ============================================================
        if obssim.empty or sims.empty:
            print("变量 %s 没有成功提取数据，跳过绘图。" % kk)
            continue

        # ============================================================
        # 时间格式转换
        # ============================================================
        obssim['Date'] = pd.to_datetime(obssim['Date'])
        obssim = obssim.set_index('Date')
        obssim = obssim.sort_index()

        sims['Date'] = pd.to_datetime(sims['Date'])
        sims = sims.set_index('Date')
        sims = sims.sort_index()

        # ============================================================
        # 当前保持原代码状态：
        # 不删除 2010-01-15 到 2010-02-18 之间的数据
        #
        # 如后续确实需要删除，再取消下面代码的注释。
        # 注意：取消注释后，图和评价指标都会变化。
        # ============================================================
        # remove_start = pd.to_datetime("2010-01-15")
        # remove_end = pd.to_datetime("2010-02-18")
        #
        # sims = sims.loc[
        #     ~((sims.index >= remove_start) & (sims.index <= remove_end))
        # ]
        #
        # obssim = obssim.loc[
        #     ~((obssim.index >= remove_start) & (obssim.index <= remove_end))
        # ]

        # ============================================================
        # 如需删除某几个单独日期，也可取消下面代码的注释
        # ============================================================
        # remove_dates = pd.to_datetime([
        #     "2010-04-19",
        #     "2010-04-20"
        # ])
        #
        # sims = sims.loc[~sims.index.normalize().isin(remove_dates)]
        # obssim = obssim.loc[~obssim.index.normalize().isin(remove_dates)]

        print()
        print("///%s 率定期模拟结果提取结束///" % kk)

        # ============================================================
        # 输出 CSV
        #
        # 保持你原来的文件命名不变。
        # 虽然名称中有 remove，但当前实际没有删除日期。
        # ============================================================
        output = pd.concat([sims, obssim[['Obs']]], axis=1)

        output_csv = result_dir + os.path.sep + \
                     r'%s_cali_continuous.csv' % kk

        output.to_csv(output_csv)

        print("输出 CSV:", output_csv)

        # ============================================================
        # 开始画图
        # ============================================================
        fig, ax = plt.subplots(1, 1, figsize=(22, 8), dpi=100)

        # ============================================================
        # 观测值散点
        # ============================================================
        ax.scatter(
            x=obssim.index,
            y=obssim['Obs'],
            label='Observation',
            color='#6F6F6F',
            s=13,
            zorder=5
        )

        # ============================================================
        # 模拟值曲线
        #
        # NN = 5 时，红线表示前 5 组模拟结果逐时间步的中位数。
        # 当前 sims 已经使用 v[nn] 读取，所以不同变量对应不同曲线。
        # ============================================================
        ax.plot(
            sims.index,
            np.median(sims, axis=1),
            label='Simulation',
            color='#C82423',
            linewidth=2,
            zorder=4
        )

        # ============================================================
        # 10% - 90% 模拟范围
        # ============================================================
        low_CI_bound = sims.quantile(0.1, axis=1)
        high_CI_bound = sims.quantile(0.9, axis=1)

        ax.fill_between(
            sims.index,
            low_CI_bound,
            high_CI_bound,
            linewidth=1,
            color='#F8AC8C',
            label='10th - 90th percentile',
            alpha=0.7,
            zorder=3
        )

        # ============================================================
        # 读取降雨并画右轴
        # ============================================================
        try:

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

            ax2.bar(
                pcp_date,
                preci,
                label='Precipitation',
                color='blue',
                linewidth=0,
                align='center',
                alpha=0.6
            )

            if len(preci) > 0:
                ax2.set_ylim(
                    float(max(preci)) * 4,
                    float(min(preci)) * 0.8
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

        except Exception as e:

            print("降雨读取失败，但主图继续输出:", e)

        # ============================================================
        # 计算评价指标
        #
        # 这里严格保持你原来的指标计算逻辑：
        # 1. 每一组参数分别计算 NSE、KGE、Rsquare、pbias
        # 2. 图中显示这些指标的平均值
        #
        # 因此，修复 D 图连续模拟曲线后，不会由于这里的修改导致
        # NSE 计算口径发生变化。
        # ============================================================
        cali_dict = {}

        sim_cols = [
            c for c in obssim.columns
            if c.startswith("Obssim_")
        ]

        for sim_col in sim_cols:

            obs = obssim['Obs'].astype(float)
            sim = obssim[sim_col].astype(float)

            mask = np.isfinite(obs) & np.isfinite(sim)

            obs = obs[mask]
            sim = sim[mask]

            cali_dict.setdefault("NSE", []).append(
                nashsutcliffe(obs, sim)
            )

            cali_dict.setdefault("KGE", []).append(
                kge(obs, sim)
            )

            cali_dict.setdefault("Rsquare", []).append(
                rsquared(obs, sim)
            )

            cali_dict.setdefault("pbias", []).append(
                pbias(obs, sim)
            )

        # ============================================================
        # 图上写评价指标
        # 保持原来只显示一个平均值的形式，例如 NSE=0.53
        # ============================================================
        for index, name in enumerate(["NSE", "KGE", "Rsquare", "pbias"]):

            metric_value = np.nanmean(cali_dict[name])

            ax.text(
                0.16,
                0.75 - index * 0.04,
                '%s=%0.2f' % (name, metric_value),
                fontsize=16,
                transform=ax.transAxes
            )

        # ============================================================
        # 设置 y 轴范围
        # ============================================================
        maxy = np.nanmax([
            np.nanmax(np.nanmax(sims, axis=1)) * 1.8,
            np.nanmax(obssim['Obs']) * 1.8
        ])

        ax.set_ylim(0, maxy)

        # ============================================================
        # 图例
        # ============================================================
        ax.legend(
            frameon=False,
            fontsize=14,
            bbox_to_anchor=(0., 1.02, 1., 0.102),
            borderaxespad=0.,
            ncol=3,
            loc='lower left',
            fancybox=True
        )

        # ============================================================
        # 设置不同变量的纵坐标名称
        #
        # F_ 开头：淹没面积，单位 km²
        # D_ 开头：水深，单位 m
        # ============================================================
        if kk.startswith('F_'):

            ax.set_ylabel(
                'Inundation area (km²)',
                fontsize=15
            )

        elif kk.startswith('D_'):

            ax.set_ylabel(
                'Water depth (m)',
                fontsize=15
            )

        else:

            ax.set_ylabel(
                '%s' % kk,
                fontsize=15
            )

        ax.tick_params(
            'both',
            length=5,
            width=2,
            which='major',
            labelsize=15
        )

        plt.tight_layout()

        # ============================================================
        # 保存图片
        #
        # 保持你原来的文件命名不变。
        # ============================================================
        fig_path = result_dir + os.path.sep + \
                    r'%s_cali_continuous.png' % kk

        plt.savefig(
            fig_path,
            dpi=300,
            bbox_inches='tight'
        )

        plt.close()

        print("输出图片:", fig_path)
        print("///////查看结果///////")
        print(" ")


if __name__ == "__main__":
    main()
