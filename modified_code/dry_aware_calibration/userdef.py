"""User defined functions.

    @author   : Liangjun Zhu

    @changelog:
    - 18-01-22  - lj - initial implementation.
    - 18-02-09  - lj - compatible with Python3.
    - 19-01-07  - lj - incorporated with PlotConfig
"""
# from __future__ import absolute_import, unicode_literals
#
# import json
# import pickle
# import os
# import sys
# from io import open
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import numpy
# import matplotlib as mpl
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from typing import Optional
# from pygeoc.utils import StringClass, is_string
#
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
# import numpy as np
# from math import isnan
#
#
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     # update Parameters collection in MongoDB
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Output population details, i.e., the simulation data, etc."""
#     # Save as json, which can be loaded by json.load()
#     # 1. Save the time series simulation data of the entire simulation period
#     all_sim_data = list()
#     os.makedirs(outdir, exist_ok=True)
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save the matched observation-simulation data of validation period,
#     #      and the simulation data separately.
#     cali_sim_obs_data = list()
#     cali_sim_data = list()
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#     # 3. Save the matched observation-simulation data of calibration period,
#     #      and the simulation data separately.
#     vali_sim_obs_data = list()
#     vali_sim_data = list()
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#     # 4. Try to plot.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#     try:
#         # Calculate 95PPU for current generation, and plot the desired variables, e.g., Q and SED
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception:
#         pass
#
# def dict_to_daily_month_matrix(ind):
#     """
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum]  或 [daily_sum, monthly_avg]
#       - 每月代表行(每月1号00:00:00): [monthly_avg]
#     返回:
#       keys_sorted: list[datetime]
#       arr: np.ndarray, shape=(N, 2), 列0=daily_sum, 列1=monthly_avg（缺失用 NaN）
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#
#     daily = []
#     monthly = []
#
#     for k, v in items:
#         d = np.nan
#         m = np.nan
#         if len(v) == 2:
#             d, m = float(v[0]), float(v[1])
#
#         # # lj
#         # elif len(v) == 1:
#         #     # 每月代表行：每月第一天的 00:00:00
#         #     if hasattr(k, "year") and k.day == 1 and k.hour == 0 and k.minute == 0 and k.second == 0:
#         #         m = float(v[0])      # 月均值
#         #     else:
#         #         d = float(v[0])      # 日总和
#
#         elif len(v) == 1:
#             d = float(v[0])  # 单值一律按日值处理
#
#
#         # 其它情况保持 NaN
#         daily.append(d)
#         monthly.append(m)
#
#     arr = np.column_stack([daily, monthly])
#     return keys_sorted, arr
#
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot the hydrographs."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#     if len(sim_data) < 2:
#         return
#     var_name = sim_obs_data[0]['var_name']
#     for idx, var in enumerate(var_name):
#         plot_validation = False
#         if vali_sim_obs_data and vali_sim_data and var in vali_sim_obs_data[0]['var_name']:
#             plot_validation = True
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif 'CONC' in var.upper():  # Concentrate
#             if 'SED' in var.upper():
#                 ylabel_str += ' (g/L)'
#             else:
#                 ylabel_str += ' (mg/L)'
#         elif 'SED' in var.upper():  # amount
#             ylabel_str += ' (kg)'
#         cali_obs_dates = sim_obs_data[0][var]['UTCDATETIME'][:]
#         if is_string(cali_obs_dates[0]):
#             cali_obs_dates = [StringClass.get_datetime(s) for s in cali_obs_dates]
#         obs_dates = cali_obs_dates[:]
#         order = 1  # By default, the calibration period is before the validation period.
#         if plot_validation:
#             vali_obs_dates = vali_sim_obs_data[0][var]['UTCDATETIME']
#             if is_string(vali_obs_dates[0]):
#                 vali_obs_dates = [StringClass.get_datetime(s) for s in vali_obs_dates]
#             if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                 order = 0
#                 obs_dates = vali_obs_dates + obs_dates
#             else:
#                 obs_dates += vali_obs_dates
#         obs_data = sim_obs_data[0][var]['Obs'][:]
#         if plot_validation:
#             if order:
#                 obs_data += vali_sim_obs_data[0][var]['Obs'][:]
#             else:
#                 obs_data = vali_sim_obs_data[0][var]['Obs'][:] + obs_data
#
#         cali_sim_dates = list(sim_data[0].keys())
#         if is_string(cali_sim_dates[0]):
#             cali_sim_dates = [StringClass.get_datetime(s) for s in cali_sim_dates]
#         sim_dates = cali_sim_dates[:]
#         if plot_validation:
#             vali_sim_dates = list(vali_sim_data[0].keys())
#             if is_string(vali_sim_dates[0]):
#                 vali_sim_dates = [StringClass.get_datetime(s) for s in vali_sim_dates]
#             if order:
#                 sim_dates += vali_sim_dates
#             else:
#                 sim_dates = vali_sim_dates + sim_dates
#         sim_data_list = list()
#         caliBestIdx = -1
#         caliBestNSE = -9999.
#         for idx2, ind in enumerate(sim_data):
#             # tmp = numpy.array(list(ind.values()))
#             # tmp = tmp[:, idx]
#             _, tmp_mat = dict_to_daily_month_matrix(ind)  # shape=(N, 2)
#             tmp = tmp_mat[:, idx]
#             if sim_obs_data[idx2][var]['NSE'] > caliBestNSE:
#                 caliBestNSE = sim_obs_data[idx2][var]['NSE']
#                 caliBestIdx = idx2
#             tmpsim = tmp.tolist()
#             if plot_validation:
#                 # tmp_data = numpy.array(list(vali_sim_data[idx2].values()))[:, idx].tolist()
#                 _, val_mat = dict_to_daily_month_matrix(vali_sim_data[idx2])
#                 tmp_data = val_mat[:, idx].tolist()
#                 if order:
#                     tmpsim += tmp_data
#                 else:
#                     tmpsim = tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         # sim_best = numpy.array(list(sim_data[caliBestIdx].values()))[:, idx]
#         _, mat = dict_to_daily_month_matrix(sim_data[caliBestIdx])
#         # 取逐日或逐月列
#         sim_best = mat[:, idx]  # idx=0 → 日数据，idx=1 → 月均值
#         sim_best = sim_best.tolist()
#         if plot_validation:
#             # tmp_data = numpy.array(list(vali_sim_data[caliBestIdx].values()))[:, idx].tolist()
#             _, val_mat = dict_to_daily_month_matrix(vali_sim_data[caliBestIdx])
#             tmp_data = val_mat[:, idx].tolist()
#             if order:
#                 sim_best += tmp_data
#             else:
#                 sim_best = tmp_data + sim_best
#         sim_data_list = numpy.array(sim_data_list)
#         ylows = numpy.percentile(sim_data_list, 2.5, 0, interpolation='nearest')
#         yups = numpy.percentile(sim_data_list, 97.5, 0, interpolation='nearest')
#
#         def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list):
#             # type: (...) -> (float, float)
#             count = 0
#             ylows_obs = list()
#             yups_obs = list()
#             for oi, ov in enumerate(obs_data_list):
#                 try:
#                     si = sim_dates_list.index(obs_dates_list[oi])
#                     ylows_obs.append(ylows[si])
#                     yups_obs.append(yups[si])
#                     if ylows[si] <= ov <= yups[si]:
#                         count += 1
#                 except Exception:
#                     continue
#             p = float(count) / len(obs_data_list)
#             ylows_obs = numpy.array(ylows_obs)
#             yups_obs = numpy.array(yups_obs)
#             r = numpy.mean(yups_obs - ylows_obs) / numpy.std(numpy.array(obs_data_list))
#             return p, r
#
#         # concatenate text
#         p_value, r_value = calculate_95ppu_efficiency(sim_obs_data[0][var]['Obs'],
#                                                       cali_obs_dates,
#                                                       list(sim_data[0].keys()))
#         txt = 'P-factor: %.2f\nR-factor: %.2f\n' % (p_value, r_value)
#         txt += u'某一最优模拟\n' if plot_cfg.plot_cn else 'One best simulation:\n'
#         txt += '    $\mathit{NSE}$: %.2f\n' \
#                '    $\mathit{RSR}$: %.2f\n' \
#                '    $\mathit{PBIAS}$: %.2f%%\n' \
#                '    $\mathit{R^2}$: %.2f' % (sim_obs_data[caliBestIdx][var]['NSE'],
#                                              sim_obs_data[caliBestIdx][var]['RSR'],
#                                              sim_obs_data[caliBestIdx][var]['PBIAS'],
#                                              sim_obs_data[caliBestIdx][var]['R-square'])
#         # concatenate text of validation if needed
#         vali_txt = ''
#         if plot_validation:
#             p_value, r_value = calculate_95ppu_efficiency(vali_sim_obs_data[0][var]['Obs'],
#                                                           vali_obs_dates,
#                                                           list(vali_sim_data[0].keys()))
#             vali_txt = 'P-factor: %.2f\nR-factor: %.2f\n\n' % (p_value, r_value)
#             vali_txt += '    $\mathit{NSE}$: %.2f\n' \
#                         '    $\mathit{RSR}$: %.2f\n' \
#                         '    $\mathit{PBIAS}$: %.2f%%\n' \
#                         '    $\mathit{R^2}$: %.2f' % (vali_sim_obs_data[caliBestIdx][var]['NSE'],
#                                                       vali_sim_obs_data[caliBestIdx][var]['RSR'],
#                                                       vali_sim_obs_data[caliBestIdx][var]['PBIAS'],
#                                                       vali_sim_obs_data[caliBestIdx][var]['R-square'])
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         obs_dates = np.asarray(obs_dates)
#         ylows = np.asarray(ylows)
#         yups = np.asarray(yups)
#
#         print("================================================")
#         print("var =", var)
#         print("len(sim_dates) =", len(sim_dates))
#         print("len(obs_dates) =", len(obs_dates))
#         print("np.isnan(sim_best).sum() =", np.isnan(sim_best).sum())
#         print("np.isnan(ylows).sum() =", np.isnan(ylows).sum())
#         print("np.isnan(yups).sum() =", np.isnan(yups).sum())
#
#         if np.isnan(sim_best).any():
#             nan_mask = np.isnan(sim_best)
#             print("sim_best 中 NaN 对应的日期：")
#             print(sim_dates[nan_mask])
#
#         if np.isnan(ylows).any():
#             nan_mask = np.isnan(ylows)
#             print("ylows 中 NaN 对应的日期：")
#             print(sim_dates[nan_mask])
#
#         if np.isnan(yups).any():
#             nan_mask = np.isnan(yups)
#             print("yups 中 NaN 对应的日期：")
#             print(sim_dates[nan_mask])
#         print("================================================")
#
#
#         vali_sim_dates = np.asarray(vali_sim_dates)
#         cali_sim_dates = np.asarray(cali_sim_dates)
#
#         # 找到 sim_dates 中与 obs_dates 匹配的位置
#         mask = np.isin(sim_dates, obs_dates)
#
#         # 筛选后的模拟结果和日期
#         sim_dates = sim_dates[mask]
#         sim_best = sim_best[mask]
#         ylows = ylows[mask]
#         yups = yups[mask]
#         # plot
#         fig, ax = plt.subplots(figsize=(12, 4))
#         ax.fill_between(sim_dates, ylows.tolist(), yups.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU')
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20,
#                    color='g', label=observed_label)
#         besesim_label = u'最优模拟' if plot_cfg.plot_cn else 'Best simulation'
#         ax.plot(sim_dates, sim_best, linestyle='--', color='red', linewidth=1,
#                 label=besesim_label)
#         ax.set_xlim(left=min(sim_dates), right=max(sim_dates))
#         ax.set_ylim(bottom=0.)
#         date_fmt = mdates.DateFormatter('%m-%d-%y')
#         ax.xaxis.set_major_formatter(date_fmt)
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2, which='major',
#                        labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2, which='major',
#                        labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#         # plot separate dash line
#         delta_dt = (sim_dates[-1] - sim_dates[0]) // 9
#         delta_dt2 = (sim_dates[-1] - sim_dates[0]) // 35
#         sep_time = sim_dates[-1]
#         time_pos = [sep_time - delta_dt]
#         time_pos2 = [sep_time - 2 * delta_dt]
#         ymax, ymin = ax.get_ylim()
#         yc = abs(ymax - ymin) * 0.9
#         if plot_validation:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             cali_vali_labels = [(u'验证期' if plot_cfg.plot_cn else 'Calibration'),
#                                 (u'率定期' if plot_cfg.plot_cn else 'Validation')]
#             if not order:
#                 cali_vali_labels.reverse()
#             time_pos = [sep_time - delta_dt, sep_time + delta_dt2]
#             time_pos2 = [sep_time - 2 * delta_dt, sep_time + delta_dt2]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=2)
#             plt.text(time_pos[0], yc, cali_vali_labels[0],
#                      fontdict={'style': 'italic', 'weight': 'bold',
#                                'size': plot_cfg.label_fsize},
#                      color='black')
#             plt.text(time_pos[1], yc, cali_vali_labels[1
#             ],
#                      fontdict={'style': 'italic', 'weight': 'bold',
#                                'size': plot_cfg.label_fsize},
#                      color='black')
#
#         # add legend
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = [labels.index('95PPU'), labels.index(observed_label),
#                      labels.index(besesim_label)]
#         ax.legend([handles[idx] for idx in figorders], [labels[idx] for idx in figorders],
#                   fontsize=plot_cfg.legend_fsize, loc=2, framealpha=0.8)
#         # add text
#         cali_pos = time_pos[0] if order else time_pos[1]
#         plt.text(cali_pos, yc * 0.5, txt, color='red', fontsize=plot_cfg.label_fsize - 1)
#         if plot_validation:
#             vali_pos = time_pos[1] if order else time_pos[0]
#             plt.text(vali_pos, yc * 0.5, vali_txt, color='red', fontsize=plot_cfg.label_fsize - 1)
#         # fig.autofmt_xdate(rotation=0, ha='center')
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         # close current plot in case of 'figure.max_open_warning'
#         plt.cla()
#         plt.clf()
#         plt.close()
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     # wp = r'D:\InstallationPath\SEIMS-master\data\TP_zamuhe\TP_zamuhe_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     # wp = r'D:\SEIMS-master\data\TP_zamuhe\TP_zamuhe_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     # wp = r'D:\InstallationPath\SEIMS-master\data\TP_xiyinghe\TP_xiyinghe_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     # wp = r'D:\SEIMS-master\data\TP_xiyinghe\TP_xiyinghe_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     # wp = r'D:\SEIMS-master\data\TP_Amu\TP_Amu_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#     all_cali_obs_data = list()
#     all_cali_data = list()
#     gen_cali_id = list()
#     with open(simdir + os.path.sep + 'gen0_caliObsData.json', 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(simdir + os.path.sep + 'gen0_caliSimData.pickle', 'r', encoding='utf-8') as f:
#         cali_data = pickle.load(f)
#     # validation data
#     with open(simdir + os.path.sep + 'gen0_valiObsData.json', 'r', encoding='utf-8') as f:
#         vali_obs_data = json.load(f)
#     with open(simdir + os.path.sep + 'gen0_valiSimData.pickle', 'r', encoding='utf-8') as f:
#         vali_data = pickle.load(f)
#
#     for a, b in zip(cali_obs_data, cali_data):
#         if [a['Gen'], a['ID']] not in gen_cali_id:
#             gen_cali_id.append([a['Gen'], a['ID']])
#             all_cali_obs_data.append(a)
#             all_cali_data.append(b)
#     calculate_95ppu(cali_obs_data, cali_data, simdir, 0, vali_obs_data, vali_data)
#     print('Total simulations %d' % len(all_cali_obs_data))






# from __future__ import absolute_import, unicode_literals
#
# import json
# import pickle
# import os
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import numpy
# import matplotlib as mpl
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from typing import Optional
# from pygeoc.utils import StringClass, is_string
#
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
# import numpy as np
#
#
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Output population details, i.e., the simulation data, etc."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save the time series simulation data of the entire simulation period.
#     all_sim_data = list()
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save the matched observation-simulation data of calibration period.
#     cali_sim_obs_data = list()
#     cali_sim_data = list()
#
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = list()
#     vali_sim_data = list()
#
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot 95PPU.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print("calculate_95ppu 画图出错：", e)
#         traceback.print_exc()
#
#
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留不删。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#
#     返回:
#       keys_sorted: list[datetime]
#       arr: np.ndarray, shape=(N, 2)
#       列0=daily_sum, 列1=monthly_avg
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#
#     daily = []
#     monthly = []
#
#     for k, v in items:
#         d = np.nan
#         m = np.nan
#
#         if isinstance(v, (list, tuple, np.ndarray)):
#             vals = list(v)
#         else:
#             vals = [v]
#
#         if len(vals) == 2:
#             try:
#                 d = float(vals[0])
#             except Exception:
#                 d = np.nan
#             try:
#                 m = float(vals[1])
#             except Exception:
#                 m = np.nan
#         elif len(vals) == 1:
#             try:
#                 d = float(vals[0])
#             except Exception:
#                 d = np.nan
#
#         daily.append(d)
#         monthly.append(m)
#
#     arr = np.column_stack([daily, monthly])
#     return keys_sorted, arr
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     新函数：按变量个数读取模拟结果。
#
#     适用于多目标变量：
#       var_name = ['F_19', 'D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
#     ind: OrderedDict[datetime -> list]
#       每个时间点的 list 应该包含所有变量值，例如：
#       [F_19, D_1312, D_1313, D_1314, D_1316, D_1321]
#
#     返回:
#       keys_sorted: list[datetime]
#       arr: np.ndarray, shape=(N, nvar)
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#
#     rows = []
#     for k, v in items:
#         if isinstance(v, (list, tuple, np.ndarray)):
#             vals = list(v)
#         else:
#             vals = [v]
#
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#
#         vals = vals[:nvar]
#
#         row = []
#         for x in vals:
#             try:
#                 row.append(float(x))
#             except Exception:
#                 row.append(np.nan)
#
#         rows.append(row)
#
#     arr = np.asarray(rows, dtype=float)
#     return keys_sorted, arr
#
#
# def _to_datetime_list(dates):
#     """Convert string datetime list to datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to float numpy array."""
#     out = []
#     for v in values:
#         out.append(_safe_float(v))
#     return np.asarray(out, dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read metric safely from sim_obs_data."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric for figure annotation."""
#     try:
#         x = float(x)
#         if np.isfinite(x):
#             return '%.2f' % x
#         return 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods are exactly the same."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot hydrographs."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print("calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。")
#         return
#
#     if len(sim_data) < 2:
#         print("calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。")
#         return
#
#     if len(sim_obs_data) == 0:
#         print("calculate_95ppu: sim_obs_data 为空，跳过画图。")
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#
#     print("calculate_95ppu: var_name =", var_name)
#     print("calculate_95ppu: nvar =", nvar)
#     print("calculate_95ppu: simulations =", len(sim_data))
#
#     for idx, var in enumerate(var_name):
#         print("================================================")
#         print("开始画变量:", var, "idx =", idx)
#
#         plot_validation = False
#         if vali_sim_obs_data and vali_sim_data:
#             if len(vali_sim_obs_data) > 0 and var in vali_sim_obs_data[0].get('var_name', []):
#                 plot_validation = True
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             if 'SED' in var.upper():
#                 ylabel_str += ' (g/L)'
#             else:
#                 ylabel_str += ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print("变量 %s 不在 sim_obs_data[0] 中，跳过。" % var)
#             continue
#
#         # -----------------------------
#         # 1. 读取率定期观测
#         # -----------------------------
#         cali_obs_dates = sim_obs_data[0][var]['UTCDATETIME'][:]
#         cali_obs_dates = _to_datetime_list(cali_obs_dates)
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#
#         # 默认：率定期在前，验证期在后
#         order = 1
#
#         # -----------------------------
#         # 2. 读取验证期观测，并判断是否与率定期完全一致
#         # -----------------------------
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#
#         if plot_validation:
#             vali_obs_dates = vali_sim_obs_data[0][var]['UTCDATETIME'][:]
#             vali_obs_dates = _to_datetime_list(vali_obs_dates)
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#
#             # 率定期和验证期不完全一样时，才拼接到同一张图上。
#             # 如果完全一样，只画一套时间序列，但不重复显示 Validation 指标。
#             if not same_cali_vali_period:
#                 if len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                     if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                         order = 0
#                         obs_dates = vali_obs_dates + obs_dates
#                         obs_data = list(vali_obs_values) + obs_data
#                     else:
#                         obs_dates = obs_dates + vali_obs_dates
#                         obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # -----------------------------
#         # 3. 读取模拟时间
#         # -----------------------------
#         cali_sim_dates = list(sim_data[0].keys())
#         cali_sim_dates = _to_datetime_list(cali_sim_dates)
#
#         sim_dates = cali_sim_dates[:]
#
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = list(vali_sim_data[0].keys())
#             vali_sim_dates = _to_datetime_list(vali_sim_dates)
#
#             if not same_cali_vali_period:
#                 if order:
#                     sim_dates = sim_dates + vali_sim_dates
#                 else:
#                     sim_dates = vali_sim_dates + sim_dates
#
#         # -----------------------------
#         # 4. 收集所有个体的当前变量模拟值
#         # -----------------------------
#         sim_data_list = list()
#         caliBestIdx = 0
#         caliBestNSE = -9999.0
#
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#
#             if idx >= tmp_mat.shape[1]:
#                 print("变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。"
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#
#             tmp = tmp_mat[:, idx]
#             tmpsim = tmp.tolist()
#
#             this_nse = _safe_metric(sim_obs_data[idx2], var, 'NSE')
#             if np.isfinite(this_nse) and this_nse > caliBestNSE:
#                 caliBestNSE = this_nse
#                 caliBestIdx = idx2
#
#             if plot_validation and (not same_cali_vali_period):
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     if idx < val_mat.shape[1]:
#                         tmp_data = val_mat[:, idx].tolist()
#                     else:
#                         tmp_data = [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#
#                 if order:
#                     tmpsim += tmp_data
#                 else:
#                     tmpsim = tmp_data + tmpsim
#
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print("变量 %s 没有可用模拟数据，跳过画图。" % var)
#             continue
#
#         # -----------------------------
#         # 5. 最优模拟
#         # -----------------------------
#         _, mat = dict_to_var_matrix(sim_data[caliBestIdx], nvar)
#         sim_best = mat[:, idx].tolist()
#
#         if plot_validation and (not same_cali_vali_period):
#             if caliBestIdx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[caliBestIdx], nvar)
#                 if idx < val_mat.shape[1]:
#                     tmp_data = val_mat[:, idx].tolist()
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#
#             if order:
#                 sim_best += tmp_data
#             else:
#                 sim_best = tmp_data + sim_best
#
#         # -----------------------------
#         # 6. 计算 95PPU
#         # -----------------------------
#         sim_data_list = np.asarray(sim_data_list, dtype=float)
#
#         try:
#             ylows = np.nanpercentile(sim_data_list, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_list, 97.5, axis=0)
#         except Exception as e:
#             print("变量 %s 计算 nanpercentile 失败：%s" % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#
#         if min_len == 0:
#             print("变量 %s 的模拟序列长度为 0，跳过画图。" % var)
#             continue
#
#         # -----------------------------
#         # 7. 计算 P-factor 和 R-factor
#         # -----------------------------
#         def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                        ylows_list, yups_list):
#             count = 0
#             ylows_obs = list()
#             yups_obs = list()
#             valid_obs = list()
#
#             sim_dates_py = list(sim_dates_list)
#
#             for oi, ov in enumerate(obs_data_list):
#                 if not np.isfinite(ov):
#                     continue
#
#                 try:
#                     si = sim_dates_py.index(obs_dates_list[oi])
#                 except Exception:
#                     continue
#
#                 if si >= len(ylows_list) or si >= len(yups_list):
#                     continue
#
#                 if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#                     continue
#
#                 ylows_obs.append(ylows_list[si])
#                 yups_obs.append(yups_list[si])
#                 valid_obs.append(ov)
#
#                 if ylows_list[si] <= ov <= yups_list[si]:
#                     count += 1
#
#             if len(valid_obs) == 0:
#                 return np.nan, np.nan
#
#             p = float(count) / float(len(valid_obs))
#
#             ylows_obs = np.asarray(ylows_obs, dtype=float)
#             yups_obs = np.asarray(yups_obs, dtype=float)
#             valid_obs = np.asarray(valid_obs, dtype=float)
#
#             obs_std = np.nanstd(valid_obs)
#             if not np.isfinite(obs_std) or obs_std == 0:
#                 r = np.nan
#             else:
#                 r = np.nanmean(yups_obs - ylows_obs) / obs_std
#
#             return p, r
#
#         cali_obs_values_arr = _safe_array(sim_obs_data[0][var]['Obs'])
#         p_value, r_value = calculate_95ppu_efficiency(
#             cali_obs_values_arr,
#             cali_obs_dates,
#             sim_dates,
#             ylows,
#             yups
#         )
#
#         # -----------------------------
#         # 8. Calibration 指标
#         # -----------------------------
#         best_nse = _safe_metric(sim_obs_data[caliBestIdx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[caliBestIdx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[caliBestIdx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[caliBestIdx], var, 'R-square')
#
#         # -----------------------------
#         # 9. Validation 指标
#         #    如果验证期和率定期完全一样，后面不显示 Validation。
#         # -----------------------------
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#
#         if plot_validation and (not same_cali_vali_period):
#             vali_obs_values_arr = _safe_array(vali_sim_obs_data[0][var]['Obs'])
#
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 vali_obs_values_arr,
#                 vali_obs_dates,
#                 sim_dates,
#                 ylows,
#                 yups
#             )
#
#             best_nse_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'R-square')
#
#         # -----------------------------
#         # 10. 调试输出
#         # -----------------------------
#         print("变量 =", var)
#         print("same_cali_vali_period =", same_cali_vali_period)
#         print("plot_validation =", plot_validation)
#         print("len(sim_dates) =", len(sim_dates))
#         print("len(obs_dates) =", len(obs_dates))
#         print("sim_best NaN 个数 =", np.isnan(sim_best).sum())
#         print("ylows NaN 个数 =", np.isnan(ylows).sum())
#         print("yups NaN 个数 =", np.isnan(yups).sum())
#
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print("sim_best min/max =", np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # -----------------------------
#         # 11. 绘图时只保留有观测日期的模拟值
#         # -----------------------------
#         mask = np.isin(sim_dates, obs_dates)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print("变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。" % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print("变量 %s：绘图日期为空，跳过。" % var)
#             continue
#
#         # -----------------------------
#         # 12. 开始画图
#         # -----------------------------
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#
#         ax.fill_between(sim_dates_plot,
#                         ylows_plot.tolist(),
#                         yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8),
#                         label='95PPU',
#                         zorder=1)
#
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates,
#                    obs_data,
#                    marker='.',
#                    s=20,
#                    color='g',
#                    label=observed_label,
#                    zorder=4)
#
#         besesim_label = u'最优模拟' if plot_cfg.plot_cn else 'Best simulation'
#         ax.plot(sim_dates_plot,
#                 sim_best_plot,
#                 linestyle='--',
#                 color='red',
#                 linewidth=1.5,
#                 label=besesim_label,
#                 zorder=5)
#
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         # -----------------------------
#         # 13. y轴范围：强制从 0 开始
#         # -----------------------------
#         y_parts = []
#
#         if np.isfinite(obs_data).any():
#             y_parts.append(obs_data[np.isfinite(obs_data)])
#         if np.isfinite(sim_best_plot).any():
#             y_parts.append(sim_best_plot[np.isfinite(sim_best_plot)])
#         if np.isfinite(ylows_plot).any():
#             y_parts.append(ylows_plot[np.isfinite(ylows_plot)])
#         if np.isfinite(yups_plot).any():
#             y_parts.append(yups_plot[np.isfinite(yups_plot)])
#
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#
#             y_high = np.nanpercentile(all_y, 99)
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#
#             pad = 0.08 * y_high
#             ax.set_ylim(bottom=0.0, top=y_high + pad)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         # -----------------------------
#         # 14. 坐标轴样式
#         # -----------------------------
#         date_fmt = mdates.DateFormatter('%m-%d-%y')
#         ax.xaxis.set_major_formatter(date_fmt)
#
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         # -----------------------------
#         # 15. Calibration / Validation 分割线
#         #     只有率定期和验证期不一样时才画
#         # -----------------------------
#         if plot_validation and (not same_cali_vali_period) and \
#                 len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#
#             ax.axvline(sep_time, color='black', linestyle='dashed',
#                        linewidth=1.5, zorder=3)
#
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#
#             ax.text(0.02, 0.92, left_label,
#                     transform=ax.transAxes,
#                     fontdict={'style': 'italic',
#                               'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black',
#                     va='top',
#                     ha='left')
#
#             ax.text(0.52, 0.92, right_label,
#                     transform=ax.transAxes,
#                     fontdict={'style': 'italic',
#                               'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black',
#                     va='top',
#                     ha='left')
#
#         # -----------------------------
#         # 16. 图例
#         # -----------------------------
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, besesim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#
#         if figorders:
#             ax.legend([handles[i] for i in figorders],
#                       [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8),
#                       loc='upper left',
#                       framealpha=0.8)
#
#         # -----------------------------
#         # 17. 指标文字：图内右上角，竖着排列
#         #     如果验证期和率定期一样，只显示 Calibration。
#         # -----------------------------
#         metric_fontsize = max(plot_cfg.label_fsize - 3, 10)
#
#         cali_text = (
#             "Calibration\n"
#             "P-factor: %s\n"
#             "R-factor: %s\n"
#             "NSE: %s\n"
#             "RSR: %s\n"
#             "PBIAS: %s%%\n"
#             "R$^2$: %s"
#             % (_format_metric(p_value),
#                _format_metric(r_value),
#                _format_metric(best_nse),
#                _format_metric(best_rsr),
#                _format_metric(best_pbias),
#                _format_metric(best_r2))
#         )
#
#         if plot_validation and (not same_cali_vali_period):
#             vali_text = (
#                 "\n\nValidation\n"
#                 "P-factor: %s\n"
#                 "R-factor: %s\n"
#                 "NSE: %s\n"
#                 "RSR: %s\n"
#                 "PBIAS: %s%%\n"
#                 "R$^2$: %s"
#                 % (_format_metric(p_value_v),
#                    _format_metric(r_value_v),
#                    _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v),
#                    _format_metric(best_pbias_v),
#                    _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(
#             0.985, 0.965,
#             metric_text,
#             transform=ax.transAxes,
#             color='red',
#             fontsize=metric_fontsize,
#             va='top',
#             ha='right',
#             linespacing=1.15,
#             bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'),
#             zorder=10
#         )
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#
#         plt.cla()
#         plt.clf()
#         plt.close()
#
#         print("变量 %s 画图完成。" % var)
#         print("================================================")
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     all_cali_obs_data = list()
#     all_cali_data = list()
#     gen_cali_id = list()
#
#     with open(simdir + os.path.sep + 'gen0_caliObsData.json', 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#
#     with open(simdir + os.path.sep + 'gen0_caliSimData.pickle', 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#
#     vali_json = simdir + os.path.sep + 'gen0_valiObsData.json'
#     vali_pickle = simdir + os.path.sep + 'gen0_valiSimData.pickle'
#
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#
#     for a, b in zip(cali_obs_data, cali_data):
#         if [a['Gen'], a['ID']] not in gen_cali_id:
#             gen_cali_id.append([a['Gen'], a['ID']])
#             all_cali_obs_data.append(a)
#             all_cali_data.append(b)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, 0, vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, 0)
#
#     print('Total simulations %d' % len(all_cali_obs_data))





# from __future__ import absolute_import, unicode_literals
#
# import json
# import pickle
# import os
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import numpy
# import matplotlib as mpl
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from typing import Optional
# from pygeoc.utils import StringClass, is_string
#
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
# import numpy as np
#
#
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Output population details, i.e., the simulation data, etc."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save the time series simulation data of the entire simulation period.
#     all_sim_data = list()
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save the matched observation-simulation data of calibration period.
#     cali_sim_obs_data = list()
#     cali_sim_data = list()
#
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = list()
#     vali_sim_data = list()
#
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot 95PPU.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print("calculate_95ppu 画图出错：", e)
#         traceback.print_exc()
#
#
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留不删。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#
#     返回:
#       keys_sorted: list[datetime]
#       arr: np.ndarray, shape=(N, 2)
#       列0=daily_sum, 列1=monthly_avg
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#
#     daily = []
#     monthly = []
#
#     for k, v in items:
#         d = np.nan
#         m = np.nan
#
#         if isinstance(v, (list, tuple, np.ndarray)):
#             vals = list(v)
#         else:
#             vals = [v]
#
#         if len(vals) == 2:
#             try:
#                 d = float(vals[0])
#             except Exception:
#                 d = np.nan
#             try:
#                 m = float(vals[1])
#             except Exception:
#                 m = np.nan
#         elif len(vals) == 1:
#             try:
#                 d = float(vals[0])
#             except Exception:
#                 d = np.nan
#
#         daily.append(d)
#         monthly.append(m)
#
#     arr = np.column_stack([daily, monthly])
#     return keys_sorted, arr
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     新函数：按变量个数读取模拟结果。
#
#     适用于多目标变量：
#       var_name = ['F_19', 'D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
#     ind: OrderedDict[datetime -> list]
#       每个时间点的 list 应该包含所有变量值，例如：
#       [F_19, D_1312, D_1313, D_1314, D_1316, D_1321]
#
#     返回:
#       keys_sorted: list[datetime]
#       arr: np.ndarray, shape=(N, nvar)
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#
#     rows = []
#     for k, v in items:
#         if isinstance(v, (list, tuple, np.ndarray)):
#             vals = list(v)
#         else:
#             vals = [v]
#
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#
#         vals = vals[:nvar]
#
#         row = []
#         for x in vals:
#             try:
#                 row.append(float(x))
#             except Exception:
#                 row.append(np.nan)
#
#         rows.append(row)
#
#     arr = np.asarray(rows, dtype=float)
#     return keys_sorted, arr
#
#
# def _to_datetime_list(dates):
#     """Convert string datetime list to datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to float numpy array."""
#     out = []
#     for v in values:
#         out.append(_safe_float(v))
#     return np.asarray(out, dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read metric safely from sim_obs_data."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric for figure annotation."""
#     try:
#         x = float(x)
#         if np.isfinite(x):
#             return '%.2f' % x
#         return 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods are exactly the same."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前代保留下来的所有参数组中，选出一套统一参数用于所有变量绘图。
#
#     规则：
#     1. 优先只在六个 NSE 都 > 0 的参数组里选；
#     2. 评分 score = 0.5 * NSE_F19 + 0.5 * mean(NSE_D1312, D1313, D1314, D1316, D1321)
#     3. 若 score 相同，则最差 NSE 更高者优先。
#     4. 如果没有任何“六个 NSE 都 > 0”的参数组，则退回到“所有 NSE 有效”的参数组中继续选。
#     """
#     area_var = 'F_19'
#     depth_vars = ['D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
#     def collect_candidates(require_all_positive=True):
#         candidates = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             area_nse = _safe_metric(one_ind, area_var, 'NSE')
#             depth_nses = np.asarray([_safe_metric(one_ind, v, 'NSE') for v in depth_vars], dtype=float)
#             all_nses = np.concatenate(([area_nse], depth_nses))
#
#             if not np.isfinite(all_nses).all():
#                 continue
#
#             if require_all_positive and np.any(all_nses <= 0.0):
#                 continue
#
#             score = 0.5 * area_nse + 0.5 * float(np.mean(depth_nses))
#             min_nse = float(np.min(all_nses))
#             mean_depth_nse = float(np.mean(depth_nses))
#
#             candidates.append({
#                 'idx': ind_idx,
#                 'score': score,
#                 'min_nse': min_nse,
#                 'area_nse': float(area_nse),
#                 'mean_depth_nse': mean_depth_nse,
#                 'all_nses': all_nses,
#                 'gen': one_ind.get('Gen', -1),
#                 'id': one_ind.get('ID', -1)
#             })
#         return candidates
#
#     candidates = collect_candidates(require_all_positive=True)
#     if len(candidates) == 0:
#         print('警告：当前代没有找到六个 NSE 都大于 0 的参数组，改为在所有有效参数组中选统一参数。')
#         candidates = collect_candidates(require_all_positive=False)
#
#     if len(candidates) == 0:
#         raise ValueError('当前代没有找到可用于统一绘图的有效参数组。')
#
#     candidates = sorted(candidates, key=lambda x: (-x['score'], -x['min_nse']))
#     best = candidates[0]
#
#     print('================================================')
#     print('当前代统一绘图参数选择结果：')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     print('score = %.6f' % best['score'])
#     print('F_19 NSE = %.6f' % best['area_nse'])
#     print('Mean D NSE = %.6f' % best['mean_depth_nse'])
#     print('Min NSE = %.6f' % best['min_nse'])
#     print('================================================')
#
#     return best['idx']
#
#
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot hydrographs."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print("calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。")
#         return
#
#     if len(sim_data) < 2:
#         print("calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。")
#         return
#
#     if len(sim_obs_data) == 0:
#         print("calculate_95ppu: sim_obs_data 为空，跳过画图。")
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#
#     print("calculate_95ppu: var_name =", var_name)
#     print("calculate_95ppu: nvar =", nvar)
#     print("calculate_95ppu: simulations =", len(sim_data))
#
#     # 关键修改：当前代先统一选择一套参数，后面所有变量都使用这同一套参数绘图
#     shared_best_idx = select_shared_best_index(sim_obs_data)
#
#     for idx, var in enumerate(var_name):
#         print("================================================")
#         print("开始画变量:", var, "idx =", idx)
#
#         plot_validation = False
#         if vali_sim_obs_data and vali_sim_data:
#             if len(vali_sim_obs_data) > 0 and var in vali_sim_obs_data[0].get('var_name', []):
#                 plot_validation = True
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             if 'SED' in var.upper():
#                 ylabel_str += ' (g/L)'
#             else:
#                 ylabel_str += ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print("变量 %s 不在 sim_obs_data[0] 中，跳过。" % var)
#             continue
#
#         # 1. 读取率定期观测
#         cali_obs_dates = sim_obs_data[0][var]['UTCDATETIME'][:]
#         cali_obs_dates = _to_datetime_list(cali_obs_dates)
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. 读取验证期观测
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#
#         if plot_validation:
#             vali_obs_dates = vali_sim_obs_data[0][var]['UTCDATETIME'][:]
#             vali_obs_dates = _to_datetime_list(vali_obs_dates)
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#
#             if not same_cali_vali_period:
#                 if len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                     if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                         order = 0
#                         obs_dates = vali_obs_dates + obs_dates
#                         obs_data = list(vali_obs_values) + obs_data
#                     else:
#                         obs_dates = obs_dates + vali_obs_dates
#                         obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. 读取模拟时间
#         cali_sim_dates = list(sim_data[0].keys())
#         cali_sim_dates = _to_datetime_list(cali_sim_dates)
#
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = list(vali_sim_data[0].keys())
#             vali_sim_dates = _to_datetime_list(vali_sim_dates)
#
#             if not same_cali_vali_period:
#                 if order:
#                     sim_dates = sim_dates + vali_sim_dates
#                 else:
#                     sim_dates = vali_sim_dates + sim_dates
#
#         # 4. 收集所有个体的当前变量模拟值，用于 95PPU
#         sim_data_list = list()
#         caliBestIdx = shared_best_idx
#
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#
#             if idx >= tmp_mat.shape[1]:
#                 print("变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。"
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#
#             tmp = tmp_mat[:, idx]
#             tmpsim = tmp.tolist()
#
#             if plot_validation and (not same_cali_vali_period):
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     if idx < val_mat.shape[1]:
#                         tmp_data = val_mat[:, idx].tolist()
#                     else:
#                         tmp_data = [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#
#                 if order:
#                     tmpsim += tmp_data
#                 else:
#                     tmpsim = tmp_data + tmpsim
#
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print("变量 %s 没有可用模拟数据，跳过画图。" % var)
#             continue
#
#         # 5. 统一参数组对应的最优模拟
#         _, mat = dict_to_var_matrix(sim_data[caliBestIdx], nvar)
#         sim_best = mat[:, idx].tolist()
#
#         if plot_validation and (not same_cali_vali_period):
#             if caliBestIdx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[caliBestIdx], nvar)
#                 if idx < val_mat.shape[1]:
#                     tmp_data = val_mat[:, idx].tolist()
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#
#             if order:
#                 sim_best += tmp_data
#             else:
#                 sim_best = tmp_data + sim_best
#
#         # 6. 计算 95PPU
#         sim_data_list = np.asarray(sim_data_list, dtype=float)
#
#         try:
#             ylows = np.nanpercentile(sim_data_list, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_list, 97.5, axis=0)
#         except Exception as e:
#             print("变量 %s 计算 nanpercentile 失败：%s" % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#
#         if min_len == 0:
#             print("变量 %s 的模拟序列长度为 0，跳过画图。" % var)
#             continue
#
#         # 7. 计算 P-factor 和 R-factor
#         def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                        ylows_list, yups_list):
#             count = 0
#             ylows_obs = list()
#             yups_obs = list()
#             valid_obs = list()
#
#             sim_dates_py = list(sim_dates_list)
#
#             for oi, ov in enumerate(obs_data_list):
#                 if not np.isfinite(ov):
#                     continue
#
#                 try:
#                     si = sim_dates_py.index(obs_dates_list[oi])
#                 except Exception:
#                     continue
#
#                 if si >= len(ylows_list) or si >= len(yups_list):
#                     continue
#
#                 if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#                     continue
#
#                 ylows_obs.append(ylows_list[si])
#                 yups_obs.append(yups_list[si])
#                 valid_obs.append(ov)
#
#                 if ylows_list[si] <= ov <= yups_list[si]:
#                     count += 1
#
#             if len(valid_obs) == 0:
#                 return np.nan, np.nan
#
#             p = float(count) / float(len(valid_obs))
#
#             ylows_obs = np.asarray(ylows_obs, dtype=float)
#             yups_obs = np.asarray(yups_obs, dtype=float)
#             valid_obs = np.asarray(valid_obs, dtype=float)
#
#             obs_std = np.nanstd(valid_obs)
#             if not np.isfinite(obs_std) or obs_std == 0:
#                 r = np.nan
#             else:
#                 r = np.nanmean(yups_obs - ylows_obs) / obs_std
#
#             return p, r
#
#         cali_obs_values_arr = _safe_array(sim_obs_data[0][var]['Obs'])
#         p_value, r_value = calculate_95ppu_efficiency(
#             cali_obs_values_arr,
#             cali_obs_dates,
#             sim_dates,
#             ylows,
#             yups
#         )
#
#         # 8. Calibration 指标：统一从 shared_best_idx 这同一套参数读取
#         best_nse = _safe_metric(sim_obs_data[caliBestIdx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[caliBestIdx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[caliBestIdx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[caliBestIdx], var, 'R-square')
#
#         # 9. Validation 指标
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#
#         if plot_validation and (not same_cali_vali_period):
#             vali_obs_values_arr = _safe_array(vali_sim_obs_data[0][var]['Obs'])
#
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 vali_obs_values_arr,
#                 vali_obs_dates,
#                 sim_dates,
#                 ylows,
#                 yups
#             )
#
#             best_nse_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'R-square')
#
#         # 10. 调试输出
#         print("变量 =", var)
#         print("same_cali_vali_period =", same_cali_vali_period)
#         print("plot_validation =", plot_validation)
#         print("len(sim_dates) =", len(sim_dates))
#         print("len(obs_dates) =", len(obs_dates))
#         print("sim_best NaN 个数 =", np.isnan(sim_best).sum())
#         print("ylows NaN 个数 =", np.isnan(ylows).sum())
#         print("yups NaN 个数 =", np.isnan(yups).sum())
#
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print("sim_best min/max =", np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 11. 绘图时只保留有观测日期的模拟值
#         mask = np.isin(sim_dates, obs_dates)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print("变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。" % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print("变量 %s：绘图日期为空，跳过。" % var)
#             continue
#
#         # 12. 开始画图
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#
#         ax.fill_between(sim_dates_plot,
#                         ylows_plot.tolist(),
#                         yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8),
#                         label='95PPU',
#                         zorder=1)
#
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates,
#                    obs_data,
#                    marker='.',
#                    s=20,
#                    color='g',
#                    label=observed_label,
#                    zorder=4)
#
#         besesim_label = u'最优模拟' if plot_cfg.plot_cn else 'Best simulation'
#         ax.plot(sim_dates_plot,
#                 sim_best_plot,
#                 linestyle='--',
#                 color='red',
#                 linewidth=1.5,
#                 label=besesim_label,
#                 zorder=5)
#
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         # 13. y轴范围：强制从 0 开始
#         y_parts = []
#
#         if np.isfinite(obs_data).any():
#             y_parts.append(obs_data[np.isfinite(obs_data)])
#         if np.isfinite(sim_best_plot).any():
#             y_parts.append(sim_best_plot[np.isfinite(sim_best_plot)])
#         if np.isfinite(ylows_plot).any():
#             y_parts.append(ylows_plot[np.isfinite(ylows_plot)])
#         if np.isfinite(yups_plot).any():
#             y_parts.append(yups_plot[np.isfinite(yups_plot)])
#
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#
#             y_high = np.nanpercentile(all_y, 99)
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#
#             pad = 0.08 * y_high
#             ax.set_ylim(bottom=0.0, top=y_high + pad)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         # 14. 坐标轴样式
#         date_fmt = mdates.DateFormatter('%m-%d-%y')
#         ax.xaxis.set_major_formatter(date_fmt)
#
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         # 15. Calibration / Validation 分割线
#         if plot_validation and (not same_cali_vali_period) and \
#                 len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#
#             ax.axvline(sep_time, color='black', linestyle='dashed',
#                        linewidth=1.5, zorder=3)
#
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#
#             ax.text(0.02, 0.92, left_label,
#                     transform=ax.transAxes,
#                     fontdict={'style': 'italic',
#                               'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black',
#                     va='top',
#                     ha='left')
#
#             ax.text(0.52, 0.92, right_label,
#                     transform=ax.transAxes,
#                     fontdict={'style': 'italic',
#                               'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black',
#                     va='top',
#                     ha='left')
#
#         # 16. 图例
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, besesim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#
#         if figorders:
#             ax.legend([handles[i] for i in figorders],
#                       [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8),
#                       loc='upper left',
#                       framealpha=0.8)
#
#         # 17. 指标文字：图内右上角，竖着排列
#         metric_fontsize = max(plot_cfg.label_fsize - 3, 10)
#
#         cali_text = (
#             "Calibration\n"
#             "P-factor: %s\n"
#             "R-factor: %s\n"
#             "NSE: %s\n"
#             "RSR: %s\n"
#             "PBIAS: %s%%\n"
#             "R$^2$: %s"
#             % (_format_metric(p_value),
#                _format_metric(r_value),
#                _format_metric(best_nse),
#                _format_metric(best_rsr),
#                _format_metric(best_pbias),
#                _format_metric(best_r2))
#         )
#
#         if plot_validation and (not same_cali_vali_period):
#             vali_text = (
#                 "\n\nValidation\n"
#                 "P-factor: %s\n"
#                 "R-factor: %s\n"
#                 "NSE: %s\n"
#                 "RSR: %s\n"
#                 "PBIAS: %s%%\n"
#                 "R$^2$: %s"
#                 % (_format_metric(p_value_v),
#                    _format_metric(r_value_v),
#                    _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v),
#                    _format_metric(best_pbias_v),
#                    _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(
#             0.985, 0.965,
#             metric_text,
#             transform=ax.transAxes,
#             color='red',
#             fontsize=metric_fontsize,
#             va='top',
#             ha='right',
#             linespacing=1.15,
#             bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'),
#             zorder=10
#         )
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#
#         plt.cla()
#         plt.clf()
#         plt.close()
#
#         print("变量 %s 画图完成。" % var)
#         print("================================================")
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     all_cali_obs_data = list()
#     all_cali_data = list()
#     gen_cali_id = list()
#
#     with open(simdir + os.path.sep + 'gen0_caliObsData.json', 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#
#     with open(simdir + os.path.sep + 'gen0_caliSimData.pickle', 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#
#     vali_json = simdir + os.path.sep + 'gen0_valiObsData.json'
#     vali_pickle = simdir + os.path.sep + 'gen0_valiSimData.pickle'
#
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#
#     for a, b in zip(cali_obs_data, cali_data):
#         if [a['Gen'], a['ID']] not in gen_cali_id:
#             gen_cali_id.append([a['Gen'], a['ID']])
#             all_cali_obs_data.append(a)
#             all_cali_data.append(b)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, 0, vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, 0)
#
#     print('Total simulations %d' % len(all_cali_obs_data))






# from __future__ import absolute_import, unicode_literals
#
# import json
# import pickle
# import os
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import numpy
# import matplotlib as mpl
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from typing import Optional
# from pygeoc.utils import StringClass, is_string
#
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
# import numpy as np
#
#
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Output population details, i.e., the simulation data, etc."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save the time series simulation data of the entire simulation period.
#     all_sim_data = list()
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save the matched observation-simulation data of calibration period.
#     cali_sim_obs_data = list()
#     cali_sim_data = list()
#
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = list()
#     vali_sim_data = list()
#
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot 95PPU.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print("calculate_95ppu 画图出错：", e)
#         traceback.print_exc()
#
#
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留不删。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#
#     返回:
#       keys_sorted: list[datetime]
#       arr: np.ndarray, shape=(N, 2)
#       列0=daily_sum, 列1=monthly_avg
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#
#     daily = []
#     monthly = []
#
#     for k, v in items:
#         d = np.nan
#         m = np.nan
#
#         if isinstance(v, (list, tuple, np.ndarray)):
#             vals = list(v)
#         else:
#             vals = [v]
#
#         if len(vals) == 2:
#             try:
#                 d = float(vals[0])
#             except Exception:
#                 d = np.nan
#             try:
#                 m = float(vals[1])
#             except Exception:
#                 m = np.nan
#         elif len(vals) == 1:
#             try:
#                 d = float(vals[0])
#             except Exception:
#                 d = np.nan
#
#         daily.append(d)
#         monthly.append(m)
#
#     arr = np.column_stack([daily, monthly])
#     return keys_sorted, arr
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     新函数：按变量个数读取模拟结果。
#
#     适用于多目标变量：
#       var_name = ['F_19', 'D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
#     ind: OrderedDict[datetime -> list]
#       每个时间点的 list 应该包含所有变量值，例如：
#       [F_19, D_1312, D_1313, D_1314, D_1316, D_1321]
#
#     返回:
#       keys_sorted: list[datetime]
#       arr: np.ndarray, shape=(N, nvar)
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#
#     rows = []
#     for k, v in items:
#         if isinstance(v, (list, tuple, np.ndarray)):
#             vals = list(v)
#         else:
#             vals = [v]
#
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#
#         vals = vals[:nvar]
#
#         row = []
#         for x in vals:
#             try:
#                 row.append(float(x))
#             except Exception:
#                 row.append(np.nan)
#
#         rows.append(row)
#
#     arr = np.asarray(rows, dtype=float)
#     return keys_sorted, arr
#
#
# def _to_datetime_list(dates):
#     """Convert string datetime list to datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to float numpy array."""
#     out = []
#     for v in values:
#         out.append(_safe_float(v))
#     return np.asarray(out, dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read metric safely from sim_obs_data."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric for figure annotation."""
#     try:
#         x = float(x)
#         if np.isfinite(x):
#             return '%.2f' % x
#         return 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods are exactly the same."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前代保留下来的所有参数组中，选出一套统一参数用于所有变量绘图。
#
#     规则：
#     1. 优先只在六个 NSE 都 > 0 的参数组里选；
#     2. 评分 score = mean(NSE_F19, NSE_D1312, NSE_D1313,
#                           NSE_D1314, NSE_D1316, NSE_D1321)；
#        即六个变量等权平均，其中 F_19 占 1/6，五个 D_* 合计占 5/6；
#     3. 若 score 相同，则最差 NSE 更高者优先；
#     4. 如果没有任何“六个 NSE 都 > 0”的参数组，则退回到“所有 NSE 有效”的参数组中继续选。
#     """
#     area_var = 'F_19'
#     depth_vars = ['D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
#     def collect_candidates(require_all_positive=True):
#         candidates = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             area_nse = _safe_metric(one_ind, area_var, 'NSE')
#             depth_nses = np.asarray([_safe_metric(one_ind, v, 'NSE') for v in depth_vars], dtype=float)
#             all_nses = np.concatenate(([area_nse], depth_nses))
#
#             if not np.isfinite(all_nses).all():
#                 continue
#
#             if require_all_positive and np.any(all_nses <= 0.0):
#                 continue
#
#             # 六个变量等权平均：F_19 占 1/6，五个 D_* 合计占 5/6
#             score = float(np.mean(all_nses))
#             min_nse = float(np.min(all_nses))
#             mean_depth_nse = float(np.mean(depth_nses))
#
#             candidates.append({
#                 'idx': ind_idx,
#                 'score': score,
#                 'min_nse': min_nse,
#                 'area_nse': float(area_nse),
#                 'mean_depth_nse': mean_depth_nse,
#                 'all_nses': all_nses,
#                 'gen': one_ind.get('Gen', -1),
#                 'id': one_ind.get('ID', -1)
#             })
#         return candidates
#
#     candidates = collect_candidates(require_all_positive=True)
#     if len(candidates) == 0:
#         print('警告：当前代没有找到六个 NSE 都大于 0 的参数组，改为在所有有效参数组中选统一参数。')
#         candidates = collect_candidates(require_all_positive=False)
#
#     if len(candidates) == 0:
#         raise ValueError('当前代没有找到可用于统一绘图的有效参数组。')
#
#     candidates = sorted(candidates, key=lambda x: (-x['score'], -x['min_nse']))
#     best = candidates[0]
#
#     print('================================================')
#     print('当前代统一绘图参数选择结果：')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     print('Mean six-variable NSE = %.6f' % best['score'])
#     print('F_19 NSE = %.6f' % best['area_nse'])
#     print('Mean D NSE = %.6f' % best['mean_depth_nse'])
#     print('Min NSE = %.6f' % best['min_nse'])
#     print('================================================')
#
#     return best['idx']
#
#
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot hydrographs."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print("calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。")
#         return
#
#     if len(sim_data) < 2:
#         print("calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。")
#         return
#
#     if len(sim_obs_data) == 0:
#         print("calculate_95ppu: sim_obs_data 为空，跳过画图。")
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#
#     print("calculate_95ppu: var_name =", var_name)
#     print("calculate_95ppu: nvar =", nvar)
#     print("calculate_95ppu: simulations =", len(sim_data))
#
#     # 关键修改：当前代先统一选择一套参数，后面所有变量都使用这同一套参数绘图
#     shared_best_idx = select_shared_best_index(sim_obs_data)
#
#     for idx, var in enumerate(var_name):
#         print("================================================")
#         print("开始画变量:", var, "idx =", idx)
#
#         plot_validation = False
#         if vali_sim_obs_data and vali_sim_data:
#             if len(vali_sim_obs_data) > 0 and var in vali_sim_obs_data[0].get('var_name', []):
#                 plot_validation = True
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             if 'SED' in var.upper():
#                 ylabel_str += ' (g/L)'
#             else:
#                 ylabel_str += ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print("变量 %s 不在 sim_obs_data[0] 中，跳过。" % var)
#             continue
#
#         # 1. 读取率定期观测
#         cali_obs_dates = sim_obs_data[0][var]['UTCDATETIME'][:]
#         cali_obs_dates = _to_datetime_list(cali_obs_dates)
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. 读取验证期观测
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#
#         if plot_validation:
#             vali_obs_dates = vali_sim_obs_data[0][var]['UTCDATETIME'][:]
#             vali_obs_dates = _to_datetime_list(vali_obs_dates)
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#
#             if not same_cali_vali_period:
#                 if len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                     if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                         order = 0
#                         obs_dates = vali_obs_dates + obs_dates
#                         obs_data = list(vali_obs_values) + obs_data
#                     else:
#                         obs_dates = obs_dates + vali_obs_dates
#                         obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. 读取模拟时间
#         cali_sim_dates = list(sim_data[0].keys())
#         cali_sim_dates = _to_datetime_list(cali_sim_dates)
#
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = list(vali_sim_data[0].keys())
#             vali_sim_dates = _to_datetime_list(vali_sim_dates)
#
#             if not same_cali_vali_period:
#                 if order:
#                     sim_dates = sim_dates + vali_sim_dates
#                 else:
#                     sim_dates = vali_sim_dates + sim_dates
#
#         # 4. 收集所有个体的当前变量模拟值，用于 95PPU
#         sim_data_list = list()
#         caliBestIdx = shared_best_idx
#
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#
#             if idx >= tmp_mat.shape[1]:
#                 print("变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。"
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#
#             tmp = tmp_mat[:, idx]
#             tmpsim = tmp.tolist()
#
#             if plot_validation and (not same_cali_vali_period):
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     if idx < val_mat.shape[1]:
#                         tmp_data = val_mat[:, idx].tolist()
#                     else:
#                         tmp_data = [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#
#                 if order:
#                     tmpsim += tmp_data
#                 else:
#                     tmpsim = tmp_data + tmpsim
#
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print("变量 %s 没有可用模拟数据，跳过画图。" % var)
#             continue
#
#         # 5. 统一参数组对应的最优模拟
#         _, mat = dict_to_var_matrix(sim_data[caliBestIdx], nvar)
#         sim_best = mat[:, idx].tolist()
#
#         if plot_validation and (not same_cali_vali_period):
#             if caliBestIdx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[caliBestIdx], nvar)
#                 if idx < val_mat.shape[1]:
#                     tmp_data = val_mat[:, idx].tolist()
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#
#             if order:
#                 sim_best += tmp_data
#             else:
#                 sim_best = tmp_data + sim_best
#
#         # 6. 计算 95PPU
#         sim_data_list = np.asarray(sim_data_list, dtype=float)
#
#         try:
#             ylows = np.nanpercentile(sim_data_list, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_list, 97.5, axis=0)
#         except Exception as e:
#             print("变量 %s 计算 nanpercentile 失败：%s" % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#
#         if min_len == 0:
#             print("变量 %s 的模拟序列长度为 0，跳过画图。" % var)
#             continue
#
#         # 7. 计算 P-factor 和 R-factor
#         def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                        ylows_list, yups_list):
#             count = 0
#             ylows_obs = list()
#             yups_obs = list()
#             valid_obs = list()
#
#             sim_dates_py = list(sim_dates_list)
#
#             for oi, ov in enumerate(obs_data_list):
#                 if not np.isfinite(ov):
#                     continue
#
#                 try:
#                     si = sim_dates_py.index(obs_dates_list[oi])
#                 except Exception:
#                     continue
#
#                 if si >= len(ylows_list) or si >= len(yups_list):
#                     continue
#
#                 if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#                     continue
#
#                 ylows_obs.append(ylows_list[si])
#                 yups_obs.append(yups_list[si])
#                 valid_obs.append(ov)
#
#                 if ylows_list[si] <= ov <= yups_list[si]:
#                     count += 1
#
#             if len(valid_obs) == 0:
#                 return np.nan, np.nan
#
#             p = float(count) / float(len(valid_obs))
#
#             ylows_obs = np.asarray(ylows_obs, dtype=float)
#             yups_obs = np.asarray(yups_obs, dtype=float)
#             valid_obs = np.asarray(valid_obs, dtype=float)
#
#             obs_std = np.nanstd(valid_obs)
#             if not np.isfinite(obs_std) or obs_std == 0:
#                 r = np.nan
#             else:
#                 r = np.nanmean(yups_obs - ylows_obs) / obs_std
#
#             return p, r
#
#         cali_obs_values_arr = _safe_array(sim_obs_data[0][var]['Obs'])
#         p_value, r_value = calculate_95ppu_efficiency(
#             cali_obs_values_arr,
#             cali_obs_dates,
#             sim_dates,
#             ylows,
#             yups
#         )
#
#         # 8. Calibration 指标：统一从 shared_best_idx 这同一套参数读取
#         best_nse = _safe_metric(sim_obs_data[caliBestIdx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[caliBestIdx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[caliBestIdx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[caliBestIdx], var, 'R-square')
#
#         # 9. Validation 指标
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#
#         if plot_validation and (not same_cali_vali_period):
#             vali_obs_values_arr = _safe_array(vali_sim_obs_data[0][var]['Obs'])
#
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 vali_obs_values_arr,
#                 vali_obs_dates,
#                 sim_dates,
#                 ylows,
#                 yups
#             )
#
#             best_nse_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[caliBestIdx], var, 'R-square')
#
#         # 10. 调试输出
#         print("变量 =", var)
#         print("same_cali_vali_period =", same_cali_vali_period)
#         print("plot_validation =", plot_validation)
#         print("len(sim_dates) =", len(sim_dates))
#         print("len(obs_dates) =", len(obs_dates))
#         print("sim_best NaN 个数 =", np.isnan(sim_best).sum())
#         print("ylows NaN 个数 =", np.isnan(ylows).sum())
#         print("yups NaN 个数 =", np.isnan(yups).sum())
#
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print("sim_best min/max =", np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 11. 绘图时只保留有观测日期的模拟值
#         mask = np.isin(sim_dates, obs_dates)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print("变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。" % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print("变量 %s：绘图日期为空，跳过。" % var)
#             continue
#
#         # 12. 开始画图
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#
#         ax.fill_between(sim_dates_plot,
#                         ylows_plot.tolist(),
#                         yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8),
#                         label='95PPU',
#                         zorder=1)
#
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates,
#                    obs_data,
#                    marker='.',
#                    s=20,
#                    color='g',
#                    label=observed_label,
#                    zorder=4)
#
#         besesim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot,
#                 sim_best_plot,
#                 linestyle='--',
#                 color='red',
#                 linewidth=1.5,
#                 label=besesim_label,
#                 zorder=5)
#
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         # 13. y轴范围：强制从 0 开始
#         y_parts = []
#
#         if np.isfinite(obs_data).any():
#             y_parts.append(obs_data[np.isfinite(obs_data)])
#         if np.isfinite(sim_best_plot).any():
#             y_parts.append(sim_best_plot[np.isfinite(sim_best_plot)])
#         if np.isfinite(ylows_plot).any():
#             y_parts.append(ylows_plot[np.isfinite(ylows_plot)])
#         if np.isfinite(yups_plot).any():
#             y_parts.append(yups_plot[np.isfinite(yups_plot)])
#
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#
#             y_high = np.nanpercentile(all_y, 99)
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#
#             pad = 0.08 * y_high
#             ax.set_ylim(bottom=0.0, top=y_high + pad)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         # 14. 坐标轴样式
#         date_fmt = mdates.DateFormatter('%m-%d-%y')
#         ax.xaxis.set_major_formatter(date_fmt)
#
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         # 15. Calibration / Validation 分割线
#         if plot_validation and (not same_cali_vali_period) and \
#                 len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#
#             ax.axvline(sep_time, color='black', linestyle='dashed',
#                        linewidth=1.5, zorder=3)
#
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#
#             ax.text(0.02, 0.92, left_label,
#                     transform=ax.transAxes,
#                     fontdict={'style': 'italic',
#                               'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black',
#                     va='top',
#                     ha='left')
#
#             ax.text(0.52, 0.92, right_label,
#                     transform=ax.transAxes,
#                     fontdict={'style': 'italic',
#                               'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black',
#                     va='top',
#                     ha='left')
#
#         # 16. 图例
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, besesim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#
#         if figorders:
#             ax.legend([handles[i] for i in figorders],
#                       [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8),
#                       loc='upper left',
#                       framealpha=0.8)
#
#         # 17. 指标文字：图内右上角，竖着排列
#         metric_fontsize = max(plot_cfg.label_fsize - 3, 10)
#
#         cali_text = (
#             "Calibration\n"
#             "P-factor: %s\n"
#             "R-factor: %s\n"
#             "NSE: %s\n"
#             "RSR: %s\n"
#             "PBIAS: %s%%\n"
#             "R$^2$: %s"
#             % (_format_metric(p_value),
#                _format_metric(r_value),
#                _format_metric(best_nse),
#                _format_metric(best_rsr),
#                _format_metric(best_pbias),
#                _format_metric(best_r2))
#         )
#
#         if plot_validation and (not same_cali_vali_period):
#             vali_text = (
#                 "\n\nValidation\n"
#                 "P-factor: %s\n"
#                 "R-factor: %s\n"
#                 "NSE: %s\n"
#                 "RSR: %s\n"
#                 "PBIAS: %s%%\n"
#                 "R$^2$: %s"
#                 % (_format_metric(p_value_v),
#                    _format_metric(r_value_v),
#                    _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v),
#                    _format_metric(best_pbias_v),
#                    _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(
#             0.985, 0.965,
#             metric_text,
#             transform=ax.transAxes,
#             color='red',
#             fontsize=metric_fontsize,
#             va='top',
#             ha='right',
#             linespacing=1.15,
#             bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'),
#             zorder=10
#         )
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#
#         plt.cla()
#         plt.clf()
#         plt.close()
#
#         print("变量 %s 画图完成。" % var)
#         print("================================================")
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     all_cali_obs_data = list()
#     all_cali_data = list()
#     gen_cali_id = list()
#
#     with open(simdir + os.path.sep + 'gen0_caliObsData.json', 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#
#     with open(simdir + os.path.sep + 'gen0_caliSimData.pickle', 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#
#     vali_json = simdir + os.path.sep + 'gen0_valiObsData.json'
#     vali_pickle = simdir + os.path.sep + 'gen0_valiSimData.pickle'
#
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#
#     for a, b in zip(cali_obs_data, cali_data):
#         if [a['Gen'], a['ID']] not in gen_cali_id:
#             gen_cali_id.append([a['Gen'], a['ID']])
#             all_cali_obs_data.append(a)
#             all_cali_data.append(b)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, 0, vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, 0)
#
#     print('Total simulations %d' % len(all_cali_obs_data))



# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # F_19 仍然会读取并绘图，但不再作为最终参数选择的主要依据。
# AREA_VAR = 'F_19'
# DEPTH_VARS = ['D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 请确认 0 是否就是你需要展示的最终候选参数集；若最终结果在其他代，修改该值。
# GEN_NUM_TO_PLOT = 0
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_19', 'D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#       每一日期的值 = [F_19, D_1312, D_1313, D_1314, D_1316, D_1321]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：五个实测水深优先，F_19 继续绘图但只辅助排序
# # ============================================================================
# def diagnose_best_nse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最大 NSE。
#
#     解释：
#     - 如果某个 D_* 的最大 NSE 仍 <= 0，说明当前候选参数中根本没有
#       能将该水深模拟到正 NSE 的参数，换选参规则也无法真正修复；
#     - 如果各个 D_* 单独最大 NSE 都 > 0，而统一参数仍有负 NSE，
#       则说明这些目标之间存在冲突，需要继续调整率定目标或模型过程。
#     """
#     var_list = [AREA_VAR] + DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最大 NSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             nse = _safe_metric(one_ind, var, 'NSE')
#             if np.isfinite(nse):
#                 valid_items.append((float(nse), ind_idx,
#                                     one_ind.get('Gen', -1), one_ind.get('ID', -1)))
#         if len(valid_items) == 0:
#             print('%s: 没有有效 NSE。' % var)
#             continue
#         best_nse, best_idx, best_gen, best_id = sorted(valid_items, key=lambda x: -x[0])[0]
#         print('%s: max NSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_nse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     选择一套统一参数供全部变量共同绘图。
#
#     选参依据：
#     1. 五个 D_* 是实测水深，为主要依据；F_19 继续绘图，但仅作为辅助排序；
#     2. 优先选择五个水深 NSE 均 > 0 的参数组；
#     3. 存在满足条件的参数组时：最大化最差水深 NSE，其次最大化平均水深 NSE，
#        最后才比较 F_19 NSE；
#     4. 若不存在五个水深均为正的参数组：优先减少非正 NSE 的水深个数，
#        随后提高最差水深 NSE、平均水深 NSE，最后参考 F_19 NSE。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         depth_nses = np.asarray([_safe_metric(one_ind, v, 'NSE') for v in DEPTH_VARS], dtype=float)
#         if not np.isfinite(depth_nses).all():
#             continue
#
#         area_nse = _safe_metric(one_ind, AREA_VAR, 'NSE')
#         area_sort_value = float(area_nse) if np.isfinite(area_nse) else -9999.0
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'area_nse': float(area_nse) if np.isfinite(area_nse) else np.nan,
#             'area_sort_value': area_sort_value,
#             'depth_nses': depth_nses,
#             'min_depth_nse': float(np.min(depth_nses)),
#             'mean_depth_nse': float(np.mean(depth_nses)),
#             'nonpositive_depth_num': int(np.sum(depth_nses <= 0.0))
#         })
#
#     if len(candidates) == 0:
#         raise ValueError('当前候选参数集中没有五个实测水深 NSE 都有效的参数组。')
#
#     feasible = [c for c in candidates if c['min_depth_nse'] > 0.0]
#
#     if len(feasible) > 0:
#         print('找到 %d 套五个实测水深 NSE 全部大于 0 的参数组。' % len(feasible))
#         print('参数选择顺序：最差水深 NSE -> 平均水深 NSE -> F_19 NSE。')
#         candidate_pool = sorted(
#             feasible,
#             key=lambda c: (-c['min_depth_nse'],
#                            -c['mean_depth_nse'],
#                            -c['area_sort_value'])
#         )
#     else:
#         print('================================================')
#         print('警告：当前候选参数集中不存在五个实测水深 NSE 全部大于 0 的参数组。')
#         print('本次选出的参数仅用于诊断绘图，不能说明负 NSE 问题已解决。')
#         print('诊断选择顺序：非正水深 NSE 个数最少 -> 最差水深 NSE 最高 -> 平均水深 NSE 最高 -> F_19 NSE 最高。')
#         print('需要进一步修改真实率定目标、参数范围或模型过程。')
#         print('================================================')
#         candidate_pool = sorted(
#             candidates,
#             key=lambda c: (c['nonpositive_depth_num'],
#                            -c['min_depth_nse'],
#                            -c['mean_depth_nse'],
#                            -c['area_sort_value'])
#         )
#
#     best = candidate_pool[0]
#
#     print('================================================')
#     print('统一参数选择结果（实测水深优先，F_19 为辅助验证）：')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     if np.isfinite(best['area_nse']):
#         print('%s NSE = %.6f' % (AREA_VAR, best['area_nse']))
#     else:
#         print('%s NSE = nan' % AREA_VAR)
#     for var, nse in zip(DEPTH_VARS, best['depth_nses']):
#         print('%s NSE = %.6f' % (var, nse))
#     print('Mean depth NSE = %.6f' % best['mean_depth_nse'])
#     print('Minimum depth NSE = %.6f' % best['min_depth_nse'])
#     print('Non-positive depth NSE number = %d' % best['nonpositive_depth_num'])
#     print('================================================')
#
#     return best['idx']
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     required_depths = [v for v in DEPTH_VARS if v not in var_name]
#     if len(required_depths) > 0:
#         raise ValueError('变量列表中缺少用于水深优先选参的变量：%s' % str(required_depths))
#
#     diagnose_best_nse_for_each_variable(sim_obs_data)
#     shared_best_idx = select_shared_best_index(sim_obs_data)
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (_format_metric(p_value), _format_metric(r_value), _format_metric(best_nse),
#                _format_metric(best_rsr), _format_metric(best_pbias), _format_metric(best_r2))
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (_format_metric(p_value_v), _format_metric(r_value_v), _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v), _format_metric(best_pbias_v), _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))




# # -*- coding: utf-8 -*-
# # F_19 仅验证/绘图；统一参数严格由五个实测水深选择。
#
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # F_19 仍然会读取并绘图，但不再作为最终参数选择的主要依据。
# AREA_VAR = 'F_19'
# DEPTH_VARS = ['D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 请确认 0 是否就是你需要展示的最终候选参数集；若最终结果在其他代，修改该值。
# GEN_NUM_TO_PLOT = 80
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_19', 'D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#       每一日期的值 = [F_19, D_1312, D_1313, D_1314, D_1316, D_1321]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：五个实测水深优先，F_19 继续绘图但只辅助排序
# # ============================================================================
# def diagnose_best_nse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最大 NSE。
#
#     解释：
#     - 如果某个 D_* 的最大 NSE 仍 <= 0，说明当前候选参数中根本没有
#       能将该水深模拟到正 NSE 的参数，换选参规则也无法真正修复；
#     - 如果各个 D_* 单独最大 NSE 都 > 0，而统一参数仍有负 NSE，
#       则说明这些目标之间存在冲突，需要继续调整率定目标或模型过程。
#     """
#     var_list = [AREA_VAR] + DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最大 NSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             nse = _safe_metric(one_ind, var, 'NSE')
#             if np.isfinite(nse):
#                 valid_items.append((float(nse), ind_idx,
#                                     one_ind.get('Gen', -1), one_ind.get('ID', -1)))
#         if len(valid_items) == 0:
#             print('%s: 没有有效 NSE。' % var)
#             continue
#         best_nse, best_idx, best_gen, best_id = sorted(valid_items, key=lambda x: -x[0])[0]
#         print('%s: max NSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_nse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     选择一套统一参数供 F_19 与五个 D_* 共同绘图。
#
#     规则：五个实测水深是唯一选参依据；F_19 只验证和绘图，
#     不参与率定，也不参与统一参数排序。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         depth_nses = np.asarray(
#             [_safe_metric(one_ind, var, 'NSE') for var in DEPTH_VARS],
#             dtype=float
#         )
#         area_nse = _safe_metric(one_ind, AREA_VAR, 'NSE')
#
#         if not np.isfinite(depth_nses).all():
#             continue
#         if np.any(depth_nses > 1.000001):
#             continue
#         if np.any(depth_nses <= -9000.0):
#             continue
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'area_nse': float(area_nse) if np.isfinite(area_nse) else np.nan,
#             'depth_nses': depth_nses,
#             'min_depth_nse': float(np.min(depth_nses)),
#             'mean_depth_nse': float(np.mean(depth_nses)),
#             'nonpositive_depth_num': int(np.sum(depth_nses <= 0.0))
#         })
#
#     if len(candidates) == 0:
#         raise ValueError('当前候选参数集中没有有效的五个水深参数组。请检查模型失败日志。')
#
#     feasible = [c for c in candidates if c['min_depth_nse'] > 0.0]
#
#     if len(feasible) > 0:
#         print('找到 %d 套五个实测水深 NSE 全部大于 0 的参数组。' % len(feasible))
#         print('选参只依据水深：最差水深 NSE -> 平均水深 NSE。')
#         candidate_pool = sorted(
#             feasible,
#             key=lambda c: (-c['min_depth_nse'], -c['mean_depth_nse'])
#         )
#     else:
#         print('================================================')
#         print('警告：不存在五个实测水深 NSE 全部大于 0 的参数组。')
#         print('本次按水深表现选择诊断参数；F_19 仅展示，不参与选择。')
#         print('================================================')
#         candidate_pool = sorted(
#             candidates,
#             key=lambda c: (
#                 c['nonpositive_depth_num'],
#                 -c['min_depth_nse'],
#                 -c['mean_depth_nse']
#             )
#         )
#
#     best = candidate_pool[0]
#
#     print('================================================')
#     print('统一参数选择结果：仅依据五个实测水深选择')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     for var, nse in zip(DEPTH_VARS, best['depth_nses']):
#         print('%s NSE = %.6f' % (var, nse))
#     print('Mean depth NSE = %.6f' % best['mean_depth_nse'])
#     print('Minimum depth NSE = %.6f' % best['min_depth_nse'])
#     print('Non-positive depth NSE number = %d' % best['nonpositive_depth_num'])
#     if np.isfinite(best['area_nse']):
#         print('%s NSE = %.6f （仅用于验证和绘图，不参与率定与选参）'
#               % (AREA_VAR, best['area_nse']))
#     else:
#         print('%s NSE = nan （仅用于验证和绘图，不参与率定与选参）' % AREA_VAR)
#     print('================================================')
#
#     return best['idx']
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     required_depths = [v for v in DEPTH_VARS if v not in var_name]
#     if len(required_depths) > 0:
#         raise ValueError('变量列表中缺少用于水深优先选参的变量：%s' % str(required_depths))
#
#     diagnose_best_nse_for_each_variable(sim_obs_data)
#     shared_best_idx = select_shared_best_index(sim_obs_data)
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (_format_metric(p_value), _format_metric(r_value), _format_metric(best_nse),
#                _format_metric(best_rsr), _format_metric(best_pbias), _format_metric(best_r2))
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (_format_metric(p_value_v), _format_metric(r_value_v), _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v), _format_metric(best_pbias_v), _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))





# # -*- coding: utf-8 -*-
# # D_1321 与 F_19 仅验证/绘图；统一参数严格由四个主要实测水深选择。
#
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积：仅用于辅助验证和绘图
# AREA_VAR = 'F_19'
#
# # 近零浅水边缘点：仅用于辅助验证和绘图
# EDGE_DEPTH_VAR = 'D_1321'
#
# # 真正参与率定和最终统一参数选择的四个水深
# CALIB_DEPTH_VARS = ['D_1312', 'D_1313', 'D_1314', 'D_1316']
#
# # 需要绘图和诊断的全部水深变量
# PLOT_DEPTH_VARS = ['D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 单独运行本文件时请将该值设为实际需要查看的最终代数。
# GEN_NUM_TO_PLOT = 80
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_19', 'D_1312', 'D_1313', 'D_1314', 'D_1316', 'D_1321']
#       每一日期的值 = [F_19, D_1312, D_1313, D_1314, D_1316, D_1321]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：四个主要实测水深决定参数；D_1321 与 F_19 仅验证
# # ============================================================================
# def diagnose_best_nse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最大 NSE。
#
#     解释：
#     - 如果某个 D_* 的最大 NSE 仍 <= 0，说明当前候选参数中根本没有
#       能将该水深模拟到正 NSE 的参数，换选参规则也无法真正修复；
#     - 如果各个 D_* 单独最大 NSE 都 > 0，而统一参数仍有负 NSE，
#       则说明这些目标之间存在冲突，需要继续调整率定目标或模型过程。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最大 NSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             nse = _safe_metric(one_ind, var, 'NSE')
#             if np.isfinite(nse):
#                 valid_items.append((float(nse), ind_idx,
#                                     one_ind.get('Gen', -1), one_ind.get('ID', -1)))
#         if len(valid_items) == 0:
#             print('%s: 没有有效 NSE。' % var)
#             continue
#         best_nse, best_idx, best_gen, best_id = sorted(valid_items, key=lambda x: -x[0])[0]
#         print('%s: max NSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_nse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     选择一套统一参数供 F_19 和五个水深变量共同绘图。
#
#     选参规则：
#     1. 只有 D_1312、D_1313、D_1314、D_1316 参与最终统一参数选择；
#     2. D_1321 观测值大部分接近 0，仅作为浅水边缘点验证；
#     3. F_19 仅作为淹没面积验证；
#     4. 优先选择四个率定水深 NSE 全部 > 0 的参数组；
#     5. 若不存在，则选择非正 NSE 数量最少、最差 NSE 最高、
#        平均 NSE 最高的一套参数。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         calib_depth_nses = np.asarray(
#             [_safe_metric(one_ind, var, 'NSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#
#         edge_depth_nse = _safe_metric(one_ind, EDGE_DEPTH_VAR, 'NSE')
#         area_nse = _safe_metric(one_ind, AREA_VAR, 'NSE')
#
#         # 四个率定水深必须具有有效 NSE
#         if not np.isfinite(calib_depth_nses).all():
#             continue
#
#         # NSE 理论最大值为 1，排除旧错误结果或异常数据
#         if np.any(calib_depth_nses > 1.000001):
#             continue
#
#         # 排除模型失败后赋予的惩罚值
#         if np.any(calib_depth_nses <= -9000.0):
#             continue
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#             'area_nse': float(area_nse) if np.isfinite(area_nse) else np.nan,
#             'edge_depth_nse': float(edge_depth_nse) if np.isfinite(edge_depth_nse) else np.nan,
#             'calib_depth_nses': calib_depth_nses,
#             'min_depth_nse': float(np.min(calib_depth_nses)),
#             'mean_depth_nse': float(np.mean(calib_depth_nses)),
#             'nonpositive_depth_num': int(np.sum(calib_depth_nses <= 0.0))
#         })
#
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选参数集中没有有效的四个率定水深参数组。'
#             '请检查模型失败日志。'
#         )
#
#     feasible = [c for c in candidates if c['min_depth_nse'] > 0.0]
#
#     if len(feasible) > 0:
#         print('找到 %d 套四个主要实测水深 NSE 全部大于 0 的参数组。'
#               % len(feasible))
#         print('选参只依据四个水深：最差水深 NSE -> 平均水深 NSE。')
#         candidate_pool = sorted(
#             feasible,
#             key=lambda c: (-c['min_depth_nse'], -c['mean_depth_nse'])
#         )
#     else:
#         print('================================================')
#         print('警告：不存在四个主要实测水深 NSE 全部大于 0 的参数组。')
#         print('本次按四个水深表现选择诊断参数；')
#         print('D_1321 和 F_19 均只展示，不参与选择。')
#         print('================================================')
#         candidate_pool = sorted(
#             candidates,
#             key=lambda c: (
#                 c['nonpositive_depth_num'],
#                 -c['min_depth_nse'],
#                 -c['mean_depth_nse']
#             )
#         )
#
#     best = candidate_pool[0]
#
#     print('================================================')
#     print('统一参数选择结果：仅依据四个主要实测水深选择')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#
#     for var, nse in zip(CALIB_DEPTH_VARS, best['calib_depth_nses']):
#         print('%s NSE = %.6f' % (var, nse))
#
#     print('Mean calibration-depth NSE = %.6f' % best['mean_depth_nse'])
#     print('Minimum calibration-depth NSE = %.6f' % best['min_depth_nse'])
#     print('Non-positive calibration-depth NSE number = %d'
#           % best['nonpositive_depth_num'])
#
#     if np.isfinite(best['edge_depth_nse']):
#         print('%s NSE = %.6f （近零边缘点验证，不参与率定与选参）'
#               % (EDGE_DEPTH_VAR, best['edge_depth_nse']))
#     else:
#         print('%s NSE = nan （近零边缘点验证，不参与率定与选参）'
#               % EDGE_DEPTH_VAR)
#
#     if np.isfinite(best['area_nse']):
#         print('%s NSE = %.6f （面积验证，不参与率定与选参）'
#               % (AREA_VAR, best['area_nse']))
#     else:
#         print('%s NSE = nan （面积验证，不参与率定与选参）'
#               % AREA_VAR)
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#
#     return best['idx'], best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     required_vars = [AREA_VAR, EDGE_DEPTH_VAR] + CALIB_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_nse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代所选出的统一参数及其指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir, 'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write('Selection rule: four calibration depths only; '
#                 'D_1321 and F_19 are validation variables only.\n')
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#
#         for var, nse in zip(CALIB_DEPTH_VARS, best_info['calib_depth_nses']):
#             f.write('%s NSE: %.6f\n' % (var, nse))
#
#         f.write('Mean calibration-depth NSE: %.6f\n'
#                 % best_info['mean_depth_nse'])
#         f.write('Minimum calibration-depth NSE: %.6f\n'
#                 % best_info['min_depth_nse'])
#
#         if np.isfinite(best_info['edge_depth_nse']):
#             f.write('%s NSE: %.6f (validation only)\n'
#                     % (EDGE_DEPTH_VAR, best_info['edge_depth_nse']))
#
#         if np.isfinite(best_info['area_nse']):
#             f.write('%s NSE: %.6f (validation only)\n'
#                     % (AREA_VAR, best_info['area_nse']))
#
#         f.write('Parameter values: %s\n' % str(best_info['gene_values']))
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (_format_metric(p_value), _format_metric(r_value), _format_metric(best_nse),
#                _format_metric(best_rsr), _format_metric(best_pbias), _format_metric(best_r2))
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (_format_metric(p_value_v), _format_metric(r_value_v), _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v), _format_metric(best_pbias_v), _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))




# # -*- coding: utf-8 -*-
# # F_19 仅验证/绘图；统一参数严格由五个实测水深选择。
#
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积：仅用于辅助验证和绘图
# AREA_VAR = 'F_19'
#
# # 真正参与率定和最终统一参数选择的五个水深
# CALIB_DEPTH_VARS = ['D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#
# # 需要绘图和诊断的全部水深变量
# PLOT_DEPTH_VARS = ['D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 单独运行本文件时请将该值设为实际需要查看的最终代数。
# GEN_NUM_TO_PLOT = 80
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_19', 'D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#       每一日期的值 = [F_19, D_1304, D_1305, D_1306, D_1309, D_1310]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：五个实测水深决定参数；F_19 仅验证
# # ============================================================================
# def diagnose_best_nse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最大 NSE。
#
#     解释：
#     - 如果某个 D_* 的最大 NSE 仍 <= 0，说明当前候选参数中根本没有
#       能将该水深模拟到正 NSE 的参数，换选参规则也无法真正修复；
#     - 如果各个 D_* 单独最大 NSE 都 > 0，而统一参数仍有负 NSE，
#       则说明这些目标之间存在冲突，需要继续调整率定目标或模型过程。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最大 NSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             nse = _safe_metric(one_ind, var, 'NSE')
#             if np.isfinite(nse):
#                 valid_items.append((float(nse), ind_idx,
#                                     one_ind.get('Gen', -1), one_ind.get('ID', -1)))
#         if len(valid_items) == 0:
#             print('%s: 没有有效 NSE。' % var)
#             continue
#         best_nse, best_idx, best_gen, best_id = sorted(valid_items, key=lambda x: -x[0])[0]
#         print('%s: max NSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_nse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     选择一套统一参数供 F_19 和五个水深变量共同绘图。
#
#     选参规则：
#     1. D_1304、D_1305、D_1306、D_1309、D_1310 五个水深参与最终统一参数选择；
#     2. F_19 仅作为淹没面积辅助验证和绘图，不参与选参；
#     3. 优先选择五个率定水深 NSE 全部 > 0 的参数组；
#     4. 若不存在，则选择非正 NSE 数量最少、最差 NSE 最高、
#        平均 NSE 最高的一套参数。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         calib_depth_nses = np.asarray(
#             [_safe_metric(one_ind, var, 'NSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#
#         area_nse = _safe_metric(one_ind, AREA_VAR, 'NSE')
#
#         # 五个率定水深必须具有有效 NSE
#         if not np.isfinite(calib_depth_nses).all():
#             continue
#
#         # NSE 理论最大值为 1，排除旧错误结果或异常数据
#         if np.any(calib_depth_nses > 1.000001):
#             continue
#
#         # 排除模型失败后赋予的惩罚值
#         if np.any(calib_depth_nses <= -9000.0):
#             continue
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#             'area_nse': float(area_nse) if np.isfinite(area_nse) else np.nan,
#             'calib_depth_nses': calib_depth_nses,
#             'min_depth_nse': float(np.min(calib_depth_nses)),
#             'mean_depth_nse': float(np.mean(calib_depth_nses)),
#             'nonpositive_depth_num': int(np.sum(calib_depth_nses <= 0.0))
#         })
#
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选参数集中没有有效的五个率定水深参数组。'
#             '请检查模型失败日志。'
#         )
#
#     feasible = [c for c in candidates if c['min_depth_nse'] > 0.0]
#
#     if len(feasible) > 0:
#         print('找到 %d 套五个实测水深 NSE 全部大于 0 的参数组。'
#               % len(feasible))
#         print('选参只依据五个水深：最差水深 NSE -> 平均水深 NSE。')
#         candidate_pool = sorted(
#             feasible,
#             key=lambda c: (-c['min_depth_nse'], -c['mean_depth_nse'])
#         )
#     else:
#         print('================================================')
#         print('警告：不存在五个实测水深 NSE 全部大于 0 的参数组。')
#         print('本次按五个水深表现选择诊断参数；')
#         print('F_19 只展示，不参与选择。')
#         print('================================================')
#         candidate_pool = sorted(
#             candidates,
#             key=lambda c: (
#                 c['nonpositive_depth_num'],
#                 -c['min_depth_nse'],
#                 -c['mean_depth_nse']
#             )
#         )
#
#     best = candidate_pool[0]
#
#     print('================================================')
#     print('统一参数选择结果：仅依据五个实测水深选择')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#
#     for var, nse in zip(CALIB_DEPTH_VARS, best['calib_depth_nses']):
#         print('%s NSE = %.6f' % (var, nse))
#
#     print('Mean calibration-depth NSE = %.6f' % best['mean_depth_nse'])
#     print('Minimum calibration-depth NSE = %.6f' % best['min_depth_nse'])
#     print('Non-positive calibration-depth NSE number = %d'
#           % best['nonpositive_depth_num'])
#
#     if np.isfinite(best['area_nse']):
#         print('%s NSE = %.6f （面积验证，不参与率定与选参）'
#               % (AREA_VAR, best['area_nse']))
#     else:
#         print('%s NSE = nan （面积验证，不参与率定与选参）'
#               % AREA_VAR)
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#
#     return best['idx'], best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     required_vars = [AREA_VAR] + CALIB_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_nse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代所选出的统一参数及其指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir, 'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write('Selection rule: five calibration depths only; '
#                 'F_19 is a validation variable only.\n')
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#
#         for var, nse in zip(CALIB_DEPTH_VARS, best_info['calib_depth_nses']):
#             f.write('%s NSE: %.6f\n' % (var, nse))
#
#         f.write('Mean calibration-depth NSE: %.6f\n'
#                 % best_info['mean_depth_nse'])
#         f.write('Minimum calibration-depth NSE: %.6f\n'
#                 % best_info['min_depth_nse'])
#
#         if np.isfinite(best_info['area_nse']):
#             f.write('%s NSE: %.6f (validation only)\n'
#                     % (AREA_VAR, best_info['area_nse']))
#
#         f.write('Parameter values: %s\n' % str(best_info['gene_values']))
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (_format_metric(p_value), _format_metric(r_value), _format_metric(best_nse),
#                _format_metric(best_rsr), _format_metric(best_pbias), _format_metric(best_r2))
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (_format_metric(p_value_v), _format_metric(r_value_v), _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v), _format_metric(best_pbias_v), _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))








