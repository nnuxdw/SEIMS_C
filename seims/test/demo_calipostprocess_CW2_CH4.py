# # -*- coding: utf-8 -*-
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
#
# import matplotlib.pyplot as plt
# import numpy as np
# import pandas as pd
# from spotpy.objectivefunctions import nashsutcliffe, rsquared, rmse, pbias, kge
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from test.demo_config import DEMO_MODELS, get_watershed_name
# from preprocess.db_mongodb import ConnectMongoDB
# from preprocess.db_read_model import ReadModelData
#
#
# # ============================================================
# # 用户设置
# # ============================================================
#
# # 率定结果目录：直接指定，不再读取 calibration.ini
# RESULT_DIR = (
#     r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model'
#     r'\CALI_NSGA2_Gen_100_Pop_80'
# )
#
# # 要读取的代数
# GENERATION_TO_PLOT = 100
#
# # 真正参与共享参数选择的三个水深点
# CALIB_DEPTH_VARS = ['D_1304', 'D_1305', 'D_1306']
#
# # 不确定性范围
# # True：2.5%—97.5%，即95PPU
# # False：10%—90%
# USE_95PPU = True
#
# # 是否绘制降雨
# PLOT_PRECIPITATION = True
#
# # MongoDB连接信息
# MONGO_HOST = '127.0.0.1'
# MONGO_PORT = 27017
#
# # 读取降雨时使用的子流域编号
# PRECIP_SUBBASIN_ID = 1
#
#
# def safe_float(value, default=np.nan):
#     """安全转换为浮点数。"""
#     try:
#         return float(value)
#     except (TypeError, ValueError):
#         return default
#
#
# def safe_metric(one_individual, variable, metric_name):
#     """从JSON记录中读取一个变量的评价指标。"""
#     try:
#         return safe_float(one_individual[variable][metric_name])
#     except (KeyError, TypeError):
#         return np.nan
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前代候选参数中选择一套共享参数。
#
#     规则：
#     1. 只使用 D_1304、D_1305、D_1306 的RMSE；
#     2. 先最小化三个点中的最大RMSE；
#     3. 再最小化三个点的平均RMSE。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         depth_rmses = np.asarray(
#             [safe_metric(one_ind, var, 'RMSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#
#         if not np.isfinite(depth_rmses).all():
#             continue
#
#         if np.any(depth_rmses < 0.0) or np.any(depth_rmses >= 9000.0):
#             continue
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#             'rmses': depth_rmses,
#             'max_rmse': float(np.max(depth_rmses)),
#             'mean_rmse': float(np.mean(depth_rmses))
#         })
#
#     if not candidates:
#         raise ValueError(
#             '没有找到同时包含 D_1304、D_1305、D_1306 '
#             '有效RMSE的候选参数组。'
#         )
#
#     candidates.sort(
#         key=lambda item: (
#             item['max_rmse'],
#             item['mean_rmse']
#         )
#     )
#
#     best = candidates[0]
#
#     print('=' * 70)
#     print('共享参数选择结果')
#     print('规则：先最小化最大RMSE，再最小化平均RMSE')
#     print('候选数组索引:', best['idx'])
#     print('Generation:', best['gen'])
#     print('Calibration ID:', best['id'])
#
#     for var, value in zip(CALIB_DEPTH_VARS, best['rmses']):
#         print('%s RMSE = %.6f' % (var, value))
#
#     print('最大RMSE = %.6f' % best['max_rmse'])
#     print('平均RMSE = %.6f' % best['mean_rmse'])
#     print('参数值 =', best['gene_values'])
#     print('=' * 70)
#
#     return best['idx'], best
#
#
# def read_generation_files(simulated_dir, generation):
#     """读取指定代的观测配对数据和连续模拟数据。"""
#     obs_file = os.path.join(
#         simulated_dir,
#         'gen%d_caliObsData.json' % generation
#     )
#
#     sim_file = os.path.join(
#         simulated_dir,
#         'gen%d_caliSimData.pickle' % generation
#     )
#
#     if not os.path.exists(obs_file):
#         raise FileNotFoundError('找不到文件: %s' % obs_file)
#
#     if not os.path.exists(sim_file):
#         raise FileNotFoundError('找不到文件: %s' % sim_file)
#
#     print('读取:', obs_file)
#     print('读取:', sim_file)
#
#     with open(obs_file, 'r', encoding='utf-8') as f:
#         sim_obs_data = json.load(f)
#
#     with open(sim_file, 'rb') as f:
#         sim_data = pickle.load(f)
#
#     if len(sim_obs_data) != len(sim_data):
#         raise ValueError(
#             'JSON候选数与pickle候选数不一致: %d != %d'
#             % (len(sim_obs_data), len(sim_data))
#         )
#
#     if len(sim_obs_data) == 0:
#         raise ValueError('当前代没有候选参数结果。')
#
#     return sim_obs_data, sim_data
#
#
# def extract_variable_series(sim_dict, variable_index, variable_name):
#     """从连续模拟字典中提取指定变量。"""
#     dates = []
#     values = []
#
#     for date_value, all_values in sorted(
#         sim_dict.items(),
#         key=lambda item: item[0]
#     ):
#         if variable_index >= len(all_values):
#             raise IndexError(
#                 '变量 %s 的索引 %d 超过模拟数组长度 %d。'
#                 '请检查var_name与pickle变量顺序。'
#                 % (
#                     variable_name,
#                     variable_index,
#                     len(all_values)
#                 )
#             )
#
#         dates.append(pd.to_datetime(date_value))
#         values.append(safe_float(all_values[variable_index]))
#
#     return pd.Series(
#         values,
#         index=pd.DatetimeIndex(dates),
#         name=variable_name
#     )
#
#
# def calculate_metrics(obs_values, sim_values):
#     """计算观测日期上的评价指标。"""
#     obs = pd.to_numeric(
#         pd.Series(obs_values),
#         errors='coerce'
#     )
#
#     sim = pd.to_numeric(
#         pd.Series(sim_values),
#         errors='coerce'
#     )
#
#     valid = (
#         np.isfinite(obs.values) &
#         np.isfinite(sim.values)
#     )
#
#     obs = obs.values[valid]
#     sim = sim.values[valid]
#
#     if len(obs) == 0:
#         return {
#             'NSE': np.nan,
#             'KGE': np.nan,
#             'Rsquare': np.nan,
#             'PBIAS': np.nan,
#             'RMSE': np.nan
#         }
#
#     return {
#         'NSE': nashsutcliffe(obs, sim),
#         'KGE': kge(obs, sim),
#         'Rsquare': rsquared(obs, sim),
#         'PBIAS': pbias(obs, sim),
#         'RMSE': rmse(obs, sim)
#     }
#
#
# def save_selected_parameter_info(
#     result_dir,
#     generation,
#     best_info,
#     selected_record
# ):
#     """保存选中的共享参数及指标。"""
#     out_file = os.path.join(
#         result_dir,
#         'Gen%d_selected_shared_parameter_set.txt'
#         % generation
#     )
#
#     with open(out_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: minimize maximum RMSE first, '
#             'then minimize mean RMSE; '
#             'D_1304, D_1305 and D_1306 only.\n'
#         )
#
#         f.write('Generation: %s\n' % str(best_info['gen']))
#         f.write('Calibration ID: %s\n' % str(best_info['id']))
#         f.write('Maximum RMSE: %.10f\n' % best_info['max_rmse'])
#         f.write('Mean RMSE: %.10f\n' % best_info['mean_rmse'])
#         f.write(
#             'Parameter values: %s\n'
#             % str(best_info['gene_values'])
#         )
#
#         f.write('\nMetrics stored in JSON:\n')
#
#         for var in selected_record.get('var_name', []):
#             if var not in selected_record:
#                 continue
#
#             f.write('\n[%s]\n' % var)
#
#             for metric_name in [
#                 'NSE',
#                 'RMSE',
#                 'RSR',
#                 'PBIAS',
#                 'R-square'
#             ]:
#                 value = safe_metric(
#                     selected_record,
#                     var,
#                     metric_name
#                 )
#
#                 f.write(
#                     '%s: %s\n'
#                     % (metric_name, str(value))
#                 )
#
#     print('输出共享参数信息:', out_file)
#
#
# def read_precipitation(
#     watershed_name,
#     start_date,
#     end_date
# ):
#     """从MongoDB读取降雨。"""
#     mongo_client = ConnectMongoDB(
#         MONGO_HOST,
#         MONGO_PORT
#     ).get_conn()
#
#     read_data = ReadModelData(
#         mongo_client,
#         DEMO_MODELS[watershed_name]
#     )
#
#     precipitation = read_data.Precipitation(
#         PRECIP_SUBBASIN_ID,
#         start_date,
#         end_date
#     )
#
#     pcp_dates = [item[0] for item in precipitation]
#     pcp_values = [safe_float(item[1], 0.0) for item in precipitation]
#
#     return pcp_dates, pcp_values
#
#
# def main():
#     print('当前运行文件:', os.path.abspath(__file__))
#     print('率定结果目录:', RESULT_DIR)
#     print('读取代数:', GENERATION_TO_PLOT)
#
#     if not os.path.exists(RESULT_DIR):
#         raise FileNotFoundError(
#             '找不到率定结果目录: %s' % RESULT_DIR
#         )
#
#     simulated_dir = os.path.join(
#         RESULT_DIR,
#         'simulated_data'
#     )
#
#     if not os.path.exists(simulated_dir):
#         raise FileNotFoundError(
#             '找不到 simulated_data 文件夹: %s'
#             % simulated_dir
#         )
#
#     wtsd_name = get_watershed_name(
#         'Specify watershed name to run postprocess.'
#     )
#
#     if wtsd_name not in DEMO_MODELS:
#         print(
#             '%s is not one of the available demo watershed: %s'
#             % (
#                 wtsd_name,
#                 ','.join(DEMO_MODELS.keys())
#             )
#         )
#         sys.exit(-1)
#
#     sim_obs_data, sim_data = read_generation_files(
#         simulated_dir,
#         GENERATION_TO_PLOT
#     )
#
#     variable_names = sim_obs_data[0].get(
#         'var_name',
#         []
#     )
#
#     if not variable_names:
#         raise ValueError(
#             'gen%d_caliObsData.json中没有var_name。'
#             % GENERATION_TO_PLOT
#         )
#
#     missing = [
#         var for var in CALIB_DEPTH_VARS
#         if var not in variable_names
#     ]
#
#     if missing:
#         raise ValueError(
#             '变量列表中缺少核心率定点: %s'
#             % ', '.join(missing)
#         )
#
#     print('输出变量顺序:', variable_names)
#
#     # 使用旧图相同的选参规则
#     shared_best_idx, best_info = select_shared_best_index(
#         sim_obs_data
#     )
#
#     selected_record = sim_obs_data[shared_best_idx]
#     selected_sim_dict = sim_data[shared_best_idx]
#
#     save_selected_parameter_info(
#         RESULT_DIR,
#         GENERATION_TO_PLOT,
#         best_info,
#         selected_record
#     )
#
#     for variable_index, variable_name in enumerate(
#         variable_names
#     ):
#         print('\n' + '=' * 70)
#         print('当前变量:', variable_name)
#         print('变量索引:', variable_index)
#
#         if variable_name not in selected_record:
#             print('该变量不在所选参数组记录中，跳过。')
#             continue
#
#         # --------------------------------------------------------
#         # 观测值及观测日期对应的模拟值
#         # --------------------------------------------------------
#         obs_dates = pd.to_datetime(
#             selected_record[variable_name]['UTCDATETIME']
#         )
#
#         obs_values = pd.to_numeric(
#             pd.Series(
#                 selected_record[variable_name]['Obs']
#             ),
#             errors='coerce'
#         ).values
#
#         sim_at_obs = pd.to_numeric(
#             pd.Series(
#                 selected_record[variable_name]['Sim']
#             ),
#             errors='coerce'
#         ).values
#
#         paired = pd.DataFrame(
#             {
#                 'Obs': obs_values,
#                 'Sim_at_obs': sim_at_obs
#             },
#             index=obs_dates
#         ).sort_index()
#
#         # --------------------------------------------------------
#         # 选中共享参数组的完整连续模拟
#         # --------------------------------------------------------
#         selected_series = extract_variable_series(
#             selected_sim_dict,
#             variable_index,
#             variable_name
#         ).sort_index()
#
#         # --------------------------------------------------------
#         # 当前代全部候选参数，用于计算不确定性范围
#         # --------------------------------------------------------
#         candidate_series = []
#
#         for candidate_dict in sim_data:
#             series = extract_variable_series(
#                 candidate_dict,
#                 variable_index,
#                 variable_name
#             ).sort_index()
#
#             if not series.index.equals(
#                 selected_series.index
#             ):
#                 series = series.reindex(
#                     selected_series.index
#                 )
#
#             candidate_series.append(
#                 series.values
#             )
#
#         candidate_array = np.asarray(
#             candidate_series,
#             dtype=float
#         )
#
#         if USE_95PPU:
#             lower = np.nanpercentile(
#                 candidate_array,
#                 2.5,
#                 axis=0
#             )
#
#             upper = np.nanpercentile(
#                 candidate_array,
#                 97.5,
#                 axis=0
#             )
#
#             uncertainty_label = '95PPU'
#         else:
#             lower = np.nanpercentile(
#                 candidate_array,
#                 10.0,
#                 axis=0
#             )
#
#             upper = np.nanpercentile(
#                 candidate_array,
#                 90.0,
#                 axis=0
#             )
#
#             uncertainty_label = (
#                 '10th - 90th percentile'
#             )
#
#         # --------------------------------------------------------
#         # 评价指标：只针对选中的同一套共享参数
#         # --------------------------------------------------------
#         metrics = calculate_metrics(
#             paired['Obs'].values,
#             paired['Sim_at_obs'].values
#         )
#
#         print('重新计算指标:')
#
#         for metric_name, metric_value in metrics.items():
#             print(
#                 '  %s = %s'
#                 % (metric_name, metric_value)
#             )
#
#         print('JSON中保存的指标:')
#
#         for metric_name in [
#             'NSE',
#             'RMSE',
#             'RSR',
#             'PBIAS',
#             'R-square'
#         ]:
#             print(
#                 '  %s = %s'
#                 % (
#                     metric_name,
#                     safe_metric(
#                         selected_record,
#                         variable_name,
#                         metric_name
#                     )
#                 )
#             )
#
#         # --------------------------------------------------------
#         # 输出CSV
#         # --------------------------------------------------------
#         output = pd.DataFrame(
#             {
#                 'Selected_simulation':
#                     selected_series.values,
#                 'Uncertainty_lower':
#                     lower,
#                 'Uncertainty_upper':
#                     upper
#             },
#             index=selected_series.index
#         )
#
#         output = pd.concat(
#             [
#                 output,
#                 paired[['Obs', 'Sim_at_obs']]
#             ],
#             axis=1
#         )
#
#         output.index.name = 'Date'
#
#         output_csv = os.path.join(
#             RESULT_DIR,
#             '%s_cali_continuous.csv'
#             % variable_name
#         )
#
#         output.to_csv(output_csv)
#
#         print('输出CSV:', output_csv)
#
#         # --------------------------------------------------------
#         # 绘图
#         # --------------------------------------------------------
#         fig, ax = plt.subplots(
#             figsize=(22, 8),
#             dpi=100
#         )
#
#         ax.scatter(
#             paired.index,
#             paired['Obs'],
#             label='Observation',
#             s=16,
#             color='#ff7f0e',
#             zorder=4
#         )
#
#         ax.plot(
#             selected_series.index,
#             selected_series.values,
#             label='Shared-parameter simulation',
#             linewidth=1.5,
#             color='#1f77b4',
#             zorder=3
#         )
#
#         # --------------------------------------------------------
#         # 指标文字
#         # --------------------------------------------------------
#         metric_lines = [
#             'NSE=%.2f' % metrics['NSE'],
#             'KGE=%.2f' % metrics['KGE'],
#             'R$^2$=%.2f' % metrics['Rsquare'],
#             'PBIAS=%.2f%%' % metrics['PBIAS'],
#             'RMSE=%.2f' % metrics['RMSE']
#         ]
#
#         ax.text(
#             0.16,
#             0.76,
#             '\n'.join(metric_lines),
#             fontsize=16,
#             transform=ax.transAxes,
#             va='top'
#         )
#
#         # --------------------------------------------------------
#         # 读取并绘制降雨
#         # --------------------------------------------------------
#         ax2 = None
#
#         if PLOT_PRECIPITATION:
#             try:
#                 pcp_dates, pcp_values = read_precipitation(
#                     wtsd_name,
#                     selected_series.index[0],
#                     selected_series.index[-1]
#                 )
#
#                 if len(pcp_values) > 0:
#                     ax2 = ax.twinx()
#
#                     ax2.bar(
#                         pcp_dates,
#                         pcp_values,
#                         label='Precipitation',
#                         linewidth=0,
#                         alpha=0.75,
#                         color='#3F007D'
#                     )
#
#                     max_precip = float(
#                         np.nanmax(pcp_values)
#                     )
#
#                     if not np.isfinite(max_precip):
#                         max_precip = 0.0
#
#                     ax2.set_ylim(
#                         max_precip * 4.0
#                         if max_precip > 0
#                         else 1.0,
#                         0.0
#                     )
#
#                     ax2.set_ylabel(
#                         'Precipitation (mm)',
#                         fontsize=14
#                     )
#
#                     ax2.tick_params(
#                         axis='y',
#                         labelsize=13
#                     )
#
#             except Exception as exc:
#                 print(
#                     '降雨读取失败，但主图继续输出:',
#                     exc
#                 )
#
#         # --------------------------------------------------------
#         # 坐标轴
#         # --------------------------------------------------------
#         if variable_name.startswith('F_'):
#             ax.set_ylabel(
#                 'Inundation area (km²)',
#                 fontsize=15
#             )
#         elif variable_name.startswith('D_'):
#             ax.set_ylabel(
#                 'Water depth (m)',
#                 fontsize=15
#             )
#         else:
#             ax.set_ylabel(
#                 variable_name,
#                 fontsize=15
#             )
#
#         ax.set_xlabel(
#             'Date time',
#             fontsize=15
#         )
#
#         ax.tick_params(
#             'both',
#             length=5,
#             width=1.5,
#             which='major',
#             labelsize=14
#         )
#
#         # --------------------------------------------------------
#         # 合并图例
#         # --------------------------------------------------------
#         handles1, labels1 = (
#             ax.get_legend_handles_labels()
#         )
#
#         if ax2 is not None:
#             handles2, labels2 = (
#                 ax2.get_legend_handles_labels()
#             )
#
#             ax.legend(
#                 handles1 + handles2,
#                 labels1 + labels2,
#                 frameon=False,
#                 fontsize=13,
#                 loc='upper left',
#                 ncol=2
#             )
#         else:
#             ax.legend(
#                 frameon=False,
#                 fontsize=13,
#                 loc='upper left',
#                 ncol=3
#             )
#
#         # --------------------------------------------------------
#         # 自动设置主Y轴范围
#         # --------------------------------------------------------
#         finite_parts = []
#
#         for values in [
#             selected_series.values,
#             paired['Obs'].values,
#             lower,
#             upper
#         ]:
#             values = np.asarray(
#                 values,
#                 dtype=float
#             )
#
#             valid_values = values[
#                 np.isfinite(values)
#             ]
#
#             if len(valid_values) > 0:
#                 finite_parts.append(valid_values)
#
#         if finite_parts:
#             finite_values = np.concatenate(
#                 finite_parts
#             )
#
#             ymax = float(
#                 np.nanpercentile(
#                     finite_values,
#                     99.5
#                 )
#             )
#
#             if (
#                 not np.isfinite(ymax)
#                 or ymax <= 0
#             ):
#                 ymax = float(
#                     np.nanmax(finite_values)
#                 )
#
#             if (
#                 np.isfinite(ymax)
#                 and ymax > 0
#             ):
#                 ax.set_ylim(
#                     0.0,
#                     ymax * 1.15
#                 )
#
#         plt.tight_layout()
#
#         fig_path = os.path.join(
#             RESULT_DIR,
#             '%s_cali_continuous.png'
#             % variable_name
#         )
#
#         plt.savefig(
#             fig_path,
#             dpi=300,
#             bbox_inches='tight'
#         )
#
#         plt.close(fig)
#
#         print('输出图片:', fig_path)
#
#     print('\n全部变量处理完成。')
#
#
# if __name__ == '__main__':
#     main()




