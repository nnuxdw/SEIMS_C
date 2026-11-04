"""Base class of calibration.

    @author   : Liangjun Zhu

    @changelog:
    - 18-01-22  - lj - design and implement.
    - 18-01-25  - lj - redesign the individual class, add 95PPU, etc.
    - 18-02-09  - lj - compatible with Python3.
    - 20-07-22  - lj - update to use global MongoClient object.
"""
from __future__ import absolute_import, unicode_literals

import time
import bisect
from pygeoc.utils import UtilClass, FileClass, StringClass
from collections import OrderedDict
import os
import sys
from copy import deepcopy
from utility.scoop_func import scoop_log

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from typing import Optional
from pygeoc.utils import FileClass

from utility import read_data_items_from_txt
import global_mongoclient as MongoDBObj
from preprocess.text import DBTableNames
from run_seims import MainSEIMS
from calibration.config import CaliConfig, get_optimization_config
from calibration.sample_lhs import lhs
import numpy
from pathlib import Path
import time, random

from datetime import datetime


# # 会话ID，避免跨实例互相影响（强烈推荐）
# if os.name == 'nt':  # Windows
#     # base_path = r'D:\InstallationPath\SEIMS-master\data\TP_xiamenhe'
#     base_path = r'D:\SEIMS_C\data\CW_2'
#     # base_path = r'D:\InstallationPath\SEIMS-master\data\TP_xiyinghe'
#     # base_path = r'D:\SEIMS-master\data\TP_zamuhe'
#     # base_path = r'D:\SEIMS-master\data\TP_xiyinghe'
#     # base_path = r'D:\SEIMS-master\data\TP_Amu'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/SEIMS_C/TP_zamuhe'
#     # base_path = '/data/user/lijing/software/SEIMS_C/SEIMS_C/TP_Amu'
# # # 淹没范围才会用，淹没面积率定不用管
# subbasin_flood_path = base_path + f'/inundation_cali/subbasin_flood'
# subbasin_ids=[1171,1176,1193,1194,1214]
#
# # 不同的实例指定不同的id，避免锁文件相互影响
# RUN_ID = 'CW_2_1_20260425'
# # RUN_ID = 'TP_Amu_1_20260123'
# cali_inundation_extent = False
# cali_inundation_area = True
# copy_output = True
# # RUN_ID =  time.strftime("%Y%m%d-%H%M%S")
#
# """ xiaodw add
# scoop有个重发机制导致bug，就是比如一代有40个参数组合，10个子进程并行跑，其中一个子进程A可能跑的很慢，
# 主进程发现他很慢之后，就会重发一个子进程（B）跑一模一样的参数，然后A又跑完了，于是删除了A对应的率定文件夹 OUTPUT-A，
# 之后B跑完了发现 OUTPUT-A文件夹不存在，就报错了。
# """
# if os.name == "nt":
#     import msvcrt
#     def try_acquire_lock(path: str):
#         Path(path).parent.mkdir(parents=True, exist_ok=True)
#         fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o644)
#         try:
#             # 非阻塞：Windows 只能用“试锁”语义模拟（会抛 OSError 时表示失败）
#             msvcrt.locking(fd, msvcrt.LK_NBLCK, 1)
#             return fd
#         except OSError:
#             os.close(fd)
#             return None
#     def release_lock(fd: int):
#         try:
#             msvcrt.locking(fd, msvcrt.LK_UNLCK, 1)
#         finally:
#             os.close(fd)
# else:
#     import fcntl
#     def try_acquire_lock(path: str):
#         Path(path).parent.mkdir(parents=True, exist_ok=True)
#         fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o644)
#         try:
#             fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)  # 非阻塞
#             return fd
#         except BlockingIOError:
#             os.close(fd)
#             return None
#     def release_lock(fd: int):
#         try:
#             fcntl.flock(fd, fcntl.LOCK_UN)
#         finally:
#             os.close(fd)
#
# class TimeseriesData(object):
#     """Time series data, for observation and simulation data."""
#
#     def __init__(self):
#         self.vars = list()
#         self.data = OrderedDict()
#
#
# class ObsSimData(object):
#     """Paired time series data of observation and simulation, associated with statistics."""
#
#     def __init__(self):
#         self.vars = list()
#         self.data = OrderedDict()
#         self.sim_obs_data = OrderedDict()
#         self.objnames = list()
#         self.objvalues = list()
#         self.valid = False
#
#     def efficiency_values(self, varname, effnames):
#         values = list()
#         tmpvars = list()
#         for name in effnames:
#             tmpvar = '%s-%s' % (varname, name)
#             if tmpvar not in self.objnames:
#                 values.append(-9999.)
#                 # tmpvars.append(tmpvar)
#             else:
#                 if name.upper() == 'PBIAS':
#                     tmpvars.append('%s-abs(PBIAS)' % varname)
#                 else:
#                     tmpvars.append(tmpvar)
#                 values.append(self.objvalues[self.objnames.index(tmpvar)]) #evaluate index, i.e., NSE
#         return values, tmpvars
#
#     def output_header(self, varname, effnames, prefix=''):
#         concate = ''
#         for name in effnames:
#             tmpvar = '%s-%s' % (varname, name)
#             if tmpvar not in self.objnames:
#                 concate += '\t'
#             else:
#                 if name.upper() == 'PBIAS':
#                     tmpvar = '%s-abs(PBIAS)' % varname
#                 if prefix != '':
#                     concate += '%s-%s\t' % (prefix, tmpvar)
#                 else:
#                     concate += '%s\t' % tmpvar
#         return concate
#
#     def output_efficiency(self, varname, effnames):
#         concate = ''
#         for name in effnames:
#             tmpvar = '%s-%s' % (varname, name)
#             if tmpvar not in self.objnames:
#                 concate += '\t'
#             else:
#                 concate += '%.3f\t' % self.objvalues[self.objnames.index(tmpvar)]
#         return concate
#
#
# class Calibration(object):
#     """Base class of automatic calibration.
#
#     Attributes:
#         ID(integer): Calibration ID in current generation, range from 0 to N-1(individuals).
#         modelrun(boolean): Has SEIMS model run successfully?
#     """
#
#     def __init__(self, cali_cfg, id=-1):
#         # type: (CaliConfig, Optional[int]) -> None
#         """Initialize."""
#         self.cfg = cali_cfg
#         self.model = cali_cfg.model
#         self.ID = id
#         self.param_defs = dict()
#         # run seims related
#         self.modelrun = False
#         self.reset_simulation_timerange()
#
#     @property
#     def ParamDefs(self):
#         """Read cali_param_rng.def file
#
#            name,lower_bound,upper_bound
#
#             e.g.,
#              Param1,0,1
#              Param2,0.5,1.2
#              Param3,-1.0,1.0
#
#         Returns:
#             a dictionary containing:
#             - names - the names of the parameters
#             - bounds - a list of lists of lower and upper bounds
#             - num_vars - a scalar indicating the number of variables
#                          (the length of names)
#         """
#         # read param_defs.json if already existed
#         if self.param_defs:
#             return self.param_defs
#         # read param_range_def file and output to json file
#         conn = MongoDBObj.client
#         db = conn[self.cfg.model.db_name]
#         collection = db['PARAMETERS']
#
#         names = list()
#         bounds = list()
#         num_vars = 0
#         if not FileClass.is_file_exists(self.cfg.param_range_def):
#             raise ValueError('Parameters definition file: %s is not'
#                              ' existed!' % self.cfg.param_range_def)
#         items = read_data_items_from_txt(self.cfg.param_range_def)
#         for item in items:
#             if len(item) < 3:
#                 continue
#             # find parameter name, print warning message if not existed
#             cursor = collection.find({'NAME': item[0]}, no_cursor_timeout=True)
#             if not cursor.count():
#                 print('WARNING: parameter %s is not existed!' % item[0])
#                 continue
#             num_vars += 1
#             names.append(item[0])
#             bounds.append([float(item[1]), float(item[2])])
#         self.param_defs = {'names': names, 'bounds': bounds, 'num_vars': num_vars}
#         return self.param_defs
#
#     def reset_simulation_timerange(self):
#         """Update simulation time range in MongoDB [FILE_IN]."""
#         conn = MongoDBObj.client
#         db = conn[self.cfg.model.db_name]
#         stime_str = self.cfg.model.simu_stime.strftime('%Y-%m-%d %H:%M:%S')
#         etime_str = self.cfg.model.simu_etime.strftime('%Y-%m-%d %H:%M:%S')
#         db[DBTableNames.main_filein].find_one_and_update({'TAG': 'STARTTIME'},
#                                                          {'$set': {'VALUE': stime_str}})
#         db[DBTableNames.main_filein].find_one_and_update({'TAG': 'ENDTIME'},
#                                                          {'$set': {'VALUE': etime_str}})
#
#     def initialize(self, n=1):
#         """Initialize parameters samples by Latin-Hypercube sampling method.
#
#         Returns:
#             A list contains parameter value at each gene location.
#         """
#         param_num = self.ParamDefs['num_vars']
#         lhs_samples = lhs(param_num, n)
#         all = list()
#         for idx in range(n):
#             gene_values = list()
#             for i, param_bound in enumerate(self.ParamDefs['bounds']):
#                 gene_values.append(lhs_samples[idx][i] * (param_bound[1] - param_bound[0]) +
#                                    param_bound[0])
#             all.append(gene_values)
#         return all
#
#
# def initialize_calibrations(cf):
#     """Initial individual of population.
#     """
#     cali = Calibration(cf)
#     return cali.initialize()
#
# """
#     xiaodw add
#     按观测值有效性过滤：只保留 Obs 是有限数(且不等于无效填充值)的时刻，
#     同步过滤 Sim 和 UTCDATETIME。
# """
# def _filter_sim_by_obs(sim_obs_dict, invalid_values=(-9999.0,)):
#     """
#     按观测值有效性过滤：只保留 Obs 是有限数(且不等于无效填充值)的时刻，
#     同步过滤 Sim 和 UTCDATETIME，并打印被过滤掉的时间。
#     """
#     out = {}
#     for param, d in sim_obs_dict.items():
#         t = d.get('UTCDATETIME', [])
#         obs = d.get('Obs', [])
#         sim = d.get('Sim', [])
#
#         obs_arr = numpy.array(obs, dtype=float)
#         sim_arr = numpy.array(sim, dtype=float)
#
#         # 掩膜：有限数 且 不等于无效值
#         mask = numpy.isfinite(obs_arr)
#         for v in (invalid_values or ()):
#             mask &= (obs_arr != v)
#
#         # 找到被过滤掉的时间
#         dropped_times = [t[i] for i in range(len(t)) if i < len(mask) and not mask[i]]
#         if dropped_times:
#             print(f"[{param}] 被过滤掉的时间点: {dropped_times}")
#
#         # 应用掩膜
#         t_f   = [t[i] for i in range(len(t)) if i < len(mask) and mask[i]]
#         obs_f = obs_arr[mask].tolist()
#         sim_f = sim_arr[mask].tolist()
#
#         out[param] = {
#             'UTCDATETIME': t_f,
#             'Obs': obs_f,
#             'Sim': sim_f,
#         }
#     return out
#
#
# def evaluate_nowait_or_skip(cali_obj, ind):
#     """
#     非阻塞加锁：
#       - 成功：执行 calibration_objectives 并返回 ind
#       - 失败：说明已有同 (gen,id) 在跑 → 直接返回 None（表示跳过，不纳入结果）
#     """
#     fd = try_acquire_lock(str(_lock_path_for(ind, cali_obj)))
#     if fd is None:
#         # 被别的 worker 占用，按你的要求：不执行、不回收，不纳入 invalid_pops
#         # 可选打印一行便于观察：
#         scoop_log(f"[SKIP] gen={getattr(ind,'gen',0)} id={ind.id} 已在运行，跳过本次。")
#         return -1
#     try:
#         scoop_log(f"[RUN] gen={getattr(ind, 'gen', 0)} id={ind.id} 开始运行。")
#         return calibration_objectives(cali_obj, ind)
#     finally:
#         release_lock(fd)
#
# import numpy as np
# import rasterio
#
# def _read_binary_extent_tif(path, nodata=None, threshold=0.5):
#     """读取淹没范围栅格并转为二值（1=淹没，0=未淹没）。"""
#     with rasterio.open(path) as ds:
#         arr = ds.read(1)
#         ndv = ds.nodata if ds.nodata is not None else nodata
#     if ndv is not None:
#         arr = np.where(arr == ndv, np.nan, arr)
#     # 若原始是概率/水深，可用阈值二值化；若已是0/1，这一步等价透传
#     bin_arr = (arr >= threshold).astype(np.uint8)
#     bin_arr[np.isnan(arr)] = 255  # 用 255 做 mask，便于后面剔除
#     return bin_arr
#
#
# def _fi_bi_from_binary(sim_bin, obs_bin):
#     """给定同尺寸二值（0/1），计算 FI 与 BI。255 视为无效掩膜。"""
#     mask = (sim_bin != 255) & (obs_bin != 255)
#     s = sim_bin[mask].astype(np.uint8)
#     o = obs_bin[mask].astype(np.uint8)
#     tp = np.sum((s == 1) & (o == 1))
#     fp = np.sum((s == 1) & (o == 0))
#     fn = np.sum((s == 0) & (o == 1))
#     # Dice / F1
#     denom = (2*tp + fp + fn)
#     fi = (2*tp / denom) if denom > 0 else 0.0
#     # Bias = (TP+FP)/(TP+FN)；若观测全为0则回退为1（中性）
#     obs_area = tp + fn
#     sim_area = tp + fp
#     if obs_area == 0 and sim_area == 0:
#         bi = 1.0
#     elif obs_area == 0:
#         bi = np.inf
#     else:
#         bi = sim_area / obs_area
#     return fi, bi
#
# def _lock_path_for(ind, cali_obj):
#     gen = getattr(ind, "gen", 0)
#     # lock = Path(base_path + "/lock/nsga2_locks") / f"{RUN_ID}_g{gen}_id{ind.id}.lock"
#     lock_path = Path(base_path + "/" + cali_obj.model.db_name + "/lock/nsga2_locks") / f"{RUN_ID}_g{gen}_id{ind.id}.lock"
#     return lock_path
#
# def _done_path_for(ind, cali_obj):
#     gen = getattr(ind, "gen", 0)
#     # lock_path = Path(base_path + "/lock/nsga2_done") / f"{RUN_ID}_g{gen}_id{ind.id}.done"
#     lock_path = Path(base_path + "/" + cali_obj.model.db_name + "/lock/nsga2_done") / f"{RUN_ID}_g{gen}_id{ind.id}.lock"
#     return lock_path
#
# def _mark_done(done_path):
#     done_path.parent.mkdir(parents=True, exist_ok=True)
#     open(done_path, "w").close()  # 空文件即表示 DONE
#
# def evaluate_blocking(cali_obj, ind, timeout_s=2400):
#     lock_path = str(_lock_path_for(ind, cali_obj))
#     done_path = _done_path_for(ind, cali_obj)
#
#     owner = f"pid={os.getpid()}"
#     start = time.time()
#     while True:
#         # 1) 已完成？立刻返回，避免再尝试拿锁
#         if done_path.exists():
#             scoop_log(f"[DONE-HIT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已完成，直接返回")
#             return -1  # 也可换成 "__DONE__"
#
#         # 2) 尝试拿锁
#         fd = try_acquire_lock(lock_path)
#         if fd is not None:
#             try:
#                 # 拿到锁后再检查一遍 DONE（极端竞态下，别人刚写好）
#                 if done_path.exists():
#                     scoop_log(f"[DONE-INSIDE][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已完成，释放锁并返回")
#                     return -1
#
#                 scoop_log(f"[RUN][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 开始运行。")
#                 gen = getattr(ind,'gen',0)
#                 res = calibration_objectives(cali_obj, gen, ind)
#                 # res = calibration_objectives(cali_obj, ind)
#
#                 # 3) 成功完成 → 写入 DONE 标记（在释放锁之前写，确保可见）
#                 _mark_done(done_path)
#                 return res
#             finally:
#                 release_lock(fd)
#
#         # 4) 没拿到锁，看看是否完成或超时
#         if done_path.exists():
#             scoop_log(f"[DONE-WAIT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已完成(等待中发现)，返回")
#             return -1
#
#         elapsed = time.time() - start
#         if timeout_s is not None and elapsed > timeout_s:
#             scoop_log(f"[TIMEOUT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 等待锁超时，返回 -1")
#             return -1
#
#         # 5) 短退避 + 进度日志
#         scoop_log(f"[WAIT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已等待 {elapsed:.1f}s，10s 后重试…")
#         time.sleep(10)
#
# def calibration_objectives(cali_obj, gen, ind):
# # def calibration_objectives(cali_obj, ind):
#     """Evaluate the objectives of given individual.
#     """
#     # gen = getattr(ind, 'gen', 0)
#
#     cali_obj.ID = ind.id
#     model_args = cali_obj.model.ConfigDict
#     model_args.setdefault('calibration_id', -1)
#     model_args['calibration_id'] = ind.id
#     model_obj = MainSEIMS(args_dict=model_args)
#
#     # Set observation data to model_obj, no need to query database
#     model_obj.SetOutletObservations(ind.obs.vars, ind.obs.data)
#
#     # Execute model
#     # model_obj.SetMongoClient()
#     # model_obj.run()
#
#     model_obj.SetMongoClient()
#
#     # ===== 打印当前这组率定参数 =====
#     param_names = cali_obj.ParamDefs['names']
#     param_values = list(ind)
#
#     print(f"[PARAM] gen={gen}, id={ind.id}, n_params={len(param_values)}", flush=True)
#
#     for pname, pval in zip(param_names, param_values):
#         print(f"[PARAM] gen={gen}, id={ind.id}, {pname} = {pval}", flush=True)
#
#
#     # 保存到 CSV
#     debug_param_log = r"D:\SEIMS_C\data\CW_2\debug_param_sets0425.csv"
#
#     header = "gen,id," + ",".join(param_names) + "\n"
#     row = f"{gen},{ind.id}," + ",".join([str(v) for v in param_values]) + "\n"
#
#     if not os.path.exists(debug_param_log):
#         with open(debug_param_log, "w", encoding="utf-8") as f:
#             f.write(header)
#     with open(debug_param_log, "a", encoding="utf-8") as f:
#         f.write(row)
#
#     # ===== 开始运行模型 =====
#     print(f"[DEBUG] BEFORE model_obj.run(), gen={gen}, id={ind.id}", flush=True)
#     model_obj.run()
#     print(f"[DEBUG] AFTER model_obj.run(), gen={gen}, id={ind.id}", flush=True)
#
#
#     time.sleep(0.1)  # Wait a moment in case of unpredictable file system error
#
#     # read simulation data of the entire simulation period (include calibration and validation)
#     # if model_obj.ReadTimeseriesSimulations():
#     if model_obj.ReadTimeseriesSimulations_new():  # ljj++
#         ind.sim.vars = model_obj.sim_vars[:]
#         ind.sim.data = deepcopy(model_obj.sim_value)
#
#     else:
#         model_obj.clean(calibration_id=ind.id)
#         model_obj.UnsetMongoClient()
#         return ind
#
#
#     # Calculate NSE, R2, RMSE, PBIAS, and RSR, etc. of calibration period
#     print(f"cali_stime: {cali_obj.cfg.cali_stime}/n, cali_etime: {cali_obj.cfg.cali_etime}/n")
#     ind.cali.vars, ind.cali.data = model_obj.ExtractSimData(cali_obj.cfg.cali_stime,
#                                                             cali_obj.cfg.cali_etime)
#     # print(f"ind.cali.vars: {ind.cali.vars}/n, ind.cali.data: {ind.cali.data}/n")
#     ind.cali.sim_obs_data = model_obj.ExtractSimObsData(cali_obj.cfg.cali_stime,
#                                                         cali_obj.cfg.cali_etime)
#
#
#     ind.cali.objnames, \
#     ind.cali.objvalues = model_obj.CalcTimeseriesStatistics(ind.cali.sim_obs_data,
#                                                             cali_obj.cfg.cali_stime,
#                                                             cali_obj.cfg.cali_etime)
#     # 输出校准期指标
#     cali_metrics = ", ".join(f"{name}:{value:.4f}" for name, value in zip(ind.cali.objnames, ind.cali.objvalues))
#     print(f"cali: {cali_metrics}")
#     if ind.cali.objnames and ind.cali.objvalues:
#         ind.cali.valid = True
#     # Calculate NSE, R2, RMSE, PBIAS, and RSR, etc. of validation period
#     if cali_obj.cfg.calc_validation:
#         ind.vali.vars, ind.vali.data = model_obj.ExtractSimData(cali_obj.cfg.vali_stime,
#                                                                 cali_obj.cfg.vali_etime)
#         ind.vali.sim_obs_data = model_obj.ExtractSimObsData(cali_obj.cfg.vali_stime,
#                                                             cali_obj.cfg.vali_etime)
#
#         ind.vali.objnames, \
#         ind.vali.objvalues = model_obj.CalcTimeseriesStatistics(ind.vali.sim_obs_data,
#                                                                 cali_obj.cfg.vali_stime,
#                                                                 cali_obj.cfg.vali_etime)
#         if ind.vali.objnames and ind.vali.objvalues:
#             ind.vali.valid = True
#
#         # 输出验证期指标
#         vali_metrics = ", ".join(f"{name}:{value:.4f}" for name, value in zip(ind.vali.objnames, ind.vali.objvalues))
#         print(f"vali: {vali_metrics}")
#     # Get timespan
#     ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = model_obj.GetTimespan()
#     ##-----------------------xiaodw add, calibrate Fi,Bi for inundation extent-----------------------------
#     fi_list, bi_list = [], []
#     if cali_inundation_extent:
#         # cur = datetime.strptime(cali_obj.cfg.cali_stime, "%Y-%m-%d %H:%M:%S")
#         # end = datetime.strptime(cali_obj.cfg.cali_etime, "%Y-%m-%d %H:%M:%S")
#         cur = cali_obj.cfg.cali_stime
#         end = cali_obj.cfg.cali_etime
#         months = []
#         while cur <= end:
#             months.append((cur.year, cur.month))
#             # 跳到下一个月初
#             if cur.month == 12:
#                 cur = cur.replace(year=cur.year + 1, month=1, day=1)
#             else:
#                 cur = cur.replace(month=cur.month + 1, day=1)
#
#         for (yy, mm) in months:
#             obs_tif = os.path.join(cali_obj.cfg.flood_obs_dir, f"{yy}{mm:02d}.tif")
#             sim_tif = os.path.join(cali_obj.cfg.flood_sim_dir, f"run{ind.id}_{yy}{mm:02d}.tif")
#             if not (os.path.exists(obs_tif) and os.path.exists(sim_tif)):
#                 continue  # 该月缺数据则跳过
#
#             obs_bin = _read_binary_extent_tif(obs_tif, nodata=0, threshold=0.5)
#             sim_bin = _read_binary_extent_tif(sim_tif, nodata=0, threshold=0.5)
#
#             # 尺寸/投影需一致；如不一致，请在导出时保证一致，或此处先重投影/重采样
#             if obs_bin.shape != sim_bin.shape:
#                 # TODO: 统一投影与分辨率（建议在生成阶段统一）
#                 print(f"[WARN] shape mismatch for {yy}-{mm}, skip")
#                 continue
#
#             fi, bi = _fi_bi_from_binary(sim_bin, obs_bin)
#             fi_list.append(fi)
#             bi_list.append(bi)
#
#         # 汇总为月平均
#         if fi_list:
#             fi_mean = float(np.mean(fi_list))
#             # BI 偏差 |1-BI|，用于“越小越好”的目标
#             bi_dev_mean = float(np.mean([abs(1.0 - b) if np.isfinite(b) else 1.0 for b in bi_list]))
#
#             # 把新指标并入已有的校准期指标列表
#             names = list(getattr(ind.vali, "objnames", []))
#             vals = list(getattr(ind.vali, "objvalues", []))
#
#             names += ["FI", "BI_DEV"]  # FI 越大越好；BI_DEV = |1-BI| 越小越好
#             vals += [fi_mean, bi_dev_mean]
#
#             ind.vali.objnames = names
#             ind.vali.objvalues = vals
#
#             # 控制台打印
#             print(f"FI_mean={fi_mean:.4f}, BI_dev_mean={bi_dev_mean:.4f}")
#     ##-----------------------end calibrate Fi,Bi for inundation extent-----------------------------
#     # delete model output directory for saving storage
#     print(f"delete output directory: {ind.id}")
#     if copy_output:
#         model_obj.copy_dir(calibration_id=ind.id,gen = gen)
#     # model_obj.clean(calibration_id=ind.id)  ###删除文件
#     model_obj.UnsetMongoClient()
#
#     return ind
#
#
# if __name__ == '__main__':
#     cf, method = get_optimization_config()
#     cfg = CaliConfig(cf, method=method)
#
#     caliobj = Calibration(cfg)
#
#     # test the picklable of Scenario class.
#     import pickle
#
#     s = pickle.dumps(caliobj)
#     # print(s)
#     new_cali = pickle.loads(s)
#     print(new_cali.bin_dir)