# # -*- coding: utf-8 -*-
# # F_19、D_1309、D_1310 仅验证/绘图；统一参数严格由三个低洼核心区实测水深选择。
#
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积：仅用于辅助验证和绘图
# AREA_VAR = 'F_19'
#
# # 真正参与率定和最终统一参数选择的三个低洼核心区水深
# CALIB_DEPTH_VARS = ['D_1304', 'D_1305', 'D_1306']
#
# # 需要绘图和诊断的全部水深变量
# # D_1309、D_1310 仍然绘图和计算指标，但不参与最终统一参数选择
# PLOT_DEPTH_VARS = ['D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 单独运行本文件时请将该值设为实际需要查看的最终代数。
# GEN_NUM_TO_PLOT = 80
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_19', 'D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#       每一日期的值 = [F_19, D_1304, D_1305, D_1306, D_1309, D_1310]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：三个核心实测水深决定参数；F_19、D_1309、D_1310 仅验证
# # ============================================================================
# def diagnose_best_nse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最大 NSE。
#
#     解释：
#     - 如果某个 D_* 的最大 NSE 仍 <= 0，说明当前候选参数中根本没有
#       能将该水深模拟到正 NSE 的参数，换选参规则也无法真正修复；
#     - 如果各个 D_* 单独最大 NSE 都 > 0，而统一参数仍有负 NSE，
#       则说明这些目标之间存在冲突，需要继续调整率定目标或模型过程。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最大 NSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             nse = _safe_metric(one_ind, var, 'NSE')
#             if np.isfinite(nse):
#                 valid_items.append((float(nse), ind_idx,
#                                     one_ind.get('Gen', -1), one_ind.get('ID', -1)))
#         if len(valid_items) == 0:
#             print('%s: 没有有效 NSE。' % var)
#             continue
#         best_nse, best_idx, best_gen, best_id = sorted(valid_items, key=lambda x: -x[0])[0]
#         print('%s: max NSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_nse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     选择一套统一参数供 F_19 和全部水深变量共同绘图。
#
#     选参规则：
#     1. D_1304、D_1305、D_1306 三个低洼核心区水深参与最终统一参数选择；
#     2. D_1309、D_1310 仅作为边缘水深点辅助验证和绘图，不参与选参；
#     3. F_19 仅作为淹没面积辅助验证和绘图，不参与选参；
#     4. 优先选择三个率定水深 NSE 全部 > 0 的参数组；
#     5. 若不存在，则选择非正 NSE 数量最少、最差 NSE 最高、平均 NSE 最高的一套参数。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         calib_depth_nses = np.asarray(
#             [_safe_metric(one_ind, var, 'NSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#
#         area_nse = _safe_metric(one_ind, AREA_VAR, 'NSE')
#
#         # 三个率定水深必须具有有效 NSE
#         if not np.isfinite(calib_depth_nses).all():
#             continue
#
#         # NSE 理论最大值为 1，排除旧错误结果或异常数据
#         if np.any(calib_depth_nses > 1.000001):
#             continue
#
#         # 排除模型失败后赋予的惩罚值
#         if np.any(calib_depth_nses <= -9000.0):
#             continue
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#             'area_nse': float(area_nse) if np.isfinite(area_nse) else np.nan,
#             'calib_depth_nses': calib_depth_nses,
#             'min_depth_nse': float(np.min(calib_depth_nses)),
#             'mean_depth_nse': float(np.mean(calib_depth_nses)),
#             'nonpositive_depth_num': int(np.sum(calib_depth_nses <= 0.0))
#         })
#
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选参数集中没有有效的三个率定水深参数组。'
#             '请检查模型失败日志。'
#         )
#
#     feasible = [c for c in candidates if c['min_depth_nse'] > 0.0]
#
#     if len(feasible) > 0:
#         print('找到 %d 套三个核心实测水深 NSE 全部大于 0 的参数组。'
#               % len(feasible))
#         print('选参只依据三个核心水深：最差水深 NSE -> 平均水深 NSE。')
#         candidate_pool = sorted(
#             feasible,
#             key=lambda c: (-c['min_depth_nse'], -c['mean_depth_nse'])
#         )
#     else:
#         print('================================================')
#         print('警告：不存在三个核心实测水深 NSE 全部大于 0 的参数组。')
#         print('本次按三个核心水深表现选择诊断参数；')
#         print('D_1309、D_1310 和 F_19 只展示，不参与选择。')
#         print('================================================')
#         candidate_pool = sorted(
#             candidates,
#             key=lambda c: (
#                 c['nonpositive_depth_num'],
#                 -c['min_depth_nse'],
#                 -c['mean_depth_nse']
#             )
#         )
#
#     best = candidate_pool[0]
#
#     print('================================================')
#     print('统一参数选择结果：仅依据三个核心实测水深选择')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#
#     for var, nse in zip(CALIB_DEPTH_VARS, best['calib_depth_nses']):
#         print('%s NSE = %.6f' % (var, nse))
#
#     print('Mean calibration-depth NSE = %.6f' % best['mean_depth_nse'])
#     print('Minimum calibration-depth NSE = %.6f' % best['min_depth_nse'])
#     print('Non-positive calibration-depth NSE number = %d'
#           % best['nonpositive_depth_num'])
#
#     # 辅助变量只打印，不参与选择
#     aux_vars = [v for v in PLOT_DEPTH_VARS if v not in CALIB_DEPTH_VARS] + [AREA_VAR]
#     for aux_var in aux_vars:
#         aux_nse = _safe_metric(sim_obs_data[best['idx']], aux_var, 'NSE')
#         if np.isfinite(aux_nse):
#             print('%s NSE = %.6f （辅助验证，不参与率定与选参）'
#                   % (aux_var, aux_nse))
#         else:
#             print('%s NSE = nan （辅助验证，不参与率定与选参）' % aux_var)
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#
#     return best['idx'], best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_nse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代所选出的统一参数及其指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir, 'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write('Selection rule: three core calibration depths only '
#                 '(D_1304, D_1305, D_1306); '
#                 'D_1309, D_1310 and F_19 are validation/plot variables only.\n')
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#
#         for var, nse in zip(CALIB_DEPTH_VARS, best_info['calib_depth_nses']):
#             f.write('%s NSE: %.6f\n' % (var, nse))
#
#         f.write('Mean calibration-depth NSE: %.6f\n'
#                 % best_info['mean_depth_nse'])
#         f.write('Minimum calibration-depth NSE: %.6f\n'
#                 % best_info['min_depth_nse'])
#
#         # 辅助验证变量指标也写入文件，但不参与选择
#         aux_vars = [v for v in PLOT_DEPTH_VARS if v not in CALIB_DEPTH_VARS] + [AREA_VAR]
#         for aux_var in aux_vars:
#             aux_nse = _safe_metric(sim_obs_data[shared_best_idx], aux_var, 'NSE')
#             if np.isfinite(aux_nse):
#                 f.write('%s NSE: %.6f (validation only)\n' % (aux_var, aux_nse))
#             else:
#                 f.write('%s NSE: nan (validation only)\n' % aux_var)
#
#         f.write('Parameter values: %s\n' % str(best_info['gene_values']))
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (_format_metric(p_value), _format_metric(r_value), _format_metric(best_nse),
#                _format_metric(best_rsr), _format_metric(best_pbias), _format_metric(best_r2))
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (_format_metric(p_value_v), _format_metric(r_value_v), _format_metric(best_nse_v),
#                    _format_metric(best_rsr_v), _format_metric(best_pbias_v), _format_metric(best_r2_v))
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_80_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))