# # -*- coding: utf-8 -*-
# """
# 四个水深点 + F_18 NSE 率定结果的独立后处理脚本。
#
# 与当前率定主程序和 calibration/userdef.py 保持一致：
# 1. 参与统一选参的五个 NSE 目标：
#    D_1366、D_1367、D_1368、D_1370、F_18；
# 2. 先最大化五个 NSE 中的最小值，再最大化五个 NSE 的平均值；
# 3. D_1375 只用于辅助验证和绘图，不参与统一选参；
# 4. 从 gen*_caliSimData.pickle 读取完整连续模拟序列，而不是只画观测日期；
# 5. 当前代所有候选参数组用于计算 95PPU；
# 6. 六个变量始终使用同一个 shared_best_idx。
# """
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
# import numpy as np
# import pandas as pd
# from spotpy.objectivefunctions import kge, nashsutcliffe, pbias, rmse, rsquared
#
# # SEIMS Python工程根目录。这样脚本放在任意文件夹也能导入工程模块。
# SEIMS_CODE_ROOT = r'D:\SEIMS_C\seims'
# for candidate_path in [
#     SEIMS_CODE_ROOT,
#     os.path.abspath(os.path.join(sys.path[0], '..')),
# ]:
#     if os.path.isdir(candidate_path) and candidate_path not in sys.path:
#         sys.path.insert(0, candidate_path)
#
# from test.demo_config import DEMO_MODELS, get_watershed_name
# from preprocess.db_mongodb import ConnectMongoDB
# from preprocess.db_read_model import ReadModelData
#
#
# # ============================================================================
# # 用户设置：运行前重点检查这里
# # ============================================================================
#
# # 修改成“4个水深点 + F_18 NSE”这一轮率定的实际结果目录。
# # 当前与你上传的userdef.py一致，先设为Pop_12。
# # 正式运行目录如果是Pop_100，只需把末尾Pop_12改成Pop_100。
# RESULT_DIR = (
#     r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model'
#     r'\CALI_NSGA2_Gen_100_Pop_100'
# )
#
# # 要读取和重画的代数。
# GENERATION_TO_PLOT = 100
#
# # 结果单独放进该文件夹，避免覆盖率定程序原图。
# OUTPUT_DIR = os.path.join(
#     RESULT_DIR,
#     'Gen%d_continuous_shared_NSE' % GENERATION_TO_PLOT
# )
#
# # 与率定主程序 multiobj 完全一致的五个 NSE 目标。
# OBJECTIVE_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
#     'F_18',
# ]
#
# # 固定输出顺序。D_1375只绘图和诊断，不参与选参。
# PLOT_VARS = [
#     'F_18',
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
#     'D_1375',
# ]
#
# # True：2.5%—97.5%（95PPU）；False：10%—90%。
# USE_95PPU = True
#
# # 是否绘制降雨。
# PLOT_PRECIPITATION = True
#
# # 当前研究对象为18号子流域。
# PRECIP_SUBBASIN_ID = 18
#
# # MongoDB。
# MONGO_HOST = '127.0.0.1'
# MONGO_PORT = 27017
#
# # 如果 DEMO_MODELS 中存在 CW_2，会直接使用；否则运行时让你选择。
# WATERSHED_NAME = 'CW_2'
#
# # 图片设置。
# FIGSIZE = (22, 8)
# FIG_DPI = 300
#
#
# # ============================================================================
# # 通用辅助函数
# # ============================================================================
#
# def safe_float(value, default=np.nan):
#     """安全转换为浮点数。"""
#     try:
#         return float(value)
#     except (TypeError, ValueError):
#         return default
#
#
# def safe_metric(one_individual, variable, metric_name):
#     """从一个JSON候选记录中安全读取评价指标。"""
#     try:
#         return safe_float(one_individual[variable][metric_name])
#     except (KeyError, TypeError):
#         return np.nan
#
#
# def normalize_datetime_index(values):
#     """统一转换为不带时区的 DatetimeIndex。"""
#     dt = pd.to_datetime(values, errors='coerce', utc=True)
#     if isinstance(dt, pd.Series):
#         dt = pd.DatetimeIndex(dt)
#     elif not isinstance(dt, pd.DatetimeIndex):
#         dt = pd.DatetimeIndex(dt)
#     return dt.tz_convert(None)
#
#
# def read_generation_files(simulated_dir, generation):
#     """读取指定代的观测配对数据和完整率定期模拟数据。"""
#     obs_file = os.path.join(
#         simulated_dir,
#         'gen%d_caliObsData.json' % generation
#     )
#     sim_file = os.path.join(
#         simulated_dir,
#         'gen%d_caliSimData.pickle' % generation
#     )
#
#     if not os.path.exists(obs_file):
#         raise FileNotFoundError('找不到文件: %s' % obs_file)
#     if not os.path.exists(sim_file):
#         raise FileNotFoundError('找不到文件: %s' % sim_file)
#
#     print('读取JSON:', obs_file)
#     print('读取pickle:', sim_file)
#
#     with open(obs_file, 'r', encoding='utf-8') as f:
#         sim_obs_data = json.load(f)
#
#     with open(sim_file, 'rb') as f:
#         sim_data = pickle.load(f)
#
#     if len(sim_obs_data) == 0:
#         raise ValueError('当前代没有候选参数结果。')
#     if len(sim_obs_data) != len(sim_data):
#         raise ValueError(
#             'JSON候选数与pickle候选数不一致: %d != %d'
#             % (len(sim_obs_data), len(sim_data))
#         )
#
#     return sim_obs_data, sim_data
#
#
# def extract_variable_series(sim_dict, variable_index, variable_name):
#     """
#     从 gen*_caliSimData.pickle 的一个候选字典中提取一个变量的
#     完整连续模拟序列。
#
#     sim_dict 的格式应为：datetime -> [F_18, D_1366, ...]。
#     """
#     rows = []
#
#     for date_value, all_values in sim_dict.items():
#         date_value = pd.to_datetime(date_value, errors='coerce', utc=True)
#         if pd.isna(date_value):
#             continue
#
#         if not isinstance(all_values, (list, tuple, np.ndarray)):
#             all_values = [all_values]
#
#         if variable_index >= len(all_values):
#             raise IndexError(
#                 '变量 %s 的索引 %d 超过模拟数组长度 %d。'
#                 '请检查JSON中的var_name与pickle中的变量顺序。'
#                 % (variable_name, variable_index, len(all_values))
#             )
#
#         rows.append((date_value.tz_convert(None), safe_float(all_values[variable_index])))
#
#     if not rows:
#         return pd.Series(dtype=float, name=variable_name)
#
#     rows.sort(key=lambda item: item[0])
#     series = pd.Series(
#         [item[1] for item in rows],
#         index=pd.DatetimeIndex([item[0] for item in rows]),
#         name=variable_name,
#         dtype=float
#     )
#
#     # 极少数情况下若出现重复日期，取同一天最后一个值，避免reindex报错。
#     if series.index.has_duplicates:
#         series = series.groupby(level=0).last()
#
#     return series.sort_index()
#
#
# def build_paired_observation_table(selected_record, variable_name):
#     """构造观测日期、观测值和观测日期对应模拟值的配对表。"""
#     one_var = selected_record[variable_name]
#
#     obs_dates = normalize_datetime_index(one_var.get('UTCDATETIME', []))
#     obs_values = pd.to_numeric(
#         pd.Series(one_var.get('Obs', [])),
#         errors='coerce'
#     ).values
#     sim_values = pd.to_numeric(
#         pd.Series(one_var.get('Sim', [])),
#         errors='coerce'
#     ).values
#
#     n = min(len(obs_dates), len(obs_values), len(sim_values))
#     if n == 0:
#         return pd.DataFrame(columns=['Obs', 'Sim_at_obs'])
#
#     paired = pd.DataFrame(
#         {
#             'Obs': obs_values[:n],
#             'Sim_at_obs': sim_values[:n],
#         },
#         index=obs_dates[:n]
#     )
#
#     paired = paired[~paired.index.isna()].sort_index()
#     if paired.index.has_duplicates:
#         paired = paired.groupby(level=0).mean()
#     return paired
#
#
# def calculate_metrics(obs_values, sim_values):
#     """在观测日期上重新计算指标。"""
#     obs = np.asarray(obs_values, dtype=float)
#     sim = np.asarray(sim_values, dtype=float)
#     valid = np.isfinite(obs) & np.isfinite(sim)
#     obs = obs[valid]
#     sim = sim[valid]
#
#     if len(obs) == 0:
#         return {
#             'NSE': np.nan,
#             'KGE': np.nan,
#             'R-square': np.nan,
#             'PBIAS': np.nan,
#             'RMSE': np.nan,
#         }
#
#     return {
#         'NSE': nashsutcliffe(obs, sim),
#         'KGE': kge(obs, sim),
#         'R-square': rsquared(obs, sim),
#         'PBIAS': pbias(obs, sim),
#         'RMSE': rmse(obs, sim),
#     }
#
#
# def format_metric(value):
#     """绘图文字格式。"""
#     return 'nan' if not np.isfinite(value) else '%.2f' % value
#
#
# # ============================================================================
# # 统一参数选择：严格与当前 userdef.py 一致
# # ============================================================================
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前代候选种群中选一套统一参数。
#
#     五个目标：D_1366、D_1367、D_1368、D_1370、F_18 的 NSE。
#     排序规则：
#       1. 最大化五个NSE中的最小NSE；
#       2. 若最小NSE相同，再最大化五个NSE的平均NSE。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         nse_values = np.asarray(
#             [safe_metric(one_ind, var, 'NSE') for var in OBJECTIVE_VARS],
#             dtype=float
#         )
#
#         if not np.isfinite(nse_values).all():
#             continue
#         if np.any(nse_values <= -9000.0):
#             continue
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#             'nse_values': nse_values,
#             'min_nse': float(np.min(nse_values)),
#             'mean_nse': float(np.mean(nse_values)),
#         })
#
#     if not candidates:
#         raise ValueError(
#             '当前代没有同时包含 D_1366、D_1367、D_1368、D_1370、F_18 '
#             '五个有效NSE的候选参数组。'
#         )
#
#     candidates.sort(
#         key=lambda item: (
#             -item['min_nse'],
#             -item['mean_nse'],
#         )
#     )
#
#     best = candidates[0]
#
#     print('=' * 78)
#     print('统一参数选择结果')
#     print('规则1：最大化五个NSE目标中的最小NSE')
#     print('规则2：再最大化五个NSE目标的平均NSE')
#     print('shared_best_idx =', best['idx'])
#     print('来源Generation =', best['gen'])
#     print('来源Calibration ID =', best['id'])
#     print('最小NSE = %.10f' % best['min_nse'])
#     print('平均NSE = %.10f' % best['mean_nse'])
#
#     for var, value in zip(OBJECTIVE_VARS, best['nse_values']):
#         print('%s NSE = %.10f' % (var, value))
#
#     print('参数值 =', best['gene_values'])
#     print('=' * 78)
#
#     return best['idx'], best
#
#
# def diagnose_best_each_variable(sim_obs_data):
#     """打印每个变量单独能达到的最大NSE和最小RMSE，仅用于诊断。"""
#     print('=' * 78)
#     print('当前代各变量单独最优指标，仅用于诊断，不用于统一选参：')
#
#     for var in PLOT_VARS:
#         nse_items = []
#         rmse_items = []
#
#         for idx, one_ind in enumerate(sim_obs_data):
#             nse_value = safe_metric(one_ind, var, 'NSE')
#             rmse_value = safe_metric(one_ind, var, 'RMSE')
#
#             if np.isfinite(nse_value) and nse_value > -9000.0:
#                 nse_items.append((nse_value, idx))
#             if np.isfinite(rmse_value) and 0.0 <= rmse_value < 9000.0:
#                 rmse_items.append((rmse_value, idx))
#
#         best_nse_text = '无有效NSE'
#         best_rmse_text = '无有效RMSE'
#
#         if nse_items:
#             best_nse, best_nse_idx = max(nse_items, key=lambda x: x[0])
#             best_nse_text = 'max NSE=%.6f, idx=%d' % (best_nse, best_nse_idx)
#
#         if rmse_items:
#             best_rmse, best_rmse_idx = min(rmse_items, key=lambda x: x[0])
#             best_rmse_text = 'min RMSE=%.6f, idx=%d' % (best_rmse, best_rmse_idx)
#
#         print('%s: %s; %s' % (var, best_nse_text, best_rmse_text))
#
#     print('=' * 78)
#
#
# # ============================================================================
# # 95PPU和参数信息
# # ============================================================================
#
# def calculate_uncertainty(candidate_array):
#     """按设置计算不确定性上下限。"""
#     if USE_95PPU:
#         lower = np.nanpercentile(candidate_array, 2.5, axis=0)
#         upper = np.nanpercentile(candidate_array, 97.5, axis=0)
#         label = '95PPU'
#     else:
#         lower = np.nanpercentile(candidate_array, 10.0, axis=0)
#         upper = np.nanpercentile(candidate_array, 90.0, axis=0)
#         label = '10th–90th percentile'
#     return lower, upper, label
#
#
# def calculate_p_r_factor(obs_series, lower_series, upper_series):
#     """在观测日期计算95PPU的P-factor和R-factor。"""
#     table = pd.concat(
#         [
#             obs_series.rename('Obs'),
#             lower_series.rename('Lower'),
#             upper_series.rename('Upper'),
#         ],
#         axis=1,
#         join='inner'
#     ).dropna()
#
#     if table.empty:
#         return np.nan, np.nan
#
#     inside = (
#         (table['Obs'] >= table['Lower']) &
#         (table['Obs'] <= table['Upper'])
#     )
#     p_factor = float(inside.sum()) / float(len(table))
#
#     obs_std = float(np.nanstd(table['Obs'].values))
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_factor = np.nan
#     else:
#         r_factor = float(
#             np.nanmean(table['Upper'].values - table['Lower'].values) /
#             obs_std
#         )
#
#     return p_factor, r_factor
#
#
# def save_selected_parameter_info(best_info, selected_record):
#     """保存统一参数、五个目标NSE和六变量诊断指标。"""
#     out_file = os.path.join(
#         OUTPUT_DIR,
#         'Gen%d_selected_shared_parameter_set.txt' % GENERATION_TO_PLOT
#     )
#
#     diagnostic_metrics = [
#         'NSE', 'NSE1', 'lnNSE', 'RMSE', 'RSR', 'PBIAS', 'R-square'
#     ]
#
#     with open(out_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: maximize the minimum NSE and then maximize the '
#             'mean NSE for D_1366, D_1367, D_1368, D_1370 and F_18.\n'
#         )
#         f.write('D_1375 is used only for auxiliary validation and plotting.\n\n')
#         f.write('Loaded generation file: %d\n' % GENERATION_TO_PLOT)
#         f.write('Shared best array index: %s\n' % str(best_info['idx']))
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#         f.write('Minimum NSE: %.12f\n' % best_info['min_nse'])
#         f.write('Mean NSE: %.12f\n' % best_info['mean_nse'])
#
#         f.write('\nFive NSE calibration objectives:\n')
#         for var, value in zip(OBJECTIVE_VARS, best_info['nse_values']):
#             f.write('%s NSE: %.12f\n' % (var, value))
#
#         f.write('\nAll-variable diagnostic metrics stored in JSON:\n')
#         for var in PLOT_VARS:
#             f.write('\n[%s]\n' % var)
#             for metric_name in diagnostic_metrics:
#                 value = safe_metric(selected_record, var, metric_name)
#                 f.write('%s: %s\n' % (metric_name, str(value)))
#
#         f.write('\nParameter values: %s\n' % str(best_info['gene_values']))
#
#     print('输出统一参数信息:', out_file)
#
#
# # ============================================================================
# # 降雨读取
# # ============================================================================
#
# def resolve_watershed_name():
#     """优先使用设置值；不存在时再使用原工程的交互选择。"""
#     if WATERSHED_NAME in DEMO_MODELS:
#         return WATERSHED_NAME
#
#     print(
#         'WATERSHED_NAME=%s 不在 DEMO_MODELS 中，将进入交互选择。'
#         % WATERSHED_NAME
#     )
#     name = get_watershed_name('Specify watershed name to run postprocess.')
#     if name not in DEMO_MODELS:
#         raise ValueError(
#             '%s不在可用流域列表中: %s'
#             % (name, ','.join(DEMO_MODELS.keys()))
#         )
#     return name
#
#
# def read_precipitation(watershed_name, start_date, end_date):
#     """从MongoDB读取18号子流域降雨。"""
#     mongo_client = ConnectMongoDB(MONGO_HOST, MONGO_PORT).get_conn()
#     try:
#         read_data = ReadModelData(
#             mongo_client,
#             DEMO_MODELS[watershed_name]
#         )
#         precipitation = read_data.Precipitation(
#             PRECIP_SUBBASIN_ID,
#             start_date,
#             end_date
#         )
#     finally:
#         try:
#             mongo_client.close()
#         except Exception:
#             pass
#
#     pcp_dates = normalize_datetime_index([item[0] for item in precipitation])
#     pcp_values = np.asarray(
#         [safe_float(item[1], 0.0) for item in precipitation],
#         dtype=float
#     )
#     return pcp_dates, pcp_values
#
#
# # ============================================================================
# # 主程序
# # ============================================================================
#
# def main():
#     print('当前运行文件:', os.path.abspath(__file__))
#     print('率定结果目录:', RESULT_DIR)
#     print('读取代数:', GENERATION_TO_PLOT)
#     print('输出目录:', OUTPUT_DIR)
#
#     if not os.path.exists(RESULT_DIR):
#         raise FileNotFoundError('找不到率定结果目录: %s' % RESULT_DIR)
#
#     simulated_dir = os.path.join(RESULT_DIR, 'simulated_data')
#     if not os.path.exists(simulated_dir):
#         raise FileNotFoundError('找不到simulated_data文件夹: %s' % simulated_dir)
#
#     os.makedirs(OUTPUT_DIR, exist_ok=True)
#
#     sim_obs_data, sim_data = read_generation_files(
#         simulated_dir,
#         GENERATION_TO_PLOT
#     )
#
#     variable_names = sim_obs_data[0].get('var_name', [])
#     if not variable_names:
#         raise ValueError(
#             'gen%d_caliObsData.json中没有var_name。'
#             % GENERATION_TO_PLOT
#         )
#
#     print('pickle变量顺序:', variable_names)
#
#     missing_objectives = [v for v in OBJECTIVE_VARS if v not in variable_names]
#     if missing_objectives:
#         raise ValueError(
#             '变量列表中缺少五个率定目标: %s'
#             % ', '.join(missing_objectives)
#         )
#
#     missing_plot_vars = [v for v in PLOT_VARS if v not in variable_names]
#     if missing_plot_vars:
#         raise ValueError(
#             '变量列表中缺少需要绘图的变量: %s'
#             % ', '.join(missing_plot_vars)
#         )
#
#     diagnose_best_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     selected_record = sim_obs_data[shared_best_idx]
#     selected_sim_dict = sim_data[shared_best_idx]
#
#     save_selected_parameter_info(best_info, selected_record)
#
#     watershed_name = None
#     if PLOT_PRECIPITATION:
#         watershed_name = resolve_watershed_name()
#
#     for variable_name in PLOT_VARS:
#         variable_index = variable_names.index(variable_name)
#
#         print('\n' + '=' * 78)
#         print('当前变量:', variable_name)
#         print('pickle变量索引:', variable_index)
#         print('统一参数数组索引:', shared_best_idx)
#
#         if variable_name not in selected_record:
#             print('所选参数记录中不存在变量%s，跳过。' % variable_name)
#             continue
#
#         paired = build_paired_observation_table(
#             selected_record,
#             variable_name
#         )
#
#         selected_series = extract_variable_series(
#             selected_sim_dict,
#             variable_index,
#             variable_name
#         )
#
#         if selected_series.empty:
#             print('变量%s的完整模拟序列为空，跳过。' % variable_name)
#             continue
#
#         # 当前代所有候选参数在完整模拟期的连续序列，用于95PPU。
#         candidate_values = []
#         valid_candidate_indices = []
#
#         for candidate_idx, candidate_dict in enumerate(sim_data):
#             series = extract_variable_series(
#                 candidate_dict,
#                 variable_index,
#                 variable_name
#             )
#             series = series.reindex(selected_series.index)
#
#             if series.notna().any():
#                 candidate_values.append(series.values)
#                 valid_candidate_indices.append(candidate_idx)
#
#         if not candidate_values:
#             raise ValueError('变量%s没有可用于计算不确定性的候选模拟。' % variable_name)
#
#         candidate_array = np.asarray(candidate_values, dtype=float)
#         lower, upper, uncertainty_label = calculate_uncertainty(candidate_array)
#
#         lower_series = pd.Series(lower, index=selected_series.index, name='Lower')
#         upper_series = pd.Series(upper, index=selected_series.index, name='Upper')
#
#         metrics_recalculated = calculate_metrics(
#             paired['Obs'].values,
#             paired['Sim_at_obs'].values
#         )
#
#         # 图中优先显示JSON保存指标，与率定程序一致；若缺失则用重新计算值。
#         json_nse = safe_metric(selected_record, variable_name, 'NSE')
#         json_kge = safe_metric(selected_record, variable_name, 'NSE1')
#         json_r2 = safe_metric(selected_record, variable_name, 'R-square')
#         json_pbias = safe_metric(selected_record, variable_name, 'PBIAS')
#         json_rmse = safe_metric(selected_record, variable_name, 'RMSE')
#         json_rsr = safe_metric(selected_record, variable_name, 'RSR')
#
#         display_metrics = {
#             'NSE': json_nse if np.isfinite(json_nse) else metrics_recalculated['NSE'],
#             'KGE': json_kge if np.isfinite(json_kge) else metrics_recalculated['KGE'],
#             'R-square': json_r2 if np.isfinite(json_r2) else metrics_recalculated['R-square'],
#             'PBIAS': json_pbias if np.isfinite(json_pbias) else metrics_recalculated['PBIAS'],
#             'RMSE': json_rmse if np.isfinite(json_rmse) else metrics_recalculated['RMSE'],
#             'RSR': json_rsr,
#         }
#
#         obs_series = paired['Obs'] if 'Obs' in paired else pd.Series(dtype=float)
#         p_factor, r_factor = calculate_p_r_factor(
#             obs_series,
#             lower_series,
#             upper_series
#         )
#
#         print('JSON中统一参数组指标:')
#         for metric_name, metric_value in display_metrics.items():
#             print('  %s = %s' % (metric_name, metric_value))
#         print('重新计算指标:', metrics_recalculated)
#         print('P-factor =', p_factor)
#         print('R-factor =', r_factor)
#         print('用于95PPU的有效候选数 =', len(valid_candidate_indices))
#
#         # 输出完整连续CSV。
#         output = pd.DataFrame(
#             {
#                 'Selected_simulation': selected_series,
#                 'Uncertainty_lower': lower_series,
#                 'Uncertainty_upper': upper_series,
#             }
#         )
#         output = pd.concat(
#             [output, paired[['Obs', 'Sim_at_obs']]],
#             axis=1
#         )
#         output.index.name = 'Date'
#
#         csv_name = 'Gen%d_RAW_%s_idx%d.csv' % (
#             GENERATION_TO_PLOT,
#             variable_name,
#             shared_best_idx
#         )
#         output_csv = os.path.join(OUTPUT_DIR, csv_name)
#         output.to_csv(output_csv, encoding='utf-8-sig')
#         print('输出CSV:', output_csv)
#
#         # 绘图：完整连续模拟 + 95PPU + 观测点 + 降雨。
#         fig, ax = plt.subplots(figsize=FIGSIZE, dpi=100)
#
#         ax.fill_between(
#             selected_series.index,
#             lower,
#             upper,
#             label=uncertainty_label,
#             alpha=0.28,
#             zorder=1
#         )
#
#         ax.plot(
#             selected_series.index,
#             selected_series.values,
#             label='Shared-parameter simulation (idx=%d)' % shared_best_idx,
#             linewidth=1.4,
#             zorder=3
#         )
#
#         if not paired.empty:
#             ax.scatter(
#                 paired.index,
#                 paired['Obs'].values,
#                 label='Observation',
#                 s=18,
#                 zorder=4
#             )
#
#         metric_lines = [
#             'P-factor=%s' % format_metric(p_factor),
#             'R-factor=%s' % format_metric(r_factor),
#             'NSE=%s' % format_metric(display_metrics['NSE']),
#             'KGE=%s' % format_metric(display_metrics['KGE']),
#             'R$^2$=%s' % format_metric(display_metrics['R-square']),
#             'PBIAS=%s%%' % format_metric(display_metrics['PBIAS']),
#             'RMSE=%s' % format_metric(display_metrics['RMSE']),
#         ]
#         if np.isfinite(display_metrics['RSR']):
#             metric_lines.append('RSR=%s' % format_metric(display_metrics['RSR']))
#
#         ax.text(
#             0.985,
#             0.97,
#             '\n'.join(metric_lines),
#             transform=ax.transAxes,
#             fontsize=13,
#             va='top',
#             ha='right',
#             bbox=dict(facecolor='white', alpha=0.75, edgecolor='none'),
#             zorder=10
#         )
#
#         ax2 = None
#         if PLOT_PRECIPITATION:
#             try:
#                 pcp_dates, pcp_values = read_precipitation(
#                     watershed_name,
#                     selected_series.index[0],
#                     selected_series.index[-1]
#                 )
#
#                 if len(pcp_values) > 0:
#                     ax2 = ax.twinx()
#                     ax2.bar(
#                         pcp_dates,
#                         pcp_values,
#                         label='Precipitation',
#                         linewidth=0,
#                         alpha=0.65,
#                         width=1.0,
#                         zorder=0
#                     )
#
#                     max_precip = float(np.nanmax(pcp_values)) \
#                         if np.isfinite(pcp_values).any() else 0.0
#                     ax2.set_ylim(max_precip * 4.0 if max_precip > 0 else 1.0, 0.0)
#                     ax2.set_ylabel('Precipitation (mm)', fontsize=14)
#                     ax2.tick_params(axis='y', labelsize=12)
#
#             except Exception as exc:
#                 print('降雨读取失败，但主图继续输出:', exc)
#
#         if variable_name.startswith('F_'):
#             ax.set_ylabel('Inundation area (km²)', fontsize=15)
#         elif variable_name.startswith('D_'):
#             ax.set_ylabel('Water depth (m)', fontsize=15)
#         else:
#             ax.set_ylabel(variable_name, fontsize=15)
#
#         ax.set_xlabel('Date time', fontsize=15)
#         ax.xaxis.set_major_locator(mdates.AutoDateLocator())
#         ax.xaxis.set_major_formatter(mdates.ConciseDateFormatter(ax.xaxis.get_major_locator()))
#         ax.tick_params(axis='both', length=5, width=1.2, labelsize=13)
#         ax.set_xlim(selected_series.index.min(), selected_series.index.max())
#
#         # 主Y轴包含观测、统一参数完整模拟和95PPU，并从0开始。
#         finite_parts = []
#         for values in [
#             selected_series.values,
#             paired['Obs'].values if not paired.empty else np.asarray([]),
#             lower,
#             upper,
#         ]:
#             values = np.asarray(values, dtype=float)
#             values = values[np.isfinite(values)]
#             if len(values) > 0:
#                 finite_parts.append(values)
#
#         if finite_parts:
#             all_values = np.concatenate(finite_parts)
#             ymax = float(np.nanpercentile(all_values, 99.5))
#             if not np.isfinite(ymax) or ymax <= 0.0:
#                 ymax = float(np.nanmax(all_values))
#             if not np.isfinite(ymax) or ymax <= 0.0:
#                 ymax = 0.01
#             ax.set_ylim(0.0, ymax * 1.12)
#         else:
#             ax.set_ylim(0.0, 0.01)
#
#         handles1, labels1 = ax.get_legend_handles_labels()
#         if ax2 is not None:
#             handles2, labels2 = ax2.get_legend_handles_labels()
#             ax.legend(
#                 handles1 + handles2,
#                 labels1 + labels2,
#                 frameon=False,
#                 fontsize=12,
#                 loc='upper left',
#                 ncol=2
#             )
#         else:
#             ax.legend(
#                 handles1,
#                 labels1,
#                 frameon=False,
#                 fontsize=12,
#                 loc='upper left',
#                 ncol=3
#             )
#
#         ax.set_title(
#             'Gen%d | %s | shared idx=%d | source Gen=%s ID=%s'
#             % (
#                 GENERATION_TO_PLOT,
#                 variable_name,
#                 shared_best_idx,
#                 str(best_info['gen']),
#                 str(best_info['id'])
#             ),
#             fontsize=15
#         )
#
#         plt.tight_layout()
#
#         fig_name = 'Gen%d_RAW_%s_idx%d.png' % (
#             GENERATION_TO_PLOT,
#             variable_name,
#             shared_best_idx
#         )
#         fig_path = os.path.join(OUTPUT_DIR, fig_name)
#         plt.savefig(fig_path, dpi=FIG_DPI, bbox_inches='tight')
#         plt.close(fig)
#         print('输出图片:', fig_path)
#
#     print('\n全部六个变量处理完成。')
#     print('统一参数数组索引 shared_best_idx =', shared_best_idx)
#     print('全部结果目录:', OUTPUT_DIR)
#
#
# if __name__ == '__main__':
#     main()



