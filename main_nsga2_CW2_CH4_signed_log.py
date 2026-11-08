# -*- coding: utf-8 -*-
# 三个甲烷点：min(maxRMSE) + min(meanLogRMSE)
from __future__ import absolute_import, division, unicode_literals

import array
import os
import random
import time
import sys
from io import open

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from typing import Dict
import numpy
from deap import base
from deap import creator
from deap import tools
from deap.benchmarks.tools import hypervolume
from copy import deepcopy
from pygeoc.utils import UtilClass

from utility.scoop_func import scoop_log
from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
from calibration.config import CaliConfig, get_optimization_config
from run_seims import MainSEIMS

from calibration.calibrate import (
    Calibration,
    initialize_calibrations,
    calibration_objectives,
    evaluate_nowait_or_skip,
    evaluate_blocking,
)
from calibration.calibrate import TimeseriesData, ObsSimData
from calibration.userdef import write_param_values_to_mongodb, output_population_details
from calibration.calibrate import cali_inundation_extent, cali_inundation_area
import inundation_cali_tools as ic_tool


# Definitions, assignments, operations, etc. that will be executed by each worker
# when paralleled by SCOOP. Thus, DEAP related operations are better defined here.

# All accepted objective function names from `postprocess::utility::calculate_statistics`
# accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
accepted_objnames = [
    'NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE',
    'lnNSE', 'NSE1', 'NSE3', 'FI', 'BI'
]

# step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', 'TEST', or 'CUSTOMIZE'.
step = 'CUSTOMIZE'
filter_ind = False  # Filter for valid population for the next generation

# Definitions of Multiobjectives:
multiobj = dict()
if step == 'Q':
    # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS|.
    multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
elif step == 'SED':
    # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, and min. |PBIAS|-SED.
    multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
elif step == 'QSED':
    # Step 3: Calibration streamflow and sediment.
    multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
    multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
elif step == 'NUTRIENT':
    # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED.
    multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
    multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
    multiobj.setdefault('Q', [['NSE', 1., -100]])
    multiobj.setdefault('SED', [['NSE', 1., -100]])
elif step == 'TEST':
    # Test example.
    multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
    multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
    multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
    multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
    multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
else:
    # ============================================================
    # T5 三个实测甲烷点，全部使用 RMSE 计算
    #
    # 原始RMSE：
    # D_1366、D_1367、D_1368
    #
    # 真正参与fitness：
    # maxRMSE：三个甲烷点RMSE中的最大值，最小化
    # meanLogRMSE：三个甲烷点有符号log空间RMSE的平均值，最小化
    #
    # D_1370：不参与fitness，只用于独立验证和绘图
    # D_1375：只有一个实测值，只用于辅助检查
    # F_18：只用于独立验证和绘图
    # ============================================================

    multiobj.setdefault(
        'C_1366',
        [
            ['RMSE', -1.0, 10.0],
        ]
    )

    multiobj.setdefault(
        'C_1367',
        [
            ['RMSE', -1.0, 10.0],
        ]
    )

    multiobj.setdefault(
        'C_1368',
        [
            ['RMSE', -1.0, 10.0],
        ]
    )


# ------------------------------------------------------------
# 会话路径设置。
# Windows 和 Linux 分开写，避免路径错误。
# 如果你实际工程目录不是 CW_2，请在这里修改 base_path 和 RUN_ID。
# ------------------------------------------------------------
if os.name == 'nt':  # Windows
    base_path = r'D:\SEIMS_C\data\CW_2'
else:  # Linux/Unix
    base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'

# 淹没范围率定才会用到；如果只是甲烷通量/面积率定，一般不用管。
subbasin_flood_path = base_path + '/inundation_cali/subbasin_flood'
subbasin_ids = [1171, 1176, 1193, 1194, 1214]

# 不同实例指定不同 RUN_ID，避免锁文件互相影响。
RUN_ID = 'CW_2_CH4_3point_maxRMSE_meanLogRMSE'


