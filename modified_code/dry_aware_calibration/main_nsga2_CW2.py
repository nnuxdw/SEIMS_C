"""Calibration by NSGA-II algorithm.

    @author   : Liangjun Zhu

    @changelog:
    - 18-01-22  - lj - initial implementation.
    - 18-02-09  - lj - compatible with Python3.
    - 18-07-10  - lj - Support MPI version of SEIMS.
    - 18-08-26  - lj - Gather the execute time of all model runs. Plot pareto graphs.
    - 18-08-29  - jz,lj,sf - Add Nutrient calibration step.
    - 18-10-22  - lj - Make the customizations of multi-objectives flexible.
"""
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import Calibration, initialize_calibrations, calibration_objectives,evaluate_nowait_or_skip,evaluate_blocking
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent,cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# #    when paralleled by SCOOP.
# # Thus, DEAP related operations (initialize, register, etc.) are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3','FI','BI']
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS| (percent)
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, min., and |PBIAS|-SED
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# #ljj++
# elif step == 'TEST':
#     #negative is minimal
#     #the number can be any float
#     # multiobj.setdefault('Q_1444', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('Q_1289', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('TOTDOCcon_1289', [['NSE1', 1., -100, '>0'],['R-square', 1., -100, '>0']])
#     # multiobj.setdefault('TOTDOCcon_1444', [['R-square', 1., -100, '>0']])
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# #######
# else:
#     # multiobj.setdefault('Q_156', [['NSE', 1., -100, '>0']])
#     # multiobj.setdefault('Q_143', [['NSE', 1., -100, '>0']])
#     # multiobj.setdefault('F_19', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('F_19', [['NSE', 1., -100, '>0']])
#     multiobj.setdefault('D_1312', [['NSE', 1., -100, '>0']])
#     multiobj.setdefault('D_1313', [['NSE', 1., -100, '>0']])
#     multiobj.setdefault('D_1314', [['NSE', 1., -100, '>0']])
#     multiobj.setdefault('D_1316', [['NSE', 1., -100, '>0']])
#     multiobj.setdefault('D_1321', [['NSE', 1., -100, '>0']])
#     # Customize your own multiobjective here, such as:
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0.'],
#     #                           ['RSR', -1., 2., '<2.'],
#     #                           ['PBIAS', -1., 50., '<50.']])
#     # multiobj.setdefault('Q_322', [['NSE', 1., -100, '>0']])
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0']])
#
#
# # 会话ID，避免跨实例互相影响（强烈推荐）
# if os.name == 'nt':  # Windows
#     # base_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1'
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
# # # 淹没范围才会用，淹没面积率定不用管
# subbasin_flood_path = base_path + f'/inundation_cali/subbasin_flood'
# subbasin_ids=[1171,1176,1193,1194,1214]
#
# # 不同的实例指定不同的id，避免锁文件相互影响
# RUN_ID = 'CW_2_20260425'
#
# # Check object variables
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
# # Get parameters from `multiobj`
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
# multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
# worse_objects = list(l[2] for v in multiobj.values() for l in v)
# conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # The FitnessMulti class equals to (as an example):
# # class FitnessMulti(base.Fitness):
# #     weights = (2., -1., -1.)
# # NOTE that to maintain the compatibility with Python2 and Python3,
# #      the com typecode=str('d') MUST NOT changed to typecode='d', since
# #      the latter will raise TypeError that 'must be char, not unicode'!
# creator.create('Individual', array.array, typecode=str('d'), fitness=creator.FitnessMulti,
#                gen=-1, id=-1,
#                obs=TimeseriesData, sim=TimeseriesData,
#                cali=ObsSimData, vali=ObsSimData,
#                io_time=0., comp_time=0., simu_time=0., runtime=0.)
# # The Individual class equals to:
# # class Individual(array.array):
# #     gen = -1  # Generation No.
# #     id = -1   # Calibration index of current generation
# #     def __init__(self):
# #         self.fitness = FitnessMulti()
#
# # Register NSGA-II related operations
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking,timeout_s=240)
#
#
# # mate and mutate
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based Scenario analysis."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # create reference point for hypervolume
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # read observation data from MongoDB
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#     #obs_vars, obs_data_dict = model_obj.ReadOutletObservations(object_vars)  #读取每个变量的观测值
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(object_vars)  #ljj++
#     #-------------------------xiaodw, if calibrate inundation extent --------------------
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start="2010-01",
#             end="2010-02",
#             band=1,
#             on_missing="warn",
#             readonly=True
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]["1171"]
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#     #-------------------------xiaodw, if calibrate inundation area --------------------
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population
#     # print(f"cfg.opt is {cfg.opt}")
#     # print(f"npop is: {cfg.opt.npop}")
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#         # print(f"ind: {ind} ")
#     param_values = numpy.array(param_values)
#     # print(f"npop: {cfg.opt.npop}")
#
#     # print(param_values)
#     # Write calibrated values to MongoDB
#     # TODO, extract this function, which is same with `Sensitivity::write_param_values_to_mongodb`.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#     # get the low and up bound of calibrated parameters
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0]
#     up = bounds[:, 1]
#     low = low.tolist()
#     up = up.tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals
#          according to calibration step."""
#         popnum = len(invalid_pops)
#         labels = list()
#         # ---------------------test for no scoop----------------------
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind,base_path,RUN_ID) for ind in invalid_pops]
#         # ---------------------end for no scoop----------------------
#
#         try:  # parallel on multi-processors or clusters using SCOOP
#             from scoop import futures
#             invalid_pops = list(futures.map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#         except ImportError or ImportWarning:  # Python build-in map (serial)
#             invalid_pops = list(map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#
#         scoop_log(f"过滤之前pops长度为:{len(invalid_pops)}")
#         # 关键：把 None（被跳过的个体）过滤掉
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log(f"过滤之后pops长度为:{len(invalid_pops)}")
#
#         for tmpind in invalid_pops:
#             labels = list()
#             tmpfitnessv = list()
#             try:
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#                     tmpfitnessv += tmpvalues[:]
#                     labels += tmplabel[:]
#
#                 # 如果结果合法才赋值
#                 if len(tmpfitnessv) == len(multi_weight):
#                     tmpind.fitness.values = tuple(tmpfitnessv)
#                 else:
#                     raise ValueError("模型返回指标维度不一致！")
#
#             # lj
#             except Exception as e:
#                 # print(f"[模型失败] id={tmpind.id}, gen={tmpind.gen}, tmpfitnessv={tmpfitnessv}, multi_weight={multi_weight}, 原因: {e}")
#                 scoop_log(f"[模型失败] id={tmpind.id}, gen={tmpind.gen}, tmpfitnessv={tmpfitnessv}, multi_weight={multi_weight}, 原因: {e}")
#                 tmpind.fitness.values = tuple([9999.0] * len(multi_weight))  # 用极差值惩罚模型失败者
#
#             # except Exception as e:
#             #     scoop_log(
#             #         f"[模型失败] id={tmpind.id}, gen={tmpind.gen}, "
#             #         f"tmpfitnessv={tmpfitnessv}, multi_weight={multi_weight}, 原因: {e}"
#             #     )
#             #
#             #     # 根据指标优化方向给模型失败个体赋极差值：
#             #     # w > 0：指标越大越好，例如 NSE，失败时赋极小值 -9999
#             #     # w < 0：指标越小越好，例如 RMSE、RSR，失败时赋极大值 9999
#             #     penalty_values = tuple(
#             #         -9999.0 if w > 0 else 9999.0
#             #         for w in multi_weight
#             #     )
#             #
#             #     tmpind.fitness.values = penalty_values
#
#         # Filter for a valid solution
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#         return invalid_pops, labels  # Currently, `invalid_pops` contains evaluated individuals
#
#     # Record the count and execute timespan of model runs during the optimization
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}  # Total time counted according to evaluate_parallel()
#     modelruns_time_sum = {0: 0.}  # Summarize time of every model runs according to pop
#
#     # Generation 0 before optimization
#     stime = time.time()
#     # print(f"***************pop: {pop}")
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # currently, len(pop) may less than pop_select_num
#     pop = toolbox.select(pop, pop_select_num)
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#     # scoop_log(logbook.stream)
#
#     # Begin the generational process
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (cfg.opt.ngens,
#                                                                            cfg.opt.npop)
#     # scoop_log(output_str)
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int] # newly added Pareto fronts
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#         # scoop_log(output_str)
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#         # method1: use crowding distance (normalized as 0~1) as eta
#         # tools.emo.assignCrowdingDist(offspring)
#         # method2: use the index of individual at the sorted offspring list as eta
#         if len(offspring) >= 2:  # when offspring size greater than 2, mate can be done
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:  # No need to continue
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#         # Count the model runs, and execute models
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population
#         # Previous version may result in duplications of the same scenario in one Pareto front,
#         #   thus, I decided to check and remove the duplications first.
#         # pop = toolbox.select(pop + valid_inds + invalid_inds, pop_select_num)
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (gen, invalid_ind_size,
#                                              curtimespan, modelruns_time_sum[gen],
#                                              hypervolume(pop, ref_pt))
#         # scoop_log(hyper_str)
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#         # scoop_log(logbook.stream)
#
#         # Count the newly generated near Pareto fronts
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot 2D near optimal pareto front graphs,
#         #   i.e., (NSE, RSR), (NSE, PBIAS), and (RSR,PBIAS)
#         # And 3D near optimal pareto front graphs, i.e., (NSE, RSR, PBIAS)
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(front, plotlables, cfg.opt.out_dir,
#                                  gen, title, plot_cfg=cali_obj.cfg.plot_cfg)
#         plot_time += time.time() - stime
#
#         # save in file
#         # Header information
#         output_str += 'generation\tcalibrationID\t'
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution
#
#     # Plot hypervolume and newly executed model count
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generations
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt('%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#                   new_fronts_count, delimiter=str(','), fmt=str('%d'))
#
#     # Save and print timespan information
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt('%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#                   allmodels_exect, delimiter=str(' '), fmt=str('%.4f'))
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % ('\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.sum(0))))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (init_time, exec_time,
#                                                      exec_time_sum, plot_time))
#
#     return pop, logbook
#
#
# if __name__ == "__main__":
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     fpop.sort(key=lambda x: x.fitness.values)
#     # scoop_log(fstats)
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         # In case of 'TypeError: write() argument 1 must be unicode, not str' in Python2.7
#         #   when using unicode_literals, please use '%s' to concatenate string!
#         f.write('%s' % fstats.__str__())
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))





# # -*- coding: utf-8 -*-
# # 仅以五个实测水深率定参数；F_19 作为独立验证并随结果绘图。
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import Calibration, initialize_calibrations, calibration_objectives,evaluate_nowait_or_skip,evaluate_blocking
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent,cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# #    when paralleled by SCOOP.
# # Thus, DEAP related operations (initialize, register, etc.) are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3','FI','BI']
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS| (percent)
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, min., and |PBIAS|-SED
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# #ljj++
# elif step == 'TEST':
#     #negative is minimal
#     #the number can be any float
#     # multiobj.setdefault('Q_1444', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('Q_1289', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('TOTDOCcon_1289', [['NSE1', 1., -100, '>0'],['R-square', 1., -100, '>0']])
#     # multiobj.setdefault('TOTDOCcon_1444', [['R-square', 1., -100, '>0']])
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# #######
# else:
#     # ------------------------------------------------------------
#     # 率定目标：仅使用五个实测水深 NSE
#     # F_19 淹没面积不进入 fitness，不参与 NSGA-II 优化；
#     # 但会作为辅助验证变量被读取、保存并绘图。
#     # ------------------------------------------------------------
#     multiobj.setdefault('D_1312', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1313', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1314', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1316', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1321', [['NSE', 1., -100]])
#     # Customize your own multiobjective here, such as:
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0.'],
#     #                           ['RSR', -1., 2., '<2.'],
#     #                           ['PBIAS', -1., 50., '<50.']])
#     # multiobj.setdefault('Q_322', [['NSE', 1., -100, '>0']])
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0']])
#
#
# # 会话ID，避免跨实例互相影响（强烈推荐）
# if os.name == 'nt':  # Windows
#     # base_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1'
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
# # # 淹没范围才会用，淹没面积率定不用管
# subbasin_flood_path = base_path + f'/inundation_cali/subbasin_flood'
# subbasin_ids=[1171,1176,1193,1194,1214]
#
# # 不同的实例指定不同的id，避免锁文件相互影响
# RUN_ID = 'CW_2_20260425'
#
# # Check object variables
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
# # Get parameters from `multiobj`
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
# multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
# worse_objects = list(l[2] for v in multiobj.values() for l in v)
# conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)
#
# # ------------------------------------------------------------
# # 辅助验证/绘图变量：读取并保存，但绝不进入 fitness。
# # output_vars 的顺序固定为 F_19 + 五个 D_*，与绘图列顺序一致。
# # ------------------------------------------------------------
# auxiliary_object_names = {
#     'F_19': ['NSE', 'RSR', 'PBIAS', 'R-square']
# }
# plot_only_vars = list(auxiliary_object_names.keys())
# output_vars = plot_only_vars + [v for v in object_vars if v not in plot_only_vars]
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # The FitnessMulti class equals to (as an example):
# # class FitnessMulti(base.Fitness):
# #     weights = (2., -1., -1.)
# # NOTE that to maintain the compatibility with Python2 and Python3,
# #      the com typecode=str('d') MUST NOT changed to typecode='d', since
# #      the latter will raise TypeError that 'must be char, not unicode'!
# creator.create('Individual', array.array, typecode=str('d'), fitness=creator.FitnessMulti,
#                gen=-1, id=-1,
#                obs=TimeseriesData, sim=TimeseriesData,
#                cali=ObsSimData, vali=ObsSimData,
#                io_time=0., comp_time=0., simu_time=0., runtime=0.)
# # The Individual class equals to:
# # class Individual(array.array):
# #     gen = -1  # Generation No.
# #     id = -1   # Calibration index of current generation
# #     def __init__(self):
# #         self.fitness = FitnessMulti()
#
# # Register NSGA-II related operations
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking,timeout_s=240)
#
#
# # mate and mutate
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based Scenario analysis."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # create reference point for hypervolume
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # read observation data from MongoDB
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#     # F_19 仅供验证/绘图；只有 multiobj 中的 D_* 会构成 fitness。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Calibration objectives (fitness only): %s' % str(object_vars))
#     scoop_log('Output/plot variables (includes auxiliary F_19): %s' % str(output_vars))
#     #-------------------------xiaodw, if calibrate inundation extent --------------------
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start="2010-01",
#             end="2010-02",
#             band=1,
#             on_missing="warn",
#             readonly=True
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]["1171"]
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#     #-------------------------xiaodw, if calibrate inundation area --------------------
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population
#     # print(f"cfg.opt is {cfg.opt}")
#     # print(f"npop is: {cfg.opt.npop}")
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#         # print(f"ind: {ind} ")
#     param_values = numpy.array(param_values)
#     # print(f"npop: {cfg.opt.npop}")
#
#     # print(param_values)
#     # Write calibrated values to MongoDB
#     # TODO, extract this function, which is same with `Sensitivity::write_param_values_to_mongodb`.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#     # get the low and up bound of calibrated parameters
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0]
#     up = bounds[:, 1]
#     low = low.tolist()
#     up = up.tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals
#          according to calibration step."""
#         popnum = len(invalid_pops)
#         labels = list()
#         # ---------------------test for no scoop----------------------
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind,base_path,RUN_ID) for ind in invalid_pops]
#         # ---------------------end for no scoop----------------------
#
#         try:  # parallel on multi-processors or clusters using SCOOP
#             from scoop import futures
#             invalid_pops = list(futures.map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#         except ImportError or ImportWarning:  # Python build-in map (serial)
#             invalid_pops = list(map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#
#         scoop_log(f"过滤之前pops长度为:{len(invalid_pops)}")
#         # 关键：把 None（被跳过的个体）过滤掉
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log(f"过滤之后pops长度为:{len(invalid_pops)}")
#
#         for tmpind in invalid_pops:
#             labels = list()
#             tmpfitnessv = list()
#             try:
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#                     tmpfitnessv += tmpvalues[:]
#                     labels += tmplabel[:]
#
#                 # 如果结果合法才赋值
#                 if len(tmpfitnessv) == len(multi_weight):
#                     tmpind.fitness.values = tuple(tmpfitnessv)
#                 else:
#                     raise ValueError("模型返回指标维度不一致！")
#
#             except Exception as e:
#                 scoop_log(
#                     f"[模型失败] id={tmpind.id}, gen={tmpind.gen}, "
#                     f"tmpfitnessv={tmpfitnessv}, multi_weight={multi_weight}, 原因: {e}"
#                 )
#
#                 # 当前 fitness 均为 NSE 最大化目标，失败时必须赋极小值。
#                 # 通用写法同时兼容未来加入最小化指标（如 RMSE、RSR）。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 仅为保存和绘图计算 F_19 指标；不追加到 tmpfitnessv，
#             # 所以 F_19 不参与 NSGA-II 优化或最终水深选参。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         f"[辅助验证变量读取失败，不影响率定] id={tmpind.id}, gen={tmpind.gen}, "
#                         f"var={aux_var}, 原因: {aux_e}"
#                     )
#
#         # Filter for a valid solution
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#         return invalid_pops, labels  # Currently, `invalid_pops` contains evaluated individuals
#
#     # Record the count and execute timespan of model runs during the optimization
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}  # Total time counted according to evaluate_parallel()
#     modelruns_time_sum = {0: 0.}  # Summarize time of every model runs according to pop
#
#     # Generation 0 before optimization
#     stime = time.time()
#     # print(f"***************pop: {pop}")
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # currently, len(pop) may less than pop_select_num
#     pop = toolbox.select(pop, pop_select_num)
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#     # scoop_log(logbook.stream)
#
#     # Begin the generational process
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (cfg.opt.ngens,
#                                                                            cfg.opt.npop)
#     # scoop_log(output_str)
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int] # newly added Pareto fronts
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#         # scoop_log(output_str)
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#         # method1: use crowding distance (normalized as 0~1) as eta
#         # tools.emo.assignCrowdingDist(offspring)
#         # method2: use the index of individual at the sorted offspring list as eta
#         if len(offspring) >= 2:  # when offspring size greater than 2, mate can be done
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:  # No need to continue
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#         # Count the model runs, and execute models
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population
#         # Previous version may result in duplications of the same scenario in one Pareto front,
#         #   thus, I decided to check and remove the duplications first.
#         # pop = toolbox.select(pop + valid_inds + invalid_inds, pop_select_num)
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (gen, invalid_ind_size,
#                                              curtimespan, modelruns_time_sum[gen],
#                                              hypervolume(pop, ref_pt))
#         # scoop_log(hyper_str)
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#         # scoop_log(logbook.stream)
#
#         # Count the newly generated near Pareto fronts
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot 2D near optimal pareto front graphs,
#         #   i.e., (NSE, RSR), (NSE, PBIAS), and (RSR,PBIAS)
#         # And 3D near optimal pareto front graphs, i.e., (NSE, RSR, PBIAS)
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(front, plotlables, cfg.opt.out_dir,
#                                  gen, title, plot_cfg=cali_obj.cfg.plot_cfg)
#         plot_time += time.time() - stime
#
#         # save in file
#         # Header information
#         output_str += 'generation\tcalibrationID\t'
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution
#
#     # Plot hypervolume and newly executed model count
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generations
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt('%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#                   new_fronts_count, delimiter=str(','), fmt=str('%d'))
#
#     # Save and print timespan information
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt('%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#                   allmodels_exect, delimiter=str(' '), fmt=str('%.4f'))
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % ('\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.sum(0))))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (init_time, exec_time,
#                                                      exec_time_sum, plot_time))
#
#     return pop, logbook
#
#
# if __name__ == "__main__":
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 最终统一参数由 calibration.userdef.py 中的 select_shared_best_index() 选择：
#     # 只依据五个实测水深，F_19 仅用于独立验证和绘图。
#     # scoop_log(fstats)
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         # In case of 'TypeError: write() argument 1 must be unicode, not str' in Python2.7
#         #   when using unicode_literals, please use '%s' to concatenate string!
#         f.write('%s' % fstats.__str__())
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))







# # -*- coding: utf-8 -*-
# # 仅以前四个主要实测水深 NSE 率定参数；D_1321 与 F_19 作为辅助验证并随结果绘图。
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import Calibration, initialize_calibrations, calibration_objectives,evaluate_nowait_or_skip,evaluate_blocking
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent,cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# #    when paralleled by SCOOP.
# # Thus, DEAP related operations (initialize, register, etc.) are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3','FI','BI']
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS| (percent)
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, min., and |PBIAS|-SED
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# #ljj++
# elif step == 'TEST':
#     #negative is minimal
#     #the number can be any float
#     # multiobj.setdefault('Q_1444', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('Q_1289', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('TOTDOCcon_1289', [['NSE1', 1., -100, '>0'],['R-square', 1., -100, '>0']])
#     # multiobj.setdefault('TOTDOCcon_1444', [['R-square', 1., -100, '>0']])
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# #######
# else:
#     # ------------------------------------------------------------
#     # 率定目标：仅使用四个主要实测水深 NSE
#     #
#     # D_1312、D_1313、D_1314、D_1316：
#     #     具有明显水深变化，参与 NSGA-II 率定和最终统一参数选择。
#     #
#     # D_1321：
#     #     观测值大部分接近 0，仅作为浅水边缘点验证，不进入 fitness。
#     #
#     # F_19：
#     #     淹没面积仅作为辅助验证和绘图变量，不进入 fitness。
#     # ------------------------------------------------------------
#     multiobj.setdefault('D_1312', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1313', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1314', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1316', [['NSE', 1., -100]])
#
#     # multiobj.setdefault('D_1304', [['NSE', 1., -100]])
#     # multiobj.setdefault('D_1305', [['NSE', 1., -100]])
#     # multiobj.setdefault('D_1306', [['NSE', 1., -100]])
#     # multiobj.setdefault('D_1309', [['NSE', 1., -100]])
#
#     # Customize your own multiobjective here, such as:
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0.'],
#     #                           ['RSR', -1., 2., '<2.'],
#     #                           ['PBIAS', -1., 50., '<50.']])
#     # multiobj.setdefault('Q_322', [['NSE', 1., -100, '>0']])
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0']])
#
#
# # 会话ID，避免跨实例互相影响（强烈推荐）
# if os.name == 'nt':  # Windows
#     # base_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1'
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
# # # 淹没范围才会用，淹没面积率定不用管
# subbasin_flood_path = base_path + f'/inundation_cali/subbasin_flood'
# subbasin_ids=[1171,1176,1193,1194,1214]
#
# # 不同的实例指定不同的id，避免锁文件相互影响
# RUN_ID = 'CW_2_20260425'
#
# # Check object variables
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
# # Get parameters from `multiobj`
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
# multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
# worse_objects = list(l[2] for v in multiobj.values() for l in v)
# conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)
#
# # ------------------------------------------------------------
# # 辅助验证/绘图变量：读取、计算指标并保存，但绝不进入 fitness。
# #
# # F_19：淹没面积辅助验证。
# # D_1321：近零浅水边缘点辅助验证。
# #
# # output_vars 的顺序固定为 F_19 + 四个率定水深 + D_1321，
# # 与 userdef.py 中绘图时读取列的顺序保持一致。
# # ------------------------------------------------------------
# auxiliary_object_names = {
#     'F_19': ['NSE', 'RSR', 'PBIAS', 'R-square'],
#     'D_1321': ['NSE', 'RSR', 'PBIAS', 'R-square']
# }
#
# output_vars = [
#     'F_19',
#     'D_1312',
#     'D_1313',
#     'D_1314',
#     'D_1316',
#     'D_1321'
# ]
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # The FitnessMulti class equals to (as an example):
# # class FitnessMulti(base.Fitness):
# #     weights = (2., -1., -1.)
# # NOTE that to maintain the compatibility with Python2 and Python3,
# #      the com typecode=str('d') MUST NOT changed to typecode='d', since
# #      the latter will raise TypeError that 'must be char, not unicode'!
# creator.create('Individual', array.array, typecode=str('d'), fitness=creator.FitnessMulti,
#                gen=-1, id=-1,
#                obs=TimeseriesData, sim=TimeseriesData,
#                cali=ObsSimData, vali=ObsSimData,
#                io_time=0., comp_time=0., simu_time=0., runtime=0.)
# # The Individual class equals to:
# # class Individual(array.array):
# #     gen = -1  # Generation No.
# #     id = -1   # Calibration index of current generation
# #     def __init__(self):
# #         self.fitness = FitnessMulti()
#
# # Register NSGA-II related operations
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking,timeout_s=240)
#
#
# # mate and mutate
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based Scenario analysis."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # create reference point for hypervolume
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # read observation data from MongoDB
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#     # F_19 与 D_1321 仅供验证/绘图；只有 multiobj 中的四个 D_* 会构成 fitness。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Calibration objectives (fitness only): %s' % str(object_vars))
#     scoop_log('Output/plot variables (includes auxiliary F_19 and D_1321): %s' % str(output_vars))
#     #-------------------------xiaodw, if calibrate inundation extent --------------------
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start="2010-01",
#             end="2010-02",
#             band=1,
#             on_missing="warn",
#             readonly=True
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]["1171"]
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#     #-------------------------xiaodw, if calibrate inundation area --------------------
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population
#     # print(f"cfg.opt is {cfg.opt}")
#     # print(f"npop is: {cfg.opt.npop}")
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#         # print(f"ind: {ind} ")
#     param_values = numpy.array(param_values)
#     # print(f"npop: {cfg.opt.npop}")
#
#     # print(param_values)
#     # Write calibrated values to MongoDB
#     # TODO, extract this function, which is same with `Sensitivity::write_param_values_to_mongodb`.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#     # get the low and up bound of calibrated parameters
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0]
#     up = bounds[:, 1]
#     low = low.tolist()
#     up = up.tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals
#          according to calibration step."""
#         popnum = len(invalid_pops)
#         labels = list()
#         # ---------------------test for no scoop----------------------
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind,base_path,RUN_ID) for ind in invalid_pops]
#         # ---------------------end for no scoop----------------------
#
#         try:  # parallel on multi-processors or clusters using SCOOP
#             from scoop import futures
#             invalid_pops = list(futures.map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#         except ImportError or ImportWarning:  # Python build-in map (serial)
#             invalid_pops = list(map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#
#         scoop_log(f"过滤之前pops长度为:{len(invalid_pops)}")
#         # 关键：把 None（被跳过的个体）过滤掉
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log(f"过滤之后pops长度为:{len(invalid_pops)}")
#
#         for tmpind in invalid_pops:
#             labels = list()
#             tmpfitnessv = list()
#             try:
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#                     tmpfitnessv += tmpvalues[:]
#                     labels += tmplabel[:]
#
#                 # 如果结果合法才赋值
#                 if len(tmpfitnessv) == len(multi_weight):
#                     tmpind.fitness.values = tuple(tmpfitnessv)
#                 else:
#                     raise ValueError("模型返回指标维度不一致！")
#
#             except Exception as e:
#                 scoop_log(
#                     f"[模型失败] id={tmpind.id}, gen={tmpind.gen}, "
#                     f"tmpfitnessv={tmpfitnessv}, multi_weight={multi_weight}, 原因: {e}"
#                 )
#
#                 # 当前 fitness 均为 NSE 最大化目标，失败时必须赋极小值。
#                 # 通用写法同时兼容未来加入最小化指标（如 RMSE、RSR）。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 仅为保存和绘图计算 F_19 与 D_1321 指标；不追加到 tmpfitnessv，
#             # 所以二者不参与 NSGA-II 优化或最终统一参数选择。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         f"[辅助验证变量读取失败，不影响率定] id={tmpind.id}, gen={tmpind.gen}, "
#                         f"var={aux_var}, 原因: {aux_e}"
#                     )
#
#         # Filter for a valid solution
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#         return invalid_pops, labels  # Currently, `invalid_pops` contains evaluated individuals
#
#     # Record the count and execute timespan of model runs during the optimization
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}  # Total time counted according to evaluate_parallel()
#     modelruns_time_sum = {0: 0.}  # Summarize time of every model runs according to pop
#
#     # Generation 0 before optimization
#     stime = time.time()
#     # print(f"***************pop: {pop}")
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # currently, len(pop) may less than pop_select_num
#     pop = toolbox.select(pop, pop_select_num)
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#     # scoop_log(logbook.stream)
#
#     # Begin the generational process
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (cfg.opt.ngens,
#                                                                            cfg.opt.npop)
#     # scoop_log(output_str)
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int] # newly added Pareto fronts
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#         # scoop_log(output_str)
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#         # method1: use crowding distance (normalized as 0~1) as eta
#         # tools.emo.assignCrowdingDist(offspring)
#         # method2: use the index of individual at the sorted offspring list as eta
#         if len(offspring) >= 2:  # when offspring size greater than 2, mate can be done
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:  # No need to continue
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#         # Count the model runs, and execute models
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population
#         # Previous version may result in duplications of the same scenario in one Pareto front,
#         #   thus, I decided to check and remove the duplications first.
#         # pop = toolbox.select(pop + valid_inds + invalid_inds, pop_select_num)
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (gen, invalid_ind_size,
#                                              curtimespan, modelruns_time_sum[gen],
#                                              hypervolume(pop, ref_pt))
#         # scoop_log(hyper_str)
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#         # scoop_log(logbook.stream)
#
#         # Count the newly generated near Pareto fronts
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot 2D near optimal pareto front graphs,
#         #   i.e., (NSE, RSR), (NSE, PBIAS), and (RSR,PBIAS)
#         # And 3D near optimal pareto front graphs, i.e., (NSE, RSR, PBIAS)
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(front, plotlables, cfg.opt.out_dir,
#                                  gen, title, plot_cfg=cali_obj.cfg.plot_cfg)
#         plot_time += time.time() - stime
#
#         # save in file
#         # Header information
#         output_str += 'generation\tcalibrationID\t'
#
#         # 四个参与率定的水深指标
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # D_1321 与 F_19 辅助验证指标；只写入日志，不参与 fitness
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 四个参与率定的水深指标
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # D_1321 与 F_19 辅助验证指标
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution
#
#     # Plot hypervolume and newly executed model count
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generations
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt('%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#                   new_fronts_count, delimiter=str(','), fmt=str('%d'))
#
#     # Save and print timespan information
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt('%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#                   allmodels_exect, delimiter=str(' '), fmt=str('%.4f'))
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % ('\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.sum(0))))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (init_time, exec_time,
#                                                      exec_time_sum, plot_time))
#
#     return pop, logbook
#
#
# if __name__ == "__main__":
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 最终统一参数由 calibration.userdef.py 中的 select_shared_best_index() 选择：
#     # 只依据四个主要实测水深；D_1321 与 F_19 仅用于辅助验证和绘图。
#     # scoop_log(fstats)
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         # In case of 'TypeError: write() argument 1 must be unicode, not str' in Python2.7
#         #   when using unicode_literals, please use '%s' to concatenate string!
#         f.write('%s' % fstats.__str__())
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))





# # -*- coding: utf-8 -*-
# # 以五个实测水深 NSE 率定参数；F_19 作为辅助验证并随结果绘图。
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import Calibration, initialize_calibrations, calibration_objectives,evaluate_nowait_or_skip,evaluate_blocking
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent,cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# #    when paralleled by SCOOP.
# # Thus, DEAP related operations (initialize, register, etc.) are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3','FI','BI']
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS| (percent)
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, min., and |PBIAS|-SED
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# #ljj++
# elif step == 'TEST':
#     #negative is minimal
#     #the number can be any float
#     # multiobj.setdefault('Q_1444', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('Q_1289', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('TOTDOCcon_1289', [['NSE1', 1., -100, '>0'],['R-square', 1., -100, '>0']])
#     # multiobj.setdefault('TOTDOCcon_1444', [['R-square', 1., -100, '>0']])
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# #######
# else:
#     # ------------------------------------------------------------
#     # 率定目标：使用五个实测水深 NSE
#     #
#     # D_1304、D_1305、D_1306、D_1309、D_1310：
#     #     参与 NSGA-II 率定和最终统一参数选择。
#     #
#     # F_19：
#     #     淹没面积仅作为辅助验证和绘图变量，不进入 fitness。
#     # ------------------------------------------------------------
#     multiobj.setdefault('D_1304', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1305', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1306', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1309', [['NSE', 1., -100]])
#     multiobj.setdefault('D_1310', [['NSE', 1., -100]])
#     # Customize your own multiobjective here, such as:
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0.'],
#     #                           ['RSR', -1., 2., '<2.'],
#     #                           ['PBIAS', -1., 50., '<50.']])
#     # multiobj.setdefault('Q_322', [['NSE', 1., -100, '>0']])
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0']])
#
#
# # 会话ID，避免跨实例互相影响（强烈推荐）
# if os.name == 'nt':  # Windows
#     # base_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1'
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
# # # 淹没范围才会用，淹没面积率定不用管
# subbasin_flood_path = base_path + f'/inundation_cali/subbasin_flood'
# subbasin_ids=[1171,1176,1193,1194,1214]
#
# # 不同的实例指定不同的id，避免锁文件相互影响
# RUN_ID = 'CW_2_20260425'
#
# # Check object variables
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
# # Get parameters from `multiobj`
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
# multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
# worse_objects = list(l[2] for v in multiobj.values() for l in v)
# conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)
#
# # ------------------------------------------------------------
# # 辅助验证/绘图变量：读取、计算指标并保存，但绝不进入 fitness。
# #
# # F_19：淹没面积辅助验证，不进入 NSGA-II fitness。
# #
# # output_vars 的顺序固定为 F_19 + 五个率定水深，
# # 与 userdef.py 中绘图时读取列的顺序保持一致。
# # ------------------------------------------------------------
# auxiliary_object_names = {
#     'F_19': ['NSE', 'RSR', 'PBIAS', 'R-square']
# }
#
# output_vars = [
#     'F_19',
#     'D_1304',
#     'D_1305',
#     'D_1306',
#     'D_1309',
#     'D_1310'
# ]
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # The FitnessMulti class equals to (as an example):
# # class FitnessMulti(base.Fitness):
# #     weights = (2., -1., -1.)
# # NOTE that to maintain the compatibility with Python2 and Python3,
# #      the com typecode=str('d') MUST NOT changed to typecode='d', since
# #      the latter will raise TypeError that 'must be char, not unicode'!
# creator.create('Individual', array.array, typecode=str('d'), fitness=creator.FitnessMulti,
#                gen=-1, id=-1,
#                obs=TimeseriesData, sim=TimeseriesData,
#                cali=ObsSimData, vali=ObsSimData,
#                io_time=0., comp_time=0., simu_time=0., runtime=0.)
# # The Individual class equals to:
# # class Individual(array.array):
# #     gen = -1  # Generation No.
# #     id = -1   # Calibration index of current generation
# #     def __init__(self):
# #         self.fitness = FitnessMulti()
#
# # Register NSGA-II related operations
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking,timeout_s=240)
#
#
# # mate and mutate
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based Scenario analysis."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # create reference point for hypervolume
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # read observation data from MongoDB
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#     # F_19 仅供验证/绘图；multiobj 中的五个 D_* 会构成 fitness。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Calibration objectives (fitness only): %s' % str(object_vars))
#     scoop_log('Output/plot variables (includes auxiliary F_19): %s' % str(output_vars))
#     #-------------------------xiaodw, if calibrate inundation extent --------------------
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start="2010-01",
#             end="2010-02",
#             band=1,
#             on_missing="warn",
#             readonly=True
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]["1171"]
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#     #-------------------------xiaodw, if calibrate inundation area --------------------
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population
#     # print(f"cfg.opt is {cfg.opt}")
#     # print(f"npop is: {cfg.opt.npop}")
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#         # print(f"ind: {ind} ")
#     param_values = numpy.array(param_values)
#     # print(f"npop: {cfg.opt.npop}")
#
#     # print(param_values)
#     # Write calibrated values to MongoDB
#     # TODO, extract this function, which is same with `Sensitivity::write_param_values_to_mongodb`.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#     # get the low and up bound of calibrated parameters
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0]
#     up = bounds[:, 1]
#     low = low.tolist()
#     up = up.tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals
#          according to calibration step."""
#         popnum = len(invalid_pops)
#         labels = list()
#         # ---------------------test for no scoop----------------------
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind,base_path,RUN_ID) for ind in invalid_pops]
#         # ---------------------end for no scoop----------------------
#
#         try:  # parallel on multi-processors or clusters using SCOOP
#             from scoop import futures
#             invalid_pops = list(futures.map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#         except ImportError or ImportWarning:  # Python build-in map (serial)
#             invalid_pops = list(map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#
#         scoop_log(f"过滤之前pops长度为:{len(invalid_pops)}")
#         # 关键：把 None（被跳过的个体）过滤掉
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log(f"过滤之后pops长度为:{len(invalid_pops)}")
#
#         for tmpind in invalid_pops:
#             labels = list()
#             tmpfitnessv = list()
#             try:
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#                     tmpfitnessv += tmpvalues[:]
#                     labels += tmplabel[:]
#
#                 # 如果结果合法才赋值
#                 if len(tmpfitnessv) == len(multi_weight):
#                     tmpind.fitness.values = tuple(tmpfitnessv)
#                 else:
#                     raise ValueError("模型返回指标维度不一致！")
#
#             except Exception as e:
#                 scoop_log(
#                     f"[模型失败] id={tmpind.id}, gen={tmpind.gen}, "
#                     f"tmpfitnessv={tmpfitnessv}, multi_weight={multi_weight}, 原因: {e}"
#                 )
#
#                 # 当前 fitness 均为 NSE 最大化目标，失败时必须赋极小值。
#                 # 通用写法同时兼容未来加入最小化指标（如 RMSE、RSR）。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 仅为保存和绘图计算 F_19 指标；不追加到 tmpfitnessv，
#             # 所以 F_19 不参与 NSGA-II 优化或最终统一参数选择。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         f"[辅助验证变量读取失败，不影响率定] id={tmpind.id}, gen={tmpind.gen}, "
#                         f"var={aux_var}, 原因: {aux_e}"
#                     )
#
#         # Filter for a valid solution
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#         return invalid_pops, labels  # Currently, `invalid_pops` contains evaluated individuals
#
#     # Record the count and execute timespan of model runs during the optimization
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}  # Total time counted according to evaluate_parallel()
#     modelruns_time_sum = {0: 0.}  # Summarize time of every model runs according to pop
#
#     # Generation 0 before optimization
#     stime = time.time()
#     # print(f"***************pop: {pop}")
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # currently, len(pop) may less than pop_select_num
#     pop = toolbox.select(pop, pop_select_num)
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#     # scoop_log(logbook.stream)
#
#     # Begin the generational process
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (cfg.opt.ngens,
#                                                                            cfg.opt.npop)
#     # scoop_log(output_str)
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int] # newly added Pareto fronts
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#         # scoop_log(output_str)
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#         # method1: use crowding distance (normalized as 0~1) as eta
#         # tools.emo.assignCrowdingDist(offspring)
#         # method2: use the index of individual at the sorted offspring list as eta
#         if len(offspring) >= 2:  # when offspring size greater than 2, mate can be done
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:  # No need to continue
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#         # Count the model runs, and execute models
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population
#         # Previous version may result in duplications of the same scenario in one Pareto front,
#         #   thus, I decided to check and remove the duplications first.
#         # pop = toolbox.select(pop + valid_inds + invalid_inds, pop_select_num)
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (gen, invalid_ind_size,
#                                              curtimespan, modelruns_time_sum[gen],
#                                              hypervolume(pop, ref_pt))
#         # scoop_log(hyper_str)
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#         # scoop_log(logbook.stream)
#
#         # Count the newly generated near Pareto fronts
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot 2D near optimal pareto front graphs,
#         #   i.e., (NSE, RSR), (NSE, PBIAS), and (RSR,PBIAS)
#         # And 3D near optimal pareto front graphs, i.e., (NSE, RSR, PBIAS)
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(front, plotlables, cfg.opt.out_dir,
#                                  gen, title, plot_cfg=cali_obj.cfg.plot_cfg)
#         plot_time += time.time() - stime
#
#         # save in file
#         # Header information
#         output_str += 'generation\tcalibrationID\t'
#
#         # 四个参与率定的水深指标
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # F_19 辅助验证指标；只写入日志，不参与 fitness
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 四个参与率定的水深指标
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # F_19 辅助验证指标
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution
#
#     # Plot hypervolume and newly executed model count
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generations
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt('%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#                   new_fronts_count, delimiter=str(','), fmt=str('%d'))
#
#     # Save and print timespan information
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt('%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#                   allmodels_exect, delimiter=str(' '), fmt=str('%.4f'))
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % ('\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.sum(0))))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (init_time, exec_time,
#                                                      exec_time_sum, plot_time))
#
#     return pop, logbook
#
#
# if __name__ == "__main__":
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 最终统一参数由 calibration.userdef.py 中的 select_shared_best_index() 选择：
#     # 依据五个实测水深；F_19 仅用于辅助验证和绘图。
#     # scoop_log(fstats)
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         # In case of 'TypeError: write() argument 1 must be unicode, not str' in Python2.7
#         #   when using unicode_literals, please use '%s' to concatenate string!
#         f.write('%s' % fstats.__str__())
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))






# # 三个实测水深率定
# # -*- coding: utf-8 -*-
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import (
#     Calibration,
#     initialize_calibrations,
#     calibration_objectives,
#     evaluate_nowait_or_skip,
#     evaluate_blocking,
# )
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent, cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# # when paralleled by SCOOP. Thus, DEAP related operations are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = [
#     'NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE',
#     'lnNSE', 'NSE1', 'NSE3', 'FI', 'BI'
# ]
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', 'TEST', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
#
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS|.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, and min. |PBIAS|-SED.
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment.
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED.
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# elif step == 'TEST':
#     # Test example.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# else:
#     # ============================================================
#     # 四个水深点统一使用NSE率定
#     #
#     # D_1366、D_1367、D_1368、D_1370参与fitness；
#     # D_1375和F_18只用于辅助验证和绘图。
#     # ============================================================
#
#     multiobj.setdefault(
#         'D_1366',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1367',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1368',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     # multiobj.setdefault(
#     #     'D_1370',
#     #     [
#     #         ['NSE', 1.0, -100.0],
#     #     ]
#     # )
#
#
# # ------------------------------------------------------------
# # 会话路径设置。
# # Windows 和 Linux 分开写，避免路径错误。
# # 如果你实际工程目录不是 CW_2，请在这里修改 base_path 和 RUN_ID。
# # ------------------------------------------------------------
# if os.name == 'nt':  # Windows
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
#
# # 淹没范围率定才会用到；如果只是水深/面积率定，一般不用管。
# subbasin_flood_path = base_path + '/inundation_cali/subbasin_flood'
# subbasin_ids = [1171, 1176, 1193, 1194, 1214]
#
# # 不同实例指定不同 RUN_ID，避免锁文件互相影响。
# RUN_ID = 'CW_2_20260425_depth3'
#
#
# # Check object variables.
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
#
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
#
# # Get parameters from `multiobj`.
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
# multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
# worse_objects = list(l[2] for v in multiobj.values() for l in v)
# conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)
#
#
# # ============================================================
# # 辅助诊断指标
# #
# # NSE已经进入fitness，因此这里不再重复计算NSE。
# # NSE1在当前统计代码中实际为KGE。
# #
# # D_1375和F_18不参与率定，只用于辅助验证和绘图。
# # ============================================================
# auxiliary_object_names = {
#     'D_1366': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1367': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1368': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1370': [
#     'NSE',
#     'NSE1',
#     'lnNSE',
#     'RMSE',
#     'RSR',
#     'PBIAS',
#     'R-square',
# ],
#
#     # 只用于独立验证
#     'D_1375': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # 只用于独立验证
#     'F_18': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
# }
#
# # 输出和绘图变量。顺序固定，便于 userdef.py 中绘图读取。
# # F_19、D_1309、D_1310 只用于辅助验证和绘图；不进入 fitness。
# output_vars = [
#     'F_18',
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
#     'D_1375',
# ]
#
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # NOTE: to maintain compatibility with Python2 and Python3,
# # typecode=str('d') MUST NOT be changed to typecode='d'.
# creator.create(
#     'Individual',
#     array.array,
#     typecode=str('d'),
#     fitness=creator.FitnessMulti,
#     gen=-1,
#     id=-1,
#     obs=TimeseriesData,
#     sim=TimeseriesData,
#     cali=ObsSimData,
#     vali=ObsSimData,
#     io_time=0.,
#     comp_time=0.,
#     simu_time=0.,
#     runtime=0.,
# )
#
#
# # Register NSGA-II related operations.
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking, timeout_s=240)
#
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based scenario analysis/calibration."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables.
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # Create reference point for hypervolume.
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # Read observation data from MongoDB.
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once.
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#
#     # 读取所有需要输出/绘图的变量。
#     # fitness 只由 object_vars 决定，即 D_1304、D_1305、D_1306。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Calibration objectives (fitness only): %s' % str(object_vars))
#     scoop_log('Output/plot variables (includes auxiliary variables): %s' % str(output_vars))
#
#     # If calibrating inundation extent.
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start='2010-01',
#             end='2010-02',
#             band=1,
#             on_missing='warn',
#             readonly=True,
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]['1171']
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population.
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#
#     param_values = numpy.array(param_values)
#
#     # Write calibrated values to MongoDB.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#     # Get the low and up bound of calibrated parameters.
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0].tolist()
#     up = bounds[:, 1].tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals."""
#         popnum = len(invalid_pops)
#         labels = list()
#
#         # Serial debug version:
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind, base_path, RUN_ID) for ind in invalid_pops]
#
#         try:
#             # Parallel on multi-processors or clusters using SCOOP.
#             from scoop import futures
#             invalid_pops = list(futures.map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#         except ImportError or ImportWarning:
#             # Python built-in map, serial mode.
#             invalid_pops = list(map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#
#         scoop_log('过滤之前pops长度为:%d' % len(invalid_pops))
#
#         # 把被跳过或失败返回 -1 的个体过滤掉。
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log('过滤之后pops长度为:%d' % len(invalid_pops))
#
#         for tmpind in invalid_pops:
#             labels = list()
#             tmpfitnessv = list()
#             try:
#                 # 只计算主率定目标并写入 fitness。
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#                     tmpfitnessv += tmpvalues[:]
#                     labels += tmplabel[:]
#
#                 # 如果结果合法才赋值。
#                 if len(tmpfitnessv) == len(multi_weight):
#                     tmpind.fitness.values = tuple(tmpfitnessv)
#                 else:
#                     raise ValueError('模型返回指标维度不一致！')
#
#             except Exception as e:
#                 scoop_log(
#                     '[模型失败] id=%s, gen=%s, tmpfitnessv=%s, multi_weight=%s, 原因: %s'
#                     % (tmpind.id, tmpind.gen, tmpfitnessv, multi_weight, e)
#                 )
#
#                 # 当前 fitness 均为 NSE 最大化目标，失败时必须赋极小值。
#                 # 该通用写法也兼容未来加入最小化指标，例如 RMSE、RSR。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 辅助变量只计算并保存指标，不追加到 tmpfitnessv。
#             # 因此 D_1309、D_1310、F_19 不参与 NSGA-II 优化或最终 fitness。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         '[辅助验证变量读取失败，不影响率定] id=%s, gen=%s, var=%s, 原因: %s'
#                         % (tmpind.id, tmpind.gen, aux_var, aux_e)
#                     )
#
#         # Filter for a valid solution.
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#
#         # Currently, invalid_pops contains evaluated individuals.
#         return invalid_pops, labels
#
#     # Record the count and execute timespan of model runs during optimization.
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}
#     modelruns_time_sum = {0: 0.}
#
#     # Generation 0 before optimization.
#     stime = time.time()
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # Currently, len(pop) may less than pop_select_num.
#     pop = toolbox.select(pop, pop_select_num)
#
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#
#     # Begin the generational process.
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (
#         cfg.opt.ngens,
#         cfg.opt.npop,
#     )
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     # Newly added Pareto fronts count.
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int]
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#
#         # Generate offspring by crossover and mutation.
#         if len(offspring) >= 2:
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness.
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB.
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#         # Count model runs and execute models.
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population.
#         # Remove duplications first.
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (
#                         gen,
#                         invalid_ind_size,
#                         curtimespan,
#                         modelruns_time_sum[gen],
#                         hypervolume(pop, ref_pt),
#                     )
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#
#         # Count the newly generated near Pareto fronts.
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot near Pareto optimal solutions.
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(
#             front,
#             plotlables,
#             cfg.opt.out_dir,
#             gen,
#             title,
#             plot_cfg=cali_obj.cfg.plot_cfg,
#         )
#         plot_time += time.time() - stime
#
#         # Save in file.
#         output_str += 'generation\tcalibrationID\t'
#
#         # 三个参与率定的水深指标。
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # 辅助验证指标：D_1309、D_1310、F_19。
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 三个参与率定的水深指标。
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # 辅助验证指标。
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution.
#
#     # Plot hypervolume and newly executed model count.
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generation.
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt(
#         '%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#         new_fronts_count,
#         delimiter=str(','),
#         fmt=str('%d'),
#     )
#
#     # Save and print timespan information.
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt(
#         '%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#         allmodels_exect,
#         delimiter=str(' '),
#         fmt=str('%.4f'),
#     )
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % (
#                   '\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.sum(0)),
#               ))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (
#                   init_time,
#                   exec_time,
#                   exec_time_sum,
#                   plot_time,
#               ))
#
#     return pop, logbook
#
#
# if __name__ == '__main__':
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 注意：最终统一参数如果在 calibration.userdef.py 的 select_shared_best_index() 中选择，
#     # 那里也应同步改成只依据 D_1304、D_1305、D_1306 选择；
#     # D_1309、D_1310、F_19 仅用于辅助验证和绘图。
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         f.write('%s' % fstats.__str__())
#
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))



# # 四个实测水深点 + F_18淹没面积NSE率定-20260810
# # -*- coding: utf-8 -*-
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import (
#     Calibration,
#     initialize_calibrations,
#     calibration_objectives,
#     evaluate_nowait_or_skip,
#     evaluate_blocking,
# )
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent, cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# # when paralleled by SCOOP. Thus, DEAP related operations are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = [
#     'NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE',
#     'lnNSE', 'NSE1', 'NSE3', 'FI', 'BI'
# ]
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', 'TEST', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
#
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS|.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, and min. |PBIAS|-SED.
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment.
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED.
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# elif step == 'TEST':
#     # Test example.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# else:
#     # ============================================================
#     # T5 四个实测水深点，全部使用 NSE 率定
#     #
#     # 参与 fitness：
#     # D_1366、D_1367、D_1368、D_1370
#     #
#     # D_1375：只有一个实测值，只用于辅助检查
#     # F_18：只用于独立验证和绘图
#     # ============================================================
#
#     multiobj.setdefault(
#         'D_1366',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1367',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1368',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1370',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#
# # ------------------------------------------------------------
# # 会话路径设置。
# # Windows 和 Linux 分开写，避免路径错误。
# # 如果你实际工程目录不是 CW_2，请在这里修改 base_path 和 RUN_ID。
# # ------------------------------------------------------------
# if os.name == 'nt':  # Windows
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
#
# # 淹没范围率定才会用到；如果只是水深/面积率定，一般不用管。
# subbasin_flood_path = base_path + '/inundation_cali/subbasin_flood'
# subbasin_ids = [1171, 1176, 1193, 1194, 1214]
#
# # 不同实例指定不同 RUN_ID，避免锁文件互相影响。
# RUN_ID = 'CW_2_depth4_F18_NSE'
#
#
# # Check object variables.
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
#
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
#
# # Get parameters from `multiobj`.
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
# multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
# worse_objects = list(l[2] for v in multiobj.values() for l in v)
# conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)
#
#
# # ============================================================
# # 辅助诊断指标
# #
# # D_1366、D_1367、D_1368、D_1370、F_18的NSE已经进入fitness，
# # 因此辅助指标中不再重复计算NSE。
# #
# # NSE1在当前统计代码中实际为KGE。
# # D_1375不参与率定，只用于辅助验证和绘图。
# # ============================================================
# auxiliary_object_names = {
#     'D_1366': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1367': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1368': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1370': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # F_18 不参与率定，作为独立验证。
#     'F_18': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # D_1375 只有一个实测值，不计算 NSE、R² 等时间序列指标。
#     'D_1375': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
# }
#
# # 输出和绘图变量。顺序固定，便于 userdef.py 中绘图读取。
# # F_19、D_1309、D_1310 只用于辅助验证和绘图；不进入 fitness。
# output_vars = [
#     'F_18',
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
#     'D_1375',
# ]
#
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # NOTE: to maintain compatibility with Python2 and Python3,
# # typecode=str('d') MUST NOT be changed to typecode='d'.
# creator.create(
#     'Individual',
#     array.array,
#     typecode=str('d'),
#     fitness=creator.FitnessMulti,
#     gen=-1,
#     id=-1,
#     obs=TimeseriesData,
#     sim=TimeseriesData,
#     cali=ObsSimData,
#     vali=ObsSimData,
#     io_time=0.,
#     comp_time=0.,
#     simu_time=0.,
#     runtime=0.,
# )
#
#
# # Register NSGA-II related operations.
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking, timeout_s=240)
#
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based scenario analysis/calibration."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables.
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # Create reference point for hypervolume.
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # Read observation data from MongoDB.
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once.
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#
#     # 读取所有需要输出/绘图的变量。
#     # fitness由object_vars决定：D_1366、D_1367、D_1368、D_1370和F_18。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Calibration objectives (fitness only): %s' % str(object_vars))
#     scoop_log('Output/plot variables (includes auxiliary variables): %s' % str(output_vars))
#
#     # If calibrating inundation extent.
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start='2010-01',
#             end='2010-02',
#             band=1,
#             on_missing='warn',
#             readonly=True,
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]['1171']
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population.
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#
#     param_values = numpy.array(param_values)
#
#     # Write calibrated values to MongoDB.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#     # Get the low and up bound of calibrated parameters.
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0].tolist()
#     up = bounds[:, 1].tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals."""
#         popnum = len(invalid_pops)
#         labels = list()
#
#         # Serial debug version:
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind, base_path, RUN_ID) for ind in invalid_pops]
#
#         try:
#             # Parallel on multi-processors or clusters using SCOOP.
#             from scoop import futures
#             invalid_pops = list(futures.map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#         except ImportError or ImportWarning:
#             # Python built-in map, serial mode.
#             invalid_pops = list(map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#
#         scoop_log('过滤之前pops长度为:%d' % len(invalid_pops))
#
#         # 把被跳过或失败返回 -1 的个体过滤掉。
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log('过滤之后pops长度为:%d' % len(invalid_pops))
#
#         for tmpind in invalid_pops:
#             labels = list()
#             tmpfitnessv = list()
#             try:
#                 # 只计算主率定目标并写入 fitness。
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#                     tmpfitnessv += tmpvalues[:]
#                     labels += tmplabel[:]
#
#                 # 如果结果合法才赋值。
#                 if len(tmpfitnessv) == len(multi_weight):
#                     tmpind.fitness.values = tuple(tmpfitnessv)
#                 else:
#                     raise ValueError('模型返回指标维度不一致！')
#
#             except Exception as e:
#                 scoop_log(
#                     '[模型失败] id=%s, gen=%s, tmpfitnessv=%s, multi_weight=%s, 原因: %s'
#                     % (tmpind.id, tmpind.gen, tmpfitnessv, multi_weight, e)
#                 )
#
#                 # 当前5个fitness均为NSE最大化目标，失败时必须赋极小值。
#                 # 该通用写法也兼容未来加入最小化指标，例如 RMSE、RSR。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 辅助变量只计算并保存指标，不追加到 tmpfitnessv。
#             # 因此D_1375不参与NSGA-II优化或fitness。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         '[辅助验证变量读取失败，不影响率定] id=%s, gen=%s, var=%s, 原因: %s'
#                         % (tmpind.id, tmpind.gen, aux_var, aux_e)
#                     )
#
#         # Filter for a valid solution.
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#
#         # Currently, invalid_pops contains evaluated individuals.
#         return invalid_pops, labels
#
#     # Record the count and execute timespan of model runs during optimization.
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}
#     modelruns_time_sum = {0: 0.}
#
#     # Generation 0 before optimization.
#     stime = time.time()
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # Currently, len(pop) may less than pop_select_num.
#     pop = toolbox.select(pop, pop_select_num)
#
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#
#     # Begin the generational process.
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (
#         cfg.opt.ngens,
#         cfg.opt.npop,
#     )
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     # Newly added Pareto fronts count.
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int]
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#
#         # Generate offspring by crossover and mutation.
#         if len(offspring) >= 2:
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness.
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB.
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#         # Count model runs and execute models.
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population.
#         # Remove duplications first.
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (
#                         gen,
#                         invalid_ind_size,
#                         curtimespan,
#                         modelruns_time_sum[gen],
#                         hypervolume(pop, ref_pt),
#                     )
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#
#         # Count the newly generated near Pareto fronts.
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot near Pareto optimal solutions.
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(
#             front,
#             plotlables,
#             cfg.opt.out_dir,
#             gen,
#             title,
#             plot_cfg=cali_obj.cfg.plot_cfg,
#         )
#         plot_time += time.time() - stime
#
#         # Save in file.
#         output_str += 'generation\tcalibrationID\t'
#
#         # 五个参与率定的指标：四个水深点和F_18。
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # 辅助诊断指标：D_1375以及主目标的其他统计指标。
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 五个参与率定的指标：四个水深点和F_18。
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # 辅助验证指标。
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution.
#
#     # Plot hypervolume and newly executed model count.
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generation.
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt(
#         '%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#         new_fronts_count,
#         delimiter=str(','),
#         fmt=str('%d'),
#     )
#
#     # Save and print timespan information.
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt(
#         '%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#         allmodels_exect,
#         delimiter=str(' '),
#         fmt=str('%.4f'),
#     )
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % (
#                   '\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.sum(0)),
#               ))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (
#                   init_time,
#                   exec_time,
#                   exec_time_sum,
#                   plot_time,
#               ))
#
#     return pop, logbook
#
#
# if __name__ == '__main__':
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 注意：最终统一参数如果在calibration.userdef.py的
#     # select_shared_best_index()中选择，后处理代码也必须同步使用
#     # D_1366、D_1367、D_1368、D_1370和F_18的NSE。
#     # D_1375仅用于辅助验证和绘图。
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         f.write('%s' % fstats.__str__())
#
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))



# # 四个实测水深点 + F_18淹没面积NSE率定
# # -*- coding: utf-8 -*-
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import (
#     Calibration,
#     initialize_calibrations,
#     calibration_objectives,
#     evaluate_nowait_or_skip,
#     evaluate_blocking,
# )
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent, cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# # when paralleled by SCOOP. Thus, DEAP related operations are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = [
#     'NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE',
#     'lnNSE', 'NSE1', 'NSE3', 'FI', 'BI'
# ]
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', 'TEST', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
#
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS|.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, and min. |PBIAS|-SED.
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment.
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED.
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# elif step == 'TEST':
#     # Test example.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# else:
#     # ============================================================
#     # T5 四个实测水深点，全部使用 NSE 计算
#     #
#     # 原始NSE：
#     # D_1366、D_1367、D_1368、D_1370
#     #
#     # 真正参与fitness：
#     # minNSE：四个水深点NSE中的最小值
#     # meanNSE：四个水深点NSE的平均值
#     #
#     # D_1375：只有一个实测值，只用于辅助检查
#     # F_18：只用于独立验证和绘图
#     # ============================================================
#
#     multiobj.setdefault(
#         'D_1366',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1367',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1368',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1370',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#
# # ------------------------------------------------------------
# # 会话路径设置。
# # Windows 和 Linux 分开写，避免路径错误。
# # 如果你实际工程目录不是 CW_2，请在这里修改 base_path 和 RUN_ID。
# # ------------------------------------------------------------
# if os.name == 'nt':  # Windows
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
#
# # 淹没范围率定才会用到；如果只是水深/面积率定，一般不用管。
# subbasin_flood_path = base_path + '/inundation_cali/subbasin_flood'
# subbasin_ids = [1171, 1176, 1193, 1194, 1214]
#
# # 不同实例指定不同 RUN_ID，避免锁文件互相影响。
# RUN_ID = 'CW_2_depth4_minNSE_meanNSE'
#
#
# # Check object variables.
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
#
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
#
# # Get parameters from `multiobj`.
# # 四个水深点原始NSE仍然通过multiobj计算和保存。
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
#
# # NSGA-II真正优化两个目标：
# # 1. minNSE
# # 2. meanNSE
# # 两个目标均为最大化。
# fitness_names = ['minNSE', 'meanNSE']
# multi_weight = (1.0, 1.0)
# worse_objects = [-100.0, -100.0]
# conditions = [None, None]
#
#
# # ============================================================
# # 辅助诊断指标
# #
# # D_1366、D_1367、D_1368、D_1370的NSE
# # 用于构造minNSE和meanNSE，因此辅助指标中不再重复计算NSE。
# #
# # NSE1在当前统计代码中实际为KGE。
# # F_18和D_1375不参与率定，只用于辅助验证和绘图。
# # ============================================================
# auxiliary_object_names = {
#     'D_1366': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1367': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1368': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1370': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # F_18 不参与率定，作为独立验证。
#     'F_18': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # D_1375 不参与率定，只用于辅助验证和绘图。
#     # 按原来的方式继续计算全部指标。
#     'D_1375': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
# }
#
# # 输出和绘图变量。顺序固定，便于 userdef.py 中绘图读取。
# # F_18和D_1375只用于辅助验证和绘图；不进入 fitness。
# output_vars = [
#     'F_18',
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
#     'D_1375',
# ]
#
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # NOTE: to maintain compatibility with Python2 and Python3,
# # typecode=str('d') MUST NOT be changed to typecode='d'.
# creator.create(
#     'Individual',
#     array.array,
#     typecode=str('d'),
#     fitness=creator.FitnessMulti,
#     gen=-1,
#     id=-1,
#     obs=TimeseriesData,
#     sim=TimeseriesData,
#     cali=ObsSimData,
#     vali=ObsSimData,
#     io_time=0.,
#     comp_time=0.,
#     simu_time=0.,
#     runtime=0.,
# )
#
#
# # Register NSGA-II related operations.
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking, timeout_s=240)
#
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based scenario analysis/calibration."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables.
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # Create reference point for hypervolume.
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # Read observation data from MongoDB.
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once.
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#
#     # 读取所有需要输出/绘图的变量。
#     # 四个水深点NSE用于构造minNSE和meanNSE。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Raw NSE variables: %s' % str(object_vars))
#     scoop_log('Calibration objectives (fitness only): %s' % str(fitness_names))
#     scoop_log('Output/plot variables (includes auxiliary variables): %s' % str(output_vars))
#
#     # If calibrating inundation extent.
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start='2010-01',
#             end='2010-02',
#             band=1,
#             on_missing='warn',
#             readonly=True,
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]['1171']
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population.
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#
#     param_values = numpy.array(param_values)
#
#     # Write calibrated values to MongoDB.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#     # Get the low and up bound of calibrated parameters.
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0].tolist()
#     up = bounds[:, 1].tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals."""
#         popnum = len(invalid_pops)
#         labels = list()
#
#         # Serial debug version:
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind, base_path, RUN_ID) for ind in invalid_pops]
#
#         try:
#             # Parallel on multi-processors or clusters using SCOOP.
#             from scoop import futures
#             invalid_pops = list(futures.map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#         except ImportError or ImportWarning:
#             # Python built-in map, serial mode.
#             invalid_pops = list(map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#
#         scoop_log('过滤之前pops长度为:%d' % len(invalid_pops))
#
#         # 把被跳过或失败返回 -1 的个体过滤掉。
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log('过滤之后pops长度为:%d' % len(invalid_pops))
#
#         for tmpind in invalid_pops:
#             labels = fitness_names[:]
#             tmpfitnessv = list()
#             try:
#                 # ------------------------------------------------------------
#                 # 先计算四个水深点各自的原始NSE。
#                 #
#                 # 顺序：
#                 # D_1366、D_1367、D_1368、D_1370。
#                 #
#                 # 这四个NSE仍然正常保存到cali结果中，
#                 # 只是它们不再直接作为4个独立fitness。
#                 # ------------------------------------------------------------
#                 raw_nse_values = list()
#
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#
#                     if len(tmpvalues) != 1:
#                         raise ValueError(
#                             '%s返回的NSE数量不等于1: %s'
#                             % (k, str(tmpvalues))
#                         )
#
#                     raw_nse_values.append(float(tmpvalues[0]))
#
#                 if len(raw_nse_values) != 4:
#                     raise ValueError(
#                         '四个水深点NSE数量不等于4: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 raw_nse_values = numpy.array(raw_nse_values, dtype=float)
#
#                 if not numpy.all(numpy.isfinite(raw_nse_values)):
#                     raise ValueError(
#                         '四个水深点NSE存在NaN或Inf: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 if numpy.any(raw_nse_values <= -9000.0):
#                     raise ValueError(
#                         '四个水深点NSE存在无效惩罚值: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 # ------------------------------------------------------------
#                 # 真正的NSGA-II双目标：
#                 #
#                 # 1. minNSE：
#                 #    四个水深点中最差的NSE，越大越好。
#                 #
#                 # 2. meanNSE：
#                 #    四个水深点NSE的平均值，越大越好。
#                 # ------------------------------------------------------------
#                 min_nse = float(numpy.min(raw_nse_values))
#                 mean_nse = float(numpy.mean(raw_nse_values))
#
#                 tmpfitnessv = [
#                     min_nse,
#                     mean_nse,
#                 ]
#
#                 tmpind.fitness.values = tuple(tmpfitnessv)
#
#             except Exception as e:
#                 scoop_log(
#                     '[模型失败] id=%s, gen=%s, tmpfitnessv=%s, multi_weight=%s, 原因: %s'
#                     % (tmpind.id, tmpind.gen, tmpfitnessv, multi_weight, e)
#                 )
#
#                 # 当前两个fitness：minNSE和meanNSE均为最大化目标，
#                 # 失败时必须赋极小值。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 辅助变量只计算并保存指标，不追加到 tmpfitnessv。
#             # 因此F_18和D_1375不参与NSGA-II优化或fitness。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         '[辅助验证变量读取失败，不影响率定] id=%s, gen=%s, var=%s, 原因: %s'
#                         % (tmpind.id, tmpind.gen, aux_var, aux_e)
#                     )
#
#         # Filter for a valid solution.
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#
#         # Currently, invalid_pops contains evaluated individuals.
#         return invalid_pops, labels
#
#     # Record the count and execute timespan of model runs during optimization.
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}
#     modelruns_time_sum = {0: 0.}
#
#     # Generation 0 before optimization.
#     stime = time.time()
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # Currently, len(pop) may less than pop_select_num.
#     pop = toolbox.select(pop, pop_select_num)
#
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#
#     # Begin the generational process.
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (
#         cfg.opt.ngens,
#         cfg.opt.npop,
#     )
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     # Newly added Pareto fronts count.
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int]
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#
#         # Generate offspring by crossover and mutation.
#         if len(offspring) >= 2:
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness.
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB.
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#         # Count model runs and execute models.
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population.
#         # Remove duplications first.
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (
#                         gen,
#                         invalid_ind_size,
#                         curtimespan,
#                         modelruns_time_sum[gen],
#                         hypervolume(pop, ref_pt),
#                     )
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#
#         # Count the newly generated near Pareto fronts.
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot near Pareto optimal solutions.
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(
#             front,
#             plotlables,
#             cfg.opt.out_dir,
#             gen,
#             title,
#             plot_cfg=cali_obj.cfg.plot_cfg,
#         )
#         plot_time += time.time() - stime
#
#         # Save in file.
#         output_str += 'generation\tcalibrationID\t'
#
#         # 四个水深点的原始NSE仍然全部输出到日志。
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # 辅助诊断指标。
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 四个水深点的原始NSE仍然全部输出到日志。
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # 辅助验证指标。
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution.
#
#     # Plot hypervolume and newly executed model count.
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generation.
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt(
#         '%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#         new_fronts_count,
#         delimiter=str(','),
#         fmt=str('%d'),
#     )
#
#     # Save and print timespan information.
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt(
#         '%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#         allmodels_exect,
#         delimiter=str(' '),
#         fmt=str('%.4f'),
#     )
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % (
#                   '\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.sum(0)),
#               ))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (
#                   init_time,
#                   exec_time,
#                   exec_time_sum,
#                   plot_time,
#               ))
#
#     return pop, logbook
#
#
# if __name__ == '__main__':
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 当前率定使用四个水深点原始NSE构造：
#     # 1. minNSE
#     # 2. meanNSE
#     # NSGA-II同时最大化这两个目标。
#     # F_18和D_1375仅用于辅助验证和绘图。
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         f.write('%s' % fstats.__str__())
#
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))



# # 三个实测水深点 + D_1370/F_18辅助验证NSE率定  20260819
# # -*- coding: utf-8 -*-
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import (
#     Calibration,
#     initialize_calibrations,
#     calibration_objectives,
#     evaluate_nowait_or_skip,
#     evaluate_blocking,
# )
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent, cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# # when paralleled by SCOOP. Thus, DEAP related operations are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = [
#     'NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE',
#     'lnNSE', 'NSE1', 'NSE3', 'FI', 'BI'
# ]
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', 'TEST', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
#
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS|.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, and min. |PBIAS|-SED.
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment.
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED.
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# elif step == 'TEST':
#     # Test example.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# else:
#     # ============================================================
#     # T5 三个实测水深点，全部使用 NSE 计算
#     #
#     # 原始NSE：
#     # D_1366、D_1367、D_1368
#     #
#     # 真正参与fitness：
#     # minNSE：三个水深点NSE中的最小值
#     # meanNSE：三个水深点NSE的平均值
#     #
#     # D_1370：不参与fitness，只用于独立验证和绘图
#     # D_1375：只有一个实测值，只用于辅助检查
#     # F_18：只用于独立验证和绘图
#     # ============================================================
#
#     multiobj.setdefault(
#         'D_1366',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1367',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1368',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#
# # ------------------------------------------------------------
# # 会话路径设置。
# # Windows 和 Linux 分开写，避免路径错误。
# # 如果你实际工程目录不是 CW_2，请在这里修改 base_path 和 RUN_ID。
# # ------------------------------------------------------------
# if os.name == 'nt':  # Windows
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
#
# # 淹没范围率定才会用到；如果只是水深/面积率定，一般不用管。
# subbasin_flood_path = base_path + '/inundation_cali/subbasin_flood'
# subbasin_ids = [1171, 1176, 1193, 1194, 1214]
#
# # 不同实例指定不同 RUN_ID，避免锁文件互相影响。
# RUN_ID = 'CW_2_depth3_minNSE_meanNSE'
#
#
# # Check object variables.
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
#
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
#
# # Get parameters from `multiobj`.
# # 三个水深点原始NSE仍然通过multiobj计算和保存。
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
#
# # NSGA-II真正优化两个目标：
# # 1. minNSE
# # 2. meanNSE
# # 两个目标均为最大化。
# fitness_names = ['minNSE', 'meanNSE']
# multi_weight = (1.0, 1.0)
# worse_objects = [-100.0, -100.0]
# conditions = [None, None]
#
#
# # ============================================================
# # 辅助诊断指标
# #
# # D_1366、D_1367、D_1368的NSE
# # 用于构造minNSE和meanNSE，因此辅助指标中不再重复计算NSE。
# #
# # NSE1在当前统计代码中实际为KGE。
# # D_1370、F_18和D_1375不参与率定，只用于辅助验证和绘图。
# # ============================================================
# auxiliary_object_names = {
#     'D_1366': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1367': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1368': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # D_1370 不参与率定，作为独立验证。
#     # 由于已经从multiobj移除，因此这里补回NSE。
#     'D_1370': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # F_18 不参与率定，作为独立验证。
#     'F_18': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # D_1375 不参与率定，只用于辅助验证和绘图。
#     # 按原来的方式继续计算全部指标。
#     'D_1375': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
# }
#
# # 输出和绘图变量。顺序固定，便于 userdef.py 中绘图读取。
# # D_1370、F_18和D_1375只用于辅助验证和绘图；不进入 fitness。
# output_vars = [
#     'F_18',
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
#     'D_1375',
# ]
#
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # NOTE: to maintain compatibility with Python2 and Python3,
# # typecode=str('d') MUST NOT be changed to typecode='d'.
# creator.create(
#     'Individual',
#     array.array,
#     typecode=str('d'),
#     fitness=creator.FitnessMulti,
#     gen=-1,
#     id=-1,
#     obs=TimeseriesData,
#     sim=TimeseriesData,
#     cali=ObsSimData,
#     vali=ObsSimData,
#     io_time=0.,
#     comp_time=0.,
#     simu_time=0.,
#     runtime=0.,
# )
#
#
# # Register NSGA-II related operations.
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking, timeout_s=240)
#
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based scenario analysis/calibration."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables.
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # Create reference point for hypervolume.
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # Read observation data from MongoDB.
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once.
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#
#     # 读取所有需要输出/绘图的变量。
#     # 三个水深点NSE用于构造minNSE和meanNSE。
#     # D_1370仍然读取，但只作为辅助验证。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Raw NSE variables: %s' % str(object_vars))
#     scoop_log('Calibration objectives (fitness only): %s' % str(fitness_names))
#     scoop_log('Output/plot variables (includes auxiliary variables): %s' % str(output_vars))
#
#     # If calibrating inundation extent.
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start='2010-01',
#             end='2010-02',
#             band=1,
#             on_missing='warn',
#             readonly=True,
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]['1171']
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population.
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#
#     param_values = numpy.array(param_values)
#
#     # Write calibrated values to MongoDB.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#     # Get the low and up bound of calibrated parameters.
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0].tolist()
#     up = bounds[:, 1].tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals."""
#         popnum = len(invalid_pops)
#         labels = list()
#
#         # Serial debug version:
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind, base_path, RUN_ID) for ind in invalid_pops]
#
#         try:
#             # Parallel on multi-processors or clusters using SCOOP.
#             from scoop import futures
#             invalid_pops = list(futures.map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#         except ImportError or ImportWarning:
#             # Python built-in map, serial mode.
#             invalid_pops = list(map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#
#         scoop_log('过滤之前pops长度为:%d' % len(invalid_pops))
#
#         # 把被跳过或失败返回 -1 的个体过滤掉。
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log('过滤之后pops长度为:%d' % len(invalid_pops))
#
#         for tmpind in invalid_pops:
#             labels = fitness_names[:]
#             tmpfitnessv = list()
#             try:
#                 # ------------------------------------------------------------
#                 # 先计算三个水深点各自的原始NSE。
#                 #
#                 # 顺序：
#                 # D_1366、D_1367、D_1368。
#                 #
#                 # 这三个NSE仍然正常保存到cali结果中，
#                 # 只是它们不再直接作为3个独立fitness。
#                 #
#                 # D_1370不在这里计算，因此不会参与minNSE和meanNSE。
#                 # ------------------------------------------------------------
#                 raw_nse_values = list()
#
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#
#                     if len(tmpvalues) != 1:
#                         raise ValueError(
#                             '%s返回的NSE数量不等于1: %s'
#                             % (k, str(tmpvalues))
#                         )
#
#                     raw_nse_values.append(float(tmpvalues[0]))
#
#                 if len(raw_nse_values) != 3:
#                     raise ValueError(
#                         '三个水深点NSE数量不等于3: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 raw_nse_values = numpy.array(raw_nse_values, dtype=float)
#
#                 if not numpy.all(numpy.isfinite(raw_nse_values)):
#                     raise ValueError(
#                         '三个水深点NSE存在NaN或Inf: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 if numpy.any(raw_nse_values <= -9000.0):
#                     raise ValueError(
#                         '三个水深点NSE存在无效惩罚值: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 # ------------------------------------------------------------
#                 # 真正的NSGA-II双目标：
#                 #
#                 # 1. minNSE：
#                 #    三个水深点中最差的NSE，越大越好。
#                 #
#                 # 2. meanNSE：
#                 #    三个水深点NSE的平均值，越大越好。
#                 # ------------------------------------------------------------
#                 min_nse = float(numpy.min(raw_nse_values))
#                 mean_nse = float(numpy.mean(raw_nse_values))
#
#                 tmpfitnessv = [
#                     min_nse,
#                     mean_nse,
#                 ]
#
#                 tmpind.fitness.values = tuple(tmpfitnessv)
#
#             except Exception as e:
#                 scoop_log(
#                     '[模型失败] id=%s, gen=%s, tmpfitnessv=%s, multi_weight=%s, 原因: %s'
#                     % (tmpind.id, tmpind.gen, tmpfitnessv, multi_weight, e)
#                 )
#
#                 # 当前两个fitness：minNSE和meanNSE均为最大化目标，
#                 # 失败时必须赋极小值。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 辅助变量只计算并保存指标，不追加到 tmpfitnessv。
#             # 因此D_1370、F_18和D_1375不参与NSGA-II优化或fitness。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         '[辅助验证变量读取失败，不影响率定] id=%s, gen=%s, var=%s, 原因: %s'
#                         % (tmpind.id, tmpind.gen, aux_var, aux_e)
#                     )
#
#         # Filter for a valid solution.
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#
#         # Currently, invalid_pops contains evaluated individuals.
#         return invalid_pops, labels
#
#     # Record the count and execute timespan of model runs during optimization.
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}
#     modelruns_time_sum = {0: 0.}
#
#     # Generation 0 before optimization.
#     stime = time.time()
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # Currently, len(pop) may less than pop_select_num.
#     pop = toolbox.select(pop, pop_select_num)
#
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#
#     # Begin the generational process.
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (
#         cfg.opt.ngens,
#         cfg.opt.npop,
#     )
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     # Newly added Pareto fronts count.
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int]
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#
#         # Generate offspring by crossover and mutation.
#         if len(offspring) >= 2:
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness.
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB.
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#         # Count model runs and execute models.
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population.
#         # Remove duplications first.
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (
#                         gen,
#                         invalid_ind_size,
#                         curtimespan,
#                         modelruns_time_sum[gen],
#                         hypervolume(pop, ref_pt),
#                     )
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#
#         # Count the newly generated near Pareto fronts.
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot near Pareto optimal solutions.
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(
#             front,
#             plotlables,
#             cfg.opt.out_dir,
#             gen,
#             title,
#             plot_cfg=cali_obj.cfg.plot_cfg,
#         )
#         plot_time += time.time() - stime
#
#         # Save in file.
#         output_str += 'generation\tcalibrationID\t'
#
#         # 三个参与率定水深点的原始NSE仍然全部输出到日志。
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # 辅助诊断指标。
#         # D_1370的NSE也会在这里正常输出。
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 三个参与率定水深点的原始NSE仍然全部输出到日志。
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # 辅助验证指标。
#             # D_1370的NSE、KGE、RMSE、PBIAS、R-square等继续正常保存。
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution.
#
#     # Plot hypervolume and newly executed model count.
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generation.
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt(
#         '%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#         new_fronts_count,
#         delimiter=str(','),
#         fmt=str('%d'),
#     )
#
#     # Save and print timespan information.
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt(
#         '%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#         allmodels_exect,
#         delimiter=str(' '),
#         fmt=str('%.4f'),
#     )
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % (
#                   '\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.sum(0)),
#               ))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#
#
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (
#                   init_time,
#                   exec_time,
#                   exec_time_sum,
#                   plot_time,
#               ))
#
#     return pop, logbook
#
#
# if __name__ == '__main__':
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 当前率定使用三个水深点原始NSE构造：
#     # D_1366、D_1367、D_1368
#     #
#     # 1. minNSE
#     # 2. meanNSE
#     #
#     # NSGA-II同时最大化这两个目标。
#     #
#     # D_1370、F_18和D_1375仅用于辅助验证和绘图，
#     # 不参与fitness。
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         f.write('%s' % fstats.__str__())
#
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))





# # -*- coding: utf-8 -*-
# # 三个核心水深点：max(minNSE) + max(meanNSE) + min(dryRMSE)
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import (
#     Calibration,
#     initialize_calibrations,
#     calibration_objectives,
#     evaluate_nowait_or_skip,
#     evaluate_blocking,
# )
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent, cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# # when paralleled by SCOOP. Thus, DEAP related operations are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = [
#     'NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE',
#     'lnNSE', 'NSE1', 'NSE3', 'FI', 'BI'
# ]
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', 'TEST', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
#
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS|.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, and min. |PBIAS|-SED.
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment.
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED.
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# elif step == 'TEST':
#     # Test example.
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# else:
#     # ============================================================
#     # T5 三个实测水深点，全部使用 NSE 计算
#     #
#     # 原始NSE：
#     # D_1366、D_1367、D_1368
#     #
#     # 真正参与fitness：
#     # minNSE：三个水深点NSE中的最小值，最大化
#     # meanNSE：三个水深点NSE的平均值，最大化
#     # dryRMSE：三个水深点所有“实测为0”日期的合并RMSE，最小化
#     #
#     # D_1370：不参与fitness，只用于独立验证和绘图
#     # D_1375：只有一个实测值，只用于辅助检查
#     # F_18：只用于独立验证和绘图
#     # ============================================================
#
#     multiobj.setdefault(
#         'D_1366',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1367',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#     multiobj.setdefault(
#         'D_1368',
#         [
#             ['NSE', 1.0, -100.0],
#         ]
#     )
#
#
# # ------------------------------------------------------------
# # 会话路径设置。
# # Windows 和 Linux 分开写，避免路径错误。
# # 如果你实际工程目录不是 CW_2，请在这里修改 base_path 和 RUN_ID。
# # ------------------------------------------------------------
# if os.name == 'nt':  # Windows
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
#
# # 淹没范围率定才会用到；如果只是水深/面积率定，一般不用管。
# subbasin_flood_path = base_path + '/inundation_cali/subbasin_flood'
# subbasin_ids = [1171, 1176, 1193, 1194, 1214]
#
# # 不同实例指定不同 RUN_ID，避免锁文件互相影响。
# RUN_ID = 'CW_2_depth3_minNSE_meanNSE_dryRMSE'
#
#
# # Check object variables.
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
#
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
#
# # Get parameters from `multiobj`.
# # 三个水深点原始NSE仍然通过multiobj计算和保存。
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
#
# # NSGA-II真正优化三个目标：
# # 1. minNSE   ：三个点中最差NSE，最大化
# # 2. meanNSE  ：三个点平均NSE，最大化
# # 3. dryRMSE  ：实测为0日期的合并RMSE，最小化
# fitness_names = ['minNSE', 'meanNSE', 'dryRMSE']
# multi_weight = (1.0, 1.0, -1.0)
#
# # Hypervolume参考点。
# # dryRMSE单位为m，10 m只是一个远差于正常水深误差的参考值，
# # 不参与NSGA-II优劣排序，只用于Hypervolume计算。
# worse_objects = [-100.0, -100.0, 10.0]
# conditions = [None, None, None]
#
# # ============================================================
# # 干态约束设置
# #
# # 只对三个核心率定水深点进行干态约束。
# # 当前采用“严格0值”定义：
# # abs(Obs) <= 1e-6 m 认为实测为0。
# #
# # 三个点所有满足条件的观测合并后计算一个dryRMSE，
# # 即每一个干态观测等权。
# # ============================================================
# DRY_VARS = [
#     'D_1366',
#     'D_1367',
#     'D_1368',
# ]
# DRY_OBS_TOL = 1.0e-6
#
#
# # ============================================================
# # 辅助诊断指标
# #
# # D_1366、D_1367、D_1368的NSE
# # 用于构造minNSE和meanNSE，因此辅助指标中不再重复计算NSE。
# #
# # NSE1在当前统计代码中实际为KGE。
# # D_1370、F_18和D_1375不参与率定，只用于辅助验证和绘图。
# # ============================================================
# auxiliary_object_names = {
#     'D_1366': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1367': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     'D_1368': [
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # D_1370 不参与率定，作为独立验证。
#     # 由于已经从multiobj移除，因此这里补回NSE。
#     'D_1370': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # F_18 不参与率定，作为独立验证。
#     'F_18': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
#
#     # D_1375 不参与率定，只用于辅助验证和绘图。
#     # 按原来的方式继续计算全部指标。
#     'D_1375': [
#         'NSE',
#         'NSE1',
#         'lnNSE',
#         'RMSE',
#         'RSR',
#         'PBIAS',
#         'R-square',
#     ],
# }
#
# # 输出和绘图变量。顺序固定，便于 userdef.py 中绘图读取。
# # D_1370、F_18和D_1375只用于辅助验证和绘图；不进入 fitness。
# output_vars = [
#     'F_18',
#     'D_1366',
#     'D_1367',
#     'D_1368',
#     'D_1370',
#     'D_1375',
# ]
#
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # NOTE: to maintain compatibility with Python2 and Python3,
# # typecode=str('d') MUST NOT be changed to typecode='d'.
# creator.create(
#     'Individual',
#     array.array,
#     typecode=str('d'),
#     fitness=creator.FitnessMulti,
#     gen=-1,
#     id=-1,
#     obs=TimeseriesData,
#     sim=TimeseriesData,
#     cali=ObsSimData,
#     vali=ObsSimData,
#     io_time=0.,
#     comp_time=0.,
#     simu_time=0.,
#     runtime=0.,
# )
#
#
# # Register NSGA-II related operations.
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking, timeout_s=240)
#
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
#
# def calculate_dry_rmse(cali_data):
#     """
#     计算三个核心水深点在“实测为0”日期上的合并dryRMSE。
#
#     参数
#     ----
#     cali_data : ObsSimData
#         当前个体率定期的观测-模拟配对数据。
#
#     返回
#     ----
#     dry_rmse : float
#         三个点所有干态观测合并后的RMSE，单位m，越小越好。
#     dry_count : int
#         三个点合计参与dryRMSE计算的干态观测数。
#     dry_count_by_var : dict
#         每个水深点参与计算的干态观测数。
#
#     干态定义
#     --------
#     abs(Obs) <= DRY_OBS_TOL
#
#     说明
#     ----
#     这里使用每个干态观测等权的合并RMSE：
#         sqrt(mean((Sim - Obs)^2))
#     不人为补充任何没有实测的数据日期。
#     """
#     if cali_data is None or not hasattr(cali_data, 'sim_obs_data'):
#         raise ValueError('cali.sim_obs_data不存在，无法计算dryRMSE。')
#
#     squared_errors = list()
#     dry_count_by_var = dict()
#
#     for var in DRY_VARS:
#         if var not in cali_data.sim_obs_data:
#             dry_count_by_var[var] = 0
#             raise ValueError(
#                 'cali.sim_obs_data中缺少干态率定变量: %s' % var
#             )
#
#         one_var = cali_data.sim_obs_data[var]
#
#         if not hasattr(one_var, 'get'):
#             dry_count_by_var[var] = 0
#             raise ValueError(
#                 '%s的sim_obs_data结构不是字典/OrderedDict。' % var
#             )
#
#         obs = numpy.asarray(one_var.get('Obs', []), dtype=float)
#         sim = numpy.asarray(one_var.get('Sim', []), dtype=float)
#
#         n = min(len(obs), len(sim))
#         if n <= 0:
#             dry_count_by_var[var] = 0
#             continue
#
#         obs = obs[:n]
#         sim = sim[:n]
#
#         valid_mask = numpy.isfinite(obs) & numpy.isfinite(sim)
#
#         # 严格0值：用极小浮点容差代替直接 Obs == 0。
#         # 同时要求Obs不小于 -DRY_OBS_TOL，避免异常负值进入干态。
#         dry_mask = (
#             valid_mask
#             & (numpy.abs(obs) <= DRY_OBS_TOL)
#             & (obs >= -DRY_OBS_TOL)
#         )
#
#         count = int(numpy.sum(dry_mask))
#         dry_count_by_var[var] = count
#
#         if count > 0:
#             err = sim[dry_mask] - obs[dry_mask]
#             squared_errors.extend(
#                 numpy.square(err).astype(float).tolist()
#             )
#
#     dry_count = len(squared_errors)
#
#     if dry_count <= 0:
#         raise ValueError(
#             'D_1366、D_1367、D_1368中没有找到实测为0的干态观测；'
#             '当前DRY_OBS_TOL=%g m。' % DRY_OBS_TOL
#         )
#
#     dry_rmse = float(
#         numpy.sqrt(
#             numpy.mean(
#                 numpy.asarray(squared_errors, dtype=float)
#             )
#         )
#     )
#
#     if not numpy.isfinite(dry_rmse):
#         raise ValueError(
#             'dryRMSE为NaN或Inf。dry_count=%d' % dry_count
#         )
#
#     return dry_rmse, dry_count, dry_count_by_var
#
# def main(cfg):
#     """Main workflow of NSGA-II based scenario analysis/calibration."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables.
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # Create reference point for hypervolume.
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # Read observation data from MongoDB.
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once.
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#
#     # 读取所有需要输出/绘图的变量。
#     # 三个水深点NSE用于构造minNSE和meanNSE；
#     # 同时从三点的Obs/Sim配对数据中计算dryRMSE。
#     # D_1370仍然读取，但只作为辅助验证。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Raw NSE variables: %s' % str(object_vars))
#     scoop_log('Calibration objectives (fitness only): %s' % str(fitness_names))
#     scoop_log('Output/plot variables (includes auxiliary variables): %s' % str(output_vars))
#
#     # If calibrating inundation extent.
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start='2010-01',
#             end='2010-02',
#             band=1,
#             on_missing='warn',
#             readonly=True,
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]['1171']
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population.
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#
#     param_values = numpy.array(param_values)
#
#     # Write calibrated values to MongoDB.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#     # Get the low and up bound of calibrated parameters.
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0].tolist()
#     up = bounds[:, 1].tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals."""
#         popnum = len(invalid_pops)
#         labels = list()
#
#         # Serial debug version:
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind, base_path, RUN_ID) for ind in invalid_pops]
#
#         try:
#             # Parallel on multi-processors or clusters using SCOOP.
#             from scoop import futures
#             invalid_pops = list(futures.map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#         except ImportError or ImportWarning:
#             # Python built-in map, serial mode.
#             invalid_pops = list(map(
#                 toolbox.evaluate,
#                 [cali_obj] * popnum,
#                 invalid_pops,
#                 [base_path] * popnum,
#                 [RUN_ID] * popnum,
#             ))
#
#         scoop_log('过滤之前pops长度为:%d' % len(invalid_pops))
#
#         # 把被跳过或失败返回 -1 的个体过滤掉。
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log('过滤之后pops长度为:%d' % len(invalid_pops))
#
#         for tmpind in invalid_pops:
#             labels = fitness_names[:]
#             tmpfitnessv = list()
#             try:
#                 # ------------------------------------------------------------
#                 # 先计算三个水深点各自的原始NSE。
#                 #
#                 # 顺序：
#                 # D_1366、D_1367、D_1368。
#                 #
#                 # 这三个NSE仍然正常保存到cali结果中，
#                 # 只是它们不再直接作为3个独立fitness。
#                 #
#                 # D_1370不在这里计算，因此不会参与minNSE和meanNSE。
#                 # ------------------------------------------------------------
#                 raw_nse_values = list()
#
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#
#                     if len(tmpvalues) != 1:
#                         raise ValueError(
#                             '%s返回的NSE数量不等于1: %s'
#                             % (k, str(tmpvalues))
#                         )
#
#                     raw_nse_values.append(float(tmpvalues[0]))
#
#                 if len(raw_nse_values) != 3:
#                     raise ValueError(
#                         '三个水深点NSE数量不等于3: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 raw_nse_values = numpy.array(raw_nse_values, dtype=float)
#
#                 if not numpy.all(numpy.isfinite(raw_nse_values)):
#                     raise ValueError(
#                         '三个水深点NSE存在NaN或Inf: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 if numpy.any(raw_nse_values <= -9000.0):
#                     raise ValueError(
#                         '三个水深点NSE存在无效惩罚值: %s'
#                         % str(raw_nse_values)
#                     )
#
#                 # ------------------------------------------------------------
#                 # 真正的NSGA-II三目标：
#                 #
#                 # 1. minNSE：
#                 #    三个水深点中最差的NSE，越大越好。
#                 #
#                 # 2. meanNSE：
#                 #    三个水深点NSE的平均值，越大越好。
#                 #
#                 # 3. dryRMSE：
#                 #    D_1366、D_1367、D_1368三个点所有“实测为0”
#                 #    日期合并后的RMSE，越小越好。
#                 # ------------------------------------------------------------
#                 min_nse = float(numpy.min(raw_nse_values))
#                 mean_nse = float(numpy.mean(raw_nse_values))
#
#                 dry_rmse, dry_count, dry_count_by_var = calculate_dry_rmse(
#                     tmpind.cali
#                 )
#
#                 tmpfitnessv = [
#                     min_nse,
#                     mean_nse,
#                     dry_rmse,
#                 ]
#
#                 tmpind.fitness.values = tuple(tmpfitnessv)
#
#                 # 同时保存到sim_obs_data顶层，便于后续userdef.py
#                 # 选参、诊断和检查；不会改变原始观测/模拟序列。
#                 tmpind.cali.sim_obs_data['Fitness_minNSE'] = min_nse
#                 tmpind.cali.sim_obs_data['Fitness_meanNSE'] = mean_nse
#                 tmpind.cali.sim_obs_data['Fitness_dryRMSE'] = dry_rmse
#                 tmpind.cali.sim_obs_data['DryObsCount'] = dry_count
#                 tmpind.cali.sim_obs_data['DryObsCountByVar'] = dry_count_by_var
#                 tmpind.cali.sim_obs_data['DryObsTolerance_m'] = DRY_OBS_TOL
#
#             except Exception as e:
#                 scoop_log(
#                     '[模型失败] id=%s, gen=%s, tmpfitnessv=%s, multi_weight=%s, 原因: %s'
#                     % (tmpind.id, tmpind.gen, tmpfitnessv, multi_weight, e)
#                 )
#
#                 # 三个fitness中：
#                 # minNSE、meanNSE为最大化目标，失败时赋极小值；
#                 # dryRMSE为最小化目标，失败时赋极大值。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 辅助变量只计算并保存指标，不追加到 tmpfitnessv。
#             # 因此D_1370、F_18和D_1375不参与NSGA-II优化或fitness。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         '[辅助验证变量读取失败，不影响率定] id=%s, gen=%s, var=%s, 原因: %s'
#                         % (tmpind.id, tmpind.gen, aux_var, aux_e)
#                     )
#
#         # Filter for a valid solution.
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#
#         # Currently, invalid_pops contains evaluated individuals.
#         return invalid_pops, labels
#
#     # Record the count and execute timespan of model runs during optimization.
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}
#     modelruns_time_sum = {0: 0.}
#
#     # Generation 0 before optimization.
#     stime = time.time()
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # Currently, len(pop) may less than pop_select_num.
#     pop = toolbox.select(pop, pop_select_num)
#
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#
#     # Begin the generational process.
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (
#         cfg.opt.ngens,
#         cfg.opt.npop,
#     )
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     # Newly added Pareto fronts count.
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int]
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#
#         # Generate offspring by crossover and mutation.
#         if len(offspring) >= 2:
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness.
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB.
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#
#         # Count model runs and execute models.
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population.
#         # Remove duplications first.
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (
#                         gen,
#                         invalid_ind_size,
#                         curtimespan,
#                         modelruns_time_sum[gen],
#                         hypervolume(pop, ref_pt),
#                     )
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#
#         # Count the newly generated near Pareto fronts.
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot near Pareto optimal solutions.
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(
#             front,
#             plotlables,
#             cfg.opt.out_dir,
#             gen,
#             title,
#             plot_cfg=cali_obj.cfg.plot_cfg,
#         )
#         plot_time += time.time() - stime
#
#         # Save in file.
#         output_str += 'generation\tcalibrationID\t'
#
#         # 三个参与率定水深点的原始NSE仍然全部输出到日志。
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # 辅助诊断指标。
#         # D_1370的NSE也会在这里正常输出。
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # 三个真正的NSGA-II目标以及干态观测数。
#         output_str += (
#             'Fitness-minNSE\t'
#             'Fitness-meanNSE\t'
#             'Fitness-dryRMSE\t'
#             'DryObsCount\t'
#         )
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 三个参与率定水深点的原始NSE仍然全部输出到日志。
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # 辅助验证指标。
#             # D_1370的NSE、KGE、RMSE、PBIAS、R-square等继续正常保存。
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # 输出三个真正fitness及干态观测数量。
#             fit_min_nse = ind.fitness.values[0]
#             fit_mean_nse = ind.fitness.values[1]
#             fit_dry_rmse = ind.fitness.values[2]
#             dry_obs_count = ind.cali.sim_obs_data.get('DryObsCount', -1)
#
#             output_str += '%.6f\t%.6f\t%.6f\t%d\t' % (
#                 fit_min_nse,
#                 fit_mean_nse,
#                 fit_dry_rmse,
#                 int(dry_obs_count),
#             )
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution.
#
#     # Plot hypervolume and newly executed model count.
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generation.
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt(
#         '%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#         new_fronts_count,
#         delimiter=str(','),
#         fmt=str('%d'),
#     )
#
#     # Save and print timespan information.
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt(
#         '%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#         allmodels_exect,
#         delimiter=str(' '),
#         fmt=str('%.4f'),
#     )
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % (
#                   '\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                   '\t'.join('%.3f' % t for t in allmodels_exect.sum(0)),
#               ))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#
#
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (
#                   init_time,
#                   exec_time,
#                   exec_time_sum,
#                   plot_time,
#               ))
#
#     return pop, logbook
#
#
# if __name__ == '__main__':
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 当前率定使用D_1366、D_1367、D_1368构造三个目标：
#     #
#     # 1. minNSE   —— 最大化
#     # 2. meanNSE  —— 最大化
#     # 3. dryRMSE  —— 最小化
#     #
#     # dryRMSE只使用三个点中实测为0的日期。
#     # D_1370、F_18和D_1375仅用于辅助验证和绘图，
#     # 不参与fitness。
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         f.write('%s' % fstats.__str__())
#
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))




# -*- coding: utf-8 -*-
# 三个核心水深点：min(maxDryAwareRMSE) + min(meanDryAwareRMSE)
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
    # T5 三个实测水深点，全部使用 RMSE 计算
    #
    # D_1366、D_1367、D_1368的普通RMSE仍计算并保存，用于诊断。
    #
    # 真正参与fitness：
    # maxDryAwareRMSE：三个水深点干湿平衡RMSE中的最大值，最小化
    # meanDryAwareRMSE：三个水深点干湿平衡RMSE的平均值，最小化
    #
    # D_1370：不参与fitness，只用于独立验证和绘图
    # D_1375：只有一个实测值，只用于辅助检查
    # F_18：只用于独立验证和绘图
    # ============================================================

    multiobj.setdefault(
        'D_1366',
        [
            ['RMSE', -1.0, 10.0],
        ]
    )

    multiobj.setdefault(
        'D_1367',
        [
            ['RMSE', -1.0, 10.0],
        ]
    )

    multiobj.setdefault(
        'D_1368',
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

# 淹没范围率定才会用到；如果只是水深/面积率定，一般不用管。
subbasin_flood_path = base_path + '/inundation_cali/subbasin_flood'
subbasin_ids = [1171, 1176, 1193, 1194, 1214]

# 不同实例指定不同 RUN_ID，避免锁文件互相影响。
RUN_ID = 'CW_2_depth3_DryAwareRMSE'


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
# 三个水深点普通RMSE仍然通过multiobj计算和保存。
object_vars = list(multiobj.keys())
object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})

# NSGA-II真正优化两个“干湿平衡RMSE”目标：
# 1. maxDryAwareRMSE  ：三个点中最大的干湿平衡RMSE，最小化
# 2. meanDryAwareRMSE ：三个点干湿平衡RMSE的平均值，最小化
fitness_names = ['maxDryAwareRMSE', 'meanDryAwareRMSE']
multi_weight = (-1.0, -1.0)

# Hypervolume参考点。
# RMSE单位为m，10 m只是一个远差于正常水深误差的参考值，
# 不参与NSGA-II优劣排序，只用于Hypervolume计算。
worse_objects = [10.0, 10.0]
conditions = [None, None]


# ============================================================
# 辅助诊断指标
#
# D_1366、D_1367、D_1368的普通RMSE通过multiobj保存，
# 因此辅助指标中不再重复计算RMSE。
# NSE改为辅助诊断指标继续输出。
#
# NSE1在当前统计代码中实际为KGE。
# D_1370、F_18和D_1375不参与率定，只用于辅助验证和绘图。
# ============================================================
auxiliary_object_names = {
    'D_1366': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    'D_1367': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    'D_1368': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    # D_1370 不参与率定，作为独立验证。
    # 由于已经从multiobj移除，因此这里补回NSE。
    'D_1370': [
        'NSE',
        'NSE1',
        'lnNSE',
        'RMSE',
        'RSR',
        'PBIAS',
        'R-square',
    ],

    # F_18 不参与率定，作为独立验证。
    'F_18': [
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
    'D_1375': [
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
    'F_18',
    'D_1366',
    'D_1367',
    'D_1368',
    'D_1370',
    'D_1375',
]


# ============================================================
# 点位水深的干湿平衡评价
#
# 观测为0表示实测干燥；为了不把浮点数和数值残余误判为淹没，
# 模拟水深不超过1 cm时不计算假淹没误差。这里的1 cm只用于
# 点位水深率定，不改变F_18淹没面积采用的0.1 m阈值。
#
# DryAwareRMSE = sqrt(
#     DRY_GROUP_WEIGHT * DryExcessRMSE^2
#     + (1 - DRY_GROUP_WEIGHT) * WetRMSE^2
# )
#
# 干、湿两组各占50%，避免高水位样本的平方误差掩盖干燥日期。
# ============================================================
OBS_DRY_EPS = 1.0e-6
SIM_WET_THRESHOLD_M = 0.01
DRY_GROUP_WEIGHT = 0.50


def calculate_dry_aware_rmse(one_var_data):
    """Calculate dry/wet-balanced RMSE and diagnostic classification metrics."""
    obs = numpy.asarray(one_var_data.get('Obs', []), dtype=float)
    sim = numpy.asarray(one_var_data.get('Sim', []), dtype=float)

    n = min(obs.size, sim.size)
    if n <= 0:
        raise ValueError('观测或模拟序列为空。')

    obs = obs[:n]
    sim = sim[:n]

    valid = numpy.isfinite(obs) & numpy.isfinite(sim)
    obs = obs[valid]
    sim = sim[valid]

    if obs.size == 0:
        raise ValueError('没有有效的观测-模拟配对数据。')

    dry_mask = obs <= OBS_DRY_EPS
    wet_mask = ~dry_mask

    dry_excess_rmse = numpy.nan
    wet_rmse = numpy.nan
    false_wet_rate = numpy.nan
    false_dry_rate = numpy.nan

    if numpy.any(dry_mask):
        # 只惩罚超过1 cm容差的模拟水深，且连续反映假淹没深度。
        dry_excess = numpy.maximum(
            sim[dry_mask] - SIM_WET_THRESHOLD_M,
            0.0,
        )
        dry_excess_rmse = float(
            numpy.sqrt(numpy.mean(dry_excess ** 2))
        )
        false_wet_rate = float(
            numpy.mean(sim[dry_mask] > SIM_WET_THRESHOLD_M)
        )

    if numpy.any(wet_mask):
        wet_error = sim[wet_mask] - obs[wet_mask]
        wet_rmse = float(
            numpy.sqrt(numpy.mean(wet_error ** 2))
        )
        false_dry_rate = float(
            numpy.mean(sim[wet_mask] <= SIM_WET_THRESHOLD_M)
        )

    if numpy.any(dry_mask) and numpy.any(wet_mask):
        dry_aware_rmse = float(
            numpy.sqrt(
                DRY_GROUP_WEIGHT * dry_excess_rmse ** 2
                + (1.0 - DRY_GROUP_WEIGHT) * wet_rmse ** 2
            )
        )
    elif numpy.any(dry_mask):
        dry_aware_rmse = dry_excess_rmse
    else:
        dry_aware_rmse = wet_rmse

    return {
        'DryAwareRMSE': dry_aware_rmse,
        'DryExcessRMSE': dry_excess_rmse,
        'WetRMSE': wet_rmse,
        'FalseWetRate': false_wet_rate,
        'FalseDryRate': false_dry_rate,
        'DryCount': int(numpy.sum(dry_mask)),
        'WetCount': int(numpy.sum(wet_mask)),
    }


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
    # 三个水深点的观测和模拟用于构造DryAwareRMSE双目标。
    # D_1370仍然读取，但只作为辅助验证。
    obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)

    missing_output_vars = [v for v in output_vars if v not in obs_vars]
    if missing_output_vars:
        raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))

    scoop_log('Dry-aware depth variables: %s' % str(object_vars))
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
        except ImportError or ImportWarning:
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
                # 计算三个核心水深点各自的干湿平衡RMSE。
                # 普通RMSE仍由CalcTimeseriesStatistics计算和保存，只是不再
                # 直接作为fitness。D_1370、D_1375和F_18仍只用于独立验证。
                # ------------------------------------------------------------
                dry_aware_rmse_values = list()

                for var_name in object_vars:
                    if var_name not in tmpind.cali.sim_obs_data:
                        raise ValueError(
                            'sim_obs_data中缺少变量: %s' % var_name
                        )

                    dry_metrics = calculate_dry_aware_rmse(
                        tmpind.cali.sim_obs_data[var_name]
                    )

                    dry_aware_rmse_values.append(
                        float(dry_metrics['DryAwareRMSE'])
                    )

                    # 保存逐点诊断指标，供JSON、绘图和最终选参读取。
                    for metric_name, metric_value in dry_metrics.items():
                        tmpind.cali.sim_obs_data[var_name][metric_name] = metric_value

                if len(dry_aware_rmse_values) != 3:
                    raise ValueError(
                        '三个水深点DryAwareRMSE数量不等于3: %s'
                        % str(dry_aware_rmse_values)
                    )

                dry_aware_rmse_values = numpy.asarray(
                    dry_aware_rmse_values,
                    dtype=float,
                )

                if not numpy.all(numpy.isfinite(dry_aware_rmse_values)):
                    raise ValueError(
                        'DryAwareRMSE存在NaN或Inf: %s'
                        % str(dry_aware_rmse_values)
                    )

                if numpy.any(dry_aware_rmse_values < 0.0):
                    raise ValueError(
                        'DryAwareRMSE存在负值: %s'
                        % str(dry_aware_rmse_values)
                    )

                max_dry_aware_rmse = float(
                    numpy.max(dry_aware_rmse_values)
                )
                mean_dry_aware_rmse = float(
                    numpy.mean(dry_aware_rmse_values)
                )

                tmpfitnessv = [
                    max_dry_aware_rmse,
                    mean_dry_aware_rmse,
                ]
                tmpind.fitness.values = tuple(tmpfitnessv)

                # 保存两个真实fitness，确保userdef.py按相同规则选择参数。
                tmpind.cali.sim_obs_data[
                    'Fitness_maxDryAwareRMSE'
                ] = max_dry_aware_rmse
                tmpind.cali.sim_obs_data[
                    'Fitness_meanDryAwareRMSE'
                ] = mean_dry_aware_rmse

            except Exception as e:
                scoop_log(
                    '[模型失败] id=%s, gen=%s, tmpfitnessv=%s, multi_weight=%s, 原因: %s'
                    % (tmpind.id, tmpind.gen, tmpfitnessv, multi_weight, e)
                )

                # 两个DryAwareRMSE fitness均为最小化目标，失败时赋极大值。
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

        # 三个参与率定水深点的原始RMSE仍然全部输出到日志。
        for kk, vv in list(object_names.items()):
            output_str += pop[0].cali.output_header(kk, vv, 'Cali')

        # 辅助诊断指标。
        # D_1370的NSE也会在这里正常输出。
        for kk, vv in list(auxiliary_object_names.items()):
            output_str += pop[0].cali.output_header(kk, vv, 'Cali')

        # 两个真正的NSGA-II目标。
        output_str += (
            'Fitness-maxDryAwareRMSE\t'
            'Fitness-meanDryAwareRMSE\t'
        )

        if cali_obj.cfg.calc_validation:
            for kkk, vvv in list(object_names.items()):
                output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
            for kkk, vvv in list(auxiliary_object_names.items()):
                output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')

        output_str += 'gene_values\n'
        for ind in pop:
            output_str += '%d\t%d\t' % (ind.gen, ind.id)

            # 三个参与率定水深点的原始RMSE仍然全部输出到日志。
            for kk, vv in list(object_names.items()):
                output_str += ind.cali.output_efficiency(kk, vv)

            # 辅助验证指标。
            # D_1370的NSE、KGE、RMSE、PBIAS、R-square等继续正常保存。
            for kk, vv in list(auxiliary_object_names.items()):
                output_str += ind.cali.output_efficiency(kk, vv)

            # 输出两个真正fitness。
            fit_max_rmse = ind.fitness.values[0]
            fit_mean_rmse = ind.fitness.values[1]

            output_str += '%.6f\t%.6f\t' % (
                fit_max_rmse,
                fit_mean_rmse,
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
    # 1. maxDryAwareRMSE  —— 最小化
    # 2. meanDryAwareRMSE —— 最小化
    #
    # D_1370、F_18和D_1375仅用于辅助验证和绘图，
    # 不参与fitness。
    with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
        f.write('%s' % fstats.__str__())

    endT = time.time()
    scoop_log('### END OF CALIBRATION OPTIMIZING ###')
    scoop_log('Running time: %.2fs' % (endT - startT))



# # -*- coding: utf-8 -*-
# # 以五个实测水深 NSE 率定参数；F_19 作为辅助验证并随结果绘图。
# # 四个点是 NSE 越大越好，D_1310 是 RMSE
#
# from __future__ import absolute_import, division, unicode_literals
#
# import array
# import os
# import random
# import time
# import sys
# from io import open
#
# if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
#
# from typing import Dict
# import numpy
# from deap import base
# from deap import creator
# from deap import tools
# from deap.benchmarks.tools import hypervolume
# from copy import deepcopy
# from pygeoc.utils import UtilClass
#
# from utility.scoop_func import scoop_log
# from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
# from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
# from calibration.config import CaliConfig, get_optimization_config
# from run_seims import MainSEIMS
#
# from calibration.calibrate import Calibration, initialize_calibrations, calibration_objectives,evaluate_nowait_or_skip,evaluate_blocking
# from calibration.calibrate import TimeseriesData, ObsSimData
# from calibration.userdef import write_param_values_to_mongodb, output_population_details
# from calibration.calibrate import cali_inundation_extent,cali_inundation_area
# import inundation_cali_tools as ic_tool
#
#
# # Definitions, assignments, operations, etc. that will be executed by each worker
# #    when paralleled by SCOOP.
# # Thus, DEAP related operations (initialize, register, etc.) are better defined here.
#
# # All accepted objective function names from `postprocess::utility::calculate_statistics`
# # accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
# accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3','FI','BI']
#
# # step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', or 'CUSTOMIZE'.
# step = 'CUSTOMIZE'
# filter_ind = False  # Filter for valid population for the next generation
# # Definitions of Multiobjectives:
# multiobj = dict()
# if step == 'Q':
#     # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS| (percent)
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'SED':
#     # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, min., and |PBIAS|-SED
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
# elif step == 'QSED':
#     # Step 3: Calibration streamflow and sediment
#     multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
#     multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
# elif step == 'NUTRIENT':
#     # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED
#     multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
#     multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
#     multiobj.setdefault('Q', [['NSE', 1., -100]])
#     multiobj.setdefault('SED', [['NSE', 1., -100]])
# #ljj++
# elif step == 'TEST':
#     #negative is minimal
#     #the number can be any float
#     # multiobj.setdefault('Q_1444', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('Q_1289', [['NSE', 1., -100, '>0'], ['PBIAS', -1., 500.]])
#     # multiobj.setdefault('TOTDOCcon_1289', [['NSE1', 1., -100, '>0'],['R-square', 1., -100, '>0']])
#     # multiobj.setdefault('TOTDOCcon_1444', [['R-square', 1., -100, '>0']])
#     multiobj.setdefault('Q_1', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE100_3', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE20_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
#     multiobj.setdefault('SOTE200_12', [['NSE', 1., -100, '>0'], ['R-square', 1., -100, '>0']])
# #######
# else:
#     # ------------------------------------------------------------
#     # 率定目标：使用五个实测水深 NSE
#     #
#     # D_1304、D_1305、D_1306、D_1309、D_1310：
#     #     参与 NSGA-II 率定和最终统一参数选择。
#     #
#     # F_19：
#     #     淹没面积仅作为辅助验证和绘图变量，不进入 fitness。
#     # ------------------------------------------------------------
#     multiobj.setdefault('D_1304', [['NSE', 1.0, -100.0]])
#     multiobj.setdefault('D_1305', [['NSE', 1.0, -100.0]])
#     multiobj.setdefault('D_1306', [['NSE', 1.0, -100.0]])
#     multiobj.setdefault('D_1309', [['NSE', 1.0, -100.0]])
#     multiobj.setdefault('D_1310', [['RMSE', -1.0, 1.0]])
#     # Customize your own multiobjective here, such as:
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0.'],
#     #                           ['RSR', -1., 2., '<2.'],
#     #                           ['PBIAS', -1., 50., '<50.']])
#     # multiobj.setdefault('Q_322', [['NSE', 1., -100, '>0']])
#     # multiobj.setdefault('Q_1171', [['NSE', 1., -100, '>0']])
#
#
# # 会话ID，避免跨实例互相影响（强烈推荐）
# if os.name == 'nt':  # Windows
#     # base_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1'
#     base_path = r'D:\SEIMS_C\data\CW_2'
# else:  # Linux/Unix
#     base_path = '/data/user/lijing/software/SEIMS_C/data/CW_2'
# # # 淹没范围才会用，淹没面积率定不用管
# subbasin_flood_path = base_path + f'/inundation_cali/subbasin_flood'
# subbasin_ids=[1171,1176,1193,1194,1214]
#
# # 不同的实例指定不同的id，避免锁文件相互影响
# RUN_ID = 'CW_2_20260425'
#
# # Check object variables
# if not multiobj:
#     print('Multiobjective MUST not be Empty!')
#     exit(1)
# for k, v in list(multiobj.items()):
#     for item in v:
#         if len(item) < 3:
#             print('Each item of objective MUST have three elements, '
#                   'i.e., object name, weight, worse value for Hypervolum calculation.')
#             exit(1)
#         if item[0] not in accepted_objnames:
#             print('Object name % is unsupported! '
#                   'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
#             exit(1)
# # Get parameters from `multiobj`
# object_vars = list(multiobj.keys())
# object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
# multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
# worse_objects = list(l[2] for v in multiobj.values() for l in v)
# conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)
#
# # ------------------------------------------------------------
# # 辅助验证/绘图变量：读取、计算指标并保存，但绝不进入 fitness。
# #
# # F_19：淹没面积辅助验证，不进入 NSGA-II fitness。
# #
# # output_vars 的顺序固定为 F_19 + 五个率定水深，
# # 与 userdef.py 中绘图时读取列的顺序保持一致。
# # ------------------------------------------------------------
# auxiliary_object_names = {
#     'F_19': ['NSE', 'RSR', 'PBIAS', 'R-square']
# }
#
# output_vars = [
#     'F_19',
#     'D_1304',
#     'D_1305',
#     'D_1306',
#     'D_1309',
#     'D_1310'
# ]
#
# creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# # The FitnessMulti class equals to (as an example):
# # class FitnessMulti(base.Fitness):
# #     weights = (2., -1., -1.)
# # NOTE that to maintain the compatibility with Python2 and Python3,
# #      the com typecode=str('d') MUST NOT changed to typecode='d', since
# #      the latter will raise TypeError that 'must be char, not unicode'!
# creator.create('Individual', array.array, typecode=str('d'), fitness=creator.FitnessMulti,
#                gen=-1, id=-1,
#                obs=TimeseriesData, sim=TimeseriesData,
#                cali=ObsSimData, vali=ObsSimData,
#                io_time=0., comp_time=0., simu_time=0., runtime=0.)
# # The Individual class equals to:
# # class Individual(array.array):
# #     gen = -1  # Generation No.
# #     id = -1   # Calibration index of current generation
# #     def __init__(self):
# #         self.fitness = FitnessMulti()
#
# # Register NSGA-II related operations
# toolbox = base.Toolbox()
# toolbox.register('gene_values', initialize_calibrations)
# toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
# toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
# # toolbox.register('evaluate', calibration_objectives)
# # toolbox.register('evaluate', evaluate_nowait_or_skip)
# toolbox.register('evaluate', evaluate_blocking,timeout_s=240)
#
#
# # mate and mutate
# toolbox.register('mate', tools.cxSimulatedBinaryBounded)
# toolbox.register('mutate', tools.mutPolynomialBounded)
# toolbox.register('select', tools.selNSGA2)
#
#
#
# def main(cfg):
#     """Main workflow of NSGA-II based Scenario analysis."""
#     random.seed()
#     scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))
#
#     # Initial timespan variables
#     stime = time.time()
#     plot_time = 0.
#     allmodels_exect = list()  # execute time of all model runs
#
#     # create reference point for hypervolume
#     ref_pt = numpy.array(worse_objects) * multi_weight * -1
#
#     stats = tools.Statistics(lambda sind: sind.fitness.values)
#     stats.register('min', numpy.min, axis=0)
#     stats.register('max', numpy.max, axis=0)
#     stats.register('avg', numpy.mean, axis=0)
#     stats.register('std', numpy.std, axis=0)
#     logbook = tools.Logbook()
#     logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'
#
#     # read observation data from MongoDB
#     cali_obj = Calibration(cfg)
#
#     # Read observation data just once
#     model_cfg_dict = cali_obj.model.ConfigDict
#     model_obj = MainSEIMS(args_dict=model_cfg_dict)
#
#     model_obj.SetMongoClient()
#     # F_19 仅供验证/绘图；multiobj 中的五个 D_* 会构成 fitness。
#     obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(output_vars)
#
#     missing_output_vars = [v for v in output_vars if v not in obs_vars]
#     if missing_output_vars:
#         raise ValueError('观测数据中缺少需要输出/绘图的变量: %s' % str(missing_output_vars))
#
#     scoop_log('Calibration objectives (fitness only): %s' % str(object_vars))
#     scoop_log('Output/plot variables (includes auxiliary F_19): %s' % str(output_vars))
#     #-------------------------xiaodw, if calibrate inundation extent --------------------
#     if cali_inundation_extent:
#         obs_inun_data_dict = ic_tool.load_monthly_tifs(
#             root_dir=subbasin_flood_path,
#             subbasin_ids=subbasin_ids,
#             start="2010-01",
#             end="2010-02",
#             band=1,
#             on_missing="warn",
#             readonly=True
#         )
#         some_month = list(obs_inun_data_dict.keys())[0]
#         arr_1171 = obs_inun_data_dict[some_month]["1171"]
#         print(some_month, arr_1171.shape, arr_1171.dtype)
#     #-------------------------xiaodw, if calibrate inundation area --------------------
#
#     model_obj.UnsetMongoClient()
#
#     # Initialize population
#     # print(f"cfg.opt is {cfg.opt}")
#     # print(f"npop is: {cfg.opt.npop}")
#     param_values = cali_obj.initialize(cfg.opt.npop)
#     pop = list()
#     for i in range(cfg.opt.npop):
#         ind = creator.Individual(param_values[i])
#         ind.gen = 0
#         ind.id = i
#         ind.obs.vars = obs_vars[:]
#         ind.obs.data = deepcopy(obs_data_dict)
#         if cali_inundation_extent:
#             ind.obs.inun_data = deepcopy(obs_inun_data_dict)
#         pop.append(ind)
#         # print(f"ind: {ind} ")
#     param_values = numpy.array(param_values)
#     # print(f"npop: {cfg.opt.npop}")
#
#     # print(param_values)
#     # Write calibrated values to MongoDB
#     # TODO, extract this function, which is same with `Sensitivity::write_param_values_to_mongodb`.
#     write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#     # get the low and up bound of calibrated parameters
#     bounds = numpy.array(cali_obj.ParamDefs['bounds'])
#     low = bounds[:, 0]
#     up = bounds[:, 1]
#     low = low.tolist()
#     up = up.tolist()
#     pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
#     init_time = time.time() - stime
#
#     def check_validation(fitvalues):
#         """Check the validation of the fitness values of an individual."""
#         flag = True
#         for condidx, condstr in enumerate(conditions):
#             if condstr is None:
#                 continue
#             if not eval('%f%s' % (fitvalues[condidx], condstr)):
#                 flag = False
#         return flag
#
#     def evaluate_parallel(invalid_pops):
#         """Evaluate model by SCOOP or map, and set fitness of individuals
#          according to calibration step."""
#         popnum = len(invalid_pops)
#         labels = list()
#         # ---------------------test for no scoop----------------------
#         # invalid_pops = [toolbox.evaluate(cali_obj, ind,base_path,RUN_ID) for ind in invalid_pops]
#         # ---------------------end for no scoop----------------------
#
#         try:  # parallel on multi-processors or clusters using SCOOP
#             from scoop import futures
#             invalid_pops = list(futures.map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#         except ImportError or ImportWarning:  # Python build-in map (serial)
#             invalid_pops = list(map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops,[base_path]* popnum,[RUN_ID]* popnum))
#
#         scoop_log(f"过滤之前pops长度为:{len(invalid_pops)}")
#         # 关键：把 None（被跳过的个体）过滤掉
#         invalid_pops = [ind for ind in invalid_pops if ind != -1]
#
#         scoop_log(f"过滤之后pops长度为:{len(invalid_pops)}")
#
#         for tmpind in invalid_pops:
#             labels = list()
#             tmpfitnessv = list()
#             try:
#                 for k, v in list(multiobj.items()):
#                     tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
#                     tmpfitnessv += tmpvalues[:]
#                     labels += tmplabel[:]
#
#                 # 如果结果合法才赋值
#                 if len(tmpfitnessv) == len(multi_weight):
#                     tmpind.fitness.values = tuple(tmpfitnessv)
#                 else:
#                     raise ValueError("模型返回指标维度不一致！")
#
#             except Exception as e:
#                 scoop_log(
#                     f"[模型失败] id={tmpind.id}, gen={tmpind.gen}, "
#                     f"tmpfitnessv={tmpfitnessv}, multi_weight={multi_weight}, 原因: {e}"
#                 )
#
#                 # 当前 fitness 均为 NSE 最大化目标，失败时必须赋极小值。
#                 # 通用写法同时兼容未来加入最小化指标（如 RMSE、RSR）。
#                 penalty_values = tuple(
#                     -9999.0 if w > 0 else 9999.0
#                     for w in multi_weight
#                 )
#                 tmpind.fitness.values = penalty_values
#
#             # 仅为保存和绘图计算 F_19 指标；不追加到 tmpfitnessv，
#             # 所以 F_19 不参与 NSGA-II 优化或最终统一参数选择。
#             for aux_var, aux_metrics in auxiliary_object_names.items():
#                 try:
#                     tmpind.cali.efficiency_values(aux_var, aux_metrics)
#                 except Exception as aux_e:
#                     scoop_log(
#                         f"[辅助验证变量读取失败，不影响率定] id={tmpind.id}, gen={tmpind.gen}, "
#                         f"var={aux_var}, 原因: {aux_e}"
#                     )
#
#         # Filter for a valid solution
#         if filter_ind:
#             invalid_pops = [tmpind for tmpind in invalid_pops
#                             if check_validation(tmpind.fitness.values)]
#             if len(invalid_pops) < 2:
#                 print('The initial population should be greater or equal than 2. '
#                       'Please check the parameters ranges or change the sampling strategy!')
#                 exit(2)
#         return invalid_pops, labels  # Currently, `invalid_pops` contains evaluated individuals
#
#     # Record the count and execute timespan of model runs during the optimization
#     modelruns_count = {0: len(pop)}
#     modelruns_time = {0: 0.}  # Total time counted according to evaluate_parallel()
#     modelruns_time_sum = {0: 0.}  # Summarize time of every model runs according to pop
#
#     # Generation 0 before optimization
#     stime = time.time()
#     # print(f"***************pop: {pop}")
#     pop, plotlables = evaluate_parallel(pop)
#     modelruns_time[0] = time.time() - stime
#     for ind in pop:
#         allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#         modelruns_time_sum[0] += ind.runtime
#
#     # currently, len(pop) may less than pop_select_num
#     pop = toolbox.select(pop, pop_select_num)
#     # Output simulated data to json or pickle files for future use.
#     output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     record = stats.compile(pop)
#     logbook.record(gen=0, evals=len(pop), **record)
#     # scoop_log(logbook.stream)
#
#     # Begin the generational process
#     output_str = '### Generation number: %d, Population size: %d ###\n' % (cfg.opt.ngens,
#                                                                            cfg.opt.npop)
#     # scoop_log(output_str)
#     UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')
#
#     modelsel_count = {0: len(pop)}  # type: Dict[int, int] # newly added Pareto fronts
#
#     for gen in range(1, cfg.opt.ngens + 1):
#         output_str = '###### Generation: %d ######\n' % gen
#         # scoop_log(output_str)
#
#         offspring = [toolbox.clone(ind) for ind in pop]
#         # method1: use crowding distance (normalized as 0~1) as eta
#         # tools.emo.assignCrowdingDist(offspring)
#         # method2: use the index of individual at the sorted offspring list as eta
#         if len(offspring) >= 2:  # when offspring size greater than 2, mate can be done
#             for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
#                 if random.random() > cfg.opt.rcross:
#                     continue
#                 eta = i
#                 toolbox.mate(ind1, ind2, eta, low, up)
#                 toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
#                 toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
#                 del ind1.fitness.values, ind2.fitness.values
#         else:
#             toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
#             del offspring[0].fitness.values
#
#         # Evaluate the individuals with an invalid fitness
#         invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
#         valid_inds = [ind for ind in offspring if ind.fitness.valid]
#         if len(invalid_inds) == 0:  # No need to continue
#             scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
#             break
#
#         # Write new calibrated parameters to MongoDB
#         param_values = list()
#         for idx, ind in enumerate(invalid_inds):
#             ind.gen = gen
#             ind.id = idx
#             param_values.append(ind[:])
#         param_values = numpy.array(param_values)
#         write_param_values_to_mongodb(cfg.model.db_name, cali_obj.ParamDefs, param_values)
#         # Count the model runs, and execute models
#         invalid_ind_size = len(invalid_inds)
#         modelruns_count.setdefault(gen, invalid_ind_size)
#         stime = time.time()
#         invalid_inds, plotlables = evaluate_parallel(invalid_inds)
#         curtimespan = time.time() - stime
#         modelruns_time.setdefault(gen, curtimespan)
#         modelruns_time_sum.setdefault(gen, 0.)
#         for ind in invalid_inds:
#             allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
#             modelruns_time_sum[gen] += ind.runtime
#
#         # Select the next generation population
#         # Previous version may result in duplications of the same scenario in one Pareto front,
#         #   thus, I decided to check and remove the duplications first.
#         # pop = toolbox.select(pop + valid_inds + invalid_inds, pop_select_num)
#         tmppop = pop + valid_inds + invalid_inds
#         pop = list()
#         unique_sces = dict()
#         for tmpind in tmppop:
#             if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
#                 continue
#             if tmpind.gen not in unique_sces:
#                 unique_sces.setdefault(tmpind.gen, [tmpind.id])
#             elif tmpind.id not in unique_sces[tmpind.gen]:
#                 unique_sces[tmpind.gen].append(tmpind.id)
#             pop.append(tmpind)
#         pop = toolbox.select(pop, pop_select_num)
#
#         output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
#         hyper_str = 'Gen: %d, New model runs: %d, ' \
#                     'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
#                     'Hypervolume: %.4f\n' % (gen, invalid_ind_size,
#                                              curtimespan, modelruns_time_sum[gen],
#                                              hypervolume(pop, ref_pt))
#         # scoop_log(hyper_str)
#         UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')
#
#         record = stats.compile(pop)
#         logbook.record(gen=gen, evals=len(invalid_inds), **record)
#         # scoop_log(logbook.stream)
#
#         # Count the newly generated near Pareto fronts
#         new_count = 0
#         for ind in pop:
#             if ind.gen == gen:
#                 new_count += 1
#         modelsel_count.setdefault(gen, new_count)
#
#         # Plot 2D near optimal pareto front graphs,
#         #   i.e., (NSE, RSR), (NSE, PBIAS), and (RSR,PBIAS)
#         # And 3D near optimal pareto front graphs, i.e., (NSE, RSR, PBIAS)
#         stime = time.time()
#         front = numpy.array([ind.fitness.values for ind in pop])
#         title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
#                  'Near Pareto optimal solutions')
#
#         plot_pareto_front_single(front, plotlables, cfg.opt.out_dir,
#                                  gen, title, plot_cfg=cali_obj.cfg.plot_cfg)
#         plot_time += time.time() - stime
#
#         # save in file
#         # Header information
#         output_str += 'generation\tcalibrationID\t'
#
#         # 四个参与率定的水深指标
#         for kk, vv in list(object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         # F_19 辅助验证指标；只写入日志，不参与 fitness
#         for kk, vv in list(auxiliary_object_names.items()):
#             output_str += pop[0].cali.output_header(kk, vv, 'Cali')
#
#         if cali_obj.cfg.calc_validation:
#             for kkk, vvv in list(object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#             for kkk, vvv in list(auxiliary_object_names.items()):
#                 output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')
#
#         output_str += 'gene_values\n'
#         for ind in pop:
#             output_str += '%d\t%d\t' % (ind.gen, ind.id)
#
#             # 四个参与率定的水深指标
#             for kk, vv in list(object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             # F_19 辅助验证指标
#             for kk, vv in list(auxiliary_object_names.items()):
#                 output_str += ind.cali.output_efficiency(kk, vv)
#
#             if cali_obj.cfg.calc_validation:
#                 for kkk, vvv in list(object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#                 for kkk, vvv in list(auxiliary_object_names.items()):
#                     output_str += ind.vali.output_efficiency(kkk, vvv)
#
#             output_str += str(ind)
#             output_str += '\n'
#         UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')
#
#         # TODO: Figure out if we should terminate the evolution
#
#     # Plot hypervolume and newly executed model count
#     plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)
#
#     # Save newly added Pareto fronts of each generations
#     new_fronts_count = numpy.array(list(modelsel_count.items()))
#     numpy.savetxt('%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
#                   new_fronts_count, delimiter=str(','), fmt=str('%d'))
#
#     # Save and print timespan information
#     allmodels_exect = numpy.array(allmodels_exect)
#     numpy.savetxt('%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
#                   allmodels_exect, delimiter=str(' '), fmt=str('%.4f'))
#     scoop_log('Running time of all SEIMS models:\n'
#               '\tIO\tCOMP\tSIMU\tRUNTIME\n'
#               'MAX\t%s\n'
#               'MIN\t%s\n'
#               'AVG\t%s\n'
#               'SUM\t%s\n' % ('\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
#                              '\t'.join('%.3f' % t for t in allmodels_exect.sum(0))))
#
#     exec_time = 0.
#     for genid, tmptime in list(modelruns_time.items()):
#         exec_time += tmptime
#     exec_time_sum = 0.
#     for genid, tmptime in list(modelruns_time_sum.items()):
#         exec_time_sum += tmptime
#     allcount = 0
#     for genid, tmpcount in list(modelruns_count.items()):
#         allcount += tmpcount
#
#     scoop_log('Initialization timespan: %.4f\n'
#               'Model execution timespan: %.4f\n'
#               'Sum of model runs timespan: %.4f\n'
#               'Plot Pareto graphs timespan: %.4f' % (init_time, exec_time,
#                                                      exec_time_sum, plot_time))
#
#     return pop, logbook
#
#
# if __name__ == "__main__":
#     cf, method = get_optimization_config()
#     cali_cfg = CaliConfig(cf, method=method)
#
#     scoop_log('### START TO CALIBRATION OPTIMIZING ###')
#     startT = time.time()
#
#     fpop, fstats = main(cali_cfg)
#
#     # 最终统一参数由 calibration.userdef.py 中的 select_shared_best_index() 选择：
#     # 依据五个实测水深；F_19 仅用于辅助验证和绘图。
#     # scoop_log(fstats)
#     with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
#         # In case of 'TypeError: write() argument 1 must be unicode, not str' in Python2.7
#         #   when using unicode_literals, please use '%s' to concatenate string!
#         f.write('%s' % fstats.__str__())
#     endT = time.time()
#     scoop_log('### END OF CALIBRATION OPTIMIZING ###')
#     scoop_log('Running time: %.2fs' % (endT - startT))