# -*- coding: utf-8 -*-
"""
四个水深点 + F_18 NSE 率定结果的独立后处理脚本。

与当前率定主程序和 calibration/userdef.py 保持一致：
1. 参与统一选参的五个 NSE 目标：
   D_1366、D_1367、D_1368、D_1370、F_18；
2. 先最大化五个 NSE 中的最小值，再最大化五个 NSE 的平均值；
3. D_1375 只用于辅助验证和绘图，不参与统一选参；
4. 从 gen*_caliSimData.pickle 读取完整连续模拟序列，而不是只画观测日期；
5. 当前代所有候选参数组用于计算 95PPU；
6. 六个变量始终使用同一个 shared_best_idx；
7. 可自动读取所选个体的gene_values，备份并写入MongoDB后单独运行模型。
"""
from __future__ import absolute_import, unicode_literals

import csv
import json
import os
import pickle
import re
import subprocess
import sys
from configparser import ConfigParser
from datetime import datetime

from pymongo import MongoClient

import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from spotpy.objectivefunctions import kge, nashsutcliffe, pbias, rmse, rsquared

# SEIMS Python工程根目录。这样脚本放在任意文件夹也能导入工程模块。
SEIMS_CODE_ROOT = r'D:\SEIMS_C\seims'
for candidate_path in [
    SEIMS_CODE_ROOT,
    os.path.abspath(os.path.join(sys.path[0], '..')),
]:
    if os.path.isdir(candidate_path) and candidate_path not in sys.path:
        sys.path.insert(0, candidate_path)