# #  用RMSE
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积：仅用于辅助验证和绘图
# AREA_VAR = 'F_18'
#
# # 真正参与率定和最终统一参数选择的三个低洼核心区水深
# CALIB_DEPTH_VARS = ['D_1366', 'D_1367', 'D_1368']
#
# # 需要绘图和诊断的全部水深变量
# # D_1309、D_1310 仍然绘图和计算指标，但不参与最终统一参数选择
# PLOT_DEPTH_VARS = ['D_1366', 'D_1367', 'D_1368', 'D_1370', 'D_1375']
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
# GEN_NUM_TO_PLOT = 100
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_19', 'D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#       每一日期的值 = [F_19, D_1304, D_1305, D_1306, D_1309, D_1310]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：三个核心实测水深决定参数；F_19、D_1309、D_1310 仅验证
# # ============================================================================
# def diagnose_best_rmse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最小 RMSE。
#
#     RMSE 越小越好。该函数仅用于诊断，不直接改变最终统一参数。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最小 RMSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             rmse = _safe_metric(one_ind, var, 'RMSE')
#             if np.isfinite(rmse) and rmse >= 0.0:
#                 valid_items.append((
#                     float(rmse),
#                     ind_idx,
#                     one_ind.get('Gen', -1),
#                     one_ind.get('ID', -1)
#                 ))
#
#         if len(valid_items) == 0:
#             print('%s: 没有有效 RMSE。' % var)
#             continue
#
#         best_rmse, best_idx, best_gen, best_id = sorted(
#             valid_items, key=lambda x: x[0]
#         )[0]
#         print('%s: min RMSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_rmse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前候选参数集中选择一套统一参数。
#
#     选参规则：
#     1. 只使用 D_1304、D_1305、D_1306 三个核心实测水深的 RMSE；
#     2. D_1309、D_1310 和 F_19 只用于辅助验证和绘图；
#     3. 首先最小化三个核心点中的最大 RMSE，避免某一个点特别差；
#     4. 若最大 RMSE 相同或接近，再最小化三个点的平均 RMSE。
#
#     排序顺序：
#         (max RMSE, mean RMSE)
#
#     两项均越小越好。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         depth_rmses = np.asarray(
#             [_safe_metric(one_ind, var, 'RMSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_nses = np.asarray(
#             [_safe_metric(one_ind, var, 'NSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_pbias = np.asarray(
#             [_safe_metric(one_ind, var, 'PBIAS') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_r2 = np.asarray(
#             [_safe_metric(one_ind, var, 'R-square') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_rsr = np.asarray(
#             [_safe_metric(one_ind, var, 'RSR') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#
#         # 三个核心点必须都有有效 RMSE。
#         if not np.isfinite(depth_rmses).all():
#             continue
#
#         # RMSE 理论上不能小于 0；同时排除模型失败后的异常惩罚值。
#         if np.any(depth_rmses < 0.0) or np.any(depth_rmses >= 9000.0):
#             continue
#
#         mean_rmse = float(np.mean(depth_rmses))
#         max_rmse = float(np.max(depth_rmses))
#         min_rmse = float(np.min(depth_rmses))
#
#         mean_nse = (
#             float(np.nanmean(depth_nses))
#             if np.isfinite(depth_nses).any() else np.nan
#         )
#         min_nse = (
#             float(np.nanmin(depth_nses))
#             if np.isfinite(depth_nses).any() else np.nan
#         )
#         mean_abs_pbias = (
#             float(np.nanmean(np.abs(depth_pbias)))
#             if np.isfinite(depth_pbias).any() else np.nan
#         )
#         mean_r2 = (
#             float(np.nanmean(depth_r2))
#             if np.isfinite(depth_r2).any() else np.nan
#         )
#         mean_rsr = (
#             float(np.nanmean(depth_rsr))
#             if np.isfinite(depth_rsr).any() else np.nan
#         )
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#             'calib_depth_rmses': depth_rmses,
#             'calib_depth_nses': depth_nses,
#             'calib_depth_pbias': depth_pbias,
#             'calib_depth_r2': depth_r2,
#             'calib_depth_rsr': depth_rsr,
#             'mean_depth_rmse': mean_rmse,
#             'max_depth_rmse': max_rmse,
#             'min_depth_rmse': min_rmse,
#             'mean_depth_nse': mean_nse,
#             'min_depth_nse': min_nse,
#             'mean_abs_pbias': mean_abs_pbias,
#             'mean_r2': mean_r2,
#             'mean_rsr': mean_rsr,
#         })
#
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选参数集中没有有效的三个核心水深 RMSE 参数组。'
#             '请检查 gen*_caliObsData.json 是否包含 '
#             'D_1304、D_1305、D_1306 的 RMSE，'
#             '并检查模型是否运行成功。'
#         )
#
#     candidate_pool = sorted(
#         candidates,
#         key=lambda c: (
#             c['max_depth_rmse'],
#             c['mean_depth_rmse']
#         )
#     )
#     best = candidate_pool[0]
#
#     print('================================================')
#     print('统一参数选择结果：只依据三个核心实测水深的 RMSE')
#     print('选参顺序：先最小化最大 RMSE，再最小化平均 RMSE')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#
#     for i, var in enumerate(CALIB_DEPTH_VARS):
#         print('%s RMSE = %.6f, NSE = %.6f, RSR = %.6f, '
#               'PBIAS = %.6f, R2 = %.6f'
#               % (
#                   var,
#                   best['calib_depth_rmses'][i],
#                   best['calib_depth_nses'][i],
#                   best['calib_depth_rsr'][i],
#                   best['calib_depth_pbias'][i],
#                   best['calib_depth_r2'][i]
#               ))
#
#     print('Mean calibration-depth RMSE = %.6f'
#           % best['mean_depth_rmse'])
#     print('Maximum calibration-depth RMSE = %.6f'
#           % best['max_depth_rmse'])
#     print('Minimum calibration-depth RMSE = %.6f'
#           % best['min_depth_rmse'])
#     print('Mean calibration-depth NSE = %.6f'
#           % best['mean_depth_nse'])
#     print('Minimum calibration-depth NSE = %.6f'
#           % best['min_depth_nse'])
#     print('Mean calibration-depth RSR = %.6f'
#           % best['mean_rsr'])
#     print('Mean absolute PBIAS = %.6f'
#           % best['mean_abs_pbias'])
#     print('Mean R2 = %.6f'
#           % best['mean_r2'])
#
#     # 辅助变量只打印，不参与选择。
#     aux_vars = (
#         [v for v in PLOT_DEPTH_VARS if v not in CALIB_DEPTH_VARS]
#         + [AREA_VAR]
#     )
#     for aux_var in aux_vars:
#         aux_rmse = _safe_metric(
#             sim_obs_data[best['idx']], aux_var, 'RMSE'
#         )
#         aux_nse = _safe_metric(
#             sim_obs_data[best['idx']], aux_var, 'NSE'
#         )
#         aux_rsr = _safe_metric(
#             sim_obs_data[best['idx']], aux_var, 'RSR'
#         )
#         aux_pbias = _safe_metric(
#             sim_obs_data[best['idx']], aux_var, 'PBIAS'
#         )
#         aux_r2 = _safe_metric(
#             sim_obs_data[best['idx']], aux_var, 'R-square'
#         )
#
#         print('%s RMSE = %.6f, NSE = %.6f, RSR = %.6f, '
#               'PBIAS = %.6f, R2 = %.6f（辅助验证，不参与选参）'
#               % (
#                   aux_var,
#                   aux_rmse,
#                   aux_nse,
#                   aux_rsr,
#                   aux_pbias,
#                   aux_r2
#               ))
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#     return best['idx'], best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_rmse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代所选出的统一参数及其指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir, 'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: minimize the maximum RMSE first, '
#             'then minimize the mean RMSE; '
#             'three core calibration depths only '
#             '(D_1304, D_1305, D_1306); '
#             'D_1309, D_1310 and F_19 are validation/plot variables only.\n'
#         )
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#
#         for i, var in enumerate(CALIB_DEPTH_VARS):
#             f.write('%s RMSE: %.6f\n'
#                     % (var, best_info['calib_depth_rmses'][i]))
#             f.write('%s NSE: %.6f\n'
#                     % (var, best_info['calib_depth_nses'][i]))
#             f.write('%s RSR: %.6f\n'
#                     % (var, best_info['calib_depth_rsr'][i]))
#             f.write('%s PBIAS: %.6f\n'
#                     % (var, best_info['calib_depth_pbias'][i]))
#             f.write('%s R2: %.6f\n'
#                     % (var, best_info['calib_depth_r2'][i]))
#
#         f.write('Mean calibration-depth RMSE: %.6f\n'
#                 % best_info['mean_depth_rmse'])
#         f.write('Maximum calibration-depth RMSE: %.6f\n'
#                 % best_info['max_depth_rmse'])
#         f.write('Minimum calibration-depth RMSE: %.6f\n'
#                 % best_info['min_depth_rmse'])
#         f.write('Mean calibration-depth NSE: %.6f\n'
#                 % best_info.get('mean_depth_nse', np.nan))
#         f.write('Minimum calibration-depth NSE: %.6f\n'
#                 % best_info.get('min_depth_nse', np.nan))
#         f.write('Mean calibration-depth RSR: %.6f\n'
#                 % best_info.get('mean_rsr', np.nan))
#         f.write('Mean absolute PBIAS: %.6f\n'
#                 % best_info.get('mean_abs_pbias', np.nan))
#         f.write('Mean R2: %.6f\n'
#                 % best_info.get('mean_r2', np.nan))
#
#         # 辅助验证变量指标也写入文件，但不参与选择。
#         aux_vars = (
#             [v for v in PLOT_DEPTH_VARS if v not in CALIB_DEPTH_VARS]
#             + [AREA_VAR]
#         )
#         for aux_var in aux_vars:
#             aux_rmse = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'RMSE'
#             )
#             aux_nse = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'NSE'
#             )
#             aux_rsr = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'RSR'
#             )
#             aux_pbias = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'PBIAS'
#             )
#             aux_r2 = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'R-square'
#             )
#
#             f.write('%s RMSE: %.6f (validation only)\n'
#                     % (aux_var, aux_rmse))
#             f.write('%s NSE: %.6f (validation only)\n'
#                     % (aux_var, aux_nse))
#             f.write('%s RSR: %.6f (validation only)\n'
#                     % (aux_var, aux_rsr))
#             f.write('%s PBIAS: %.6f (validation only)\n'
#                     % (aux_var, aux_pbias))
#             f.write('%s R2: %.6f (validation only)\n'
#                     % (aux_var, aux_r2))
#
#         f.write('Parameter values: %s\n'
#                 % str(best_info['gene_values']))
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rmse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'RMSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (
#                 _format_metric(p_value),
#                 _format_metric(r_value),
#                 _format_metric(best_nse),
#                 _format_metric(best_rmse),
#                 _format_metric(best_rsr),
#                 _format_metric(best_pbias),
#                 _format_metric(best_r2)
#             )
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'RMSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (
#                     _format_metric(p_value_v),
#                     _format_metric(r_value_v),
#                     _format_metric(best_nse_v),
#                     _format_metric(best_rmse_v),
#                     _format_metric(best_rsr_v),
#                     _format_metric(best_pbias_v),
#                     _format_metric(best_r2_v)
#                 )
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     # 注意：这里要改成你实际的率定结果文件夹。
#     # 你前面发的是 Gen100 图，所以这里先写 Gen_100_Pop_12；如果实际文件夹不是这个，需要手动改。
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))