# DEBUG_CALI_LOG = r"D:\SEIMS_C\data\CW_2\debug_cali.log"
# DEBUG_PARAM_LOG = r"D:\SEIMS_C\data\CW_2\debug_param_sets.csv"
#
#
# def logcali(msg):
#     s = f"{datetime.now().strftime('%Y-%m-%d %H:%M:%S')} {msg}"
#     print(s, flush=True)
#     with open(DEBUG_CALI_LOG, "a", encoding="utf-8") as f:
#         f.write(s + "\n")
#
#
# # 会话ID，避免跨实例互相影响
# if os.name == 'nt':
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:
#     base_path = '/data/user/lijing/software/SEIMS_C/SEIMS_C/TP_zamuhe'
#
# # 淹没范围才会用，淹没面积率定不用管
# subbasin_flood_path = base_path + f'/inundation_cali/subbasin_flood'
# subbasin_ids = [1171, 1176, 1193, 1194, 1214]
#
# # 动态 RUN_ID，避免旧锁/旧 done 干扰
# RUN_ID = time.strftime("%Y%m%d_%H%M%S")
# cali_inundation_extent = False
# cali_inundation_area = True
# copy_output = False
#
#
# if os.name == "nt":
#     import msvcrt
#
#     def try_acquire_lock(path: str):
#         Path(path).parent.mkdir(parents=True, exist_ok=True)
#         fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o644)
#         try:
#             msvcrt.locking(fd, msvcrt.LK_NBLCK, 1)
#             return fd
#         except OSError:
#             os.close(fd)
#             return None
#
#     def release_lock(fd: int):
#         try:
#             msvcrt.locking(fd, msvcrt.LK_UNLCK, 1)
#         finally:
#             os.close(fd)
# else:
#     import fcntl
#
#     def try_acquire_lock(path: str):
#         Path(path).parent.mkdir(parents=True, exist_ok=True)
#         fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o644)
#         try:
#             fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
#             return fd
#         except BlockingIOError:
#             os.close(fd)
#             return None
#
#     def release_lock(fd: int):
#         try:
#             fcntl.flock(fd, fcntl.LOCK_UN)
#         finally:
#             os.close(fd)
#
#
# class TimeseriesData(object):
#     """Time series data, for observation and simulation data."""
#
#     def __init__(self):
#         self.vars = list()
#         self.data = OrderedDict()
#
#
# class ObsSimData(object):
#     """Paired time series data of observation and simulation, associated with statistics."""
#
#     def __init__(self):
#         self.vars = list()
#         self.data = OrderedDict()
#         self.sim_obs_data = OrderedDict()
#         self.objnames = list()
#         self.objvalues = list()
#         self.valid = False
#
#     def efficiency_values(self, varname, effnames):
#         values = list()
#         tmpvars = list()
#         for name in effnames:
#             tmpvar = '%s-%s' % (varname, name)
#             if tmpvar not in self.objnames:
#                 values.append(-9999.)
#             else:
#                 if name.upper() == 'PBIAS':
#                     tmpvars.append('%s-abs(PBIAS)' % varname)
#                 else:
#                     tmpvars.append(tmpvar)
#                 values.append(self.objvalues[self.objnames.index(tmpvar)])
#         return values, tmpvars
#
#     def output_header(self, varname, effnames, prefix=''):
#         concate = ''
#         for name in effnames:
#             tmpvar = '%s-%s' % (varname, name)
#             if tmpvar not in self.objnames:
#                 concate += '\t'
#             else:
#                 if name.upper() == 'PBIAS':
#                     tmpvar = '%s-abs(PBIAS)' % varname
#                 if prefix != '':
#                     concate += '%s-%s\t' % (prefix, tmpvar)
#                 else:
#                     concate += '%s\t' % tmpvar
#         return concate
#
#     def output_efficiency(self, varname, effnames):
#         concate = ''
#         for name in effnames:
#             tmpvar = '%s-%s' % (varname, name)
#             if tmpvar not in self.objnames:
#                 concate += '\t'
#             else:
#                 concate += '%.3f\t' % self.objvalues[self.objnames.index(tmpvar)]
#         return concate
#
#
# class Calibration(object):
#     """Base class of automatic calibration."""
#
#     def __init__(self, cali_cfg, id=-1):
#         self.cfg = cali_cfg
#         self.model = cali_cfg.model
#         self.ID = id
#         self.param_defs = dict()
#         self.modelrun = False
#         self.reset_simulation_timerange()
#
#     @property
#     def ParamDefs(self):
#         """Read cali_param_rng.def file."""
#         if self.param_defs:
#             return self.param_defs
#
#         conn = MongoDBObj.client
#         db = conn[self.cfg.model.db_name]
#         collection = db['PARAMETERS']
#
#         names = list()
#         bounds = list()
#         num_vars = 0
#         if not FileClass.is_file_exists(self.cfg.param_range_def):
#             raise ValueError('Parameters definition file: %s is not existed!' % self.cfg.param_range_def)
#
#         items = read_data_items_from_txt(self.cfg.param_range_def)
#         for item in items:
#             if len(item) < 3:
#                 continue
#             cursor = collection.find({'NAME': item[0]}, no_cursor_timeout=True)
#             if not cursor.count():
#                 print('WARNING: parameter %s is not existed!' % item[0])
#                 continue
#             num_vars += 1
#             names.append(item[0])
#             bounds.append([float(item[1]), float(item[2])])
#
#         self.param_defs = {'names': names, 'bounds': bounds, 'num_vars': num_vars}
#         return self.param_defs
#
#     def reset_simulation_timerange(self):
#         """Update simulation time range in MongoDB [FILE_IN]."""
#         conn = MongoDBObj.client
#         db = conn[self.cfg.model.db_name]
#         stime_str = self.cfg.model.simu_stime.strftime('%Y-%m-%d %H:%M:%S')
#         etime_str = self.cfg.model.simu_etime.strftime('%Y-%m-%d %H:%M:%S')
#         db[DBTableNames.main_filein].find_one_and_update({'TAG': 'STARTTIME'},
#                                                          {'$set': {'VALUE': stime_str}})
#         db[DBTableNames.main_filein].find_one_and_update({'TAG': 'ENDTIME'},
#                                                          {'$set': {'VALUE': etime_str}})
#
#     def initialize(self, n=1):
#         """Initialize parameters samples by Latin-Hypercube sampling method."""
#         param_num = self.ParamDefs['num_vars']
#         lhs_samples = lhs(param_num, n)
#         all_values = list()
#         for idx in range(n):
#             gene_values = list()
#             for i, param_bound in enumerate(self.ParamDefs['bounds']):
#                 gene_values.append(lhs_samples[idx][i] * (param_bound[1] - param_bound[0]) +
#                                    param_bound[0])
#             all_values.append(gene_values)
#         return all_values
#
#
# def initialize_calibrations(cf):
#     """Initial individual of population."""
#     cali = Calibration(cf)
#     return cali.initialize()
#
#
# def _filter_sim_by_obs(sim_obs_dict, invalid_values=(-9999.0,)):
#     """
#     按观测值有效性过滤：只保留 Obs 是有限数(且不等于无效填充值)的时刻，
#     同步过滤 Sim 和 UTCDATETIME，并打印被过滤掉的时间。
#     """
#     out = {}
#     for param, d in sim_obs_dict.items():
#         t = d.get('UTCDATETIME', [])
#         obs = d.get('Obs', [])
#         sim = d.get('Sim', [])
#
#         obs_arr = numpy.array(obs, dtype=float)
#         sim_arr = numpy.array(sim, dtype=float)
#
#         mask = numpy.isfinite(obs_arr)
#         for v in (invalid_values or ()):
#             mask &= (obs_arr != v)
#
#         dropped_times = [t[i] for i in range(len(t)) if i < len(mask) and not mask[i]]
#         if dropped_times:
#             print(f"[{param}] 被过滤掉的时间点: {dropped_times}")
#
#         t_f = [t[i] for i in range(len(t)) if i < len(mask) and mask[i]]
#         obs_f = obs_arr[mask].tolist()
#         sim_f = sim_arr[mask].tolist()
#
#         out[param] = {
#             'UTCDATETIME': t_f,
#             'Obs': obs_f,
#             'Sim': sim_f,
#         }
#     return out
#
#
# def evaluate_nowait_or_skip(cali_obj, gen, ind):
#     """
#     非阻塞加锁：
#       - 成功：执行 calibration_objectives 并返回 ind
#       - 失败：说明已有同 (gen,id) 在跑 → 直接返回 -1
#     """
#     fd = try_acquire_lock(str(_lock_path_for(ind, cali_obj)))
#     if fd is None:
#         scoop_log(f"[SKIP] gen={gen} id={ind.id} 已在运行，跳过本次。")
#         return -1
#     try:
#         scoop_log(f"[RUN] gen={gen} id={ind.id} 开始运行。")
#         return calibration_objectives(cali_obj, gen, ind)
#     finally:
#         release_lock(fd)
#
#
# def _read_binary_extent_tif(path, nodata=None, threshold=0.5):
#     """读取淹没范围栅格并转为二值（1=淹没，0=未淹没）。"""
#     with rasterio.open(path) as ds:
#         arr = ds.read(1)
#         ndv = ds.nodata if ds.nodata is not None else nodata
#     if ndv is not None:
#         arr = np.where(arr == ndv, np.nan, arr)
#     bin_arr = (arr >= threshold).astype(np.uint8)
#     bin_arr[np.isnan(arr)] = 255
#     return bin_arr
#
#
# def _fi_bi_from_binary(sim_bin, obs_bin):
#     """给定同尺寸二值（0/1），计算 FI 与 BI。255 视为无效掩膜。"""
#     mask = (sim_bin != 255) & (obs_bin != 255)
#     s = sim_bin[mask].astype(np.uint8)
#     o = obs_bin[mask].astype(np.uint8)
#     tp = np.sum((s == 1) & (o == 1))
#     fp = np.sum((s == 1) & (o == 0))
#     fn = np.sum((s == 0) & (o == 1))
#     denom = (2 * tp + fp + fn)
#     fi = (2 * tp / denom) if denom > 0 else 0.0
#
#     obs_area = tp + fn
#     sim_area = tp + fp
#     if obs_area == 0 and sim_area == 0:
#         bi = 1.0
#     elif obs_area == 0:
#         bi = np.inf
#     else:
#         bi = sim_area / obs_area
#     return fi, bi
#
#
# def _lock_path_for(ind, cali_obj):
#     gen = getattr(ind, "gen", 0)
#     lock_path = Path(base_path + "/" + cali_obj.model.db_name + "/lock/nsga2_locks") / f"{RUN_ID}_g{gen}_id{ind.id}.lock"
#     return lock_path
#
#
# def _done_path_for(ind, cali_obj):
#     gen = getattr(ind, "gen", 0)
#     done_path = Path(base_path + "/" + cali_obj.model.db_name + "/lock/nsga2_done") / f"{RUN_ID}_g{gen}_id{ind.id}.done"
#     return done_path
#
#
# def _mark_done(done_path):
#     done_path.parent.mkdir(parents=True, exist_ok=True)
#     open(done_path, "w").close()
#
#
# def evaluate_blocking(cali_obj, gen, ind, timeout_s=2400):
#     lock_path = str(_lock_path_for(ind, cali_obj))
#     done_path = _done_path_for(ind, cali_obj)
#
#     owner = f"pid={os.getpid()}"
#     start = time.time()
#
#     logcali(f"[ENTER evaluate_blocking] gen={gen} id={ind.id}")
#     logcali(f"[LOCK PATH] {lock_path}")
#     logcali(f"[DONE PATH] {done_path}")
#
#     while True:
#         if done_path.exists():
#             scoop_log(f"[DONE-HIT][{owner}] gen={gen} id={ind.id} 已完成，直接返回")
#             logcali(f"[DONE-HIT][{owner}] gen={gen} id={ind.id} 已完成，直接返回")
#             return -1
#
#         fd = try_acquire_lock(lock_path)
#         if fd is not None:
#             try:
#                 if done_path.exists():
#                     scoop_log(f"[DONE-INSIDE][{owner}] gen={gen} id={ind.id} 已完成，释放锁并返回")
#                     logcali(f"[DONE-INSIDE][{owner}] gen={gen} id={ind.id} 已完成，释放锁并返回")
#                     return -1
#
#                 scoop_log(f"[RUN][{owner}] gen={gen} id={ind.id} 开始运行。")
#                 logcali(f"[RUN][{owner}] gen={gen} id={ind.id} 开始运行。")
#
#                 res = calibration_objectives(cali_obj, gen, ind)
#
#                 _mark_done(done_path)
#                 logcali(f"[MARK-DONE][{owner}] gen={gen} id={ind.id}")
#                 return res
#             finally:
#                 release_lock(fd)
#                 logcali(f"[LOCK-RELEASED][{owner}] gen={gen} id={ind.id}")
#
#         if done_path.exists():
#             scoop_log(f"[DONE-WAIT][{owner}] gen={gen} id={ind.id} 已完成(等待中发现)，返回")
#             logcali(f"[DONE-WAIT][{owner}] gen={gen} id={ind.id} 已完成(等待中发现)，返回")
#             return -1
#
#         elapsed = time.time() - start
#         if timeout_s is not None and elapsed > timeout_s:
#             scoop_log(f"[TIMEOUT][{owner}] gen={gen} id={ind.id} 等待锁超时，返回 -1")
#             logcali(f"[TIMEOUT][{owner}] gen={gen} id={ind.id} 等待锁超时，返回 -1")
#             return -1
#
#         scoop_log(f"[WAIT][{owner}] gen={gen} id={ind.id} 已等待 {elapsed:.1f}s，10s 后重试…")
#         logcali(f"[WAIT][{owner}] gen={gen} id={ind.id} 已等待 {elapsed:.1f}s，10s 后重试…")
#         time.sleep(10)
#
#
# def calibration_objectives(cali_obj, gen, ind):
#     """Evaluate the objectives of given individual."""
#     logcali("=" * 80)
#     logcali(f"[START calibration_objectives] gen={gen}, id={ind.id}")
#
#     cali_obj.ID = ind.id
#     model_args = cali_obj.model.ConfigDict
#     model_args.setdefault('calibration_id', -1)
#     model_args['calibration_id'] = ind.id
#
#     logcali("[1] BEFORE MainSEIMS")
#     model_obj = MainSEIMS(args_dict=model_args)
#     logcali("[1] AFTER MainSEIMS")
#
#     logcali("[2] BEFORE SetOutletObservations")
#     model_obj.SetOutletObservations(ind.obs.vars, ind.obs.data)
#     logcali("[2] AFTER SetOutletObservations")
#
#     logcali("[3] BEFORE SetMongoClient")
#     model_obj.SetMongoClient()
#     logcali("[3] AFTER SetMongoClient")
#
#     # ===== 打印并保存当前参数组合 =====
#     try:
#         param_names = cali_obj.ParamDefs['names']
#         param_values = list(ind)
#
#         logcali(f"[PARAM] gen={gen}, id={ind.id}, n_params={len(param_values)}")
#         logcali(f"[PARAM_CHECK] len_names={len(param_names)}, len_values={len(param_values)}")
#
#         for pname, pval in zip(param_names, param_values):
#             logcali(f"[PARAM] {pname} = {pval}")
#
#         one_line = ", ".join([f"{n}={v}" for n, v in zip(param_names, param_values)])
#         logcali(f"[PARAM_ONE_LINE] gen={gen}, id={ind.id}: {one_line}")
#
#         header = "gen,id," + ",".join(param_names) + "\n"
#         row = f"{gen},{ind.id}," + ",".join([str(v) for v in param_values]) + "\n"
#
#         if not os.path.exists(DEBUG_PARAM_LOG):
#             with open(DEBUG_PARAM_LOG, "w", encoding="utf-8") as f:
#                 f.write(header)
#
#         with open(DEBUG_PARAM_LOG, "a", encoding="utf-8") as f:
#             f.write(row)
#
#     except Exception as e:
#         logcali(f"[PARAM_LOG_ERROR] gen={gen}, id={ind.id}, error={e}")
#
#     logcali("[4] BEFORE model_obj.run()")
#     t0 = time.time()
#     model_obj.run()
#     logcali(f"[4] AFTER model_obj.run(), dt={time.time() - t0:.2f}s")
#
#     time.sleep(0.1)
#
#     logcali("[5] BEFORE ReadTimeseriesSimulations_new()")
#     ok = model_obj.ReadTimeseriesSimulations_new()
#     logcali(f"[5] AFTER ReadTimeseriesSimulations_new(), ok={ok}")
#
#     if ok:
#         ind.sim.vars = model_obj.sim_vars[:]
#         ind.sim.data = deepcopy(model_obj.sim_value)
#         logcali(f"[5.1] sim vars = {ind.sim.vars}")
#         try:
#             logcali(f"[5.2] sim data len = {len(ind.sim.data)}")
#         except Exception:
#             logcali("[5.2] sim data len = unknown")
#     else:
#         logcali("[5.FAIL] ReadTimeseriesSimulations_new failed")
#         model_obj.UnsetMongoClient()
#         return ind
#
#     logcali("[6] BEFORE ExtractSimData(cali)")
#     ind.cali.vars, ind.cali.data = model_obj.ExtractSimData(
#         cali_obj.cfg.cali_stime,
#         cali_obj.cfg.cali_etime
#     )
#     logcali("[6] AFTER ExtractSimData(cali)")
#
#     logcali("[7] BEFORE ExtractSimObsData(cali)")
#     ind.cali.sim_obs_data = model_obj.ExtractSimObsData(
#         cali_obj.cfg.cali_stime,
#         cali_obj.cfg.cali_etime
#     )
#     logcali("[7] AFTER ExtractSimObsData(cali)")
#
#     logcali("[8] BEFORE CalcTimeseriesStatistics(cali)")
#     ind.cali.objnames, ind.cali.objvalues = model_obj.CalcTimeseriesStatistics(
#         ind.cali.sim_obs_data,
#         cali_obj.cfg.cali_stime,
#         cali_obj.cfg.cali_etime
#     )
#     logcali("[8] AFTER CalcTimeseriesStatistics(cali)")
#     logcali(f"[8.1] objnames = {ind.cali.objnames}")
#     logcali(f"[8.2] objvalues = {ind.cali.objvalues}")
#
#     if ind.cali.objnames and ind.cali.objvalues:
#         ind.cali.valid = True
#
#     if cali_obj.cfg.calc_validation:
#         logcali("[9] BEFORE validation block")
#
#         ind.vali.vars, ind.vali.data = model_obj.ExtractSimData(
#             cali_obj.cfg.vali_stime,
#             cali_obj.cfg.vali_etime
#         )
#         logcali("[9.1] AFTER ExtractSimData(vali)")
#
#         ind.vali.sim_obs_data = model_obj.ExtractSimObsData(
#             cali_obj.cfg.vali_stime,
#             cali_obj.cfg.vali_etime
#         )
#         logcali("[9.2] AFTER ExtractSimObsData(vali)")
#
#         ind.vali.objnames, ind.vali.objvalues = model_obj.CalcTimeseriesStatistics(
#             ind.vali.sim_obs_data,
#             cali_obj.cfg.vali_stime,
#             cali_obj.cfg.vali_etime
#         )
#         logcali("[9.3] AFTER CalcTimeseriesStatistics(vali)")
#         logcali(f"[9.4] vali objnames = {ind.vali.objnames}")
#         logcali(f"[9.5] vali objvalues = {ind.vali.objvalues}")
#
#         if ind.vali.objnames and ind.vali.objvalues:
#             ind.vali.valid = True
#
#     ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = model_obj.GetTimespan()
#     logcali(f"[10] runtime = {ind.runtime}")
#
#     fi_list, bi_list = [], []
#     if cali_inundation_extent:
#         cur = cali_obj.cfg.cali_stime
#         end = cali_obj.cfg.cali_etime
#         months = []
#         while cur <= end:
#             months.append((cur.year, cur.month))
#             if cur.month == 12:
#                 cur = cur.replace(year=cur.year + 1, month=1, day=1)
#             else:
#                 cur = cur.replace(month=cur.month + 1, day=1)
#
#         for (yy, mm) in months:
#             obs_tif = os.path.join(cali_obj.cfg.flood_obs_dir, f"{yy}{mm:02d}.tif")
#             sim_tif = os.path.join(cali_obj.cfg.flood_sim_dir, f"run{ind.id}_{yy}{mm:02d}.tif")
#             if not (os.path.exists(obs_tif) and os.path.exists(sim_tif)):
#                 continue
#
#             obs_bin = _read_binary_extent_tif(obs_tif, nodata=0, threshold=0.5)
#             sim_bin = _read_binary_extent_tif(sim_tif, nodata=0, threshold=0.5)
#
#             if obs_bin.shape != sim_bin.shape:
#                 logcali(f"[WARN] shape mismatch for {yy}-{mm}, skip")
#                 continue
#
#             fi, bi = _fi_bi_from_binary(sim_bin, obs_bin)
#             fi_list.append(fi)
#             bi_list.append(bi)
#
#         if fi_list:
#             fi_mean = float(np.mean(fi_list))
#             bi_dev_mean = float(np.mean([abs(1.0 - b) if np.isfinite(b) else 1.0 for b in bi_list]))
#
#             names = list(getattr(ind.vali, "objnames", []))
#             vals = list(getattr(ind.vali, "objvalues", []))
#
#             names += ["FI", "BI_DEV"]
#             vals += [fi_mean, bi_dev_mean]
#
#             ind.vali.objnames = names
#             ind.vali.objvalues = vals
#
#             logcali(f"FI_mean={fi_mean:.4f}, BI_dev_mean={bi_dev_mean:.4f}")
#
#     if copy_output:
#         logcali("[11] BEFORE copy_dir")
#         model_obj.copy_dir(calibration_id=ind.id, gen=gen)
#         logcali("[11] AFTER copy_dir")
#
#     logcali("[12] SKIP clean for debug")
#     # model_obj.clean(calibration_id=ind.id)
#
#     logcali("[13] BEFORE UnsetMongoClient")
#     model_obj.UnsetMongoClient()
#     logcali("[13] AFTER UnsetMongoClient")
#
#     logcali(f"[END calibration_objectives] gen={gen}, id={ind.id}")
#     logcali("=" * 80)
#     return ind
#
#
# if __name__ == '__main__':
#     cf, method = get_optimization_config()
#     cfg = CaliConfig(cf, method=method)
#
#     caliobj = Calibration(cfg)
#
#     import pickle
#     s = pickle.dumps(caliobj)
#     new_cali = pickle.loads(s)
#     print(new_cali.bin_dir)