from test.demo_config import DEMO_MODELS, get_watershed_name
from preprocess.db_mongodb import ConnectMongoDB
from preprocess.db_read_model import ReadModelData
from utility import read_data_items_from_txt


# ============================================================================
# 用户设置：运行前重点检查这里
# ============================================================================

# 修改成“4个水深点 + F_18 NSE”这一轮率定的实际结果目录。
# 当前与你上传的userdef.py一致，先设为Pop_12。
# 正式运行目录如果是Pop_100，只需把末尾Pop_12改成Pop_100。
RESULT_DIR = (
    r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model'
    r'\CALI_NSGA2_Gen_100_Pop_100'
)

# 要读取和重画的代数。
GENERATION_TO_PLOT = 100

# 结果单独放进该文件夹，避免覆盖率定程序原图。
OUTPUT_DIR = os.path.join(
    RESULT_DIR,
    'Gen%d_continuous_shared_NSE' % GENERATION_TO_PLOT
)

# 与率定主程序 multiobj 完全一致的五个 NSE 目标。
OBJECTIVE_VARS = [
    'D_1366',
    'D_1367',
    'D_1368',
    # 'D_1370',
    # 'F_18',
]

# 固定输出顺序。D_1375只绘图和诊断，不参与选参。
PLOT_VARS = [
    'F_18',
    'D_1366',
    'D_1367',
    'D_1368',
    'D_1370',
    'D_1375',
]