# # 六变量输出、4个NSE目标NSGA-II结果后处理与统一参数选择
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积变量
# AREA_VAR = 'F_18'
#
# # 三个核心区水深点
# CORE_DEPTH_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
# ]
#
# # 两个边缘浅水点
# EDGE_DEPTH_VARS = [
#     'D_1370',
#     'D_1375',
# ]
#
# # 参与率定的水深变量：三个核心点 + D_1370
# CALIB_DEPTH_VARS = CORE_DEPTH_VARS + ['D_1370']
#
# # 所有需要绘图和诊断的水深变量
# PLOT_DEPTH_VARS = CORE_DEPTH_VARS + EDGE_DEPTH_VARS
#
# # 与主率定程序multiobj完全一致的4个NSE目标。
# # NSE越大越好，理想值为1。
# OBJECTIVE_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
# ]
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
# GEN_NUM_TO_PLOT = 100
#
#
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_18', 'D_1366', 'D_1367', 'D_1368', 'D_1370', 'D_1375']
#       每一日期的值 = [F_18, D_1366, D_1367, D_1368, D_1370, D_1375]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：4个水深点的NSE共同决定共享参数
# # ============================================================================
# def diagnose_best_rmse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最小 RMSE。
#
#     RMSE 越小越好。该函数仅用于诊断，不直接改变最终统一参数。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最小 RMSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             rmse = _safe_metric(one_ind, var, 'RMSE')
#             if np.isfinite(rmse) and rmse >= 0.0:
#                 valid_items.append((
#                     float(rmse),
#                     ind_idx,
#                     one_ind.get('Gen', -1),
#                     one_ind.get('ID', -1)
#                 ))
#
#         if len(valid_items) == 0:
#             print('%s: 没有有效 RMSE。' % var)
#             continue
#
#         best_rmse, best_idx, best_gen, best_id = sorted(
#             valid_items, key=lambda x: x[0]
#         )[0]
#         print('%s: min RMSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_rmse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前一代NSGA-II候选种群中选择一套统一参数。
#
#     参与统一选参的4个目标：
#       D_1366 NSE
#       D_1367 NSE
#       D_1368 NSE
#       D_1370 NSE
#
#     D_1375和F_18只用于输出、绘图和独立验证。
#
#     排序规则：
#       1. 最大化4个水深点中的最小NSE；
#       2. 若最小NSE相同，再最大化4个点的平均NSE。
#
#     最终六张图使用同一个shared_best_idx。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         nse_values = []
#         candidate_valid = True
#
#         for var in OBJECTIVE_VARS:
#             nse = _safe_metric(one_ind, var, 'NSE')
#
#             if not np.isfinite(nse) or nse <= -9000.0:
#                 candidate_valid = False
#                 break
#
#             nse_values.append(float(nse))
#
#         if not candidate_valid:
#             continue
#
#         nse_values = np.asarray(nse_values, dtype=float)
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
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选种群中没有同时具备4个有效NSE的参数组。'
#         )
#
#     # NSE越大越好：
#     # 第一优先最大化最差点NSE，第二优先最大化平均NSE。
#     candidates.sort(
#         key=lambda item: (
#             -item['min_nse'],
#             -item['mean_nse'],
#         )
#     )
#
#     best = candidates[0]
#     shared_best_idx = best['idx']
#
#     print('================================================')
#     print('统一参数选择结果：当前候选种群、4个NSE目标')
#     print('1. 最大化4个水深点中的最小NSE')
#     print('2. 最大化4个水深点的平均NSE')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     print('minimum NSE = %.6f' % best['min_nse'])
#     print('mean NSE = %.6f' % best['mean_nse'])
#     print('---------------- 4个NSE目标 ----------------')
#
#     for var, nse in zip(OBJECTIVE_VARS, best['nse_values']):
#         print('%s NSE = %.10f' % (var, nse))
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#     return shared_best_idx, best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_rmse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代选出的统一参数、4个NSE目标及诊断指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir,
#         'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: four water-depth NSE objectives are used '
#             'together. D_1375 and F_18 are used only for independent '
#             'validation and plotting.\n'
#         )
#         f.write(
#             'The selected parameter set first maximizes the minimum NSE '
#             'among the four water-depth variables and then maximizes '
#             'their mean NSE.\n\n'
#         )
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#         f.write('Shared best index: %s\n' % str(best_info['idx']))
#         f.write('Minimum NSE: %.10f\n' % best_info['min_nse'])
#         f.write('Mean NSE: %.10f\n' % best_info['mean_nse'])
#
#         f.write('\nFour NSE calibration objectives:\n')
#         for var, nse in zip(OBJECTIVE_VARS, best_info['nse_values']):
#             f.write('%s NSE: %.12f\n' % (var, nse))
#
#         # 同时保存所选参数组的其他诊断指标。
#         f.write('\nAll-variable diagnostic metrics:\n')
#         all_vars = [
#             'D_1366',
#             'D_1367',
#             'D_1368',
#             'D_1370',
#             'D_1375',
#             'F_18',
#         ]
#         diagnostic_metrics = [
#             'NSE',
#             'NSE1',
#             'lnNSE',
#             'RMSE',
#             'RSR',
#             'PBIAS',
#             'R-square',
#         ]
#         selected_result = sim_obs_data[shared_best_idx]
#
#         for var in all_vars:
#             f.write('\n[%s]\n' % var)
#             for metric in diagnostic_metrics:
#                 metric_value = _safe_metric(selected_result, var, metric)
#                 f.write('%s: %.12f\n' % (metric, metric_value))
#
#         f.write(
#             '\nParameter values: %s\n'
#             % str(best_info['gene_values'])
#         )
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_kge = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE1')
#         best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_kge_v = np.nan
#         best_rmse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_kge_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE1')
#             best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'KGE: %s\n'
#             'RMSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (
#                 _format_metric(p_value),
#                 _format_metric(r_value),
#                 _format_metric(best_nse),
#                 _format_metric(best_kge),
#                 _format_metric(best_rmse),
#                 _format_metric(best_rsr),
#                 _format_metric(best_pbias),
#                 _format_metric(best_r2)
#             )
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'KGE: %s\n'
#                 'RMSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (
#                     _format_metric(p_value_v),
#                     _format_metric(r_value_v),
#                     _format_metric(best_nse_v),
#                     _format_metric(best_kge_v),
#                     _format_metric(best_rmse_v),
#                     _format_metric(best_rsr_v),
#                     _format_metric(best_pbias_v),
#                     _format_metric(best_r2_v)
#                 )
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     # 注意：这里要改成你实际的率定结果文件夹。
#     # 你前面发的是 Gen100 图，所以这里先写 Gen_100_Pop_12；如果实际文件夹不是这个，需要手动改。
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))