# 最新版本
cali_inundation_extent = False
cali_inundation_area = True
copy_output = True
# RUN_ID =  time.strftime("%Y%m%d-%H%M%S")

""" xiaodw add
scoop有个重发机制导致bug，就是比如一代有40个参数组合，10个子进程并行跑，其中一个子进程A可能跑的很慢，
主进程发现他很慢之后，就会重发一个子进程（B）跑一模一样的参数，然后A又跑完了，于是删除了A对应的率定文件夹 OUTPUT-A，
之后B跑完了发现 OUTPUT-A文件夹不存在，就报错了。
"""
if os.name == "nt":
    import msvcrt
    def try_acquire_lock(path: str):
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o644)
        try:
            # 非阻塞：Windows 只能用“试锁”语义模拟（会抛 OSError 时表示失败）
            msvcrt.locking(fd, msvcrt.LK_NBLCK, 1)
            return fd
        except OSError:
            os.close(fd)
            return None
    def release_lock(fd: int):
        try:
            msvcrt.locking(fd, msvcrt.LK_UNLCK, 1)
        finally:
            os.close(fd)
else:
    import fcntl
    def try_acquire_lock(path: str):
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o644)
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)  # 非阻塞
            return fd
        except BlockingIOError:
            os.close(fd)
            return None
    def release_lock(fd: int):
        try:
            fcntl.flock(fd, fcntl.LOCK_UN)
        finally:
            os.close(fd)