# Check object variables.
if not multiobj:
    print('Multiobjective MUST not be Empty!')
    exit(1)

for k, v in list(multiobj.items()):
    for item in v:
        if len(item) < 3:
            print('Each item of objective MUST have three elements, '
                  'i.e., object name, weight, worse value for Hypervolum calculation.')
            exit(1)
        if item[0] not in accepted_objnames:
            print('Object name % is unsupported! '
                  'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
            exit(1)

# Get parameters from `multiobj`.
# 三个甲烷点原始RMSE仍然通过multiobj计算和保存。
object_vars = list(multiobj.keys())
object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})

# NSGA-II真正优化两个目标：
# 1. maxRMSE  ：三个点中最大的RMSE，最小化
# 2. meanLogRMSE：三个点有符号log空间RMSE的平均值，最小化
fitness_names = ['maxRMSE', 'meanLogRMSE']
multi_weight = (-1.0, -1.0)

# Hypervolume参考点。
# 第一个参考值对应原始甲烷通量RMSE，第二个对应有符号log空间RMSE。
# 参考值不参与NSGA-II优劣排序，只用于Hypervolume计算。
worse_objects = [1000.0, 10.0]
conditions = [None, None]


# ============================================================
# 辅助诊断指标
#
# D_1366、D_1367、D_1368的RMSE
# 用于构造maxRMSE；有符号log空间RMSE在配对观测/模拟值上另外计算。
# NSE改为辅助诊断指标继续输出。
#
# NSE1在当前统计代码中实际为KGE。
# D_1370、F_18和D_1375不参与率定，只用于辅助验证和绘图。
# ============================================================
auxiliary_object_names = {
    'C_1366': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    'C_1367': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    'C_1368': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    # D_1370 不参与率定，作为独立验证。
    # 由于已经从multiobj移除，因此这里补回NSE。
    'C_1370': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RMSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    # D_1375 不参与率定，只用于辅助验证和绘图。
    # 按原来的方式继续计算全部指标。
    'C_1375': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RMSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],
}

# 输出和绘图变量。顺序固定，便于 userdef.py 中绘图读取。
# D_1370、F_18和D_1375只用于辅助验证和绘图；不进入 fitness。
output_vars = [
    'C_1366',
    'C_1367',
    'C_1368',
    'C_1370',
    'C_1375',
]


creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# NOTE: to maintain compatibility with Python2 and Python3,
# typecode=str('d') MUST NOT be changed to typecode='d'.
creator.create(
    'Individual',
    array.array,
    typecode=str('d'),
    fitness=creator.FitnessMulti,
    gen=-1,
    id=-1,
    obs=TimeseriesData,
    sim=TimeseriesData,
    cali=ObsSimData,
    vali=ObsSimData,
    io_time=0.,
    comp_time=0.,
    simu_time=0.,
    runtime=0.,
)


# Register NSGA-II related operations.
toolbox = base.Toolbox()
toolbox.register('gene_values', initialize_calibrations)
toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# toolbox.register('evaluate', calibration_objectives)
# toolbox.register('evaluate', evaluate_nowait_or_skip)
toolbox.register('evaluate', evaluate_blocking, timeout_s=240)

toolbox.register('mate', tools.cxSimulatedBinaryBounded)
toolbox.register('mutate', tools.mutPolynomialBounded)
toolbox.register('select', tools.selNSGA2)





# CH4 flux scale used by the sign-preserving log transform.
# Unit: mg CH4 m-2 h-1. The observations in the input file remain unchanged.
CH4_LOG_SCALE = 1.0


def signed_log1p(values, scale=CH4_LOG_SCALE):
    """Apply a sign-preserving log1p transform to CH4 flux values."""
    arr = numpy.asarray(values, dtype=float)
    if scale <= 0.0:
        raise ValueError('CH4_LOG_SCALE must be greater than zero.')
    return numpy.sign(arr) * numpy.log1p(numpy.abs(arr) / scale)