# # 六变量输出、3个核心点NSE目标NSGA-II结果后处理与统一参数选择
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积变量
# AREA_VAR = 'F_18'
#
# # 三个核心区水深点
# CORE_DEPTH_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
# ]
#
# # 两个边缘浅水点
# EDGE_DEPTH_VARS = [
#     'D_1370',
#     'D_1375',
# ]
#
# # 参与率定和最终统一选参的水深变量：三个核心点
# CALIB_DEPTH_VARS = list(CORE_DEPTH_VARS)
#
# # 所有需要绘图和诊断的水深变量
# # D_1370和D_1375仍然保留，用于辅助验证和绘图。
# PLOT_DEPTH_VARS = CORE_DEPTH_VARS + EDGE_DEPTH_VARS
#
# # 与主率定程序multiobj完全一致的三个NSE目标。
# # NSE越大越好，理想值为1。
# OBJECTIVE_VARS = list(CORE_DEPTH_VARS)
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
# GEN_NUM_TO_PLOT = 100
#
#
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_18', 'D_1366', 'D_1367', 'D_1368', 'D_1370', 'D_1375']
#       每一日期的值 = [F_18, D_1366, D_1367, D_1368, D_1370, D_1375]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：三个核心水深点的NSE共同决定共享参数
# # ============================================================================
# def diagnose_best_rmse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最小 RMSE。
#
#     RMSE 越小越好。该函数仅用于诊断，不直接改变最终统一参数。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最小 RMSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             rmse = _safe_metric(one_ind, var, 'RMSE')
#             if np.isfinite(rmse) and rmse >= 0.0:
#                 valid_items.append((
#                     float(rmse),
#                     ind_idx,
#                     one_ind.get('Gen', -1),
#                     one_ind.get('ID', -1)
#                 ))
#
#         if len(valid_items) == 0:
#             print('%s: 没有有效 RMSE。' % var)
#             continue
#
#         best_rmse, best_idx, best_gen, best_id = sorted(
#             valid_items, key=lambda x: x[0]
#         )[0]
#         print('%s: min RMSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_rmse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前一代NSGA-II候选种群中选择一套统一参数。
#
#     参与统一选参的三个目标：
#       D_1366 NSE
#       D_1367 NSE
#       D_1368 NSE
#
#     D_1370、D_1375和F_18只用于输出、绘图和独立验证。
#
#     排序规则：
#       1. 最大化三个核心水深点中的最小NSE；
#       2. 若最小NSE相同，再最大化三个核心点的平均NSE。
#
#     最终六张图使用同一个shared_best_idx。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         nse_values = []
#         candidate_valid = True
#
#         for var in OBJECTIVE_VARS:
#             nse = _safe_metric(one_ind, var, 'NSE')
#
#             if not np.isfinite(nse) or nse <= -9000.0:
#                 candidate_valid = False
#                 break
#
#             nse_values.append(float(nse))
#
#         if not candidate_valid:
#             continue
#
#         nse_values = np.asarray(nse_values, dtype=float)
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
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选种群中没有同时具备三个有效NSE的参数组。'
#         )
#
#     # NSE越大越好：
#     # 第一优先最大化三个核心点中的最差NSE；
#     # 第二优先最大化三个核心点的平均NSE。
#     candidates.sort(
#         key=lambda item: (
#             -item['min_nse'],
#             -item['mean_nse'],
#         )
#     )
#
#     best = candidates[0]
#     shared_best_idx = best['idx']
#
#     print('================================================')
#     print('统一参数选择结果：当前候选种群、三个核心点NSE目标')
#     print('1. 最大化三个核心水深点中的最小NSE')
#     print('2. 最大化三个核心水深点的平均NSE')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     print('minimum NSE = %.6f' % best['min_nse'])
#     print('mean NSE = %.6f' % best['mean_nse'])
#     print('---------------- 三个核心点NSE目标 ----------------')
#
#     for var, nse in zip(OBJECTIVE_VARS, best['nse_values']):
#         print('%s NSE = %.10f' % (var, nse))
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#     return shared_best_idx, best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_rmse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代选出的统一参数、三个核心点NSE目标及诊断指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir,
#         'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: three core water-depth NSE objectives are used '
#             'together. D_1370, D_1375 and F_18 are used only for independent '
#             'validation and plotting.\n'
#         )
#         f.write(
#             'The selected parameter set first maximizes the minimum NSE '
#             'among the three core water-depth variables and then maximizes '
#             'their mean NSE.\n\n'
#         )
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#         f.write('Shared best index: %s\n' % str(best_info['idx']))
#         f.write('Minimum NSE: %.10f\n' % best_info['min_nse'])
#         f.write('Mean NSE: %.10f\n' % best_info['mean_nse'])
#
#         f.write('\nThree core NSE calibration objectives:\n')
#         for var, nse in zip(OBJECTIVE_VARS, best_info['nse_values']):
#             f.write('%s NSE: %.12f\n' % (var, nse))
#
#         # 同时保存所选参数组的其他诊断指标。
#         f.write('\nAll-variable diagnostic metrics:\n')
#         all_vars = [
#             'D_1366',
#             'D_1367',
#             'D_1368',
#             'D_1370',
#             'D_1375',
#             'F_18',
#         ]
#         diagnostic_metrics = [
#             'NSE',
#             'NSE1',
#             'lnNSE',
#             'RMSE',
#             'RSR',
#             'PBIAS',
#             'R-square',
#         ]
#         selected_result = sim_obs_data[shared_best_idx]
#
#         for var in all_vars:
#             f.write('\n[%s]\n' % var)
#             for metric in diagnostic_metrics:
#                 metric_value = _safe_metric(selected_result, var, metric)
#                 f.write('%s: %.12f\n' % (metric, metric_value))
#
#         f.write(
#             '\nParameter values: %s\n'
#             % str(best_info['gene_values'])
#         )
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_kge = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE1')
#         best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_kge_v = np.nan
#         best_rmse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_kge_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE1')
#             best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'KGE: %s\n'
#             'RMSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (
#                 _format_metric(p_value),
#                 _format_metric(r_value),
#                 _format_metric(best_nse),
#                 _format_metric(best_kge),
#                 _format_metric(best_rmse),
#                 _format_metric(best_rsr),
#                 _format_metric(best_pbias),
#                 _format_metric(best_r2)
#             )
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'KGE: %s\n'
#                 'RMSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (
#                     _format_metric(p_value_v),
#                     _format_metric(r_value_v),
#                     _format_metric(best_nse_v),
#                     _format_metric(best_kge_v),
#                     _format_metric(best_rmse_v),
#                     _format_metric(best_rsr_v),
#                     _format_metric(best_pbias_v),
#                     _format_metric(best_r2_v)
#                 )
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     # 注意：这里要改成你实际的率定结果文件夹。
#     # 你前面发的是 Gen100 图，所以这里先写 Gen_100_Pop_12；如果实际文件夹不是这个，需要手动改。
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))






# # 六变量输出、4个水深点NSE目标NSGA-II结果后处理与统一参数选择
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积变量
# AREA_VAR = 'F_18'
#
# # 三个核心区水深点
# CORE_DEPTH_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
# ]
#
# # 两个边缘浅水点
# EDGE_DEPTH_VARS = [
#     'D_1370',
#     'D_1375',
# ]
#
# # 参与率定和最终统一选参的水深变量：三个核心点 + D_1370
# CALIB_DEPTH_VARS = CORE_DEPTH_VARS + ['D_1370']
#
# # 所有需要绘图和诊断的水深变量
# # D_1370参与率定；D_1375仅用于辅助验证和绘图。
# PLOT_DEPTH_VARS = CORE_DEPTH_VARS + EDGE_DEPTH_VARS
#
# # 与主率定程序 multiobj 完全一致的4个NSE目标：
# # D_1366、D_1367、D_1368、D_1370。
# # F_18 和 D_1375 仅用于辅助验证和绘图，不参与最终统一参数选择。
# # NSE越大越好，理想值为1。
# OBJECTIVE_VARS = CALIB_DEPTH_VARS
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = False
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
# GEN_NUM_TO_PLOT = 100
#
#
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_18', 'D_1366', 'D_1367', 'D_1368', 'D_1370', 'D_1375']
#       每一日期的值 = [F_18, D_1366, D_1367, D_1368, D_1370, D_1375]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：四个水深点的NSE共同决定共享参数
# # ============================================================================
# def diagnose_best_rmse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最小 RMSE。
#
#     RMSE 越小越好。该函数仅用于诊断，不直接改变最终统一参数。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最小 RMSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             rmse = _safe_metric(one_ind, var, 'RMSE')
#             if np.isfinite(rmse) and rmse >= 0.0:
#                 valid_items.append((
#                     float(rmse),
#                     ind_idx,
#                     one_ind.get('Gen', -1),
#                     one_ind.get('ID', -1)
#                 ))
#
#         if len(valid_items) == 0:
#             print('%s: 没有有效 RMSE。' % var)
#             continue
#
#         best_rmse, best_idx, best_gen, best_id = sorted(
#             valid_items, key=lambda x: x[0]
#         )[0]
#         print('%s: min RMSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_rmse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前一代NSGA-II候选种群中选择一套统一参数。
#
#     参与统一选参的四个目标：
#       D_1366 NSE
#       D_1367 NSE
#       D_1368 NSE
#       D_1370 NSE
#
#     F_18和D_1375只用于输出、绘图和独立验证，
#     不参与统一参数选择。
#
#     排序规则：
#       1. 最大化四个水深点中的最小NSE；
#       2. 若最小NSE相同，再最大化四个水深点的平均NSE。
#
#     最终六张图仍然使用同一个shared_best_idx。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         nse_values = []
#         candidate_valid = True
#
#         for var in OBJECTIVE_VARS:
#             nse = _safe_metric(one_ind, var, 'NSE')
#
#             if not np.isfinite(nse) or nse <= -9000.0:
#                 candidate_valid = False
#                 break
#
#             nse_values.append(float(nse))
#
#         if not candidate_valid:
#             continue
#
#         nse_values = np.asarray(nse_values, dtype=float)
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
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选种群中没有同时具备四个有效水深NSE的参数组。'
#         )
#
#     # NSE越大越好：
#     # 第一优先最大化四个水深点中的最差NSE；
#     # 第二优先最大化四个水深点的平均NSE。
#     candidates.sort(
#         key=lambda item: (
#             -item['min_nse'],
#             -item['mean_nse'],
#         )
#     )
#
#     best = candidates[0]
#     shared_best_idx = best['idx']
#
#     print('================================================')
#     print('统一参数选择结果：当前候选种群、四个水深点NSE目标')
#     print('1. 最大化四个水深点NSE中的最小NSE')
#     print('2. 最大化四个水深点NSE的平均NSE')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     print('minimum NSE = %.6f' % best['min_nse'])
#     print('mean NSE = %.6f' % best['mean_nse'])
#     print('---------------- 四个NSE率定目标 ----------------')
#
#     for var, nse in zip(OBJECTIVE_VARS, best['nse_values']):
#         print('%s NSE = %.10f' % (var, nse))
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#     return shared_best_idx, best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_rmse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代选出的统一参数、四个水深点NSE目标及诊断指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir,
#         'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: four water-depth NSE objectives '
#             '(D_1366, D_1367, D_1368, D_1370) are used for '
#             'shared-parameter selection. F_18 and D_1375 are used only '
#             'for independent validation and plotting.\n'
#         )
#         f.write(
#              'The selected parameter set first maximizes the minimum NSE '
#              'among the four water-depth objectives and then maximizes '
#              'their mean NSE.\n\n'
#         )
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#         f.write('Shared best index: %s\n' % str(best_info['idx']))
#         f.write('Minimum NSE: %.10f\n' % best_info['min_nse'])
#         f.write('Mean NSE: %.10f\n' % best_info['mean_nse'])
#
#         f.write('\nFour NSE calibration objectives:\n')
#         for var, nse in zip(OBJECTIVE_VARS, best_info['nse_values']):
#             f.write('%s NSE: %.12f\n' % (var, nse))
#
#         # 同时保存所选参数组的其他诊断指标。
#         f.write('\nAll-variable diagnostic metrics:\n')
#         all_vars = [
#             'D_1366',
#             'D_1367',
#             'D_1368',
#             'D_1370',
#             'D_1375',
#             'F_18',
#         ]
#         diagnostic_metrics = [
#             'NSE',
#             'NSE1',
#             'lnNSE',
#             'RMSE',
#             'RSR',
#             'PBIAS',
#             'R-square',
#         ]
#         selected_result = sim_obs_data[shared_best_idx]
#
#         for var in all_vars:
#             f.write('\n[%s]\n' % var)
#             for metric in diagnostic_metrics:
#                 metric_value = _safe_metric(selected_result, var, metric)
#                 f.write('%s: %.12f\n' % (metric, metric_value))
#
#         f.write(
#             '\nParameter values: %s\n'
#             % str(best_info['gene_values'])
#         )
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         # 日期必须与dict_to_var_matrix()中的模拟值采用相同的排序顺序。
#
#         cali_sim_date_keys, _ = dict_to_var_matrix(
#             sim_data[0],
#             nvar
#         )
#         cali_sim_dates = _to_datetime_list(
#             cali_sim_date_keys
#         )
#
#         # 检查是否有重复日期
#         if len(cali_sim_dates) != len(set(cali_sim_dates)):
#             raise ValueError('率定期模拟日期中存在重复日期。')
#
#         # 检查日期是否严格升序
#         if any(
#             cali_sim_dates[i] > cali_sim_dates[i + 1]
#             for i in range(len(cali_sim_dates) - 1)
#         ):
#             raise ValueError('率定期模拟日期没有按照时间升序排列。')
#
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#
#         if plot_validation:
#             vali_sim_date_keys, _ = dict_to_var_matrix(
#                 vali_sim_data[0],
#                 nvar
#             )
#             vali_sim_dates = _to_datetime_list(
#                 vali_sim_date_keys
#             )
#
#             if len(vali_sim_dates) != len(set(vali_sim_dates)):
#                 raise ValueError('验证期模拟日期中存在重复日期。')
#
#             if any(
#                 vali_sim_dates[i] > vali_sim_dates[i + 1]
#                 for i in range(len(vali_sim_dates) - 1)
#             ):
#                 raise ValueError('验证期模拟日期没有按照时间升序排列。')
#
#             if not same_cali_vali_period:
#                 if order:
#                     sim_dates = sim_dates + vali_sim_dates
#                 else:
#                     sim_dates = vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_kge = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE1')
#         best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_kge_v = np.nan
#         best_rmse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_kge_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE1')
#             best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'KGE: %s\n'
#             'RMSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (
#                 _format_metric(p_value),
#                 _format_metric(r_value),
#                 _format_metric(best_nse),
#                 _format_metric(best_kge),
#                 _format_metric(best_rmse),
#                 _format_metric(best_rsr),
#                 _format_metric(best_pbias),
#                 _format_metric(best_r2)
#             )
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'KGE: %s\n'
#                 'RMSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (
#                     _format_metric(p_value_v),
#                     _format_metric(r_value_v),
#                     _format_metric(best_nse_v),
#                     _format_metric(best_kge_v),
#                     _format_metric(best_rmse_v),
#                     _format_metric(best_rsr_v),
#                     _format_metric(best_pbias_v),
#                     _format_metric(best_r2_v)
#                 )
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     # 注意：这里要改成你实际的率定结果文件夹。
#     # 必须改成“4个水深点NSE”新一轮率定的实际结果文件夹。
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
#     # wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))