class TimeseriesData(object):
    """Time series data, for observation and simulation data."""

    def __init__(self):
        self.vars = list()
        self.data = OrderedDict()


class ObsSimData(object):
    """Paired time series data of observation and simulation, associated with statistics."""

    def __init__(self):
        self.vars = list()
        self.data = OrderedDict()
        self.sim_obs_data = OrderedDict()
        self.objnames = list()
        self.objvalues = list()
        self.valid = False

    def efficiency_values(self, varname, effnames):
        values = list()
        tmpvars = list()
        for name in effnames:
            tmpvar = '%s-%s' % (varname, name)
            if tmpvar not in self.objnames:
                values.append(-9999.)
                # tmpvars.append(tmpvar)
            else:
                if name.upper() == 'PBIAS':
                    tmpvars.append('%s-abs(PBIAS)' % varname)
                else:
                    tmpvars.append(tmpvar)
                values.append(self.objvalues[self.objnames.index(tmpvar)]) #evaluate index, i.e., NSE
        return values, tmpvars

    def output_header(self, varname, effnames, prefix=''):
        concate = ''
        for name in effnames:
            tmpvar = '%s-%s' % (varname, name)
            if tmpvar not in self.objnames:
                concate += '\t'
            else:
                if name.upper() == 'PBIAS':
                    tmpvar = '%s-abs(PBIAS)' % varname
                if prefix != '':
                    concate += '%s-%s\t' % (prefix, tmpvar)
                else:
                    concate += '%s\t' % tmpvar
        return concate

    def output_efficiency(self, varname, effnames):
        concate = ''
        for name in effnames:
            tmpvar = '%s-%s' % (varname, name)
            if tmpvar not in self.objnames:
                concate += '\t'
            else:
                concate += '%.3f\t' % self.objvalues[self.objnames.index(tmpvar)]
        return concate