# True：2.5%—97.5%（95PPU）；False：10%—90%。
USE_95PPU = True

# 是否绘制降雨。
PLOT_PRECIPITATION = True

# 当前研究对象为18号子流域。
PRECIP_SUBBASIN_ID = 18

# MongoDB。
MONGO_HOST = '127.0.0.1'
MONGO_PORT = 27017

# 自动将选中的共享参数写入数据库并重新运行模型。
# True：执行自动写入/运行；False：只做后处理和绘图。
APPLY_SELECTED_PARAMETERS_AND_RUN = True

# True：只检查自动选中的参数，不修改数据库、不运行模型。
# 第一次运行建议先设为True，确认参数名和数量后再改为False。
# DRY_RUN = True
DRY_RUN = False

# 写入参数后是否自动运行一次模型。
RUN_MODEL_AFTER_WRITE = True

# 模型和配置路径。
MODEL_DIR = os.path.dirname(RESULT_DIR)
WORKSPACE_DIR = r'D:\SEIMS_C\data\CW_2\workspace'
CALIBRATION_INI = os.path.join(WORKSPACE_DIR, 'calibration.ini')
DATABASE_NAME = 'CW_2_longterm_model'
RUN_MODEL_SCRIPT = os.path.join(
    SEIMS_CODE_ROOT,
    'test',
    'demo_runmodel.py'
)

# 如果 DEMO_MODELS 中存在 CW_2，会直接使用；否则运行时让你选择。
WATERSHED_NAME = 'CW_2'

# 图片设置。
FIGSIZE = (22, 8)
FIG_DPI = 300


# ============================================================================
# 通用辅助函数
# ============================================================================

def safe_float(value, default=np.nan):
    """安全转换为浮点数。"""
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def safe_metric(one_individual, variable, metric_name):
    """从一个JSON候选记录中安全读取评价指标。"""
    try:
        return safe_float(one_individual[variable][metric_name])
    except (KeyError, TypeError):
        return np.nan


def normalize_datetime_index(values):
    """统一转换为不带时区的 DatetimeIndex。"""
    dt = pd.to_datetime(values, errors='coerce', utc=True)
    if isinstance(dt, pd.Series):
        dt = pd.DatetimeIndex(dt)
    elif not isinstance(dt, pd.DatetimeIndex):
        dt = pd.DatetimeIndex(dt)
    return dt.tz_convert(None)