# # 六变量输出、3个水深点NSE目标NSGA-II结果后处理与统一参数选择  20260819
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积变量
# AREA_VAR = 'F_18'
#
# # 三个核心区水深点
# CORE_DEPTH_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
# ]
#
# # 两个边缘浅水点
# EDGE_DEPTH_VARS = [
#     'D_1370',
#     'D_1375',
# ]
#
# # 参与率定和最终统一选参的水深变量：三个核心点
# CALIB_DEPTH_VARS = CORE_DEPTH_VARS
#
# # 所有需要绘图和诊断的水深变量
# # D_1370和D_1375仅用于辅助验证和绘图。
# PLOT_DEPTH_VARS = CORE_DEPTH_VARS + EDGE_DEPTH_VARS
#
# # 与主率定程序 multiobj 完全一致的3个NSE目标：
# # D_1366、D_1367、D_1368。
# # D_1370、F_18 和 D_1375 仅用于辅助验证和绘图，不参与最终统一参数选择。
# # NSE越大越好，理想值为1。
# OBJECTIVE_VARS = CALIB_DEPTH_VARS
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = False
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
# GEN_NUM_TO_PLOT = 100
#
#
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_18', 'D_1366', 'D_1367', 'D_1368', 'D_1370', 'D_1375']
#       每一日期的值 = [F_18, D_1366, D_1367, D_1368, D_1370, D_1375]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：三个水深点的NSE共同决定共享参数
# # ============================================================================
# def diagnose_best_rmse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最小 RMSE。
#
#     RMSE 越小越好。该函数仅用于诊断，不直接改变最终统一参数。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最小 RMSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             rmse = _safe_metric(one_ind, var, 'RMSE')
#             if np.isfinite(rmse) and rmse >= 0.0:
#                 valid_items.append((
#                     float(rmse),
#                     ind_idx,
#                     one_ind.get('Gen', -1),
#                     one_ind.get('ID', -1)
#                 ))
#
#         if len(valid_items) == 0:
#             print('%s: 没有有效 RMSE。' % var)
#             continue
#
#         best_rmse, best_idx, best_gen, best_id = sorted(
#             valid_items, key=lambda x: x[0]
#         )[0]
#         print('%s: min RMSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_rmse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前一代NSGA-II候选种群中选择一套统一参数。
#
#     参与统一选参的三个目标：
#       D_1366 NSE
#       D_1367 NSE
#       D_1368 NSE
#
#     D_1370、F_18和D_1375只用于输出、绘图和独立验证，
#     不参与统一参数选择。
#
#     排序规则：
#       1. 最大化三个水深点中的最小NSE；
#       2. 若最小NSE相同，再最大化三个水深点的平均NSE。
#
#     最终六张图仍然使用同一个shared_best_idx。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         nse_values = []
#         candidate_valid = True
#
#         for var in OBJECTIVE_VARS:
#             nse = _safe_metric(one_ind, var, 'NSE')
#
#             if not np.isfinite(nse) or nse <= -9000.0:
#                 candidate_valid = False
#                 break
#
#             nse_values.append(float(nse))
#
#         if not candidate_valid:
#             continue
#
#         nse_values = np.asarray(nse_values, dtype=float)
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
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选种群中没有同时具备三个有效水深NSE的参数组。'
#         )
#
#     # NSE越大越好：
#     # 第一优先最大化三个水深点中的最差NSE；
#     # 第二优先最大化三个水深点的平均NSE。
#     candidates.sort(
#         key=lambda item: (
#             -item['min_nse'],
#             -item['mean_nse'],
#         )
#     )
#
#     best = candidates[0]
#     shared_best_idx = best['idx']
#
#     print('================================================')
#     print('统一参数选择结果：当前候选种群、三个水深点NSE目标')
#     print('1. 最大化三个水深点NSE中的最小NSE')
#     print('2. 最大化三个水深点NSE的平均NSE')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     print('minimum NSE = %.6f' % best['min_nse'])
#     print('mean NSE = %.6f' % best['mean_nse'])
#     print('---------------- 三个NSE率定目标 ----------------')
#
#     for var, nse in zip(OBJECTIVE_VARS, best['nse_values']):
#         print('%s NSE = %.10f' % (var, nse))
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#     return shared_best_idx, best
#
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_rmse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代选出的统一参数、三个水深点NSE目标及诊断指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir,
#         'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: three water-depth NSE objectives '
#             '(D_1366, D_1367, D_1368) are used for '
#             'shared-parameter selection. D_1370, F_18 and D_1375 are used only '
#             'for independent validation and plotting.\n'
#         )
#         f.write(
#              'The selected parameter set first maximizes the minimum NSE '
#              'among the three water-depth objectives and then maximizes '
#              'their mean NSE.\n\n'
#         )
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#         f.write('Shared best index: %s\n' % str(best_info['idx']))
#         f.write('Minimum NSE: %.10f\n' % best_info['min_nse'])
#         f.write('Mean NSE: %.10f\n' % best_info['mean_nse'])
#
#         f.write('\nThree NSE calibration objectives:\n')
#         for var, nse in zip(OBJECTIVE_VARS, best_info['nse_values']):
#             f.write('%s NSE: %.12f\n' % (var, nse))
#
#         # 同时保存所选参数组的其他诊断指标。
#         f.write('\nAll-variable diagnostic metrics:\n')
#         all_vars = [
#             'D_1366',
#             'D_1367',
#             'D_1368',
#             'D_1370',
#             'D_1375',
#             'F_18',
#         ]
#         diagnostic_metrics = [
#             'NSE',
#             'NSE1',
#             'lnNSE',
#             'RMSE',
#             'RSR',
#             'PBIAS',
#             'R-square',
#         ]
#         selected_result = sim_obs_data[shared_best_idx]
#
#         for var in all_vars:
#             f.write('\n[%s]\n' % var)
#             for metric in diagnostic_metrics:
#                 metric_value = _safe_metric(selected_result, var, metric)
#                 f.write('%s: %.12f\n' % (metric, metric_value))
#
#         f.write(
#             '\nParameter values: %s\n'
#             % str(best_info['gene_values'])
#         )
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         # 日期必须与dict_to_var_matrix()中的模拟值采用相同的排序顺序。
#
#         cali_sim_date_keys, _ = dict_to_var_matrix(
#             sim_data[0],
#             nvar
#         )
#         cali_sim_dates = _to_datetime_list(
#             cali_sim_date_keys
#         )
#
#         # 检查是否有重复日期
#         if len(cali_sim_dates) != len(set(cali_sim_dates)):
#             raise ValueError('率定期模拟日期中存在重复日期。')
#
#         # 检查日期是否严格升序
#         if any(
#             cali_sim_dates[i] > cali_sim_dates[i + 1]
#             for i in range(len(cali_sim_dates) - 1)
#         ):
#             raise ValueError('率定期模拟日期没有按照时间升序排列。')
#
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#
#         if plot_validation:
#             vali_sim_date_keys, _ = dict_to_var_matrix(
#                 vali_sim_data[0],
#                 nvar
#             )
#             vali_sim_dates = _to_datetime_list(
#                 vali_sim_date_keys
#             )
#
#             if len(vali_sim_dates) != len(set(vali_sim_dates)):
#                 raise ValueError('验证期模拟日期中存在重复日期。')
#
#             if any(
#                 vali_sim_dates[i] > vali_sim_dates[i + 1]
#                 for i in range(len(vali_sim_dates) - 1)
#             ):
#                 raise ValueError('验证期模拟日期没有按照时间升序排列。')
#
#             if not same_cali_vali_period:
#                 if order:
#                     sim_dates = sim_dates + vali_sim_dates
#                 else:
#                     sim_dates = vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_kge = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE1')
#         best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_kge_v = np.nan
#         best_rmse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_kge_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE1')
#             best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#
#         # 95PPU：完整连续模拟日期
#         ax.fill_between(
#             sim_dates_plot,
#             ylows_plot.tolist(),
#             yups_plot.tolist(),
#             color=(0.8, 0.8, 0.8),
#             label='95PPU',
#             zorder=1
#         )
#
#         # 统一参数完整连续模拟曲线
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(
#             sim_dates_plot,
#             sim_best_plot,
#             linestyle='--',
#             color='red',
#             linewidth=1.5,
#             label=bestsim_label,
#             zorder=3
#         )
#
#         # 实测值
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         valid_obs_mask = np.isfinite(obs_data)
#
#         ax.scatter(
#             obs_dates,
#             obs_data,
#             marker='.',
#             s=20,
#             color='g',
#             label=observed_label,
#             zorder=4
#         )
#
#         # 横轴同时考虑模拟日期和实测日期，防止首尾实测点被裁掉
#         all_plot_dates = list(sim_dates_plot) + list(obs_dates[valid_obs_mask])
#
#         if len(all_plot_dates) > 0:
#             ax.set_xlim(
#                 left=min(all_plot_dates),
#                 right=max(all_plot_dates)
#             )
#
#         # 纵轴必须使用真正最大值，而不是99%分位数，
#         # 保证所有实测点、模拟值和95PPU都能够显示
#         y_parts = []
#
#         for values in [
#             obs_data[valid_obs_mask],
#             sim_best_plot,
#             ylows_plot,
#             yups_plot
#         ]:
#             values = np.asarray(values, dtype=float)
#             valid_values = values[np.isfinite(values)]
#
#             if len(valid_values) > 0:
#                 y_parts.append(valid_values)
#
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#
#             y_high = np.nanmax(all_y)
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#
#             # 下边界稍微低于0，
#             # 这样 Obs=0 的绿色实测点不会被横轴遮住
#             ax.set_ylim(
#                 bottom=0.0,
#                 top=1.08 * y_high
#             )
#         else:
#             ax.set_ylim(
#                 bottom=0.0,
#                 top=0.01
#             )
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'KGE: %s\n'
#             'RMSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (
#                 _format_metric(p_value),
#                 _format_metric(r_value),
#                 _format_metric(best_nse),
#                 _format_metric(best_kge),
#                 _format_metric(best_rmse),
#                 _format_metric(best_rsr),
#                 _format_metric(best_pbias),
#                 _format_metric(best_r2)
#             )
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'KGE: %s\n'
#                 'RMSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (
#                     _format_metric(p_value_v),
#                     _format_metric(r_value_v),
#                     _format_metric(best_nse_v),
#                     _format_metric(best_kge_v),
#                     _format_metric(best_rmse_v),
#                     _format_metric(best_rsr_v),
#                     _format_metric(best_pbias_v),
#                     _format_metric(best_r2_v)
#                 )
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     # 注意：这里要改成你实际的率定结果文件夹。
#     # 必须改成“3个水深点NSE”新一轮率定的实际结果文件夹。
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
#     # wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))








# # 六变量输出、3个核心点 minNSE + meanNSE + dryRMSE 结果后处理与统一参数选择
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积变量
# AREA_VAR = 'F_18'
#
# # 三个核心区水深点
# CORE_DEPTH_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
# ]
#
# # 两个边缘浅水点
# EDGE_DEPTH_VARS = [
#     'D_1370',
#     'D_1375',
# ]
#
# # 参与率定和最终统一选参的水深变量：三个核心点
# CALIB_DEPTH_VARS = CORE_DEPTH_VARS
#
# # 所有需要绘图和诊断的水深变量
# # D_1370和D_1375仅用于辅助验证和绘图。
# PLOT_DEPTH_VARS = CORE_DEPTH_VARS + EDGE_DEPTH_VARS
#
# # 三个核心点，用于构造：
# # 1. minNSE   —— 最大化
# # 2. meanNSE  —— 最大化
# # 3. dryRMSE  —— 最小化
# #
# # D_1370、F_18、D_1375仅用于辅助验证和绘图。
# OBJECTIVE_VARS = CALIB_DEPTH_VARS
#
# # 与主率定程序保持一致：
# # 只有“实测严格为0（考虑浮点容差）”的日期进入dryRMSE。
# DRY_OBS_TOL = 1.0e-6
#
# # 主程序写入gen*_caliObsData.json的三个fitness字段。
# FITNESS_MIN_NSE_KEY = 'Fitness_minNSE'
# FITNESS_MEAN_NSE_KEY = 'Fitness_meanNSE'
# FITNESS_DRY_RMSE_KEY = 'Fitness_dryRMSE'
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = False
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
# GEN_NUM_TO_PLOT = 100
#
#
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_18', 'D_1366', 'D_1367', 'D_1368', 'D_1370', 'D_1375']
#       每一日期的值 = [F_18, D_1366, D_1367, D_1368, D_1370, D_1375]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：minNSE + meanNSE + dryRMSE
# # ============================================================================
# def _calculate_dry_rmse_from_saved_obs_sim(one_ind):
#     """
#     当旧结果文件中没有Fitness_dryRMSE时，按主率定程序同样的规则重新计算。
#
#     干态定义：
#         abs(Obs) <= DRY_OBS_TOL
#
#     三个核心点D_1366、D_1367、D_1368的所有干态观测合并后计算：
#         dryRMSE = sqrt(mean((Sim - Obs)^2))
#
#     返回：
#         dry_rmse, dry_count, dry_count_by_var
#     """
#     squared_errors = []
#     dry_count_by_var = {}
#
#     for var in OBJECTIVE_VARS:
#         if var not in one_ind:
#             dry_count_by_var[var] = 0
#             raise ValueError('结果中缺少干态变量：%s' % var)
#
#         one_var = one_ind[var]
#         obs = _safe_array(one_var.get('Obs', []))
#         sim = _safe_array(one_var.get('Sim', []))
#
#         n = min(len(obs), len(sim))
#         if n <= 0:
#             dry_count_by_var[var] = 0
#             continue
#
#         obs = obs[:n]
#         sim = sim[:n]
#
#         valid = np.isfinite(obs) & np.isfinite(sim)
#         dry_mask = (
#             valid
#             & (np.abs(obs) <= DRY_OBS_TOL)
#             & (obs >= -DRY_OBS_TOL)
#         )
#
#         count = int(np.sum(dry_mask))
#         dry_count_by_var[var] = count
#
#         if count > 0:
#             err = sim[dry_mask] - obs[dry_mask]
#             squared_errors.extend(np.square(err).astype(float).tolist())
#
#     dry_count = len(squared_errors)
#     if dry_count <= 0:
#         raise ValueError(
#             'D_1366、D_1367、D_1368中没有找到实测为0的干态观测；'
#             'DRY_OBS_TOL=%g m。' % DRY_OBS_TOL
#         )
#
#     dry_rmse = float(np.sqrt(np.mean(np.asarray(squared_errors, dtype=float))))
#     return dry_rmse, dry_count, dry_count_by_var
#
#
# def _read_three_fitness(one_ind):
#     """
#     读取与率定主程序完全一致的三个fitness。
#
#     优先读取主程序已经保存的：
#         Fitness_minNSE
#         Fitness_meanNSE
#         Fitness_dryRMSE
#
#     如果缺失，则根据当前JSON中的三个NSE和Obs/Sim重新计算。
#     """
#     nse_values = np.asarray(
#         [_safe_metric(one_ind, var, 'NSE') for var in OBJECTIVE_VARS],
#         dtype=float
#     )
#
#     if not np.isfinite(nse_values).all():
#         raise ValueError('三个核心点NSE存在NaN/Inf。')
#     if np.any(nse_values <= -9000.0) or np.any(nse_values > 1.000001):
#         raise ValueError('三个核心点NSE存在无效值：%s' % str(nse_values))
#
#     min_nse_saved = _safe_float(one_ind.get(FITNESS_MIN_NSE_KEY, np.nan))
#     mean_nse_saved = _safe_float(one_ind.get(FITNESS_MEAN_NSE_KEY, np.nan))
#     dry_rmse_saved = _safe_float(one_ind.get(FITNESS_DRY_RMSE_KEY, np.nan))
#
#     min_nse = (
#         float(min_nse_saved)
#         if np.isfinite(min_nse_saved)
#         else float(np.min(nse_values))
#     )
#     mean_nse = (
#         float(mean_nse_saved)
#         if np.isfinite(mean_nse_saved)
#         else float(np.mean(nse_values))
#     )
#
#     dry_count = one_ind.get('DryObsCount', None)
#     dry_count_by_var = one_ind.get('DryObsCountByVar', None)
#
#     if np.isfinite(dry_rmse_saved) and dry_rmse_saved >= 0.0:
#         dry_rmse = float(dry_rmse_saved)
#     else:
#         dry_rmse, dry_count_calc, dry_count_by_var_calc = \
#             _calculate_dry_rmse_from_saved_obs_sim(one_ind)
#
#         if dry_count is None:
#             dry_count = dry_count_calc
#         if dry_count_by_var is None:
#             dry_count_by_var = dry_count_by_var_calc
#
#     return {
#         'nse_values': nse_values,
#         'min_nse': min_nse,
#         'mean_nse': mean_nse,
#         'dry_rmse': dry_rmse,
#         'dry_count': dry_count,
#         'dry_count_by_var': dry_count_by_var,
#     }
#
#
# def diagnose_three_fitness(sim_obs_data):
#     """输出当前候选种群三个真实率定目标的范围，便于检查率定是否在改善。"""
#     values = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         try:
#             fit = _read_three_fitness(one_ind)
#         except Exception:
#             continue
#
#         values.append((
#             fit['min_nse'],
#             fit['mean_nse'],
#             fit['dry_rmse'],
#             ind_idx,
#             one_ind.get('Gen', -1),
#             one_ind.get('ID', -1),
#         ))
#
#     if len(values) == 0:
#         print('当前候选种群没有可诊断的三目标参数组。')
#         return
#
#     min_nse_best = max(values, key=lambda x: x[0])
#     mean_nse_best = max(values, key=lambda x: x[1])
#     dry_best = min(values, key=lambda x: x[2])
#
#     print('================================================')
#     print('当前候选种群三个真实率定目标的单目标最好值：')
#     print('max minNSE = %.6f, idx=%d, Gen=%s, ID=%s'
#           % (min_nse_best[0], min_nse_best[3],
#              str(min_nse_best[4]), str(min_nse_best[5])))
#     print('max meanNSE = %.6f, idx=%d, Gen=%s, ID=%s'
#           % (mean_nse_best[1], mean_nse_best[3],
#              str(mean_nse_best[4]), str(mean_nse_best[5])))
#     print('min dryRMSE = %.6f m, idx=%d, Gen=%s, ID=%s'
#           % (dry_best[2], dry_best[3],
#              str(dry_best[4]), str(dry_best[5])))
#     print('================================================')
#
#
# def _normalize_benefit(values):
#     """
#     将“越大越好”的目标归一化到[0,1]。
#     1表示当前候选集中最好。
#     """
#     values = np.asarray(values, dtype=float)
#     vmin = float(np.min(values))
#     vmax = float(np.max(values))
#
#     if abs(vmax - vmin) <= 1.0e-12:
#         return np.ones(values.shape, dtype=float)
#
#     return (values - vmin) / (vmax - vmin)
#
#
# def _normalize_cost(values):
#     """
#     将“越小越好”的目标归一化到[0,1]。
#     1表示当前候选集中最好。
#     """
#     values = np.asarray(values, dtype=float)
#     vmin = float(np.min(values))
#     vmax = float(np.max(values))
#
#     if abs(vmax - vmin) <= 1.0e-12:
#         return np.ones(values.shape, dtype=float)
#
#     return (vmax - values) / (vmax - vmin)
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前一代NSGA-II候选种群中选择一套统一参数供六个变量共同绘图。
#
#     率定主程序的三个真实目标：
#         1. minNSE   最大化
#         2. meanNSE  最大化
#         3. dryRMSE  最小化
#
#     最终统一参数不能再只按NSE排序，否则dryRMSE几乎不起作用。
#     这里采用“三目标归一化后距理想点最近”的折中解：
#
#         ideal = (1, 1, 1)
#
#     其中：
#         minNSE、meanNSE：越大归一化值越接近1
#         dryRMSE：越小归一化值越接近1
#
#     三个目标等权。这样三个目标都会真正影响最终共享参数选择，
#     同时避免NSE和dryRMSE量纲不同造成直接相加不合理。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         try:
#             fit = _read_three_fitness(one_ind)
#         except Exception as e:
#             print(
#                 '跳过无效候选参数组 idx=%d, Gen=%s, ID=%s，原因：%s'
#                 % (
#                     ind_idx,
#                     str(one_ind.get('Gen', -1)),
#                     str(one_ind.get('ID', -1)),
#                     str(e)
#                 )
#             )
#             continue
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#             'nse_values': fit['nse_values'],
#             'min_nse': fit['min_nse'],
#             'mean_nse': fit['mean_nse'],
#             'dry_rmse': fit['dry_rmse'],
#             'dry_count': fit['dry_count'],
#             'dry_count_by_var': fit['dry_count_by_var'],
#         })
#
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选种群中没有同时具备minNSE、meanNSE和dryRMSE的有效参数组。'
#         )
#
#     min_nse_array = np.asarray([c['min_nse'] for c in candidates], dtype=float)
#     mean_nse_array = np.asarray([c['mean_nse'] for c in candidates], dtype=float)
#     dry_rmse_array = np.asarray([c['dry_rmse'] for c in candidates], dtype=float)
#
#     min_nse_good = _normalize_benefit(min_nse_array)
#     mean_nse_good = _normalize_benefit(mean_nse_array)
#     dry_rmse_good = _normalize_cost(dry_rmse_array)
#
#     for i, c in enumerate(candidates):
#         c['norm_min_nse'] = float(min_nse_good[i])
#         c['norm_mean_nse'] = float(mean_nse_good[i])
#         c['norm_dry_rmse'] = float(dry_rmse_good[i])
#
#         # 到三目标理想点(1,1,1)的欧氏距离，越小越好。
#         c['ideal_distance'] = float(np.sqrt(
#             (1.0 - c['norm_min_nse']) ** 2
#             + (1.0 - c['norm_mean_nse']) ** 2
#             + (1.0 - c['norm_dry_rmse']) ** 2
#         ))
#
#     # 第一优先：三目标折中距离最小；
#     # 若极少数情况下完全相同，则依次偏向：
#     # minNSE更高 -> dryRMSE更小 -> meanNSE更高。
#     candidates.sort(
#         key=lambda c: (
#             c['ideal_distance'],
#             -c['min_nse'],
#             c['dry_rmse'],
#             -c['mean_nse'],
#         )
#     )
#
#     best = candidates[0]
#     shared_best_idx = best['idx']
#
#     print('================================================')
#     print('统一参数选择结果：minNSE + meanNSE + dryRMSE 三目标折中')
#     print('选择方法：三个目标归一化后，选择距离理想点最近的参数组')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#     print('minNSE = %.6f' % best['min_nse'])
#     print('meanNSE = %.6f' % best['mean_nse'])
#     print('dryRMSE = %.6f m' % best['dry_rmse'])
#     print('DryObsCount =', best['dry_count'])
#     print('DryObsCountByVar =', best['dry_count_by_var'])
#     print('normalized minNSE = %.6f' % best['norm_min_nse'])
#     print('normalized meanNSE = %.6f' % best['norm_mean_nse'])
#     print('normalized dryRMSE = %.6f' % best['norm_dry_rmse'])
#     print('ideal-point distance = %.6f' % best['ideal_distance'])
#     print('---------------- 三个核心点NSE ----------------')
#
#     for var, nse in zip(OBJECTIVE_VARS, best['nse_values']):
#         print('%s NSE = %.10f' % (var, nse))
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#
#     return shared_best_idx, best
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_three_fitness(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代选出的统一参数、三个真实率定目标及诊断指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir,
#         'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: minNSE + meanNSE + dryRMSE. '
#             'D_1366, D_1367 and D_1368 are the calibration depth variables. '
#             'D_1370, D_1375 and F_18 are used only for independent validation '
#             'and plotting.\n'
#         )
#         f.write(
#             'The final shared parameter set is selected by equal-weight '
#             'normalized distance to the three-objective ideal point: '
#             'maximize minNSE, maximize meanNSE, minimize dryRMSE.\n\n'
#         )
#
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#         f.write('Shared best index: %s\n' % str(best_info['idx']))
#         f.write('Minimum NSE: %.10f\n' % best_info['min_nse'])
#         f.write('Mean NSE: %.10f\n' % best_info['mean_nse'])
#         f.write('Dry RMSE: %.10f m\n' % best_info['dry_rmse'])
#         f.write('Dry observation count: %s\n' % str(best_info['dry_count']))
#         f.write('Dry observation count by variable: %s\n'
#                 % str(best_info['dry_count_by_var']))
#         f.write('Normalized minNSE: %.10f\n' % best_info['norm_min_nse'])
#         f.write('Normalized meanNSE: %.10f\n' % best_info['norm_mean_nse'])
#         f.write('Normalized dryRMSE: %.10f\n' % best_info['norm_dry_rmse'])
#         f.write('Ideal-point distance: %.10f\n' % best_info['ideal_distance'])
#
#         f.write('\nThree core-depth NSE values:\n')
#         for var, nse in zip(OBJECTIVE_VARS, best_info['nse_values']):
#             f.write('%s NSE: %.12f\n' % (var, nse))
#
#         # 同时保存所选参数组的其他诊断指标。
#         f.write('\nAll-variable diagnostic metrics:\n')
#         all_vars = [
#             'D_1366',
#             'D_1367',
#             'D_1368',
#             'D_1370',
#             'D_1375',
#             'F_18',
#         ]
#         diagnostic_metrics = [
#             'NSE',
#             'NSE1',
#             'lnNSE',
#             'RMSE',
#             'RSR',
#             'PBIAS',
#             'R-square',
#         ]
#         selected_result = sim_obs_data[shared_best_idx]
#
#         for var in all_vars:
#             f.write('\n[%s]\n' % var)
#             for metric in diagnostic_metrics:
#                 metric_value = _safe_metric(selected_result, var, metric)
#                 f.write('%s: %.12f\n' % (metric, metric_value))
#
#         f.write(
#             '\nParameter values: %s\n'
#             % str(best_info['gene_values'])
#         )
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         # 日期必须与dict_to_var_matrix()中的模拟值采用相同的排序顺序。
#
#         cali_sim_date_keys, _ = dict_to_var_matrix(
#             sim_data[0],
#             nvar
#         )
#         cali_sim_dates = _to_datetime_list(
#             cali_sim_date_keys
#         )
#
#         # 检查是否有重复日期
#         if len(cali_sim_dates) != len(set(cali_sim_dates)):
#             raise ValueError('率定期模拟日期中存在重复日期。')
#
#         # 检查日期是否严格升序
#         if any(
#             cali_sim_dates[i] > cali_sim_dates[i + 1]
#             for i in range(len(cali_sim_dates) - 1)
#         ):
#             raise ValueError('率定期模拟日期没有按照时间升序排列。')
#
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#
#         if plot_validation:
#             vali_sim_date_keys, _ = dict_to_var_matrix(
#                 vali_sim_data[0],
#                 nvar
#             )
#             vali_sim_dates = _to_datetime_list(
#                 vali_sim_date_keys
#             )
#
#             if len(vali_sim_dates) != len(set(vali_sim_dates)):
#                 raise ValueError('验证期模拟日期中存在重复日期。')
#
#             if any(
#                 vali_sim_dates[i] > vali_sim_dates[i + 1]
#                 for i in range(len(vali_sim_dates) - 1)
#             ):
#                 raise ValueError('验证期模拟日期没有按照时间升序排列。')
#
#             if not same_cali_vali_period:
#                 if order:
#                     sim_dates = sim_dates + vali_sim_dates
#                 else:
#                     sim_dates = vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_kge = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE1')
#         best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_kge_v = np.nan
#         best_rmse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_kge_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE1')
#             best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#
#         # 95PPU：完整连续模拟日期
#         ax.fill_between(
#             sim_dates_plot,
#             ylows_plot.tolist(),
#             yups_plot.tolist(),
#             color=(0.8, 0.8, 0.8),
#             label='95PPU',
#             zorder=1
#         )
#
#         # 统一参数完整连续模拟曲线
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(
#             sim_dates_plot,
#             sim_best_plot,
#             linestyle='--',
#             color='red',
#             linewidth=1.5,
#             label=bestsim_label,
#             zorder=3
#         )
#
#         # 实测值
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         valid_obs_mask = np.isfinite(obs_data)
#
#         ax.scatter(
#             obs_dates,
#             obs_data,
#             marker='.',
#             s=20,
#             color='g',
#             label=observed_label,
#             zorder=4
#         )
#
#         # 横轴同时考虑模拟日期和实测日期，防止首尾实测点被裁掉
#         all_plot_dates = list(sim_dates_plot) + list(obs_dates[valid_obs_mask])
#
#         if len(all_plot_dates) > 0:
#             ax.set_xlim(
#                 left=min(all_plot_dates),
#                 right=max(all_plot_dates)
#             )
#
#         # 纵轴必须使用真正最大值，而不是99%分位数，
#         # 保证所有实测点、模拟值和95PPU都能够显示
#         y_parts = []
#
#         for values in [
#             obs_data[valid_obs_mask],
#             sim_best_plot,
#             ylows_plot,
#             yups_plot
#         ]:
#             values = np.asarray(values, dtype=float)
#             valid_values = values[np.isfinite(values)]
#
#             if len(valid_values) > 0:
#                 y_parts.append(valid_values)
#
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#
#             y_high = np.nanmax(all_y)
#
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#
#             # 下边界稍微低于0，
#             # 这样 Obs=0 的绿色实测点不会被横轴遮住
#             ax.set_ylim(
#                 bottom=0.0,
#                 top=1.08 * y_high
#             )
#         else:
#             ax.set_ylim(
#                 bottom=0.0,
#                 top=0.01
#             )
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'KGE: %s\n'
#             'RMSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (
#                 _format_metric(p_value),
#                 _format_metric(r_value),
#                 _format_metric(best_nse),
#                 _format_metric(best_kge),
#                 _format_metric(best_rmse),
#                 _format_metric(best_rsr),
#                 _format_metric(best_pbias),
#                 _format_metric(best_r2)
#             )
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'KGE: %s\n'
#                 'RMSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (
#                     _format_metric(p_value_v),
#                     _format_metric(r_value_v),
#                     _format_metric(best_nse_v),
#                     _format_metric(best_kge_v),
#                     _format_metric(best_rmse_v),
#                     _format_metric(best_rsr_v),
#                     _format_metric(best_pbias_v),
#                     _format_metric(best_r2_v)
#                 )
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     # 注意：这里要改成你实际的率定结果文件夹。
#     # 必须改成“3个水深点NSE”新一轮率定的实际结果文件夹。
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
#     # wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))








# 六变量输出、3个核心点 maxRMSE + meanRMSE 结果后处理与统一参数选择
from __future__ import absolute_import, unicode_literals

import json
import os
import pickle
import sys
from io import open
from typing import Optional

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

import matplotlib as mpl
import numpy
import numpy as np

if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
    mpl.use('Agg', warn=False)

import matplotlib.dates as mdates
import matplotlib.pyplot as plt

from pygeoc.utils import StringClass, is_string
from utility import save_png_eps, PlotConfig
import global_mongoclient as MongoDBObj
from parameters_sensitivity.sensitivity import SpecialJsonEncoder


# ============================================================================
# 用户可调整设置
# ============================================================================
# 淹没面积变量
AREA_VAR = 'F_18'

# 三个核心区水深点
CORE_DEPTH_VARS = [
    'D_1366',
    'D_1367',
    'D_1368',
]

# 两个边缘浅水点
EDGE_DEPTH_VARS = [
    'D_1370',
    'D_1375',
]

# 参与率定和最终统一选参的水深变量：三个核心点
CALIB_DEPTH_VARS = CORE_DEPTH_VARS

# 所有需要绘图和诊断的水深变量
# D_1370和D_1375仅用于辅助验证和绘图。
PLOT_DEPTH_VARS = CORE_DEPTH_VARS + EDGE_DEPTH_VARS

# 三个核心点，用于构造：
# 1. maxDryAwareRMSE  —— 最小化
# 2. meanDryAwareRMSE —— 最小化
#
# D_1370、F_18、D_1375仅用于辅助验证和绘图。
OBJECTIVE_VARS = CALIB_DEPTH_VARS

# 主程序写入gen*_caliObsData.json的两个干湿平衡fitness字段。
FITNESS_MAX_RMSE_KEY = 'Fitness_maxDryAwareRMSE'
FITNESS_MEAN_RMSE_KEY = 'Fitness_meanDryAwareRMSE'

# 与原代码一致：绘图时只显示存在观测值的日期。
# 如果想看完整连续模拟过程，可改为 False。
PLOT_ONLY_OBS_DATES = False

# 独立运行本脚本时读取哪一个 gen 文件。
# 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
GEN_NUM_TO_PLOT = 100




# ============================================================================
# 结果保存函数
# ============================================================================
def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
    """Update calibration parameter values to MongoDB."""
    conn = MongoDBObj.client
    db = conn[spatial_db]
    collection = db['PARAMETERS']
    collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
    for idx, pname in enumerate(param_defs['names']):
        v2str = ','.join(str(v) for v in param_values[:, idx])
        collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})


def output_population_details(pops, outdir, gen_num,
                              plot_cfg=None  # type: Optional[PlotConfig]
                              ):
    """Save one NSGA-II generation and plot its current 95PPU results."""
    os.makedirs(outdir, exist_ok=True)

    # 1. Save time-series simulation data for the entire simulation period.
    all_sim_data = []
    pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
    with open(pickle_file, 'wb') as f:
        for ind in pops:
            all_sim_data.append(ind.sim.data)
        pickle.dump(all_sim_data, f)

    # 2. Save matched observation-simulation data for calibration period.
    cali_sim_obs_data = []
    cali_sim_data = []
    json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
    with open(json_file, 'w', encoding='utf-8') as f:
        for ind in pops:
            ind.cali.sim_obs_data['Gen'] = ind.gen
            ind.cali.sim_obs_data['ID'] = ind.id
            ind.cali.sim_obs_data['var_name'] = ind.cali.vars
            # 保存当前个体的完整参数向量，供最终统一参数固定使用
            ind.cali.sim_obs_data['gene_values'] = list(ind)
            cali_sim_obs_data.append(ind.cali.sim_obs_data)
        json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
        f.write('%s' % json_data)

    pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
    with open(pickle_file, 'wb') as f:
        for ind in pops:
            cali_sim_data.append(ind.cali.data)
        pickle.dump(cali_sim_data, f)

    # 3. Save validation data if it exists.
    vali_sim_obs_data = []
    vali_sim_data = []
    if pops[0].vali.valid:
        json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
        with open(json_file, 'w', encoding='utf-8') as f:
            for ind in pops:
                ind.vali.sim_obs_data['Gen'] = ind.gen
                ind.vali.sim_obs_data['ID'] = ind.id
                ind.vali.sim_obs_data['var_name'] = ind.vali.vars
                vali_sim_obs_data.append(ind.vali.sim_obs_data)
            json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
            f.write('%s' % json_data)

        pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
        with open(pickle_file, 'wb') as f:
            for ind in pops:
                vali_sim_data.append(ind.vali.data)
            pickle.dump(vali_sim_data, f)

    # 4. Plot current-generation results.
    if plot_cfg is None:
        plot_cfg = PlotConfig()

    try:
        calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
                        vali_sim_obs_data, vali_sim_data,
                        plot_cfg=plot_cfg)
    except Exception as e:
        import traceback
        print('calculate_95ppu 画图出错：', e)
        traceback.print_exc()


# ============================================================================
# 数据读取辅助函数
# ============================================================================
def dict_to_daily_month_matrix(ind):
    """
    旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。

    ind: OrderedDict[datetime -> list]
      - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
      - 每月代表行: [monthly_avg]
    """
    items = sorted(ind.items(), key=lambda kv: kv[0])
    keys_sorted = [k for k, _ in items]
    daily = []
    monthly = []

    for _, v in items:
        d = np.nan
        m = np.nan
        vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
        if len(vals) == 2:
            d = _safe_float(vals[0])
            m = _safe_float(vals[1])
        elif len(vals) == 1:
            d = _safe_float(vals[0])
        daily.append(d)
        monthly.append(m)

    return keys_sorted, np.column_stack([daily, monthly])


def dict_to_var_matrix(ind, nvar):
    """
    按变量数读取当前多目标模拟结果。

    对当前研究应满足：
      var_name = ['F_18', 'D_1366', 'D_1367', 'D_1368', 'D_1370', 'D_1375']
      每一日期的值 = [F_18, D_1366, D_1367, D_1368, D_1370, D_1375]
    """
    items = sorted(ind.items(), key=lambda kv: kv[0])
    keys_sorted = [k for k, _ in items]
    rows = []

    for _, v in items:
        vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
        if len(vals) < nvar:
            vals = vals + [np.nan] * (nvar - len(vals))
        vals = vals[:nvar]
        rows.append([_safe_float(x) for x in vals])

    return keys_sorted, np.asarray(rows, dtype=float)


def _to_datetime_list(dates):
    """Convert a string datetime list to a datetime list if needed."""
    if dates is None:
        return []
    dates = list(dates)
    if len(dates) == 0:
        return dates
    if is_string(dates[0]):
        return [StringClass.get_datetime(s) for s in dates]
    return dates


def _safe_float(x, default=np.nan):
    """Convert a value to float safely."""
    try:
        return float(x)
    except Exception:
        return default


def _safe_array(values):
    """Convert list-like values to a float numpy array safely."""
    return np.asarray([_safe_float(v) for v in values], dtype=float)


def _safe_metric(sim_obs_data_one, var, metric_name):
    """Read one metric from sim_obs_data safely."""
    try:
        return _safe_float(sim_obs_data_one[var][metric_name])
    except Exception:
        return np.nan


def _format_metric(x):
    """Format metric text in figure annotations."""
    try:
        x = float(x)
        return '%.2f' % x if np.isfinite(x) else 'nan'
    except Exception:
        return 'nan'


def _is_same_period(cali_dates, vali_dates):
    """Judge whether calibration and validation periods have the same endpoints."""
    if len(cali_dates) == 0 or len(vali_dates) == 0:
        return False
    return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]


# ============================================================================
# 统一参数诊断与选择：maxDryAwareRMSE + meanDryAwareRMSE
# ============================================================================
def _read_two_fitness(one_ind):
    """
    读取与率定主程序完全一致的两个干湿平衡fitness。

    优先读取主程序已经保存的：
        Fitness_maxDryAwareRMSE
        Fitness_meanDryAwareRMSE

    如果缺失，则根据当前JSON中D_1366、D_1367、D_1368
    各自保存的DryAwareRMSE重新计算：

        maxDryAwareRMSE  = max(BRMSE_1366, BRMSE_1367, BRMSE_1368)
        meanDryAwareRMSE = mean(BRMSE_1366, BRMSE_1367, BRMSE_1368)
    """
    rmse_values = np.asarray(
        [_safe_metric(one_ind, var, 'DryAwareRMSE') for var in OBJECTIVE_VARS],
        dtype=float
    )

    if not np.isfinite(rmse_values).all():
        raise ValueError('三个核心点DryAwareRMSE存在NaN/Inf。')
    if np.any(rmse_values < 0.0) or np.any(rmse_values >= 9000.0):
        raise ValueError('三个核心点DryAwareRMSE存在无效值：%s' % str(rmse_values))

    max_rmse_saved = _safe_float(one_ind.get(FITNESS_MAX_RMSE_KEY, np.nan))
    mean_rmse_saved = _safe_float(one_ind.get(FITNESS_MEAN_RMSE_KEY, np.nan))

    max_rmse = (
        float(max_rmse_saved)
        if np.isfinite(max_rmse_saved) and max_rmse_saved >= 0.0
        else float(np.max(rmse_values))
    )
    mean_rmse = (
        float(mean_rmse_saved)
        if np.isfinite(mean_rmse_saved) and mean_rmse_saved >= 0.0
        else float(np.mean(rmse_values))
    )

    return {
        'rmse_values': rmse_values,
        'max_rmse': max_rmse,
        'mean_rmse': mean_rmse,
    }


def diagnose_two_fitness(sim_obs_data):
    """输出当前候选种群两个真实率定目标的范围，便于检查率定是否在改善。"""
    values = []

    for ind_idx, one_ind in enumerate(sim_obs_data):
        try:
            fit = _read_two_fitness(one_ind)
        except Exception:
            continue

        values.append((
            fit['max_rmse'],
            fit['mean_rmse'],
            ind_idx,
            one_ind.get('Gen', -1),
            one_ind.get('ID', -1),
        ))

    if len(values) == 0:
        print('当前候选种群没有可诊断的双DryAwareRMSE参数组。')
        return

    max_rmse_best = min(values, key=lambda x: x[0])
    mean_rmse_best = min(values, key=lambda x: x[1])

    print('================================================')
    print('当前候选种群两个真实率定目标的单目标最好值：')
    print('min maxDryAwareRMSE = %.6f m, idx=%d, Gen=%s, ID=%s'
          % (max_rmse_best[0], max_rmse_best[2],
             str(max_rmse_best[3]), str(max_rmse_best[4])))
    print('min meanDryAwareRMSE = %.6f m, idx=%d, Gen=%s, ID=%s'
          % (mean_rmse_best[1], mean_rmse_best[2],
             str(mean_rmse_best[3]), str(mean_rmse_best[4])))
    print('================================================')


def select_shared_best_index(sim_obs_data):
    """
    从当前一代NSGA-II候选种群中选择一套统一参数供六个变量共同绘图。

    与主率定程序保持一致，两个真实目标都是最小化：
        1. maxDryAwareRMSE
        2. meanDryAwareRMSE

    最终统一参数采用“最差点优先”的字典序选择：
        第一优先：maxDryAwareRMSE最小；
        第二优先：若maxDryAwareRMSE相同，则meanDryAwareRMSE最小。

    这样不会再使用普通RMSE、minNSE或meanNSE进行最终红线选择。
    """
    candidates = []

    for ind_idx, one_ind in enumerate(sim_obs_data):
        try:
            fit = _read_two_fitness(one_ind)
        except Exception as e:
            print(
                '跳过无效候选参数组 idx=%d, Gen=%s, ID=%s，原因：%s'
                % (
                    ind_idx,
                    str(one_ind.get('Gen', -1)),
                    str(one_ind.get('ID', -1)),
                    str(e)
                )
            )
            continue

        candidates.append({
            'idx': ind_idx,
            'gen': one_ind.get('Gen', -1),
            'id': one_ind.get('ID', -1),
            'gene_values': one_ind.get('gene_values', None),
            'rmse_values': fit['rmse_values'],
            'max_rmse': fit['max_rmse'],
            'mean_rmse': fit['mean_rmse'],
        })

    if len(candidates) == 0:
        raise ValueError(
            '当前候选种群中没有同时具备maxDryAwareRMSE和'
            'meanDryAwareRMSE的有效参数组。'
        )

    candidates.sort(
        key=lambda c: (
            c['max_rmse'],
            c['mean_rmse'],
        )
    )

    best = candidates[0]
    shared_best_idx = best['idx']

    print('================================================')
    print('统一参数选择结果：maxDryAwareRMSE + meanDryAwareRMSE')
    print('选择方法：优先最小maxDryAwareRMSE；若相同，再最小meanDryAwareRMSE')
    print('shared_best_idx =', best['idx'])
    print('source generation =', best['gen'])
    print('source calibrationID =', best['id'])
    print('maxDryAwareRMSE = %.6f m' % best['max_rmse'])
    print('meanDryAwareRMSE = %.6f m' % best['mean_rmse'])
    print('------------- 三个核心点DryAwareRMSE -------------')

    for var, rmse in zip(OBJECTIVE_VARS, best['rmse_values']):
        print('%s DryAwareRMSE = %.10f m' % (var, rmse))

    if best['gene_values'] is not None:
        print('Selected parameter values =', best['gene_values'])

    print('================================================')

    return shared_best_idx, best


def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
                                ylows_list, yups_list):
    """Calculate P-factor and R-factor at observation dates."""
    count = 0
    ylows_obs = []
    yups_obs = []
    valid_obs = []
    sim_dates_py = list(sim_dates_list)

    for oi, ov in enumerate(obs_data_list):
        if not np.isfinite(ov) or oi >= len(obs_dates_list):
            continue
        try:
            si = sim_dates_py.index(obs_dates_list[oi])
        except Exception:
            continue
        if si >= len(ylows_list) or si >= len(yups_list):
            continue
        if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
            continue

        ylows_obs.append(ylows_list[si])
        yups_obs.append(yups_list[si])
        valid_obs.append(ov)
        if ylows_list[si] <= ov <= yups_list[si]:
            count += 1

    if len(valid_obs) == 0:
        return np.nan, np.nan

    p_value = float(count) / float(len(valid_obs))
    ylows_obs = np.asarray(ylows_obs, dtype=float)
    yups_obs = np.asarray(yups_obs, dtype=float)
    valid_obs = np.asarray(valid_obs, dtype=float)
    obs_std = np.nanstd(valid_obs)
    if not np.isfinite(obs_std) or obs_std == 0.0:
        r_value = np.nan
    else:
        r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
    return p_value, r_value