class Calibration(object):
    """Base class of automatic calibration.

    Attributes:
        ID(integer): Calibration ID in current generation, range from 0 to N-1(individuals).
        modelrun(boolean): Has SEIMS model run successfully?
    """

    def __init__(self, cali_cfg, id=-1):
        # type: (CaliConfig, Optional[int]) -> None
        """Initialize."""
        self.cfg = cali_cfg
        self.model = cali_cfg.model
        self.ID = id
        self.param_defs = dict()
        # run seims related
        self.modelrun = False
        self.reset_simulation_timerange()

    @property
    def ParamDefs(self):
        """Read cali_param_rng.def file

           name,lower_bound,upper_bound

            e.g.,
             Param1,0,1
             Param2,0.5,1.2
             Param3,-1.0,1.0

        Returns:
            a dictionary containing:
            - names - the names of the parameters
            - bounds - a list of lists of lower and upper bounds
            - num_vars - a scalar indicating the number of variables
                         (the length of names)
        """
        # read param_defs.json if already existed
        if self.param_defs:
            return self.param_defs
        # read param_range_def file and output to json file
        conn = MongoDBObj.client
        db = conn[self.cfg.model.db_name]
        collection = db['PARAMETERS']

        names = list()
        bounds = list()
        num_vars = 0
        if not FileClass.is_file_exists(self.cfg.param_range_def):
            raise ValueError('Parameters definition file: %s is not'
                             ' existed!' % self.cfg.param_range_def)
        items = read_data_items_from_txt(self.cfg.param_range_def)
        for item in items:
            if len(item) < 3:
                continue
            # find parameter name, print warning message if not existed
            cursor = collection.find({'NAME': item[0]}, no_cursor_timeout=True)
            if not cursor.count():
                print('WARNING: parameter %s is not existed!' % item[0])
                continue
            num_vars += 1
            names.append(item[0])
            bounds.append([float(item[1]), float(item[2])])
        self.param_defs = {'names': names, 'bounds': bounds, 'num_vars': num_vars}
        return self.param_defs

    def reset_simulation_timerange(self):
        """Update simulation time range in MongoDB [FILE_IN]."""
        conn = MongoDBObj.client
        db = conn[self.cfg.model.db_name]
        stime_str = self.cfg.model.simu_stime.strftime('%Y-%m-%d %H:%M:%S')
        etime_str = self.cfg.model.simu_etime.strftime('%Y-%m-%d %H:%M:%S')
        db[DBTableNames.main_filein].find_one_and_update({'TAG': 'STARTTIME'},
                                                         {'$set': {'VALUE': stime_str}})
        db[DBTableNames.main_filein].find_one_and_update({'TAG': 'ENDTIME'},
                                                         {'$set': {'VALUE': etime_str}})

    def initialize(self, n=1):
        """Initialize parameters samples by Latin-Hypercube sampling method.

        Returns:
            A list contains parameter value at each gene location.
        """
        param_num = self.ParamDefs['num_vars']
        lhs_samples = lhs(param_num, n)
        all = list()
        for idx in range(n):
            gene_values = list()
            for i, param_bound in enumerate(self.ParamDefs['bounds']):
                gene_values.append(lhs_samples[idx][i] * (param_bound[1] - param_bound[0]) +
                                   param_bound[0])
            all.append(gene_values)
        return all