def read_generation_files(simulated_dir, generation):
    """读取指定代的观测配对数据和完整率定期模拟数据。"""
    obs_file = os.path.join(
        simulated_dir,
        'gen%d_caliObsData.json' % generation
    )
    sim_file = os.path.join(
        simulated_dir,
        'gen%d_caliSimData.pickle' % generation
    )

    if not os.path.exists(obs_file):
        raise FileNotFoundError('找不到文件: %s' % obs_file)
    if not os.path.exists(sim_file):
        raise FileNotFoundError('找不到文件: %s' % sim_file)

    print('读取JSON:', obs_file)
    print('读取pickle:', sim_file)

    with open(obs_file, 'r', encoding='utf-8') as f:
        sim_obs_data = json.load(f)

    with open(sim_file, 'rb') as f:
        sim_data = pickle.load(f)

    if len(sim_obs_data) == 0:
        raise ValueError('当前代没有候选参数结果。')
    if len(sim_obs_data) != len(sim_data):
        raise ValueError(
            'JSON候选数与pickle候选数不一致: %d != %d'
            % (len(sim_obs_data), len(sim_data))
        )

    return sim_obs_data, sim_data


def extract_variable_series(sim_dict, variable_index, variable_name):
    """
    从 gen*_caliSimData.pickle 的一个候选字典中提取一个变量的
    完整连续模拟序列。

    sim_dict 的格式应为：datetime -> [F_18, D_1366, ...]。
    """
    rows = []

    for date_value, all_values in sim_dict.items():
        date_value = pd.to_datetime(date_value, errors='coerce', utc=True)
        if pd.isna(date_value):
            continue

        if not isinstance(all_values, (list, tuple, np.ndarray)):
            all_values = [all_values]

        if variable_index >= len(all_values):
            raise IndexError(
                '变量 %s 的索引 %d 超过模拟数组长度 %d。'
                '请检查JSON中的var_name与pickle中的变量顺序。'
                % (variable_name, variable_index, len(all_values))
            )

        rows.append((date_value.tz_convert(None), safe_float(all_values[variable_index])))

    if not rows:
        return pd.Series(dtype=float, name=variable_name)

    rows.sort(key=lambda item: item[0])
    series = pd.Series(
        [item[1] for item in rows],
        index=pd.DatetimeIndex([item[0] for item in rows]),
        name=variable_name,
        dtype=float
    )

    # 极少数情况下若出现重复日期，取同一天最后一个值，避免reindex报错。
    if series.index.has_duplicates:
        series = series.groupby(level=0).last()

    return series.sort_index()


def build_paired_observation_table(selected_record, variable_name):
    """构造观测日期、观测值和观测日期对应模拟值的配对表。"""
    one_var = selected_record[variable_name]

    obs_dates = normalize_datetime_index(one_var.get('UTCDATETIME', []))
    obs_values = pd.to_numeric(
        pd.Series(one_var.get('Obs', [])),
        errors='coerce'
    ).values
    sim_values = pd.to_numeric(
        pd.Series(one_var.get('Sim', [])),
        errors='coerce'
    ).values

    n = min(len(obs_dates), len(obs_values), len(sim_values))
    if n == 0:
        return pd.DataFrame(columns=['Obs', 'Sim_at_obs'])

    paired = pd.DataFrame(
        {
            'Obs': obs_values[:n],
            'Sim_at_obs': sim_values[:n],
        },
        index=obs_dates[:n]
    )

    paired = paired[~paired.index.isna()].sort_index()
    if paired.index.has_duplicates:
        paired = paired.groupby(level=0).mean()
    return paired


def calculate_metrics(obs_values, sim_values):
    """在观测日期上重新计算指标。"""
    obs = np.asarray(obs_values, dtype=float)
    sim = np.asarray(sim_values, dtype=float)
    valid = np.isfinite(obs) & np.isfinite(sim)
    obs = obs[valid]
    sim = sim[valid]

    if len(obs) == 0:
        return {
            'NSE': np.nan,
            'KGE': np.nan,
            'R-square': np.nan,
            'PBIAS': np.nan,
            'RMSE': np.nan,
        }

    return {
        'NSE': nashsutcliffe(obs, sim),
        'KGE': kge(obs, sim),
        'R-square': rsquared(obs, sim),
        'PBIAS': pbias(obs, sim),
        'RMSE': rmse(obs, sim),
    }


def format_metric(value):
    """绘图文字格式。"""
    return 'nan' if not np.isfinite(value) else '%.2f' % value


# ============================================================================
# 统一参数选择：严格与当前 userdef.py 一致
# ============================================================================

def select_shared_best_index(sim_obs_data):
    """
    从当前代候选种群中选一套统一参数。

    五个目标：D_1366、D_1367、D_1368、D_1370、F_18 的 NSE。
    排序规则：
      1. 最大化五个NSE中的最小NSE；
      2. 若最小NSE相同，再最大化五个NSE的平均NSE。
    """
    candidates = []

    for ind_idx, one_ind in enumerate(sim_obs_data):
        nse_values = np.asarray(
            [safe_metric(one_ind, var, 'NSE') for var in OBJECTIVE_VARS],
            dtype=float
        )

        if not np.isfinite(nse_values).all():
            continue
        if np.any(nse_values <= -9000.0):
            continue

        candidates.append({
            'idx': ind_idx,
            'gen': one_ind.get('Gen', -1),
            'id': one_ind.get('ID', -1),
            'gene_values': one_ind.get('gene_values', None),
            'nse_values': nse_values,
            'min_nse': float(np.min(nse_values)),
            'mean_nse': float(np.mean(nse_values)),
        })

    if not candidates:
        raise ValueError(
            '当前代没有同时包含 D_1366、D_1367、D_1368、D_1370、F_18 '
            '五个有效NSE的候选参数组。'
        )

    candidates.sort(
        key=lambda item: (
            -item['min_nse'],
            -item['mean_nse'],
        )
    )

    best = candidates[0]

    print('=' * 78)
    print('统一参数选择结果')
    print('规则1：最大化五个NSE目标中的最小NSE')
    print('规则2：再最大化五个NSE目标的平均NSE')
    print('shared_best_idx =', best['idx'])
    print('来源Generation =', best['gen'])
    print('来源Calibration ID =', best['id'])
    print('最小NSE = %.10f' % best['min_nse'])
    print('平均NSE = %.10f' % best['mean_nse'])

    for var, value in zip(OBJECTIVE_VARS, best['nse_values']):
        print('%s NSE = %.10f' % (var, value))

    print('参数值 =', best['gene_values'])
    print('=' * 78)

    return best['idx'], best


def diagnose_best_each_variable(sim_obs_data):
    """打印每个变量单独能达到的最大NSE和最小RMSE，仅用于诊断。"""
    print('=' * 78)
    print('当前代各变量单独最优指标，仅用于诊断，不用于统一选参：')

    for var in PLOT_VARS:
        nse_items = []
        rmse_items = []

        for idx, one_ind in enumerate(sim_obs_data):
            nse_value = safe_metric(one_ind, var, 'NSE')
            rmse_value = safe_metric(one_ind, var, 'RMSE')

            if np.isfinite(nse_value) and nse_value > -9000.0:
                nse_items.append((nse_value, idx))
            if np.isfinite(rmse_value) and 0.0 <= rmse_value < 9000.0:
                rmse_items.append((rmse_value, idx))

        best_nse_text = '无有效NSE'
        best_rmse_text = '无有效RMSE'

        if nse_items:
            best_nse, best_nse_idx = max(nse_items, key=lambda x: x[0])
            best_nse_text = 'max NSE=%.6f, idx=%d' % (best_nse, best_nse_idx)

        if rmse_items:
            best_rmse, best_rmse_idx = min(rmse_items, key=lambda x: x[0])
            best_rmse_text = 'min RMSE=%.6f, idx=%d' % (best_rmse, best_rmse_idx)

        print('%s: %s; %s' % (var, best_nse_text, best_rmse_text))

    print('=' * 78)


# ============================================================================
# 95PPU和参数信息
# ============================================================================

def calculate_uncertainty(candidate_array):
    """按设置计算不确定性上下限。"""
    if USE_95PPU:
        lower = np.nanpercentile(candidate_array, 2.5, axis=0)
        upper = np.nanpercentile(candidate_array, 97.5, axis=0)
        label = '95PPU'
    else:
        lower = np.nanpercentile(candidate_array, 10.0, axis=0)
        upper = np.nanpercentile(candidate_array, 90.0, axis=0)
        label = '10th–90th percentile'
    return lower, upper, label


def calculate_p_r_factor(obs_series, lower_series, upper_series):
    """在观测日期计算95PPU的P-factor和R-factor。"""
    table = pd.concat(
        [
            obs_series.rename('Obs'),
            lower_series.rename('Lower'),
            upper_series.rename('Upper'),
        ],
        axis=1,
        join='inner'
    ).dropna()

    if table.empty:
        return np.nan, np.nan

    inside = (
        (table['Obs'] >= table['Lower']) &
        (table['Obs'] <= table['Upper'])
    )
    p_factor = float(inside.sum()) / float(len(table))

    obs_std = float(np.nanstd(table['Obs'].values))
    if not np.isfinite(obs_std) or obs_std == 0.0:
        r_factor = np.nan
    else:
        r_factor = float(
            np.nanmean(table['Upper'].values - table['Lower'].values) /
            obs_std
        )

    return p_factor, r_factor


def save_selected_parameter_info(best_info, selected_record):
    """保存统一参数、五个目标NSE和六变量诊断指标。"""
    out_file = os.path.join(
        OUTPUT_DIR,
        'Gen%d_selected_shared_parameter_set.txt' % GENERATION_TO_PLOT
    )

    diagnostic_metrics = [
        'NSE', 'NSE1', 'lnNSE', 'RMSE', 'RSR', 'PBIAS', 'R-square'
    ]

    with open(out_file, 'w', encoding='utf-8') as f:
        f.write(
            'Selection rule: maximize the minimum NSE and then maximize the '
            'mean NSE for D_1366, D_1367, D_1368, D_1370 and F_18.\n'
        )
        f.write('D_1375 is used only for auxiliary validation and plotting.\n\n')
        f.write('Loaded generation file: %d\n' % GENERATION_TO_PLOT)
        f.write('Shared best array index: %s\n' % str(best_info['idx']))
        f.write('Source generation: %s\n' % str(best_info['gen']))
        f.write('Source calibration ID: %s\n' % str(best_info['id']))
        f.write('Minimum NSE: %.12f\n' % best_info['min_nse'])
        f.write('Mean NSE: %.12f\n' % best_info['mean_nse'])

        f.write('\nFive NSE calibration objectives:\n')
        for var, value in zip(OBJECTIVE_VARS, best_info['nse_values']):
            f.write('%s NSE: %.12f\n' % (var, value))

        f.write('\nAll-variable diagnostic metrics stored in JSON:\n')
        for var in PLOT_VARS:
            f.write('\n[%s]\n' % var)
            for metric_name in diagnostic_metrics:
                value = safe_metric(selected_record, var, metric_name)
                f.write('%s: %s\n' % (metric_name, str(value)))

        f.write('\nParameter values: %s\n' % str(best_info['gene_values']))

    print('输出统一参数信息:', out_file)



# ============================================================================
# 自动提取共享个体参数、写入MongoDB并运行模型
# ============================================================================