# ============================================================================
# 95PPU 与统一参数结果绘图
# ============================================================================
def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
                    vali_sim_obs_data=None, vali_sim_data=None,
                    plot_cfg=None  # type: Optional[PlotConfig]
                    ):
    """Calculate 95% prediction uncertainty and plot all variables."""
    if plot_cfg is None:
        plot_cfg = PlotConfig()

    plt.rcParams['xtick.direction'] = 'out'
    plt.rcParams['ytick.direction'] = 'out'
    plt.rcParams['font.family'] = plot_cfg.font_name
    plt.rcParams['timezone'] = 'UTC'
    plt.rcParams['mathtext.fontset'] = 'custom'
    plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
    plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'

    if sim_obs_data is None or sim_data is None:
        print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
        return
    if len(sim_data) < 2:
        print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
        return
    if len(sim_obs_data) == 0:
        print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
        return

    var_name = sim_obs_data[0]['var_name']
    nvar = len(var_name)
    print('calculate_95ppu: var_name =', var_name)
    print('calculate_95ppu: nvar =', nvar)
    print('calculate_95ppu: simulations =', len(sim_data))

    # 检查全部需要绘图/验证的变量是否存在
    required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
    missing_vars = [v for v in required_vars if v not in var_name]
    if len(missing_vars) > 0:
        raise ValueError(
            '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
            % str(missing_vars)
        )

    diagnose_two_fitness(sim_obs_data)
    shared_best_idx, best_info = select_shared_best_index(sim_obs_data)

    # 将当前代选出的统一参数、两个真实率定目标及诊断指标单独保存。
    # 最终正式模拟应使用最后一代对应文件中的参数值。
    selected_file = os.path.join(
        outdir,
        'Gen%d_selected_shared_parameter_set.txt' % gen_num
    )

    with open(selected_file, 'w', encoding='utf-8') as f:
        f.write(
            'Selection rule: minimize maxDryAwareRMSE first, then minimize '
            'meanDryAwareRMSE. '
            'D_1366, D_1367 and D_1368 are the calibration depth variables. '
            'D_1370, D_1375 and F_18 are used only for independent validation '
            'and plotting.\n\n'
        )

        f.write('Source generation: %s\n' % str(best_info['gen']))
        f.write('Source calibration ID: %s\n' % str(best_info['id']))
        f.write('Shared best index: %s\n' % str(best_info['idx']))
        f.write('Maximum DryAwareRMSE: %.10f m\n' % best_info['max_rmse'])
        f.write('Mean DryAwareRMSE: %.10f m\n' % best_info['mean_rmse'])

        f.write('\nThree core-depth DryAwareRMSE values:\n')
        for var, rmse in zip(OBJECTIVE_VARS, best_info['rmse_values']):
            f.write('%s DryAwareRMSE: %.12f m\n' % (var, rmse))

        # 同时保存所选参数组的其他诊断指标。
        f.write('\nAll-variable diagnostic metrics:\n')
        all_vars = [
            'D_1366',
            'D_1367',
            'D_1368',
            'D_1370',
            'D_1375',
            'F_18',
        ]
        diagnostic_metrics = [
            'NSE',
            'NSE1',
            'lnNSE',
            'RMSE',
            'DryAwareRMSE',
            'DryExcessRMSE',
            'WetRMSE',
            'FalseWetRate',
            'FalseDryRate',
            'DryCount',
            'WetCount',
            'RSR',
            'PBIAS',
            'R-square',
        ]
        selected_result = sim_obs_data[shared_best_idx]

        for var in all_vars:
            f.write('\n[%s]\n' % var)
            for metric in diagnostic_metrics:
                metric_value = _safe_metric(selected_result, var, metric)
                f.write('%s: %.12f\n' % (metric, metric_value))

        f.write(
            '\nParameter values: %s\n'
            % str(best_info['gene_values'])
        )

    for idx, var in enumerate(var_name):
        print('================================================')
        print('开始画变量:', var, 'idx =', idx)

        plot_validation = bool(vali_sim_obs_data and vali_sim_data and
                               len(vali_sim_obs_data) > 0 and
                               var in vali_sim_obs_data[0].get('var_name', []))

        ylabel_str = var
        if var in ['Q', 'QI', 'QG', 'QS']:
            ylabel_str += ' (m$^3$/s)'
        elif var.startswith('D_') or var == 'D':
            ylabel_str += ' (m)'
        elif var.startswith('F_') or var == 'F':
            ylabel_str += ' (km$^2$)'
        elif 'CONC' in var.upper():
            ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
        elif 'SED' in var.upper():
            ylabel_str += ' (kg)'

        if var not in sim_obs_data[0]:
            print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
            continue

        # 1. Read calibration observations.
        cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
        cali_obs_values = sim_obs_data[0][var]['Obs'][:]
        obs_dates = cali_obs_dates[:]
        obs_data = list(cali_obs_values)
        order = 1

        # 2. Read validation observations.
        vali_obs_dates = []
        vali_obs_values = []
        same_cali_vali_period = False
        if plot_validation:
            vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
            vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
            same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
            if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
                if vali_obs_dates[-1] <= cali_obs_dates[0]:
                    order = 0
                    obs_dates = vali_obs_dates + obs_dates
                    obs_data = list(vali_obs_values) + obs_data
                else:
                    obs_dates = obs_dates + vali_obs_dates
                    obs_data = obs_data + list(vali_obs_values)

        obs_dates = np.asarray(obs_dates)
        obs_data = _safe_array(obs_data)

        # 3. Read simulation dates.
        # 日期必须与dict_to_var_matrix()中的模拟值采用相同的排序顺序。

        cali_sim_date_keys, _ = dict_to_var_matrix(
            sim_data[0],
            nvar
        )
        cali_sim_dates = _to_datetime_list(
            cali_sim_date_keys
        )

        # 检查是否有重复日期
        if len(cali_sim_dates) != len(set(cali_sim_dates)):
            raise ValueError('率定期模拟日期中存在重复日期。')

        # 检查日期是否严格升序
        if any(
            cali_sim_dates[i] > cali_sim_dates[i + 1]
            for i in range(len(cali_sim_dates) - 1)
        ):
            raise ValueError('率定期模拟日期没有按照时间升序排列。')

        sim_dates = cali_sim_dates[:]
        vali_sim_dates = []

        if plot_validation:
            vali_sim_date_keys, _ = dict_to_var_matrix(
                vali_sim_data[0],
                nvar
            )
            vali_sim_dates = _to_datetime_list(
                vali_sim_date_keys
            )

            if len(vali_sim_dates) != len(set(vali_sim_dates)):
                raise ValueError('验证期模拟日期中存在重复日期。')

            if any(
                vali_sim_dates[i] > vali_sim_dates[i + 1]
                for i in range(len(vali_sim_dates) - 1)
            ):
                raise ValueError('验证期模拟日期没有按照时间升序排列。')

            if not same_cali_vali_period:
                if order:
                    sim_dates = sim_dates + vali_sim_dates
                else:
                    sim_dates = vali_sim_dates + sim_dates

        # 4. Collect current-variable simulations of all candidate parameter sets.
        sim_data_list = []
        for idx2, ind in enumerate(sim_data):
            _, tmp_mat = dict_to_var_matrix(ind, nvar)
            if idx >= tmp_mat.shape[1]:
                print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
                      % (var, idx, tmp_mat.shape[1]))
                continue
            tmpsim = tmp_mat[:, idx].tolist()

            if plot_validation and not same_cali_vali_period:
                if idx2 < len(vali_sim_data):
                    _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
                    tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
                        else [np.nan] * len(vali_sim_dates)
                else:
                    tmp_data = [np.nan] * len(vali_sim_dates)
                tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
            sim_data_list.append(tmpsim)

        if len(sim_data_list) == 0:
            print('变量 %s 没有可用模拟数据，跳过画图。' % var)
            continue

        # 5. Simulation by the single selected parameter set.
        _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
        sim_best = best_mat[:, idx].tolist()
        if plot_validation and not same_cali_vali_period:
            if shared_best_idx < len(vali_sim_data):
                _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
                tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
                    else [np.nan] * len(vali_sim_dates)
            else:
                tmp_data = [np.nan] * len(vali_sim_dates)
            sim_best = sim_best + tmp_data if order else tmp_data + sim_best

        # 6. Calculate 95PPU using all candidate parameter sets.
        try:
            sim_data_array = np.asarray(sim_data_list, dtype=float)
            ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
            yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
        except Exception as e:
            print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
            continue

        sim_dates = np.asarray(sim_dates)
        sim_best = np.asarray(sim_best, dtype=float)
        ylows = np.asarray(ylows, dtype=float)
        yups = np.asarray(yups, dtype=float)
        min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
        sim_dates = sim_dates[:min_len]
        sim_best = sim_best[:min_len]
        ylows = ylows[:min_len]
        yups = yups[:min_len]
        if min_len == 0:
            print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
            continue

        # 7. Calculate P-factor and R-factor.
        p_value, r_value = calculate_95ppu_efficiency(
            _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
            sim_dates, ylows, yups)

        # 8. Metrics from the same selected parameter set.
        best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
        best_kge = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE1')
        best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
        best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
        best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
        best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')

        p_value_v = np.nan
        r_value_v = np.nan
        best_nse_v = np.nan
        best_kge_v = np.nan
        best_rmse_v = np.nan
        best_rsr_v = np.nan
        best_pbias_v = np.nan
        best_r2_v = np.nan
        if plot_validation and not same_cali_vali_period:
            p_value_v, r_value_v = calculate_95ppu_efficiency(
                _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
                sim_dates, ylows, yups)
            best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
            best_kge_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE1')
            best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
            best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
            best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
            best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')

        print('变量 =', var)
        print('same_cali_vali_period =', same_cali_vali_period)
        print('plot_validation =', plot_validation)
        print('len(sim_dates) =', len(sim_dates))
        print('len(obs_dates) =', len(obs_dates))
        print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
        print('ylows NaN 个数 =', np.isnan(ylows).sum())
        print('yups NaN 个数 =', np.isnan(yups).sum())
        if len(sim_best) > 0 and np.isfinite(sim_best).any():
            print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))

        # 9. Determine displayed dates.
        if PLOT_ONLY_OBS_DATES:
            mask = np.isin(sim_dates, obs_dates)
        else:
            mask = np.ones(len(sim_dates), dtype=bool)

        if np.any(mask):
            sim_dates_plot = sim_dates[mask]
            sim_best_plot = sim_best[mask]
            ylows_plot = ylows[mask]
            yups_plot = yups[mask]
        else:
            print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
            sim_dates_plot = sim_dates
            sim_best_plot = sim_best
            ylows_plot = ylows
            yups_plot = yups

        if len(sim_dates_plot) == 0:
            print('变量 %s：绘图日期为空，跳过。' % var)
            continue

        # 10. Plot.
        fig, ax = plt.subplots(figsize=(14, 4.8))

        # 95PPU：完整连续模拟日期
        ax.fill_between(
            sim_dates_plot,
            ylows_plot.tolist(),
            yups_plot.tolist(),
            color=(0.8, 0.8, 0.8),
            label='95PPU',
            zorder=1
        )

        # 统一参数完整连续模拟曲线
        bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
        ax.plot(
            sim_dates_plot,
            sim_best_plot,
            linestyle='--',
            color='red',
            linewidth=1.5,
            label=bestsim_label,
            zorder=3
        )

        # 实测值
        observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
        valid_obs_mask = np.isfinite(obs_data)

        ax.scatter(
            obs_dates,
            obs_data,
            marker='.',
            s=20,
            color='g',
            label=observed_label,
            zorder=4
        )

        # 横轴同时考虑模拟日期和实测日期，防止首尾实测点被裁掉
        all_plot_dates = list(sim_dates_plot) + list(obs_dates[valid_obs_mask])

        if len(all_plot_dates) > 0:
            ax.set_xlim(
                left=min(all_plot_dates),
                right=max(all_plot_dates)
            )

        # 纵轴必须使用真正最大值，而不是99%分位数，
        # 保证所有实测点、模拟值和95PPU都能够显示
        y_parts = []

        for values in [
            obs_data[valid_obs_mask],
            sim_best_plot,
            ylows_plot,
            yups_plot
        ]:
            values = np.asarray(values, dtype=float)
            valid_values = values[np.isfinite(values)]

            if len(valid_values) > 0:
                y_parts.append(valid_values)

        if len(y_parts) > 0:
            all_y = np.concatenate(y_parts)

            y_high = np.nanmax(all_y)

            if not np.isfinite(y_high) or y_high <= 0:
                y_high = 0.01

            # 下边界稍微低于0，
            # 这样 Obs=0 的绿色实测点不会被横轴遮住
            ax.set_ylim(
                bottom=0.0,
                top=1.08 * y_high
            )
        else:
            ax.set_ylim(
                bottom=0.0,
                top=0.01
            )

        ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
        ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
                       which='major', labelsize=plot_cfg.tick_fsize)
        ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
                       which='major', labelsize=plot_cfg.tick_fsize)
        plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
                   fontsize=plot_cfg.axislabel_fsize)
        plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)

        if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
            sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
                else cali_sim_dates[0]
            ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
            if order:
                left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
                right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
            else:
                left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
                right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
            ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
                    fontdict={'style': 'italic', 'weight': 'bold',
                              'size': max(plot_cfg.label_fsize - 2, 9)},
                    color='black', va='top', ha='left')
            ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
                    fontdict={'style': 'italic', 'weight': 'bold',
                              'size': max(plot_cfg.label_fsize - 2, 9)},
                    color='black', va='top', ha='left')

        handles, labels = ax.get_legend_handles_labels()
        figorders = []
        for lab in ['95PPU', observed_label, bestsim_label]:
            if lab in labels:
                figorders.append(labels.index(lab))
        if figorders:
            ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
                      fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)

        cali_text = (
            'Calibration\n'
            'P-factor: %s\n'
            'R-factor: %s\n'
            'NSE: %s\n'
            'KGE: %s\n'
            'RMSE: %s\n'
            'RSR: %s\n'
            'PBIAS: %s%%\n'
            'R$^2$: %s'
            % (
                _format_metric(p_value),
                _format_metric(r_value),
                _format_metric(best_nse),
                _format_metric(best_kge),
                _format_metric(best_rmse),
                _format_metric(best_rsr),
                _format_metric(best_pbias),
                _format_metric(best_r2)
            )
        )
        if plot_validation and not same_cali_vali_period:
            vali_text = (
                '\n\nValidation\n'
                'P-factor: %s\n'
                'R-factor: %s\n'
                'NSE: %s\n'
                'KGE: %s\n'
                'RMSE: %s\n'
                'RSR: %s\n'
                'PBIAS: %s%%\n'
                'R$^2$: %s'
                % (
                    _format_metric(p_value_v),
                    _format_metric(r_value_v),
                    _format_metric(best_nse_v),
                    _format_metric(best_kge_v),
                    _format_metric(best_rmse_v),
                    _format_metric(best_rsr_v),
                    _format_metric(best_pbias_v),
                    _format_metric(best_r2_v)
                )
            )
            metric_text = cali_text + vali_text
        else:
            metric_text = cali_text

        ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
                fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
                linespacing=1.15,
                bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)

        plt.tight_layout()
        save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
        plt.cla()
        plt.clf()
        plt.close()
        print('变量 %s 画图完成。' % var)
        print('================================================')


# ============================================================================
# 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# ============================================================================
def load_generation_results(simdir, gen_num):
    """Load saved calibration/validation results for one generation."""
    cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
    cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
    with open(cali_json, 'r', encoding='utf-8') as f:
        cali_obs_data = json.load(f)
    with open(cali_pickle, 'rb') as f:
        cali_data = pickle.load(f)

    vali_obs_data = None
    vali_data = None
    vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
    vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
    if os.path.exists(vali_json) and os.path.exists(vali_pickle):
        with open(vali_json, 'r', encoding='utf-8') as f:
            vali_obs_data = json.load(f)
        with open(vali_pickle, 'rb') as f:
            vali_data = pickle.load(f)
    return cali_obs_data, cali_data, vali_obs_data, vali_data


if __name__ == '__main__':
    # 注意：这里要改成你实际的率定结果文件夹。
    # 必须改成“3个水深点maxRMSE + meanRMSE”新一轮率定的实际结果文件夹。
    wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
    # wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_100'
    simdir = wp + os.path.sep + 'simulated_data'

    cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
        simdir, GEN_NUM_TO_PLOT)

    if vali_obs_data is not None and vali_data is not None:
        calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
                        vali_obs_data, vali_data)
    else:
        calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)

    print('Total simulations %d' % len(cali_obs_data))