def initialize_calibrations(cf):
    """Initial individual of population.
    """
    cali = Calibration(cf)
    return cali.initialize()

"""
    xiaodw add
    按观测值有效性过滤：只保留 Obs 是有限数(且不等于无效填充值)的时刻，
    同步过滤 Sim 和 UTCDATETIME。
"""
def _filter_sim_by_obs(sim_obs_dict, invalid_values=(-9999.0,)):
    """
    按观测值有效性过滤：只保留 Obs 是有限数(且不等于无效填充值)的时刻，
    同步过滤 Sim 和 UTCDATETIME，并打印被过滤掉的时间。
    """
    out = {}
    for param, d in sim_obs_dict.items():
        t = d.get('UTCDATETIME', [])
        obs = d.get('Obs', [])
        sim = d.get('Sim', [])

        obs_arr = numpy.array(obs, dtype=float)
        sim_arr = numpy.array(sim, dtype=float)

        # 掩膜：有限数 且 不等于无效值
        mask = numpy.isfinite(obs_arr)
        for v in (invalid_values or ()):
            mask &= (obs_arr != v)

        # 找到被过滤掉的时间
        dropped_times = [t[i] for i in range(len(t)) if i < len(mask) and not mask[i]]
        if dropped_times:
            print(f"[{param}] 被过滤掉的时间点: {dropped_times}")

        # 应用掩膜
        t_f   = [t[i] for i in range(len(t)) if i < len(mask) and mask[i]]
        obs_f = obs_arr[mask].tolist()
        sim_f = sim_arr[mask].tolist()

        out[param] = {
            'UTCDATETIME': t_f,
            'Obs': obs_f,
            'Sim': sim_f,
        }
    return out


