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

""" xiaodw add
scoop有个重发机制导致bug，就是比如一代有40个参数组合，10个子进程并行跑，其中一个子进程A可能跑的很慢，
主进程发现他很慢之后，就会重发一个子进程（B）跑一模一样的参数，然后A又跑完了，于是删除了A对应的率定文件夹 OUTPUT-A，
之后B跑完了发现 OUTPUT-A文件夹不存在，就报错了。
"""
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
                #values.append(-9999.)
                tmpvars.append(tmpvar)
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

# def _lock_path_for(ind, cali_obj):
#     # 按你的任务唯一标识来建锁文件（建议用 gen+id）
#     gen = getattr(ind, "gen", 0)
#     return Path("/tmp/nsga2_locks") / f"gen{gen}_id{ind.id}.lock"
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

def _lock_key(ind):
    return f"g{getattr(ind,'gen',0)}:id{ind.id}"

def evaluate_nowait_or_skip(cali_obj, ind, state, lock):
    key = _lock_key(ind)

    # 原子占位（同一把 key 只让一个进程成功）
    with lock:
        if state.get(key, 0) == 1:
            scoop_log(f"[SKIP] gen={getattr(ind,'gen',0)} id={ind.id} 已在运行，跳过。")
            return -1
        state[key] = 1

    try:
        scoop_log(f"[RUN] gen={getattr(ind,'gen',0)} id={ind.id} 开始运行。")
        return calibration_objectives(cali_obj, ind)
    finally:
        # 释放占位
        with lock:
            scoop_log(f"[RELEASE] gen={getattr(ind, 'gen', 0)} id={ind.id} 释放。")
            state.pop(key, None)

def calibration_objectives(cali_obj, ind):
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
    model_obj.run()
    time.sleep(0.1)  # Wait a moment in case of unpredictable file system error

    # read simulation data of the entire simulation period (include calibration and validation)
    #if model_obj.ReadTimeseriesSimulations():
    if model_obj.ReadTimeseriesSimulations_new():   #ljj++
        ind.sim.vars = model_obj.sim_vars[:]
        ind.sim.data = deepcopy(model_obj.sim_value)
    else:
        model_obj.clean(calibration_id=ind.id)
        model_obj.UnsetMongoClient()
        return ind

    # Calculate NSE, R2, RMSE, PBIAS, and RSR, etc. of calibration period
    # print(f"cali_stime: {cali_obj.cfg.cali_stime}/n, cali_etime: {cali_obj.cfg.cali_etime}/n")
    ind.cali.vars, ind.cali.data = model_obj.ExtractSimData(cali_obj.cfg.cali_stime,
                                                            cali_obj.cfg.cali_etime)
    # print(f"ind.cali.vars: {ind.cali.vars}/n, ind.cali.data: {ind.cali.data}/n")
    ind.cali.sim_obs_data = model_obj.ExtractSimObsData(cali_obj.cfg.cali_stime,
                                                        cali_obj.cfg.cali_etime)
    # xiaodw add, to remove simdata(for some time step)  which doesn't have obs data(no need to do this, which is done in timeseries_data.py: if sim_date not in obs_dict)
    # print(ind.cali.sim_obs_data)
    # ind.cali.sim_obs_data = _filter_sim_by_obs(ind.cali.sim_obs_data)

    ind.cali.objnames, \
    ind.cali.objvalues = model_obj.CalcTimeseriesStatistics(ind.cali.sim_obs_data,
                                                            cali_obj.cfg.cali_stime,
                                                            cali_obj.cfg.cali_etime)
    # 输出校准期指标
    cali_metrics = ", ".join(f"{name}:{value:.4f}" for name, value in zip(ind.cali.objnames, ind.cali.objvalues))
    print(f"cali: {cali_metrics}")
    if ind.cali.objnames and ind.cali.objvalues:
        ind.cali.valid = True
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

        # 输出验证期指标
        vali_metrics = ", ".join(f"{name}:{value:.4f}" for name, value in zip(ind.vali.objnames, ind.vali.objvalues))
        print(f"vali: {vali_metrics}")
    # Get timespan
    ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = model_obj.GetTimespan()

    # delete model output directory for saving storage
    print(f"delete output directory: {ind.id}")
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