# # 五个水深混合目标：D_1304、D_1305、D_1306、D_1309 使用 NSE；D_1310 使用 RMSE
# from __future__ import absolute_import, unicode_literals
#
# import json
# import os
# import pickle
# import sys
# from io import open
# from typing import Optional
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# import matplotlib as mpl
# import numpy
# import numpy as np
#
# if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
#     mpl.use('Agg', warn=False)
#
# import matplotlib.dates as mdates
# import matplotlib.pyplot as plt
#
# from pygeoc.utils import StringClass, is_string
# from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
# from parameters_sensitivity.sensitivity import SpecialJsonEncoder
#
#
# # ============================================================================
# # 用户可调整设置
# # ============================================================================
# # 淹没面积：仅用于辅助验证和绘图
# AREA_VAR = 'F_19'
#
# # 使用 NSE 进行率定和最终统一参数选择的四个水深点
# NSE_CALIB_DEPTH_VARS = ['D_1304', 'D_1305', 'D_1306', 'D_1309']
#
# # D_1310 的观测值大量接近 0，使用 RMSE 作为率定目标
# RMSE_CALIB_DEPTH_VAR = 'D_1310'
#
# # 五个水深点全部参与率定和最终统一参数选择
# CALIB_DEPTH_VARS = NSE_CALIB_DEPTH_VARS + [RMSE_CALIB_DEPTH_VAR]
#
# # 需要绘图和诊断的全部水深变量
# PLOT_DEPTH_VARS = ['D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#
# # 与原代码一致：绘图时只显示存在观测值的日期。
# # 如果想看完整连续模拟过程，可改为 False。
# PLOT_ONLY_OBS_DATES = True
#
# # 独立运行本脚本时读取哪一个 gen 文件。
# # 你前面发的是 Gen100 图，所以这里先设为 100；如果要重画第 80 代，就改回 80。
# GEN_NUM_TO_PLOT = 100
#
#
# # ============================================================================
# # 结果保存函数
# # ============================================================================
# def write_param_values_to_mongodb(spatial_db, param_defs, param_values):
#     """Update calibration parameter values to MongoDB."""
#     conn = MongoDBObj.client
#     db = conn[spatial_db]
#     collection = db['PARAMETERS']
#     collection.update_many({}, {'$unset': {'CALI_VALUES': ''}})
#     for idx, pname in enumerate(param_defs['names']):
#         v2str = ','.join(str(v) for v in param_values[:, idx])
#         collection.find_one_and_update({'NAME': pname}, {'$set': {'CALI_VALUES': v2str}})
#
#
# def output_population_details(pops, outdir, gen_num,
#                               plot_cfg=None  # type: Optional[PlotConfig]
#                               ):
#     """Save one NSGA-II generation and plot its current 95PPU results."""
#     os.makedirs(outdir, exist_ok=True)
#
#     # 1. Save time-series simulation data for the entire simulation period.
#     all_sim_data = []
#     pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             all_sim_data.append(ind.sim.data)
#         pickle.dump(all_sim_data, f)
#
#     # 2. Save matched observation-simulation data for calibration period.
#     cali_sim_obs_data = []
#     cali_sim_data = []
#     json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     with open(json_file, 'w', encoding='utf-8') as f:
#         for ind in pops:
#             ind.cali.sim_obs_data['Gen'] = ind.gen
#             ind.cali.sim_obs_data['ID'] = ind.id
#             ind.cali.sim_obs_data['var_name'] = ind.cali.vars
#             # 保存当前个体的完整参数向量，供最终统一参数固定使用
#             ind.cali.sim_obs_data['gene_values'] = list(ind)
#             cali_sim_obs_data.append(ind.cali.sim_obs_data)
#         json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#         f.write('%s' % json_data)
#
#     pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(pickle_file, 'wb') as f:
#         for ind in pops:
#             cali_sim_data.append(ind.cali.data)
#         pickle.dump(cali_sim_data, f)
#
#     # 3. Save validation data if it exists.
#     vali_sim_obs_data = []
#     vali_sim_data = []
#     if pops[0].vali.valid:
#         json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#         with open(json_file, 'w', encoding='utf-8') as f:
#             for ind in pops:
#                 ind.vali.sim_obs_data['Gen'] = ind.gen
#                 ind.vali.sim_obs_data['ID'] = ind.id
#                 ind.vali.sim_obs_data['var_name'] = ind.vali.vars
#                 vali_sim_obs_data.append(ind.vali.sim_obs_data)
#             json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
#             f.write('%s' % json_data)
#
#         pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#         with open(pickle_file, 'wb') as f:
#             for ind in pops:
#                 vali_sim_data.append(ind.vali.data)
#             pickle.dump(vali_sim_data, f)
#
#     # 4. Plot current-generation results.
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     try:
#         calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
#                         vali_sim_obs_data, vali_sim_data,
#                         plot_cfg=plot_cfg)
#     except Exception as e:
#         import traceback
#         print('calculate_95ppu 画图出错：', e)
#         traceback.print_exc()
#
#
# # ============================================================================
# # 数据读取辅助函数
# # ============================================================================
# def dict_to_daily_month_matrix(ind):
#     """
#     旧函数，保留以兼容历史结果；当前六变量绘图不使用本函数。
#
#     ind: OrderedDict[datetime -> list]
#       - 每日行: [daily_sum] 或 [daily_sum, monthly_avg]
#       - 每月代表行: [monthly_avg]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     daily = []
#     monthly = []
#
#     for _, v in items:
#         d = np.nan
#         m = np.nan
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) == 2:
#             d = _safe_float(vals[0])
#             m = _safe_float(vals[1])
#         elif len(vals) == 1:
#             d = _safe_float(vals[0])
#         daily.append(d)
#         monthly.append(m)
#
#     return keys_sorted, np.column_stack([daily, monthly])
#
#
# def dict_to_var_matrix(ind, nvar):
#     """
#     按变量数读取当前多目标模拟结果。
#
#     对当前研究应满足：
#       var_name = ['F_19', 'D_1304', 'D_1305', 'D_1306', 'D_1309', 'D_1310']
#       每一日期的值 = [F_19, D_1304, D_1305, D_1306, D_1309, D_1310]
#     """
#     items = sorted(ind.items(), key=lambda kv: kv[0])
#     keys_sorted = [k for k, _ in items]
#     rows = []
#
#     for _, v in items:
#         vals = list(v) if isinstance(v, (list, tuple, np.ndarray)) else [v]
#         if len(vals) < nvar:
#             vals = vals + [np.nan] * (nvar - len(vals))
#         vals = vals[:nvar]
#         rows.append([_safe_float(x) for x in vals])
#
#     return keys_sorted, np.asarray(rows, dtype=float)
#
#
# def _to_datetime_list(dates):
#     """Convert a string datetime list to a datetime list if needed."""
#     if dates is None:
#         return []
#     dates = list(dates)
#     if len(dates) == 0:
#         return dates
#     if is_string(dates[0]):
#         return [StringClass.get_datetime(s) for s in dates]
#     return dates
#
#
# def _safe_float(x, default=np.nan):
#     """Convert a value to float safely."""
#     try:
#         return float(x)
#     except Exception:
#         return default
#
#
# def _safe_array(values):
#     """Convert list-like values to a float numpy array safely."""
#     return np.asarray([_safe_float(v) for v in values], dtype=float)
#
#
# def _safe_metric(sim_obs_data_one, var, metric_name):
#     """Read one metric from sim_obs_data safely."""
#     try:
#         return _safe_float(sim_obs_data_one[var][metric_name])
#     except Exception:
#         return np.nan
#
#
# def _format_metric(x):
#     """Format metric text in figure annotations."""
#     try:
#         x = float(x)
#         return '%.2f' % x if np.isfinite(x) else 'nan'
#     except Exception:
#         return 'nan'
#
#
# def _is_same_period(cali_dates, vali_dates):
#     """Judge whether calibration and validation periods have the same endpoints."""
#     if len(cali_dates) == 0 or len(vali_dates) == 0:
#         return False
#     return cali_dates[0] == vali_dates[0] and cali_dates[-1] == vali_dates[-1]
#
#
# # ============================================================================
# # 统一参数诊断与选择：五个水深共同决定参数；F_19 仅用于辅助验证
# # ============================================================================
# def diagnose_best_rmse_for_each_variable(sim_obs_data):
#     """
#     输出每个变量在当前候选参数集中的最小 RMSE。
#
#     RMSE 越小越好。该函数仅用于诊断，不直接改变最终统一参数。
#     """
#     var_list = [AREA_VAR] + PLOT_DEPTH_VARS
#
#     print('================================================')
#     print('当前候选参数集中，各变量单独能够达到的最小 RMSE：')
#     for var in var_list:
#         valid_items = []
#         for ind_idx, one_ind in enumerate(sim_obs_data):
#             rmse = _safe_metric(one_ind, var, 'RMSE')
#             if np.isfinite(rmse) and rmse >= 0.0:
#                 valid_items.append((
#                     float(rmse),
#                     ind_idx,
#                     one_ind.get('Gen', -1),
#                     one_ind.get('ID', -1)
#                 ))
#
#         if len(valid_items) == 0:
#             print('%s: 没有有效 RMSE。' % var)
#             continue
#
#         best_rmse, best_idx, best_gen, best_id = sorted(
#             valid_items, key=lambda x: x[0]
#         )[0]
#         print('%s: min RMSE = %.6f, idx = %d, Gen = %s, ID = %s'
#               % (var, best_rmse, best_idx, str(best_gen), str(best_id)))
#     print('================================================')
#
#
# def select_shared_best_index(sim_obs_data):
#     """
#     从当前 NSGA-II 候选参数集中选择一套统一参数。
#
#     与主率定程序保持一致：
#     1. D_1304、D_1305、D_1306、D_1309：NSE 越大越好；
#     2. D_1310：RMSE 越小越好；
#     3. F_19 仅用于辅助验证和绘图，不参与统一参数选择。
#
#     因为 NSE 与 RMSE 的量纲和数值范围不同，不能直接相加。
#     本函数先在当前候选集中把五个目标分别归一化为 0～1 的“损失”：
#         - NSE：当前候选中的最大值对应损失 0，最小值对应损失 1；
#         - RMSE：当前候选中的最小值对应损失 0，最大值对应损失 1。
#
#     最终依次最小化：
#         1. 五个目标中的最大归一化损失，避免某一个水深点特别差；
#         2. 五个目标的平均归一化损失，选择总体更均衡的参数；
#         3. 到理想点的归一化欧氏距离，作为进一步的并列判别。
#     """
#     candidates = []
#
#     for ind_idx, one_ind in enumerate(sim_obs_data):
#         depth_rmses = np.asarray(
#             [_safe_metric(one_ind, var, 'RMSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_nses = np.asarray(
#             [_safe_metric(one_ind, var, 'NSE') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_pbias = np.asarray(
#             [_safe_metric(one_ind, var, 'PBIAS') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_r2 = np.asarray(
#             [_safe_metric(one_ind, var, 'R-square') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         depth_rsr = np.asarray(
#             [_safe_metric(one_ind, var, 'RSR') for var in CALIB_DEPTH_VARS],
#             dtype=float
#         )
#
#         nse_objectives = np.asarray(
#             [_safe_metric(one_ind, var, 'NSE')
#              for var in NSE_CALIB_DEPTH_VARS],
#             dtype=float
#         )
#         rmse_1310 = _safe_metric(
#             one_ind, RMSE_CALIB_DEPTH_VAR, 'RMSE'
#         )
#
#         # 四个 NSE 和 D_1310 的 RMSE 必须全部有效。
#         if not np.isfinite(nse_objectives).all():
#             continue
#         if not np.isfinite(rmse_1310):
#             continue
#
#         # 排除模型失败后的异常惩罚值。
#         if np.any(nse_objectives <= -9000.0):
#             continue
#         if rmse_1310 < 0.0 or rmse_1310 >= 9000.0:
#             continue
#
#         mean_rmse = (
#             float(np.nanmean(depth_rmses))
#             if np.isfinite(depth_rmses).any() else np.nan
#         )
#         max_rmse = (
#             float(np.nanmax(depth_rmses))
#             if np.isfinite(depth_rmses).any() else np.nan
#         )
#         min_rmse = (
#             float(np.nanmin(depth_rmses))
#             if np.isfinite(depth_rmses).any() else np.nan
#         )
#         mean_nse = (
#             float(np.nanmean(depth_nses))
#             if np.isfinite(depth_nses).any() else np.nan
#         )
#         min_nse = (
#             float(np.nanmin(depth_nses))
#             if np.isfinite(depth_nses).any() else np.nan
#         )
#         mean_abs_pbias = (
#             float(np.nanmean(np.abs(depth_pbias)))
#             if np.isfinite(depth_pbias).any() else np.nan
#         )
#         mean_r2 = (
#             float(np.nanmean(depth_r2))
#             if np.isfinite(depth_r2).any() else np.nan
#         )
#         mean_rsr = (
#             float(np.nanmean(depth_rsr))
#             if np.isfinite(depth_rsr).any() else np.nan
#         )
#
#         candidates.append({
#             'idx': ind_idx,
#             'gen': one_ind.get('Gen', -1),
#             'id': one_ind.get('ID', -1),
#             'gene_values': one_ind.get('gene_values', None),
#
#             # 五个点的完整诊断指标，供打印、保存和绘图使用。
#             'calib_depth_rmses': depth_rmses,
#             'calib_depth_nses': depth_nses,
#             'calib_depth_pbias': depth_pbias,
#             'calib_depth_r2': depth_r2,
#             'calib_depth_rsr': depth_rsr,
#
#             # 真正参与混合目标选择的指标。
#             'nse_objectives': nse_objectives,
#             'rmse_1310': float(rmse_1310),
#
#             # 汇总诊断值。
#             'mean_depth_rmse': mean_rmse,
#             'max_depth_rmse': max_rmse,
#             'min_depth_rmse': min_rmse,
#             'mean_depth_nse': mean_nse,
#             'min_depth_nse': min_nse,
#             'mean_nse_objective': float(np.mean(nse_objectives)),
#             'min_nse_objective': float(np.min(nse_objectives)),
#             'mean_abs_pbias': mean_abs_pbias,
#             'mean_r2': mean_r2,
#             'mean_rsr': mean_rsr,
#         })
#
#     if len(candidates) == 0:
#         raise ValueError(
#             '当前候选参数集中没有有效的五水深混合目标参数组。'
#             '请检查 gen*_caliObsData.json 是否包含 '
#             'D_1304、D_1305、D_1306、D_1309 的 NSE，'
#             '以及 D_1310 的 RMSE。'
#         )
#
#     # ------------------------------------------------------------------
#     # 在当前候选集中，对四个 NSE 和一个 RMSE 分别归一化。
#     # objective_losses 每一列均为 0～1，越小越好。
#     # ------------------------------------------------------------------
#     nse_matrix = np.asarray(
#         [c['nse_objectives'] for c in candidates],
#         dtype=float
#     )
#     rmse_vector = np.asarray(
#         [c['rmse_1310'] for c in candidates],
#         dtype=float
#     )
#
#     loss_columns = []
#
#     # NSE：最大值最好（loss=0），最小值最差（loss=1）。
#     for col_idx in range(nse_matrix.shape[1]):
#         values = nse_matrix[:, col_idx]
#         best_value = float(np.max(values))
#         worst_value = float(np.min(values))
#         span = best_value - worst_value
#
#         if span <= 1.0e-12:
#             losses = np.zeros_like(values, dtype=float)
#         else:
#             losses = (best_value - values) / span
#         loss_columns.append(losses)
#
#     # D_1310 RMSE：最小值最好（loss=0），最大值最差（loss=1）。
#     best_rmse = float(np.min(rmse_vector))
#     worst_rmse = float(np.max(rmse_vector))
#     rmse_span = worst_rmse - best_rmse
#
#     if rmse_span <= 1.0e-12:
#         rmse_losses = np.zeros_like(rmse_vector, dtype=float)
#     else:
#         rmse_losses = (rmse_vector - best_rmse) / rmse_span
#     loss_columns.append(rmse_losses)
#
#     objective_loss_matrix = np.column_stack(loss_columns)
#
#     for idx, candidate in enumerate(candidates):
#         losses = objective_loss_matrix[idx, :]
#         candidate['normalized_objective_losses'] = losses
#         candidate['worst_normalized_loss'] = float(np.max(losses))
#         candidate['mean_normalized_loss'] = float(np.mean(losses))
#         candidate['ideal_distance'] = float(
#             np.sqrt(np.mean(np.square(losses)))
#         )
#
#     candidate_pool = sorted(
#         candidates,
#         key=lambda c: (
#             c['worst_normalized_loss'],
#             c['mean_normalized_loss'],
#             c['ideal_distance']
#         )
#     )
#     best = candidate_pool[0]
#
#     print('================================================')
#     print('统一参数选择结果：五个水深混合目标共同选参')
#     print('D_1304、D_1305、D_1306、D_1309：最大化 NSE')
#     print('D_1310：最小化 RMSE')
#     print('选参顺序：先最小化最大归一化损失，'
#           '再最小化平均归一化损失和理想点距离')
#     print('shared_best_idx =', best['idx'])
#     print('source generation =', best['gen'])
#     print('source calibrationID =', best['id'])
#
#     for i, var in enumerate(CALIB_DEPTH_VARS):
#         print('%s RMSE = %.6f, NSE = %.6f, RSR = %.6f, '
#               'PBIAS = %.6f, R2 = %.6f'
#               % (
#                   var,
#                   best['calib_depth_rmses'][i],
#                   best['calib_depth_nses'][i],
#                   best['calib_depth_rsr'][i],
#                   best['calib_depth_pbias'][i],
#                   best['calib_depth_r2'][i]
#               ))
#
#     print('Four-NSE mean = %.6f' % best['mean_nse_objective'])
#     print('Four-NSE minimum = %.6f' % best['min_nse_objective'])
#     print('D_1310 RMSE = %.6f' % best['rmse_1310'])
#     print('Normalized objective losses = %s'
#           % str(best['normalized_objective_losses'].tolist()))
#     print('Worst normalized loss = %.6f'
#           % best['worst_normalized_loss'])
#     print('Mean normalized loss = %.6f'
#           % best['mean_normalized_loss'])
#     print('Ideal-point distance = %.6f'
#           % best['ideal_distance'])
#
#     print('Mean five-depth RMSE = %.6f'
#           % best['mean_depth_rmse'])
#     print('Maximum five-depth RMSE = %.6f'
#           % best['max_depth_rmse'])
#     print('Minimum five-depth RMSE = %.6f'
#           % best['min_depth_rmse'])
#     print('Mean five-depth NSE = %.6f'
#           % best['mean_depth_nse'])
#     print('Minimum five-depth NSE = %.6f'
#           % best['min_depth_nse'])
#     print('Mean five-depth RSR = %.6f'
#           % best['mean_rsr'])
#     print('Mean absolute PBIAS = %.6f'
#           % best['mean_abs_pbias'])
#     print('Mean R2 = %.6f'
#           % best['mean_r2'])
#
#     # 只有 F_19 是辅助验证变量，不参与选择。
#     aux_rmse = _safe_metric(
#         sim_obs_data[best['idx']], AREA_VAR, 'RMSE'
#     )
#     aux_nse = _safe_metric(
#         sim_obs_data[best['idx']], AREA_VAR, 'NSE'
#     )
#     aux_rsr = _safe_metric(
#         sim_obs_data[best['idx']], AREA_VAR, 'RSR'
#     )
#     aux_pbias = _safe_metric(
#         sim_obs_data[best['idx']], AREA_VAR, 'PBIAS'
#     )
#     aux_r2 = _safe_metric(
#         sim_obs_data[best['idx']], AREA_VAR, 'R-square'
#     )
#
#     print('%s RMSE = %.6f, NSE = %.6f, RSR = %.6f, '
#           'PBIAS = %.6f, R2 = %.6f（辅助验证，不参与选参）'
#           % (
#               AREA_VAR,
#               aux_rmse,
#               aux_nse,
#               aux_rsr,
#               aux_pbias,
#               aux_r2
#           ))
#
#     if best['gene_values'] is not None:
#         print('Selected parameter values =', best['gene_values'])
#
#     print('================================================')
#     return best['idx'], best
#
# def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list,
#                                 ylows_list, yups_list):
#     """Calculate P-factor and R-factor at observation dates."""
#     count = 0
#     ylows_obs = []
#     yups_obs = []
#     valid_obs = []
#     sim_dates_py = list(sim_dates_list)
#
#     for oi, ov in enumerate(obs_data_list):
#         if not np.isfinite(ov) or oi >= len(obs_dates_list):
#             continue
#         try:
#             si = sim_dates_py.index(obs_dates_list[oi])
#         except Exception:
#             continue
#         if si >= len(ylows_list) or si >= len(yups_list):
#             continue
#         if not np.isfinite(ylows_list[si]) or not np.isfinite(yups_list[si]):
#             continue
#
#         ylows_obs.append(ylows_list[si])
#         yups_obs.append(yups_list[si])
#         valid_obs.append(ov)
#         if ylows_list[si] <= ov <= yups_list[si]:
#             count += 1
#
#     if len(valid_obs) == 0:
#         return np.nan, np.nan
#
#     p_value = float(count) / float(len(valid_obs))
#     ylows_obs = np.asarray(ylows_obs, dtype=float)
#     yups_obs = np.asarray(yups_obs, dtype=float)
#     valid_obs = np.asarray(valid_obs, dtype=float)
#     obs_std = np.nanstd(valid_obs)
#     if not np.isfinite(obs_std) or obs_std == 0.0:
#         r_value = np.nan
#     else:
#         r_value = np.nanmean(yups_obs - ylows_obs) / obs_std
#     return p_value, r_value
#
#
# # ============================================================================
# # 95PPU 与统一参数结果绘图
# # ============================================================================
# def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
#                     vali_sim_obs_data=None, vali_sim_data=None,
#                     plot_cfg=None  # type: Optional[PlotConfig]
#                     ):
#     """Calculate 95% prediction uncertainty and plot all variables."""
#     if plot_cfg is None:
#         plot_cfg = PlotConfig()
#
#     plt.rcParams['xtick.direction'] = 'out'
#     plt.rcParams['ytick.direction'] = 'out'
#     plt.rcParams['font.family'] = plot_cfg.font_name
#     plt.rcParams['timezone'] = 'UTC'
#     plt.rcParams['mathtext.fontset'] = 'custom'
#     plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
#     plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
#
#     if sim_obs_data is None or sim_data is None:
#         print('calculate_95ppu: sim_obs_data 或 sim_data 为空，跳过画图。')
#         return
#     if len(sim_data) < 2:
#         print('calculate_95ppu: 当前代有效模拟个体少于 2 个，跳过 95PPU 画图。')
#         return
#     if len(sim_obs_data) == 0:
#         print('calculate_95ppu: sim_obs_data 为空，跳过画图。')
#         return
#
#     var_name = sim_obs_data[0]['var_name']
#     nvar = len(var_name)
#     print('calculate_95ppu: var_name =', var_name)
#     print('calculate_95ppu: nvar =', nvar)
#     print('calculate_95ppu: simulations =', len(sim_data))
#
#     # 检查全部需要绘图/验证的变量是否存在
#     required_vars = [AREA_VAR] + PLOT_DEPTH_VARS
#     missing_vars = [v for v in required_vars if v not in var_name]
#     if len(missing_vars) > 0:
#         raise ValueError(
#             '变量列表中缺少用于率定结果展示或辅助验证的变量：%s'
#             % str(missing_vars)
#         )
#
#     diagnose_best_rmse_for_each_variable(sim_obs_data)
#     shared_best_idx, best_info = select_shared_best_index(sim_obs_data)
#
#     # 将当前代所选出的统一参数及其指标单独保存。
#     # 最终正式模拟应使用最后一代对应文件中的参数值。
#     selected_file = os.path.join(
#         outdir, 'Gen%d_selected_shared_parameter_set.txt' % gen_num
#     )
#     with open(selected_file, 'w', encoding='utf-8') as f:
#         f.write(
#             'Selection rule: five-depth mixed objectives; '
#             'maximize NSE for D_1304, D_1305, D_1306 and D_1309; '
#             'minimize RMSE for D_1310. '
#             'Each objective is normalized within the current candidate set. '
#             'Minimize the worst normalized loss first, then the mean normalized '
#             'loss and the ideal-point distance. F_19 is validation only.\n'
#         )
#         f.write('Source generation: %s\n' % str(best_info['gen']))
#         f.write('Source calibration ID: %s\n' % str(best_info['id']))
#
#         for i, var in enumerate(CALIB_DEPTH_VARS):
#             f.write('%s RMSE: %.6f\n'
#                     % (var, best_info['calib_depth_rmses'][i]))
#             f.write('%s NSE: %.6f\n'
#                     % (var, best_info['calib_depth_nses'][i]))
#             f.write('%s RSR: %.6f\n'
#                     % (var, best_info['calib_depth_rsr'][i]))
#             f.write('%s PBIAS: %.6f\n'
#                     % (var, best_info['calib_depth_pbias'][i]))
#             f.write('%s R2: %.6f\n'
#                     % (var, best_info['calib_depth_r2'][i]))
#
#         f.write('Mean calibration-depth RMSE: %.6f\n'
#                 % best_info['mean_depth_rmse'])
#         f.write('Maximum calibration-depth RMSE: %.6f\n'
#                 % best_info['max_depth_rmse'])
#         f.write('Minimum calibration-depth RMSE: %.6f\n'
#                 % best_info['min_depth_rmse'])
#         f.write('Mean calibration-depth NSE: %.6f\n'
#                 % best_info.get('mean_depth_nse', np.nan))
#         f.write('Minimum calibration-depth NSE: %.6f\n'
#                 % best_info.get('min_depth_nse', np.nan))
#         f.write('Mean calibration-depth RSR: %.6f\n'
#                 % best_info.get('mean_rsr', np.nan))
#         f.write('Mean absolute PBIAS: %.6f\n'
#                 % best_info.get('mean_abs_pbias', np.nan))
#         f.write('Mean R2: %.6f\n'
#                 % best_info.get('mean_r2', np.nan))
#         f.write('Four-NSE mean: %.6f\n'
#                 % best_info.get('mean_nse_objective', np.nan))
#         f.write('Four-NSE minimum: %.6f\n'
#                 % best_info.get('min_nse_objective', np.nan))
#         f.write('D_1310 objective RMSE: %.6f\n'
#                 % best_info.get('rmse_1310', np.nan))
#         f.write('Normalized objective losses: %s\n'
#                 % str(best_info.get('normalized_objective_losses', [])))
#         f.write('Worst normalized loss: %.6f\n'
#                 % best_info.get('worst_normalized_loss', np.nan))
#         f.write('Mean normalized loss: %.6f\n'
#                 % best_info.get('mean_normalized_loss', np.nan))
#         f.write('Ideal-point distance: %.6f\n'
#                 % best_info.get('ideal_distance', np.nan))
#
#         # F_19 辅助验证指标也写入文件，但不参与选择。
#         aux_vars = (
#             [v for v in PLOT_DEPTH_VARS if v not in CALIB_DEPTH_VARS]
#             + [AREA_VAR]
#         )
#         for aux_var in aux_vars:
#             aux_rmse = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'RMSE'
#             )
#             aux_nse = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'NSE'
#             )
#             aux_rsr = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'RSR'
#             )
#             aux_pbias = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'PBIAS'
#             )
#             aux_r2 = _safe_metric(
#                 sim_obs_data[shared_best_idx], aux_var, 'R-square'
#             )
#
#             f.write('%s RMSE: %.6f (validation only)\n'
#                     % (aux_var, aux_rmse))
#             f.write('%s NSE: %.6f (validation only)\n'
#                     % (aux_var, aux_nse))
#             f.write('%s RSR: %.6f (validation only)\n'
#                     % (aux_var, aux_rsr))
#             f.write('%s PBIAS: %.6f (validation only)\n'
#                     % (aux_var, aux_pbias))
#             f.write('%s R2: %.6f (validation only)\n'
#                     % (aux_var, aux_r2))
#
#         f.write('Parameter values: %s\n'
#                 % str(best_info['gene_values']))
#
#     for idx, var in enumerate(var_name):
#         print('================================================')
#         print('开始画变量:', var, 'idx =', idx)
#
#         plot_validation = bool(vali_sim_obs_data and vali_sim_data and
#                                len(vali_sim_obs_data) > 0 and
#                                var in vali_sim_obs_data[0].get('var_name', []))
#
#         ylabel_str = var
#         if var in ['Q', 'QI', 'QG', 'QS']:
#             ylabel_str += ' (m$^3$/s)'
#         elif var.startswith('D_') or var == 'D':
#             ylabel_str += ' (m)'
#         elif var.startswith('F_') or var == 'F':
#             ylabel_str += ' (km$^2$)'
#         elif 'CONC' in var.upper():
#             ylabel_str += ' (g/L)' if 'SED' in var.upper() else ' (mg/L)'
#         elif 'SED' in var.upper():
#             ylabel_str += ' (kg)'
#
#         if var not in sim_obs_data[0]:
#             print('变量 %s 不在 sim_obs_data[0] 中，跳过。' % var)
#             continue
#
#         # 1. Read calibration observations.
#         cali_obs_dates = _to_datetime_list(sim_obs_data[0][var]['UTCDATETIME'][:])
#         cali_obs_values = sim_obs_data[0][var]['Obs'][:]
#         obs_dates = cali_obs_dates[:]
#         obs_data = list(cali_obs_values)
#         order = 1
#
#         # 2. Read validation observations.
#         vali_obs_dates = []
#         vali_obs_values = []
#         same_cali_vali_period = False
#         if plot_validation:
#             vali_obs_dates = _to_datetime_list(vali_sim_obs_data[0][var]['UTCDATETIME'][:])
#             vali_obs_values = vali_sim_obs_data[0][var]['Obs'][:]
#             same_cali_vali_period = _is_same_period(cali_obs_dates, vali_obs_dates)
#             if not same_cali_vali_period and len(vali_obs_dates) > 0 and len(cali_obs_dates) > 0:
#                 if vali_obs_dates[-1] <= cali_obs_dates[0]:
#                     order = 0
#                     obs_dates = vali_obs_dates + obs_dates
#                     obs_data = list(vali_obs_values) + obs_data
#                 else:
#                     obs_dates = obs_dates + vali_obs_dates
#                     obs_data = obs_data + list(vali_obs_values)
#
#         obs_dates = np.asarray(obs_dates)
#         obs_data = _safe_array(obs_data)
#
#         # 3. Read simulation dates.
#         cali_sim_dates = _to_datetime_list(list(sim_data[0].keys()))
#         sim_dates = cali_sim_dates[:]
#         vali_sim_dates = []
#         if plot_validation:
#             vali_sim_dates = _to_datetime_list(list(vali_sim_data[0].keys()))
#             if not same_cali_vali_period:
#                 sim_dates = sim_dates + vali_sim_dates if order else vali_sim_dates + sim_dates
#
#         # 4. Collect current-variable simulations of all candidate parameter sets.
#         sim_data_list = []
#         for idx2, ind in enumerate(sim_data):
#             _, tmp_mat = dict_to_var_matrix(ind, nvar)
#             if idx >= tmp_mat.shape[1]:
#                 print('变量 %s 的 idx=%d 超出模拟矩阵列数 %d，跳过该个体。'
#                       % (var, idx, tmp_mat.shape[1]))
#                 continue
#             tmpsim = tmp_mat[:, idx].tolist()
#
#             if plot_validation and not same_cali_vali_period:
#                 if idx2 < len(vali_sim_data):
#                     _, val_mat = dict_to_var_matrix(vali_sim_data[idx2], nvar)
#                     tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                         else [np.nan] * len(vali_sim_dates)
#                 else:
#                     tmp_data = [np.nan] * len(vali_sim_dates)
#                 tmpsim = tmpsim + tmp_data if order else tmp_data + tmpsim
#             sim_data_list.append(tmpsim)
#
#         if len(sim_data_list) == 0:
#             print('变量 %s 没有可用模拟数据，跳过画图。' % var)
#             continue
#
#         # 5. Simulation by the single selected parameter set.
#         _, best_mat = dict_to_var_matrix(sim_data[shared_best_idx], nvar)
#         sim_best = best_mat[:, idx].tolist()
#         if plot_validation and not same_cali_vali_period:
#             if shared_best_idx < len(vali_sim_data):
#                 _, val_mat = dict_to_var_matrix(vali_sim_data[shared_best_idx], nvar)
#                 tmp_data = val_mat[:, idx].tolist() if idx < val_mat.shape[1] \
#                     else [np.nan] * len(vali_sim_dates)
#             else:
#                 tmp_data = [np.nan] * len(vali_sim_dates)
#             sim_best = sim_best + tmp_data if order else tmp_data + sim_best
#
#         # 6. Calculate 95PPU using all candidate parameter sets.
#         try:
#             sim_data_array = np.asarray(sim_data_list, dtype=float)
#             ylows = np.nanpercentile(sim_data_array, 2.5, axis=0)
#             yups = np.nanpercentile(sim_data_array, 97.5, axis=0)
#         except Exception as e:
#             print('变量 %s 计算 nanpercentile 失败：%s' % (var, e))
#             continue
#
#         sim_dates = np.asarray(sim_dates)
#         sim_best = np.asarray(sim_best, dtype=float)
#         ylows = np.asarray(ylows, dtype=float)
#         yups = np.asarray(yups, dtype=float)
#         min_len = min(len(sim_dates), len(sim_best), len(ylows), len(yups))
#         sim_dates = sim_dates[:min_len]
#         sim_best = sim_best[:min_len]
#         ylows = ylows[:min_len]
#         yups = yups[:min_len]
#         if min_len == 0:
#             print('变量 %s 的模拟序列长度为 0，跳过画图。' % var)
#             continue
#
#         # 7. Calculate P-factor and R-factor.
#         p_value, r_value = calculate_95ppu_efficiency(
#             _safe_array(sim_obs_data[0][var]['Obs']), cali_obs_dates,
#             sim_dates, ylows, yups)
#
#         # 8. Metrics from the same selected parameter set.
#         best_nse = _safe_metric(sim_obs_data[shared_best_idx], var, 'NSE')
#         best_rmse = _safe_metric(sim_obs_data[shared_best_idx], var, 'RMSE')
#         best_rsr = _safe_metric(sim_obs_data[shared_best_idx], var, 'RSR')
#         best_pbias = _safe_metric(sim_obs_data[shared_best_idx], var, 'PBIAS')
#         best_r2 = _safe_metric(sim_obs_data[shared_best_idx], var, 'R-square')
#
#         p_value_v = np.nan
#         r_value_v = np.nan
#         best_nse_v = np.nan
#         best_rmse_v = np.nan
#         best_rsr_v = np.nan
#         best_pbias_v = np.nan
#         best_r2_v = np.nan
#         if plot_validation and not same_cali_vali_period:
#             p_value_v, r_value_v = calculate_95ppu_efficiency(
#                 _safe_array(vali_sim_obs_data[0][var]['Obs']), vali_obs_dates,
#                 sim_dates, ylows, yups)
#             best_nse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'NSE')
#             best_rmse_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RMSE')
#             best_rsr_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'RSR')
#             best_pbias_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'PBIAS')
#             best_r2_v = _safe_metric(vali_sim_obs_data[shared_best_idx], var, 'R-square')
#
#         print('变量 =', var)
#         print('same_cali_vali_period =', same_cali_vali_period)
#         print('plot_validation =', plot_validation)
#         print('len(sim_dates) =', len(sim_dates))
#         print('len(obs_dates) =', len(obs_dates))
#         print('sim_best NaN 个数 =', np.isnan(sim_best).sum())
#         print('ylows NaN 个数 =', np.isnan(ylows).sum())
#         print('yups NaN 个数 =', np.isnan(yups).sum())
#         if len(sim_best) > 0 and np.isfinite(sim_best).any():
#             print('sim_best min/max =', np.nanmin(sim_best), np.nanmax(sim_best))
#
#         # 9. Determine displayed dates.
#         if PLOT_ONLY_OBS_DATES:
#             mask = np.isin(sim_dates, obs_dates)
#         else:
#             mask = np.ones(len(sim_dates), dtype=bool)
#
#         if np.any(mask):
#             sim_dates_plot = sim_dates[mask]
#             sim_best_plot = sim_best[mask]
#             ylows_plot = ylows[mask]
#             yups_plot = yups[mask]
#         else:
#             print('变量 %s：模拟日期和观测日期没有匹配上，绘制完整模拟序列。' % var)
#             sim_dates_plot = sim_dates
#             sim_best_plot = sim_best
#             ylows_plot = ylows
#             yups_plot = yups
#
#         if len(sim_dates_plot) == 0:
#             print('变量 %s：绘图日期为空，跳过。' % var)
#             continue
#
#         # 10. Plot.
#         fig, ax = plt.subplots(figsize=(14, 4.8))
#         ax.fill_between(sim_dates_plot, ylows_plot.tolist(), yups_plot.tolist(),
#                         color=(0.8, 0.8, 0.8), label='95PPU', zorder=1)
#         observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
#         ax.scatter(obs_dates, obs_data, marker='.', s=20, color='g',
#                    label=observed_label, zorder=4)
#         bestsim_label = u'统一参数组模拟' if plot_cfg.plot_cn else 'Shared-parameter simulation'
#         ax.plot(sim_dates_plot, sim_best_plot, linestyle='--', color='red', linewidth=1.5,
#                 label=bestsim_label, zorder=5)
#         ax.set_xlim(left=min(sim_dates_plot), right=max(sim_dates_plot))
#
#         y_parts = []
#         for values in [obs_data, sim_best_plot, ylows_plot, yups_plot]:
#             if np.isfinite(values).any():
#                 y_parts.append(values[np.isfinite(values)])
#         if len(y_parts) > 0:
#             all_y = np.concatenate(y_parts)
#             y_high = np.nanpercentile(all_y, 99)
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = np.nanmax(all_y) if np.isfinite(all_y).any() else 0.01
#             if not np.isfinite(y_high) or y_high <= 0:
#                 y_high = 0.01
#             ax.set_ylim(bottom=0.0, top=y_high + 0.08 * y_high)
#         else:
#             ax.set_ylim(bottom=0.0, top=0.01)
#
#         ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d-%y'))
#         ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         ax.tick_params(axis='y', left=True, right=False, length=5, width=2,
#                        which='major', labelsize=plot_cfg.tick_fsize)
#         plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
#                    fontsize=plot_cfg.axislabel_fsize)
#         plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
#
#         if plot_validation and not same_cali_vali_period and len(vali_sim_dates) > 0 and len(cali_sim_dates) > 0:
#             sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
#                 else cali_sim_dates[0]
#             ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=1.5, zorder=3)
#             if order:
#                 left_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#                 right_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#             else:
#                 left_label = u'验证期' if plot_cfg.plot_cn else 'Validation'
#                 right_label = u'率定期' if plot_cfg.plot_cn else 'Calibration'
#             ax.text(0.02, 0.92, left_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#             ax.text(0.52, 0.92, right_label, transform=ax.transAxes,
#                     fontdict={'style': 'italic', 'weight': 'bold',
#                               'size': max(plot_cfg.label_fsize - 2, 9)},
#                     color='black', va='top', ha='left')
#
#         handles, labels = ax.get_legend_handles_labels()
#         figorders = []
#         for lab in ['95PPU', observed_label, bestsim_label]:
#             if lab in labels:
#                 figorders.append(labels.index(lab))
#         if figorders:
#             ax.legend([handles[i] for i in figorders], [labels[i] for i in figorders],
#                       fontsize=max(plot_cfg.legend_fsize - 2, 8), loc='upper left', framealpha=0.8)
#
#         cali_text = (
#             'Calibration\n'
#             'P-factor: %s\n'
#             'R-factor: %s\n'
#             'NSE: %s\n'
#             'RMSE: %s\n'
#             'RSR: %s\n'
#             'PBIAS: %s%%\n'
#             'R$^2$: %s'
#             % (
#                 _format_metric(p_value),
#                 _format_metric(r_value),
#                 _format_metric(best_nse),
#                 _format_metric(best_rmse),
#                 _format_metric(best_rsr),
#                 _format_metric(best_pbias),
#                 _format_metric(best_r2)
#             )
#         )
#         if plot_validation and not same_cali_vali_period:
#             vali_text = (
#                 '\n\nValidation\n'
#                 'P-factor: %s\n'
#                 'R-factor: %s\n'
#                 'NSE: %s\n'
#                 'RMSE: %s\n'
#                 'RSR: %s\n'
#                 'PBIAS: %s%%\n'
#                 'R$^2$: %s'
#                 % (
#                     _format_metric(p_value_v),
#                     _format_metric(r_value_v),
#                     _format_metric(best_nse_v),
#                     _format_metric(best_rmse_v),
#                     _format_metric(best_rsr_v),
#                     _format_metric(best_pbias_v),
#                     _format_metric(best_r2_v)
#                 )
#             )
#             metric_text = cali_text + vali_text
#         else:
#             metric_text = cali_text
#
#         ax.text(0.985, 0.965, metric_text, transform=ax.transAxes, color='red',
#                 fontsize=max(plot_cfg.label_fsize - 3, 10), va='top', ha='right',
#                 linespacing=1.15,
#                 bbox=dict(facecolor='white', alpha=0.70, edgecolor='none'), zorder=10)
#
#         plt.tight_layout()
#         save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
#         plt.cla()
#         plt.clf()
#         plt.close()
#         print('变量 %s 画图完成。' % var)
#         print('================================================')
#
#
# # ============================================================================
# # 独立后处理入口：读取已经保存好的某一代结果后重新绘图
# # ============================================================================
# def load_generation_results(simdir, gen_num):
#     """Load saved calibration/validation results for one generation."""
#     cali_json = simdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
#     cali_pickle = simdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
#     with open(cali_json, 'r', encoding='utf-8') as f:
#         cali_obs_data = json.load(f)
#     with open(cali_pickle, 'rb') as f:
#         cali_data = pickle.load(f)
#
#     vali_obs_data = None
#     vali_data = None
#     vali_json = simdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
#     vali_pickle = simdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
#     if os.path.exists(vali_json) and os.path.exists(vali_pickle):
#         with open(vali_json, 'r', encoding='utf-8') as f:
#             vali_obs_data = json.load(f)
#         with open(vali_pickle, 'rb') as f:
#             vali_data = pickle.load(f)
#     return cali_obs_data, cali_data, vali_obs_data, vali_data
#
#
# if __name__ == '__main__':
#     # 注意：这里要改成你实际的率定结果文件夹。
#     # 你前面发的是 Gen100 图，所以这里先写 Gen_100_Pop_12；如果实际文件夹不是这个，需要手动改。
#     wp = r'D:\SEIMS_C\data\CW_2\CW_2_longterm_model\CALI_NSGA2_Gen_100_Pop_12'
#     simdir = wp + os.path.sep + 'simulated_data'
#
#     cali_obs_data, cali_data, vali_obs_data, vali_data = load_generation_results(
#         simdir, GEN_NUM_TO_PLOT)
#
#     if vali_obs_data is not None and vali_data is not None:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT,
#                         vali_obs_data, vali_data)
#     else:
#         calculate_95ppu(cali_obs_data, cali_data, simdir, GEN_NUM_TO_PLOT)
#
#     print('Total simulations %d' % len(cali_obs_data))