def evaluate_nowait_or_skip(cali_obj, ind):
    """
    非阻塞加锁：
      - 成功：执行 calibration_objectives 并返回 ind
      - 失败：说明已有同 (gen,id) 在跑 → 直接返回 None（表示跳过，不纳入结果）
    """
    fd = try_acquire_lock(str(_lock_path_for(ind, cali_obj)))
    if fd is None:
        # 被别的 worker 占用，按你的要求：不执行、不回收，不纳入 invalid_pops
        # 可选打印一行便于观察：
        scoop_log(f"[SKIP] gen={getattr(ind,'gen',0)} id={ind.id} 已在运行，跳过本次。")
        return -1
    try:
        scoop_log(f"[RUN] gen={getattr(ind, 'gen', 0)} id={ind.id} 开始运行。")
        return calibration_objectives(cali_obj, ind)
    finally:
        release_lock(fd)

import numpy as np
import rasterio

def _read_binary_extent_tif(path, nodata=None, threshold=0.5):
    """读取淹没范围栅格并转为二值（1=淹没，0=未淹没）。"""
    with rasterio.open(path) as ds:
        arr = ds.read(1)
        ndv = ds.nodata if ds.nodata is not None else nodata
    if ndv is not None:
        arr = np.where(arr == ndv, np.nan, arr)
    # 若原始是概率/水深，可用阈值二值化；若已是0/1，这一步等价透传
    bin_arr = (arr >= threshold).astype(np.uint8)
    bin_arr[np.isnan(arr)] = 255  # 用 255 做 mask，便于后面剔除
    return bin_arr


def _fi_bi_from_binary(sim_bin, obs_bin):
    """给定同尺寸二值（0/1），计算 FI 与 BI。255 视为无效掩膜。"""
    mask = (sim_bin != 255) & (obs_bin != 255)
    s = sim_bin[mask].astype(np.uint8)
    o = obs_bin[mask].astype(np.uint8)
    tp = np.sum((s == 1) & (o == 1))
    fp = np.sum((s == 1) & (o == 0))
    fn = np.sum((s == 0) & (o == 1))
    # Dice / F1
    denom = (2*tp + fp + fn)
    fi = (2*tp / denom) if denom > 0 else 0.0
    # Bias = (TP+FP)/(TP+FN)；若观测全为0则回退为1（中性）
    obs_area = tp + fn
    sim_area = tp + fp
    if obs_area == 0 and sim_area == 0:
        bi = 1.0
    elif obs_area == 0:
        bi = np.inf
    else:
        bi = sim_area / obs_area
    return fi, bi

def _lock_path_for(ind, cali_obj,base_path,RUN_ID):
    gen = getattr(ind, "gen", 0)
    # lock = Path(base_path + "/lock/nsga2_locks") / f"{RUN_ID}_g{gen}_id{ind.id}.lock"
    lock_path = Path(base_path + "/" + cali_obj.model.db_name + "/lock/nsga2_locks") / f"{RUN_ID}_g{gen}_id{ind.id}.lock"
    return lock_path

def _done_path_for(ind, cali_obj,base_path,RUN_ID):
    gen = getattr(ind, "gen", 0)
    # lock_path = Path(base_path + "/lock/nsga2_done") / f"{RUN_ID}_g{gen}_id{ind.id}.done"
    lock_path = Path(base_path + "/" + cali_obj.model.db_name + "/lock/nsga2_done") / f"{RUN_ID}_g{gen}_id{ind.id}.lock"
    return lock_path

def _mark_done(done_path):
    done_path.parent.mkdir(parents=True, exist_ok=True)
    open(done_path, "w").close()  # 空文件即表示 DONE