def normalize_gene_values(gene_values):
    """
    将JSON中的gene_values统一转换成一维float列表。

    支持：
    1. list / tuple / numpy.ndarray；
    2. "[1, 2, 3]"；
    3. "array([1, 2, 3])"；
    4. 科学计数法。
    """
    if isinstance(gene_values, np.ndarray):
        values = gene_values.astype(float).ravel().tolist()
    elif isinstance(gene_values, (list, tuple)):
        values = np.asarray(gene_values, dtype=float).ravel().tolist()
    else:
        text_value = str(gene_values).strip()
        if not text_value or text_value.lower() == 'none':
            raise ValueError('所选个体的gene_values为空。')

        # 只解析最外层方括号中的内容，避免把dtype=float64中的64误当参数。
        left = text_value.find('[')
        right = text_value.rfind(']')
        if left >= 0 and right > left:
            text_value = text_value[left:right + 1]

        number_pattern = (
            r'[-+]?(?:\d+\.\d*|\.\d+|\d+)'
            r'(?:[eE][-+]?\d+)?'
        )
        values = [
            float(item)
            for item in re.findall(number_pattern, text_value)
        ]

    if not values:
        raise ValueError(
            '无法从所选个体的gene_values中解析参数值: %s'
            % str(gene_values)
        )

    return values


def read_parameter_names():
    """按率定时的顺序读取参数名称。"""
    if not os.path.exists(CALIBRATION_INI):
        raise FileNotFoundError(
            '找不到calibration.ini: %s' % CALIBRATION_INI
        )

    cf = ConfigParser()
    cf.read(CALIBRATION_INI)

    if not cf.has_option('CALI_Settings', 'paramrngdef'):
        raise KeyError(
            'calibration.ini中缺少CALI_Settings.paramrngdef'
        )

    param_range_def = cf.get(
        'CALI_Settings',
        'paramrngdef'
    ).strip()

    if os.path.isabs(param_range_def):
        candidate_files = [param_range_def]
    else:
        candidate_files = [
            os.path.join(MODEL_DIR, param_range_def),
            os.path.join(WORKSPACE_DIR, param_range_def),
        ]

    param_file = None
    for candidate in candidate_files:
        if os.path.exists(candidate):
            param_file = candidate
            break

    if param_file is None:
        raise FileNotFoundError(
            '找不到参数范围定义文件，检查过:\n%s'
            % '\n'.join(candidate_files)
        )

    items = read_data_items_from_txt(param_file)
    names = []

    for item in items:
        if len(item) < 3:
            continue
        names.append(item[0])

    if not names:
        raise ValueError(
            '参数范围定义文件中没有读取到有效参数: %s'
            % param_file
        )

    print('参数范围文件:', param_file)
    print('读取参数数量:', len(names))
    return names


def save_selected_parameters_csv(
    parameter_names,
    selected_values,
    best_info
):
    """保存自动选中的参数名称和值。"""
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    output_file = os.path.join(
        OUTPUT_DIR,
        'Gen%d_selected_parameters_idx%d.csv'
        % (GENERATION_TO_PLOT, best_info['idx'])
    )

    with open(
        output_file,
        'w',
        newline='',
        encoding='utf-8-sig'
    ) as file_obj:
        writer = csv.writer(file_obj)
        writer.writerow([
            'Shared_best_idx',
            best_info['idx']
        ])
        writer.writerow([
            'Source_Generation',
            best_info['gen']
        ])
        writer.writerow([
            'Source_Calibration_ID',
            best_info['id']
        ])
        writer.writerow([
            'Minimum_NSE',
            best_info['min_nse']
        ])
        writer.writerow([
            'Mean_NSE',
            best_info['mean_nse']
        ])
        writer.writerow([])
        writer.writerow([
            'Index',
            'NAME',
            'IMPACT'
        ])

        for index, (name, value) in enumerate(
            zip(parameter_names, selected_values),
            start=1
        ):
            writer.writerow([
                index,
                name,
                value
            ])

    print('自动选中参数已保存:', output_file)
    return output_file


def verify_selected_parameters(
    collection,
    parameter_names,
    selected_values
):
    """检查参数数量、顺序和数据库记录。"""
    if len(parameter_names) != len(selected_values):
        raise ValueError(
            '参数名称数量与gene_values数量不一致: %d != %d'
            % (
                len(parameter_names),
                len(selected_values)
            )
        )

    missing_names = []

    print('\n自动选中参数映射关系:')
    print('-' * 100)

    for index, (name, value) in enumerate(
        zip(parameter_names, selected_values),
        start=1
    ):
        document = collection.find_one(
            {'NAME': name},
            {'NAME': 1, 'IMPACT': 1, 'VALUE': 1, 'CHANGE': 1}
        )

        if document is None:
            missing_names.append(name)
            old_impact = '<数据库中不存在>'
            base_value = None
            change_type = None
        else:
            old_impact = document.get('IMPACT', None)
            base_value = document.get('VALUE', None)
            change_type = document.get('CHANGE', None)

        print(
            '%02d  %-28s  VALUE=%-12s  CHANGE=%-4s  '
            '旧IMPACT=%-15s  新IMPACT=%s'
            % (
                index,
                name,
                str(base_value),
                str(change_type),
                str(old_impact),
                str(value)
            )
        )

    print('-' * 100)

    if missing_names:
        raise RuntimeError(
            '数据库PARAMETERS中缺少以下参数:\n%s'
            % '\n'.join(missing_names)
        )


def backup_current_impacts(
    collection,
    parameter_names,
    selected_values
):
    """写入前备份当前IMPACT。"""
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    backup_file = os.path.join(
        OUTPUT_DIR,
        'selected_parameter_impact_backup_%s.csv'
        % timestamp
    )

    with open(
        backup_file,
        'w',
        newline='',
        encoding='utf-8-sig'
    ) as file_obj:
        writer = csv.writer(file_obj)
        writer.writerow([
            'Index',
            'NAME',
            'Old_IMPACT',
            'Selected_IMPACT'
        ])

        for index, (name, new_value) in enumerate(
            zip(parameter_names, selected_values),
            start=1
        ):
            document = collection.find_one(
                {'NAME': name},
                {'NAME': 1, 'IMPACT': 1}
            )

            old_value = (
                None if document is None
                else document.get('IMPACT', None)
            )

            writer.writerow([
                index,
                name,
                old_value,
                new_value
            ])

    print('原IMPACT已备份:', backup_file)
    return backup_file


def write_selected_impacts(
    collection,
    parameter_names,
    selected_values
):
    """把自动选中个体的gene_values写入PARAMETERS.IMPACT。"""
    for index, (name, value) in enumerate(
        zip(parameter_names, selected_values),
        start=1
    ):
        result = collection.update_one(
            {'NAME': name},
            {'$set': {'IMPACT': float(value)}}
        )

        if result.matched_count != 1:
            raise RuntimeError(
                '写入失败，参数不存在或匹配异常: %s'
                % name
            )

        print(
            '已写入 %02d/%02d: %-28s IMPACT=%s'
            % (
                index,
                len(parameter_names),
                name,
                value
            )
        )


def run_selected_model():
    """使用当前Python环境单独运行一次CW_2模型。"""
    if not os.path.exists(RUN_MODEL_SCRIPT):
        raise FileNotFoundError(
            '找不到模型运行脚本: %s'
            % RUN_MODEL_SCRIPT
        )

    command = [
        sys.executable,
        RUN_MODEL_SCRIPT,
        '-name',
        WATERSHED_NAME
    ]

    print('\n开始运行自动选中的共享参数:')
    print(' '.join(command))

    completed = subprocess.run(
        command,
        cwd=os.path.dirname(RUN_MODEL_SCRIPT)
    )

    if completed.returncode != 0:
        raise RuntimeError(
            '模型运行失败，退出码: %d'
            % completed.returncode
        )

    print('模型运行完成，退出码为0。')


def apply_selected_parameters_and_run(best_info):
    """
    自动完成：
    gene_values解析 → 参数名读取 → 数据库检查 → 备份 → 写入 → 运行。
    """
    selected_values = normalize_gene_values(
        best_info['gene_values']
    )
    parameter_names = read_parameter_names()

    print('\n所选个体参数值数量:', len(selected_values))
    print('参数定义文件参数数量:', len(parameter_names))

    save_selected_parameters_csv(
        parameter_names,
        selected_values,
        best_info
    )

    client = MongoClient(
        MONGO_HOST,
        MONGO_PORT
    )

    try:
        database = client[DATABASE_NAME]
        collection = database['PARAMETERS']

        verify_selected_parameters(
            collection,
            parameter_names,
            selected_values
        )

        if DRY_RUN:
            print(
                '\nDRY_RUN=True：只完成自动选参和参数检查，'
                '未修改数据库，也未运行模型。'
            )
            return selected_values

        backup_current_impacts(
            collection,
            parameter_names,
            selected_values
        )

        write_selected_impacts(
            collection,
            parameter_names,
            selected_values
        )

        print(
            '\n自动选中的共享参数已全部写入'
            '%s.PARAMETERS.IMPACT。'
            % DATABASE_NAME
        )

    finally:
        client.close()

    if RUN_MODEL_AFTER_WRITE:
        run_selected_model()

    return selected_values


# ============================================================================
# 降雨读取
# ============================================================================

def resolve_watershed_name():
    """优先使用设置值；不存在时再使用原工程的交互选择。"""
    if WATERSHED_NAME in DEMO_MODELS:
        return WATERSHED_NAME

    print(
        'WATERSHED_NAME=%s 不在 DEMO_MODELS 中，将进入交互选择。'
        % WATERSHED_NAME
    )
    name = get_watershed_name('Specify watershed name to run postprocess.')
    if name not in DEMO_MODELS:
        raise ValueError(
            '%s不在可用流域列表中: %s'
            % (name, ','.join(DEMO_MODELS.keys()))
        )
    return name


def read_precipitation(watershed_name, start_date, end_date):
    """从MongoDB读取18号子流域降雨。"""
    mongo_client = ConnectMongoDB(MONGO_HOST, MONGO_PORT).get_conn()
    try:
        read_data = ReadModelData(
            mongo_client,
            DEMO_MODELS[watershed_name]
        )
        precipitation = read_data.Precipitation(
            PRECIP_SUBBASIN_ID,
            start_date,
            end_date
        )
    finally:
        try:
            mongo_client.close()
        except Exception:
            pass

    pcp_dates = normalize_datetime_index([item[0] for item in precipitation])
    pcp_values = np.asarray(
        [safe_float(item[1], 0.0) for item in precipitation],
        dtype=float
    )
    return pcp_dates, pcp_values


# ============================================================================
# 主程序
# ============================================================================