def calculate_ch4_log_rmse(obs_values, sim_values, scale=CH4_LOG_SCALE):
    """Calculate RMSE in sign-preserving log space for paired CH4 data."""
    obs_arr = numpy.asarray(obs_values, dtype=float)
    sim_arr = numpy.asarray(sim_values, dtype=float)

    if obs_arr.shape != sim_arr.shape:
        raise ValueError('Observed and simulated CH4 arrays have different shapes.')

    valid = numpy.isfinite(obs_arr) & numpy.isfinite(sim_arr)
    if not numpy.any(valid):
        raise ValueError('No finite observed-simulated CH4 pairs are available.')

    obs_log = signed_log1p(obs_arr[valid], scale)
    sim_log = signed_log1p(sim_arr[valid], scale)
    return float(numpy.sqrt(numpy.mean((sim_log - obs_log) ** 2)))


def main(cfg):
    """Main workflow of NSGA-II based scenario analysis/calibration."""
    random.seed()
    scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))

    # Initial timespan variables.
    stime = time.time()
    plot_time = 0.
    allmodels_exect = list()  # execute time of all model runs

    # Create reference point for hypervolume.
    ref_pt = numpy.array(worse_objects) * multi_weight * -1

    stats = tools.Statistics(lambda sind: sind.fitness.values)
    stats.register('min', numpy.min, axis=0)
    stats.register('max', numpy.max, axis=0)
    stats.register('avg', numpy.mean, axis=0)
    stats.register('std', numpy.std, axis=0)
    logbook = tools.Logbook()
    logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'

    # Read observation data from MongoDB.
    cali_obj = Calibration(cfg)

    # Read observation data just once.
    model_cfg_dict = cali_obj.model.ConfigDict
    model_obj = MainSEIMS(args_dict=model_cfg_dict)

    model_obj.SetMongoClient()

    # 读取所有需要输出/绘图的变量。
    # 三个甲烷点用于构造maxRMSE和meanLogRMSE。
    # D_1370仍然读取，但只作为辅助验证。
    obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)

    missing_output_vars = [v for v in output_vars if v not in obs_vars]
    if missing_output_vars:
        raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))

    scoop_log('Raw RMSE variables: %s' % str(object_vars))
    scoop_log('Calibration objectives (fitness only): %s' % str(fitness_names))
    scoop_log('Output/plot variables (includes auxiliary variables): %s' % str(output_vars))

    # If calibrating inundation extent.
    if cali_inundation_extent:
        obs_inun_data_dict = ic_tool.load_monthly_tifs(
            root_dir=subbasin_flood_path,
            subbasin_ids=subbasin_ids,
            start='2010-01',
            end='2010-02',
            band=1,
            on_missing='warn',
            readonly=True,
        )
        some_month = list(obs_inun_data_dict.keys())[0]
        arr_1171 = obs_inun_data_dict[some_month]['1171']
        print(some_month, arr_1171.shape, arr_1171.dtype)

    model_obj.UnsetMongoClient()

    # Initialize population.
    param_values = cali_obj.initialize(cfg.opt.npop)
    pop = list()
    for i in range(cfg.opt.npop):
        ind = creator.Individual(param_values[i])
        ind.gen = 0
        ind.id = i
        ind.obs.vars = obs_vars[:]
        ind.obs.data = deepcopy(obs_data_dict)
        if cali_inundation_extent:
            ind.obs.inun_data = deepcopy(obs_inun_data_dict)
        pop.append(ind)

    param_values = numpy.array(param_values)

    # Write calibrated values to MongoDB.
    write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)

    # Get the low and up bound of calibrated parameters.
    bounds = numpy.array(cali_obj.ParamDefs['bounds'])
    low = bounds[:, 0].tolist()
    up = bounds[:, 1].tolist()
    pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
    init_time = time.time() - stime

    def check_validation(fitvalues):
        """Check the validation of the fitness values of an individual."""
        flag = True
        for condidx, condstr in enumerate(conditions):
            if condstr is None:
                continue
            if not eval('%f%s' % (fitvalues[condidx], condstr)):
                flag = False
        return flag

    def evaluate_parallel(invalid_pops):
        """Evaluate model by SCOOP or map, and set fitness of individuals."""
        popnum = len(invalid_pops)
        labels = list()

        # Serial debug version:
        # invalid_pops = [toolbox.evaluate(cali_obj, ind, base_path, RUN_ID) for ind in invalid_pops]

        try:
            # Parallel on multi-processors or clusters using SCOOP.
            from scoop import futures
            invalid_pops = list(futures.map(
                toolbox.evaluate,
                [cali_obj] * popnum,
                invalid_pops,
                [base_path] * popnum,
                [RUN_ID] * popnum,
            ))
        except (ImportError, ImportWarning):
            # Python built-in map, serial mode.
            invalid_pops = list(map(
                toolbox.evaluate,
                [cali_obj] * popnum,
                invalid_pops,
                [base_path] * popnum,
                [RUN_ID] * popnum,
            ))

        scoop_log('过滤之前pops长度为:%d' % len(invalid_pops))

        # 把被跳过或失败返回 -1 的个体过滤掉。
        invalid_pops = [ind for ind in invalid_pops if ind != -1]

        scoop_log('过滤之后pops长度为:%d' % len(invalid_pops))

        for tmpind in invalid_pops:
            labels = fitness_names[:]
            tmpfitnessv = list()
            try:
                # ------------------------------------------------------------
                # 先计算三个甲烷点各自的原始RMSE。
                #
                # 顺序：
                # D_1366、D_1367、D_1368。
                #
                # 这三个RMSE仍然正常保存到cali结果中，
                # 只是它们不再直接作为3个独立fitness。
                #
                # D_1370不在这里计算，因此不会参与maxRMSE和meanLogRMSE。
                # ------------------------------------------------------------
                raw_rmse_values = list()
                log_rmse_values = list()

                for k, v in list(multiobj.items()):
                    tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])

                    if len(tmpvalues) != 1:
                        raise ValueError(
                            '%s返回的RMSE数量不等于1: %s'
                            % (k, str(tmpvalues))
                        )

                    raw_rmse_values.append(float(tmpvalues[0]))

                    pair_data = tmpind.cali.sim_obs_data.get(k)
                    if not isinstance(pair_data, dict):
                        raise ValueError('%s缺少配对观测/模拟数据。' % k)
                    if 'Obs' not in pair_data or 'Sim' not in pair_data:
                        raise ValueError('%s缺少Obs或Sim字段。' % k)

                    log_rmse_values.append(
                        calculate_ch4_log_rmse(
                            pair_data['Obs'],
                            pair_data['Sim'],
                        )
                    )

                if len(raw_rmse_values) != 3:
                    raise ValueError(
                        '三个甲烷点RMSE数量不等于3: %s'
                        % str(raw_rmse_values)
                    )

                if len(log_rmse_values) != 3:
                    raise ValueError(
                        '三个甲烷点log空间RMSE数量不等于3: %s'
                        % str(log_rmse_values)
                    )

                raw_rmse_values = numpy.array(raw_rmse_values, dtype=float)
                log_rmse_values = numpy.array(log_rmse_values, dtype=float)

                if not numpy.all(numpy.isfinite(raw_rmse_values)):
                    raise ValueError(
                        '三个甲烷点RMSE存在NaN或Inf: %s'
                        % str(raw_rmse_values)
                    )

                if numpy.any(raw_rmse_values < 0.0):
                    raise ValueError(
                        '三个甲烷点RMSE存在负值: %s'
                        % str(raw_rmse_values)
                    )

                if not numpy.all(numpy.isfinite(log_rmse_values)):
                    raise ValueError(
                        '三个甲烷点log空间RMSE存在NaN或Inf: %s'
                        % str(log_rmse_values)
                    )

                if numpy.any(log_rmse_values < 0.0):
                    raise ValueError(
                        '三个甲烷点log空间RMSE存在负值: %s'
                        % str(log_rmse_values)
                    )

                # ------------------------------------------------------------
                # 真正的NSGA-II双目标：
                #
                # 1. maxRMSE：
                #    三个甲烷点中最大的RMSE，越小越好。
                #
                # 2. meanLogRMSE：
                #    三个甲烷点有符号log空间RMSE的平均值，越小越好。
                #
                # 原始空间meanRMSE只作为诊断值保存，不参与优化。
                # ------------------------------------------------------------
                max_rmse = float(numpy.max(raw_rmse_values))
                mean_log_rmse = float(numpy.mean(log_rmse_values))
                mean_raw_rmse = float(numpy.mean(raw_rmse_values))

                tmpfitnessv = [
                    max_rmse,
                    mean_log_rmse,
                ]

                tmpind.fitness.values = tuple(tmpfitnessv)

                # 同时保存到sim_obs_data顶层，便于后续userdef.py
                # 选参、诊断和检查；不会改变原始观测/模拟序列。
                tmpind.cali.sim_obs_data['Fitness_maxRMSE'] = max_rmse
                tmpind.cali.sim_obs_data['Fitness_meanLogRMSE'] = mean_log_rmse

                # Compatibility alias for an existing userdef.py that may still
                # read Fitness_meanRMSE. Its value is now meanLogRMSE.
                tmpind.cali.sim_obs_data['Fitness_meanRMSE'] = mean_log_rmse
                tmpind.cali.sim_obs_data['Diagnostic_meanRawRMSE'] = mean_raw_rmse

            except Exception as e:
                scoop_log(
                    '[模型失败] id=%s, gen=%s, tmpfitnessv=%s, multi_weight=%s, 原因: %s'
                    % (tmpind.id, tmpind.gen, tmpfitnessv, multi_weight, e)
                )

                # 两个fitness中：
                # maxRMSE、meanLogRMSE均为最小化目标，失败时赋极大值。
                penalty_values = tuple(
                    -9999.0 if w > 0 else 9999.0
                    for w in multi_weight
                )
                tmpind.fitness.values = penalty_values

            # 辅助变量只计算并保存指标，不追加到 tmpfitnessv。
            # 因此D_1370、F_18和D_1375不参与NSGA-II优化或fitness。
            for aux_var, aux_metrics in auxiliary_object_names.items():
                try:
                    tmpind.cali.efficiency_values(aux_var, aux_metrics)
                except Exception as aux_e:
                    scoop_log(
                        '[辅助验证变量读取失败，不影响率定] id=%s, gen=%s, var=%s, 原因: %s'
                        % (tmpind.id, tmpind.gen, aux_var, aux_e)
                    )

        # Filter for a valid solution.
        if filter_ind:
            invalid_pops = [tmpind for tmpind in invalid_pops
                            if check_validation(tmpind.fitness.values)]
            if len(invalid_pops) < 2:
                print('The initial population should be greater or equal than 2. '
                      'Please check the parameters ranges or change the sampling strategy!')
                exit(2)

        # Currently, invalid_pops contains evaluated individuals.
        return invalid_pops, labels

    # Record the count and execute timespan of model runs during optimization.
    modelruns_count = {0: len(pop)}
    modelruns_time = {0: 0.}
    modelruns_time_sum = {0: 0.}

    # Generation 0 before optimization.
    stime = time.time()
    pop, plotlables = evaluate_parallel(pop)
    modelruns_time[0] = time.time() - stime
    for ind in pop:
        allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
        modelruns_time_sum[0] += ind.runtime

    # Currently, len(pop) may less than pop_select_num.
    pop = toolbox.select(pop, pop_select_num)

    # Output simulated data to json or pickle files for future use.
    output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)

    record = stats.compile(pop)
    logbook.record(gen=0, evals=len(pop), **record)

    # Begin the generational process.
    output_str = '### Generation number: %d, Population size: %d ###\n' % (
        cfg.opt.ngens,
        cfg.opt.npop,
    )
    UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')

    # Newly added Pareto fronts count.
    modelsel_count = {0: len(pop)}  # type: Dict[int, int]

    for gen in range(1, cfg.opt.ngens + 1):
        output_str = '###### Generation: %d ######\n' % gen

        offspring = [toolbox.clone(ind) for ind in pop]

        # Generate offspring by crossover and mutation.
        if len(offspring) >= 2:
            for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
                if random.random() > cfg.opt.rcross:
                    continue
                eta = i
                toolbox.mate(ind1, ind2, eta, low, up)
                toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
                toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
                del ind1.fitness.values, ind2.fitness.values
        else:
            toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
            del offspring[0].fitness.values

        # Evaluate the individuals with an invalid fitness.
        invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
        valid_inds = [ind for ind in offspring if ind.fitness.valid]
        if len(invalid_inds) == 0:
            scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
            break

        # Write new calibrated parameters to MongoDB.
        param_values = list()
        for idx, ind in enumerate(invalid_inds):
            ind.gen = gen
            ind.id = idx
            param_values.append(ind[:])
        param_values = numpy.array(param_values)
        write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)

        # Count model runs and execute models.
        invalid_ind_size = len(invalid_inds)
        modelruns_count.setdefault(gen, invalid_ind_size)
        stime = time.time()
        invalid_inds, plotlables = evaluate_parallel(invalid_inds)
        curtimespan = time.time() - stime
        modelruns_time.setdefault(gen, curtimespan)
        modelruns_time_sum.setdefault(gen, 0.)
        for ind in invalid_inds:
            allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
            modelruns_time_sum[gen] += ind.runtime

        # Select the next generation population.
        # Remove duplications first.
        tmppop = pop + valid_inds + invalid_inds
        pop = list()
        unique_sces = dict()
        for tmpind in tmppop:
            if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
                continue
            if tmpind.gen not in unique_sces:
                unique_sces.setdefault(tmpind.gen, [tmpind.id])
            elif tmpind.id not in unique_sces[tmpind.gen]:
                unique_sces[tmpind.gen].append(tmpind.id)
            pop.append(tmpind)
        pop = toolbox.select(pop, pop_select_num)

        output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)

        hyper_str = 'Gen: %d, New model runs: %d, ' \
                    'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
                    'Hypervolume: %.4f\n' % (
                        gen,
                        invalid_ind_size,
                        curtimespan,
                        modelruns_time_sum[gen],
                        hypervolume(pop, ref_pt),
                    )
        UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')

        record = stats.compile(pop)
        logbook.record(gen=gen, evals=len(invalid_inds), **record)

        # Count the newly generated near Pareto fronts.
        new_count = 0
        for ind in pop:
            if ind.gen == gen:
                new_count += 1
        modelsel_count.setdefault(gen, new_count)

        # Plot near Pareto optimal solutions.
        stime = time.time()
        front = numpy.array([ind.fitness.values for ind in pop])
        title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
                 'Near Pareto optimal solutions')

        plot_pareto_front_single(
            front,
            plotlables,
            cfg.opt.out_dir,
            gen,
            title,
            plot_cfg=cali_obj.cfg.plot_cfg,
        )
        plot_time += time.time() - stime

        # Save in file.
        output_str += 'generation\tcalibrationID\t'

        # 三个参与率定甲烷点的原始RMSE仍然全部输出到日志。
        for kk, vv in list(object_names.items()):
            output_str += pop[0].cali.output_header(kk, vv, 'Cali')

        # 辅助诊断指标。
        # D_1370的NSE也会在这里正常输出。
        for kk, vv in list(auxiliary_object_names.items()):
            output_str += pop[0].cali.output_header(kk, vv, 'Cali')

        # 两个真正的NSGA-II目标。
        output_str += (
            'Fitness-maxRMSE\t'
            'Fitness-meanLogRMSE\t'
            'Diagnostic-meanRawRMSE\t'
        )

        if cali_obj.cfg.calc_validation:
            for kkk, vvv in list(object_names.items()):
                output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
            for kkk, vvv in list(auxiliary_object_names.items()):
                output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')

        output_str += 'gene_values\n'
        for ind in pop:
            output_str += '%d\t%d\t' % (ind.gen, ind.id)

            # 三个参与率定甲烷点的原始RMSE仍然全部输出到日志。
            for kk, vv in list(object_names.items()):
                output_str += ind.cali.output_efficiency(kk, vv)

            # 辅助验证指标。
            # D_1370的NSE、KGE、RMSE、PBIAS、R-square等继续正常保存。
            for kk, vv in list(auxiliary_object_names.items()):
                output_str += ind.cali.output_efficiency(kk, vv)

            # 输出两个真正fitness。
            fit_max_rmse = ind.fitness.values[0]
            fit_mean_log_rmse = ind.fitness.values[1]
            diagnostic_mean_raw_rmse = ind.cali.sim_obs_data.get(
                'Diagnostic_meanRawRMSE',
                numpy.nan,
            )

            output_str += '%.6f\t%.6f\t%.6f\t' % (
                fit_max_rmse,
                fit_mean_log_rmse,
                diagnostic_mean_raw_rmse,
            )

            if cali_obj.cfg.calc_validation:
                for kkk, vvv in list(object_names.items()):
                    output_str += ind.vali.output_efficiency(kkk, vvv)
                for kkk, vvv in list(auxiliary_object_names.items()):
                    output_str += ind.vali.output_efficiency(kkk, vvv)

            output_str += str(ind)
            output_str += '\n'
        UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')

        # TODO: Figure out if we should terminate the evolution.

    # Plot hypervolume and newly executed model count.
    plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)

    # Save newly added Pareto fronts of each generation.
    new_fronts_count = numpy.array(list(modelsel_count.items()))
    numpy.savetxt(
        '%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
        new_fronts_count,
        delimiter=str(','),
        fmt=str('%d'),
    )

    # Save and print timespan information.
    allmodels_exect = numpy.array(allmodels_exect)
    numpy.savetxt(
        '%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
        allmodels_exect,
        delimiter=str(' '),
        fmt=str('%.4f'),
    )
    scoop_log('Running time of all SEIMS models:\n'
              '\tIO\tCOMP\tSIMU\tRUNTIME\n'
              'MAX\t%s\n'
              'MIN\t%s\n'
              'AVG\t%s\n'
              'SUM\t%s\n' % (
                  '\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
                  '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
                  '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
                  '\t'.join('%.3f' % t for t in allmodels_exect.sum(0)),
              ))

    exec_time = 0.
    for genid, tmptime in list(modelruns_time.items()):
        exec_time += tmptime

    exec_time_sum = 0.
    for genid, tmptime in list(modelruns_time_sum.items()):
        exec_time_sum += tmptime


    allcount = 0
    for genid, tmpcount in list(modelruns_count.items()):
        allcount += tmpcount

    scoop_log('Initialization timespan: %.4f\n'
              'Model execution timespan: %.4f\n'
              'Sum of model runs timespan: %.4f\n'
              'Plot Pareto graphs timespan: %.4f' % (
                  init_time,
                  exec_time,
                  exec_time_sum,
                  plot_time,
              ))

    return pop, logbook


if __name__ == '__main__':
    cf, method = get_optimization_config()
    cali_cfg = CaliConfig(cf, method=method)

    scoop_log('### START TO CALIBRATION OPTIMIZING ###')
    startT = time.time()

    fpop, fstats = main(cali_cfg)

    # 当前率定使用D_1366、D_1367、D_1368构造两个目标：
    #
    # 1. maxRMSE  —— 最小化
    # 2. meanLogRMSE —— 最小化
    #
    # D_1370、F_18和D_1375仅用于辅助验证和绘图，
    # 不参与fitness。
    with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
        f.write('%s' % fstats.__str__())

    endT = time.time()
    scoop_log('### END OF CALIBRATION OPTIMIZING ###')
    scoop_log('Running time: %.2fs' % (endT - startT))