def evaluate_blocking(cali_obj, ind,base_path,RUN_ID, timeout_s=2400):
    lock_path = str(_lock_path_for(ind, cali_obj,base_path,RUN_ID))
    done_path = _done_path_for(ind, cali_obj,base_path,RUN_ID)

    owner = f"pid={os.getpid()}"
    start = time.time()
    while True:
        # 1) 已完成？立刻返回，避免再尝试拿锁
        if done_path.exists():
            scoop_log(f"[DONE-HIT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已完成，直接返回")
            return -1  # 也可换成 "__DONE__"

        # 2) 尝试拿锁
        fd = try_acquire_lock(lock_path)
        if fd is not None:
            try:
                # 拿到锁后再检查一遍 DONE（极端竞态下，别人刚写好）
                if done_path.exists():
                    scoop_log(f"[DONE-INSIDE][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已完成，释放锁并返回")
                    return -1

                scoop_log(f"[RUN][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 开始运行。")
                gen = getattr(ind,'gen',0)
                res = calibration_objectives(cali_obj, gen, ind)

                # 3) 成功完成 → 写入 DONE 标记（在释放锁之前写，确保可见）
                _mark_done(done_path)
                return res
            finally:
                release_lock(fd)

        # 4) 没拿到锁，看看是否完成或超时
        if done_path.exists():
            scoop_log(f"[DONE-WAIT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已完成(等待中发现)，返回")
            return -1

        elapsed = time.time() - start
        if timeout_s is not None and elapsed > timeout_s:
            scoop_log(f"[TIMEOUT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 等待锁超时，返回 -1")
            return -1

        # 5) 短退避 + 进度日志
        scoop_log(f"[WAIT][{owner}] gen={getattr(ind,'gen',0)} id={ind.id} 已等待 {elapsed:.1f}s，10s 后重试…")
        time.sleep(10)

def calibration_objectives(cali_obj, gen, ind):
    """Evaluate the objectives of given individual.
    """
    cali_obj.ID = ind.id
    model_args = cali_obj.model.ConfigDict
    model_args.setdefault('calibration_id', -1)
    model_args['calibration_id'] = ind.id
    model_obj = MainSEIMS(args_dict=model_args)

    # Set observation data to model_obj, no need to query database
    model_obj.SetOutletObservations(ind.obs.vars, ind.obs.data)

    # Execute model
    model_obj.SetMongoClient()
    run_success = model_obj.run(timeout=240)
    time.sleep(0.1)  # Wait a moment in case of unpredictable file system error

    # read simulation data of the entire simulation period (include calibration and validation)
    #if model_obj.ReadTimeseriesSimulations():
    if run_success and model_obj.ReadTimeseriesSimulations_new():   #ljj++
        ind.sim.vars = model_obj.sim_vars[:]
        ind.sim.data = deepcopy(model_obj.sim_value)
    else:
        model_obj.clean(calibration_id=ind.id)
        model_obj.UnsetMongoClient()
        # 模型运行失败，设置 NSE 为 -9999
        ind.cali.objnames = [f"{var}-NSE" for var in ind.obs.vars]
        ind.cali.objvalues = [-9999.0] * len(ind.cali.objnames)
        ind.cali.valid = False
        return ind

    # Calculate NSE, R2, RMSE, PBIAS, and RSR, etc. of calibration period
    # print(f"cali_stime: {cali_obj.cfg.cali_stime}/n, cali_etime: {cali_obj.cfg.cali_etime}/n")
    ind.cali.vars, ind.cali.data = model_obj.ExtractSimData(cali_obj.cfg.cali_stime,
                                                            cali_obj.cfg.cali_etime)
    # print(f"ind.cali.vars: {ind.cali.vars}/n, ind.cali.data: {ind.cali.data}/n")
    ind.cali.sim_obs_data = model_obj.ExtractSimObsData(cali_obj.cfg.cali_stime,
                                                        cali_obj.cfg.cali_etime)


    ind.cali.objnames, \
    ind.cali.objvalues = model_obj.CalcTimeseriesStatistics(ind.cali.sim_obs_data,
                                                            cali_obj.cfg.cali_stime,
                                                            cali_obj.cfg.cali_etime)
    # 输出校准期指标
    cali_metrics = ", ".join(f"{name}:{value:.4f}" for name, value in zip(ind.cali.objnames, ind.cali.objvalues))
    # print(f"cali: {cali_metrics}")
    if ind.cali.objnames and ind.cali.objvalues:
        ind.cali.valid = True
    else:
        # 计算指标失败，设置 NSE 为 -9999
        ind.cali.objnames = [f"{var}-NSE" for var in ind.obs.vars]
        ind.cali.objvalues = [-9999.0] * len(ind.cali.objnames)
        ind.cali.valid = False
    # Calculate NSE, R2, RMSE, PBIAS, and RSR, etc. of validation period
    if cali_obj.cfg.calc_validation:
        ind.vali.vars, ind.vali.data = model_obj.ExtractSimData(cali_obj.cfg.vali_stime,
                                                                cali_obj.cfg.vali_etime)
        ind.vali.sim_obs_data = model_obj.ExtractSimObsData(cali_obj.cfg.vali_stime,
                                                            cali_obj.cfg.vali_etime)

        ind.vali.objnames, \
        ind.vali.objvalues = model_obj.CalcTimeseriesStatistics(ind.vali.sim_obs_data,
                                                                cali_obj.cfg.vali_stime,
                                                                cali_obj.cfg.vali_etime)
        if ind.vali.objnames and ind.vali.objvalues:
            ind.vali.valid = True
        else:
            # 计算指标失败，设置 NSE 为 -9999
            ind.vali.objnames = [f"{var}-NSE" for var in ind.obs.vars]
            ind.vali.objvalues = [-9999.0] * len(ind.vali.objnames)
            ind.vali.valid = False

        # 输出验证期指标
        vali_metrics = ", ".join(f"{name}:{value:.4f}" for name, value in zip(ind.vali.objnames, ind.vali.objvalues))
        # print(f"vali: {vali_metrics}")
    # Get timespan
    ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = model_obj.GetTimespan()
    ##-----------------------xiaodw add, calibrate Fi,Bi for inundation extent-----------------------------
    fi_list, bi_list = [], []
    if cali_inundation_extent:
        # cur = datetime.strptime(cali_obj.cfg.cali_stime, "%Y-%m-%d %H:%M:%S")
        # end = datetime.strptime(cali_obj.cfg.cali_etime, "%Y-%m-%d %H:%M:%S")
        cur = cali_obj.cfg.cali_stime
        end = cali_obj.cfg.cali_etime
        months = []
        while cur <= end:
            months.append((cur.year, cur.month))
            # 跳到下一个月初
            if cur.month == 12:
                cur = cur.replace(year=cur.year + 1, month=1, day=1)
            else:
                cur = cur.replace(month=cur.month + 1, day=1)

        for (yy, mm) in months:
            obs_tif = os.path.join(cali_obj.cfg.flood_obs_dir, f"{yy}{mm:02d}.tif")
            sim_tif = os.path.join(cali_obj.cfg.flood_sim_dir, f"run{ind.id}_{yy}{mm:02d}.tif")
            if not (os.path.exists(obs_tif) and os.path.exists(sim_tif)):
                continue  # 该月缺数据则跳过

            obs_bin = _read_binary_extent_tif(obs_tif, nodata=0, threshold=0.5)
            sim_bin = _read_binary_extent_tif(sim_tif, nodata=0, threshold=0.5)

            # 尺寸/投影需一致；如不一致，请在导出时保证一致，或此处先重投影/重采样
            if obs_bin.shape != sim_bin.shape:
                # TODO: 统一投影与分辨率（建议在生成阶段统一）
                print(f"[WARN] shape mismatch for {yy}-{mm}, skip")
                continue

            fi, bi = _fi_bi_from_binary(sim_bin, obs_bin)
            fi_list.append(fi)
            bi_list.append(bi)

        # 汇总为月平均
        if fi_list:
            fi_mean = float(np.mean(fi_list))
            # BI 偏差 |1-BI|，用于“越小越好”的目标
            bi_dev_mean = float(np.mean([abs(1.0 - b) if np.isfinite(b) else 1.0 for b in bi_list]))

            # 把新指标并入已有的校准期指标列表
            names = list(getattr(ind.vali, "objnames", []))
            vals = list(getattr(ind.vali, "objvalues", []))

            names += ["FI", "BI_DEV"]  # FI 越大越好；BI_DEV = |1-BI| 越小越好
            vals += [fi_mean, bi_dev_mean]

            ind.vali.objnames = names
            ind.vali.objvalues = vals

            # 控制台打印
            print(f"FI_mean={fi_mean:.4f}, BI_dev_mean={bi_dev_mean:.4f}")
    ##-----------------------end calibrate Fi,Bi for inundation extent-----------------------------
    # delete model output directory for saving storage
    print(f"delete output directory: {ind.id}")
    if copy_output:
        model_obj.copy_dir(calibration_id=ind.id,gen = gen)
    model_obj.clean(calibration_id=ind.id)
    model_obj.UnsetMongoClient()

    return ind


if __name__ == '__main__':
    cf, method = get_optimization_config()
    cfg = CaliConfig(cf, method=method)

    caliobj = Calibration(cfg)

    # test the picklable of Scenario class.
    import pickle

    s = pickle.dumps(caliobj)
    # print(s)
    new_cali = pickle.loads(s)
    print(new_cali.bin_dir)