def main():
    print('当前运行文件:', os.path.abspath(__file__))
    print('率定结果目录:', RESULT_DIR)
    print('读取代数:', GENERATION_TO_PLOT)
    print('输出目录:', OUTPUT_DIR)

    if not os.path.exists(RESULT_DIR):
        raise FileNotFoundError('找不到率定结果目录: %s' % RESULT_DIR)

    simulated_dir = os.path.join(RESULT_DIR, 'simulated_data')
    if not os.path.exists(simulated_dir):
        raise FileNotFoundError('找不到simulated_data文件夹: %s' % simulated_dir)

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    sim_obs_data, sim_data = read_generation_files(
        simulated_dir,
        GENERATION_TO_PLOT
    )

    variable_names = sim_obs_data[0].get('var_name', [])
    if not variable_names:
        raise ValueError(
            'gen%d_caliObsData.json中没有var_name。'
            % GENERATION_TO_PLOT
        )

    print('pickle变量顺序:', variable_names)

    missing_objectives = [v for v in OBJECTIVE_VARS if v not in variable_names]
    if missing_objectives:
        raise ValueError(
            '变量列表中缺少五个率定目标: %s'
            % ', '.join(missing_objectives)
        )

    missing_plot_vars = [v for v in PLOT_VARS if v not in variable_names]
    if missing_plot_vars:
        raise ValueError(
            '变量列表中缺少需要绘图的变量: %s'
            % ', '.join(missing_plot_vars)
        )

    diagnose_best_each_variable(sim_obs_data)
    shared_best_idx, best_info = select_shared_best_index(sim_obs_data)

    selected_record = sim_obs_data[shared_best_idx]
    selected_sim_dict = sim_data[shared_best_idx]

    save_selected_parameter_info(best_info, selected_record)

    # 自动从所选共享个体读取gene_values，并按参数文件顺序写入数据库。
    # 第一次建议DRY_RUN=True，只检查映射；确认后改为False正式运行。
    if APPLY_SELECTED_PARAMETERS_AND_RUN:
        apply_selected_parameters_and_run(best_info)

    watershed_name = None
    if PLOT_PRECIPITATION:
        watershed_name = resolve_watershed_name()

    for variable_name in PLOT_VARS:
        variable_index = variable_names.index(variable_name)

        print('\n' + '=' * 78)
        print('当前变量:', variable_name)
        print('pickle变量索引:', variable_index)
        print('统一参数数组索引:', shared_best_idx)

        if variable_name not in selected_record:
            print('所选参数记录中不存在变量%s，跳过。' % variable_name)
            continue

        paired = build_paired_observation_table(
            selected_record,
            variable_name
        )

        selected_series = extract_variable_series(
            selected_sim_dict,
            variable_index,
            variable_name
        )

        if selected_series.empty:
            print('变量%s的完整模拟序列为空，跳过。' % variable_name)
            continue

        # 当前代所有候选参数在完整模拟期的连续序列，用于95PPU。
        candidate_values = []
        valid_candidate_indices = []

        for candidate_idx, candidate_dict in enumerate(sim_data):
            series = extract_variable_series(
                candidate_dict,
                variable_index,
                variable_name
            )
            series = series.reindex(selected_series.index)

            if series.notna().any():
                candidate_values.append(series.values)
                valid_candidate_indices.append(candidate_idx)

        if not candidate_values:
            raise ValueError('变量%s没有可用于计算不确定性的候选模拟。' % variable_name)

        candidate_array = np.asarray(candidate_values, dtype=float)
        lower, upper, uncertainty_label = calculate_uncertainty(candidate_array)

        lower_series = pd.Series(lower, index=selected_series.index, name='Lower')
        upper_series = pd.Series(upper, index=selected_series.index, name='Upper')

        metrics_recalculated = calculate_metrics(
            paired['Obs'].values,
            paired['Sim_at_obs'].values
        )

        # 图中优先显示JSON保存指标，与率定程序一致；若缺失则用重新计算值。
        json_nse = safe_metric(selected_record, variable_name, 'NSE')
        json_kge = safe_metric(selected_record, variable_name, 'NSE1')
        json_r2 = safe_metric(selected_record, variable_name, 'R-square')
        json_pbias = safe_metric(selected_record, variable_name, 'PBIAS')
        json_rmse = safe_metric(selected_record, variable_name, 'RMSE')
        json_rsr = safe_metric(selected_record, variable_name, 'RSR')

        display_metrics = {
            'NSE': json_nse if np.isfinite(json_nse) else metrics_recalculated['NSE'],
            'KGE': json_kge if np.isfinite(json_kge) else metrics_recalculated['KGE'],
            'R-square': json_r2 if np.isfinite(json_r2) else metrics_recalculated['R-square'],
            'PBIAS': json_pbias if np.isfinite(json_pbias) else metrics_recalculated['PBIAS'],
            'RMSE': json_rmse if np.isfinite(json_rmse) else metrics_recalculated['RMSE'],
            'RSR': json_rsr,
        }

        obs_series = paired['Obs'] if 'Obs' in paired else pd.Series(dtype=float)
        p_factor, r_factor = calculate_p_r_factor(
            obs_series,
            lower_series,
            upper_series
        )

        print('JSON中统一参数组指标:')
        for metric_name, metric_value in display_metrics.items():
            print('  %s = %s' % (metric_name, metric_value))
        print('重新计算指标:', metrics_recalculated)
        print('P-factor =', p_factor)
        print('R-factor =', r_factor)
        print('用于95PPU的有效候选数 =', len(valid_candidate_indices))

        # 输出完整连续CSV。
        output = pd.DataFrame(
            {
                'Selected_simulation': selected_series,
                'Uncertainty_lower': lower_series,
                'Uncertainty_upper': upper_series,
            }
        )
        output = pd.concat(
            [output, paired[['Obs', 'Sim_at_obs']]],
            axis=1
        )
        output.index.name = 'Date'

        csv_name = 'Gen%d_RAW_%s_idx%d.csv' % (
            GENERATION_TO_PLOT,
            variable_name,
            shared_best_idx
        )
        output_csv = os.path.join(OUTPUT_DIR, csv_name)
        output.to_csv(output_csv, encoding='utf-8-sig')
        print('输出CSV:', output_csv)

        # 绘图：完整连续模拟 + 95PPU + 观测点 + 降雨。
        fig, ax = plt.subplots(figsize=FIGSIZE, dpi=100)

        ax.fill_between(
            selected_series.index,
            lower,
            upper,
            label=uncertainty_label,
            alpha=0.28,
            zorder=1
        )

        ax.plot(
            selected_series.index,
            selected_series.values,
            label='Shared-parameter simulation (idx=%d)' % shared_best_idx,
            linewidth=1.4,
            zorder=3
        )

        if not paired.empty:
            ax.scatter(
                paired.index,
                paired['Obs'].values,
                label='Observation',
                s=18,
                zorder=4
            )

        metric_lines = [
            'P-factor=%s' % format_metric(p_factor),
            'R-factor=%s' % format_metric(r_factor),
            'NSE=%s' % format_metric(display_metrics['NSE']),
            'KGE=%s' % format_metric(display_metrics['KGE']),
            'R$^2$=%s' % format_metric(display_metrics['R-square']),
            'PBIAS=%s%%' % format_metric(display_metrics['PBIAS']),
            'RMSE=%s' % format_metric(display_metrics['RMSE']),
        ]
        if np.isfinite(display_metrics['RSR']):
            metric_lines.append('RSR=%s' % format_metric(display_metrics['RSR']))

        ax.text(
            0.985,
            0.97,
            '\n'.join(metric_lines),
            transform=ax.transAxes,
            fontsize=13,
            va='top',
            ha='right',
            bbox=dict(facecolor='white', alpha=0.75, edgecolor='none'),
            zorder=10
        )

        ax2 = None
        if PLOT_PRECIPITATION:
            try:
                pcp_dates, pcp_values = read_precipitation(
                    watershed_name,
                    selected_series.index[0],
                    selected_series.index[-1]
                )

                if len(pcp_values) > 0:
                    ax2 = ax.twinx()
                    ax2.bar(
                        pcp_dates,
                        pcp_values,
                        label='Precipitation',
                        linewidth=0,
                        alpha=0.65,
                        width=1.0,
                        zorder=0
                    )

                    max_precip = float(np.nanmax(pcp_values)) \
                        if np.isfinite(pcp_values).any() else 0.0
                    ax2.set_ylim(max_precip * 4.0 if max_precip > 0 else 1.0, 0.0)
                    ax2.set_ylabel('Precipitation (mm)', fontsize=14)
                    ax2.tick_params(axis='y', labelsize=12)

            except Exception as exc:
                print('降雨读取失败，但主图继续输出:', exc)

        if variable_name.startswith('F_'):
            ax.set_ylabel('Inundation area (km²)', fontsize=15)
        elif variable_name.startswith('D_'):
            ax.set_ylabel('Water depth (m)', fontsize=15)
        else:
            ax.set_ylabel(variable_name, fontsize=15)

        ax.set_xlabel('Date time', fontsize=15)
        ax.xaxis.set_major_locator(mdates.AutoDateLocator())
        ax.xaxis.set_major_formatter(mdates.ConciseDateFormatter(ax.xaxis.get_major_locator()))
        ax.tick_params(axis='both', length=5, width=1.2, labelsize=13)
        ax.set_xlim(selected_series.index.min(), selected_series.index.max())

        # 主Y轴包含观测、统一参数完整模拟和95PPU，并从0开始。
        finite_parts = []
        for values in [
            selected_series.values,
            paired['Obs'].values if not paired.empty else np.asarray([]),
            lower,
            upper,
        ]:
            values = np.asarray(values, dtype=float)
            values = values[np.isfinite(values)]
            if len(values) > 0:
                finite_parts.append(values)

        if finite_parts:
            all_values = np.concatenate(finite_parts)
            ymax = float(np.nanpercentile(all_values, 99.5))
            if not np.isfinite(ymax) or ymax <= 0.0:
                ymax = float(np.nanmax(all_values))
            if not np.isfinite(ymax) or ymax <= 0.0:
                ymax = 0.01
            ax.set_ylim(0.0, ymax * 1.12)
        else:
            ax.set_ylim(0.0, 0.01)

        handles1, labels1 = ax.get_legend_handles_labels()
        if ax2 is not None:
            handles2, labels2 = ax2.get_legend_handles_labels()
            ax.legend(
                handles1 + handles2,
                labels1 + labels2,
                frameon=False,
                fontsize=12,
                loc='upper left',
                ncol=2
            )
        else:
            ax.legend(
                handles1,
                labels1,
                frameon=False,
                fontsize=12,
                loc='upper left',
                ncol=3
            )

        ax.set_title(
            'Gen%d | %s | shared idx=%d | source Gen=%s ID=%s'
            % (
                GENERATION_TO_PLOT,
                variable_name,
                shared_best_idx,
                str(best_info['gen']),
                str(best_info['id'])
            ),
            fontsize=15
        )

        plt.tight_layout()

        fig_name = 'Gen%d_RAW_%s_idx%d.png' % (
            GENERATION_TO_PLOT,
            variable_name,
            shared_best_idx
        )
        fig_path = os.path.join(OUTPUT_DIR, fig_name)
        plt.savefig(fig_path, dpi=FIG_DPI, bbox_inches='tight')
        plt.close(fig)
        print('输出图片:', fig_path)

    print('\n全部六个变量处理完成。')
    print('统一参数数组索引 shared_best_idx =', shared_best_idx)
    print('全部结果目录:', OUTPUT_DIR)


if __name__ == '__main__':
    main()
