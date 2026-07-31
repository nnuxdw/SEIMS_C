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
from __future__ import absolute_import, division, unicode_literals

import array
import math
import os
import random
import time
import sys
from configparser import ConfigParser
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

from calibration.calibrate import Calibration, initialize_calibrations, evaluate_blocking
from calibration.calibrate import TimeseriesData, ObsSimData
from calibration.userdef import write_param_values_to_mongodb_by_ids, output_population_details
from calibration.calibrate import cali_inundation_extent,cali_inundation_area, base_path,subbasin_flood_path,subbasin_ids
import inundation_cali_tools as ic_tool


# Definitions, assignments, operations, etc. that will be executed by each worker
#    when paralleled by SCOOP.
# Thus, DEAP related operations (initialize, register, etc.) are better defined here.

# All accepted objective function names from `postprocess::utility::calculate_statistics`
# accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']
accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3','FI','BI', 'KGE', 'lnKGE']

filter_ind = False  # Filter for valid population for the next generation
DEFAULT_OBJECTIVES = 'Q_1:NSE:1:-100:>0'


def _arg_value(*names):
    for idx, arg in enumerate(sys.argv):
        if arg in names and idx + 1 < len(sys.argv):
            return sys.argv[idx + 1]
        for name in names:
            if arg.startswith(name + '='):
                return arg.split('=', 1)[1]
    return None


def _project_dir_from_argv():
    project_dir = _arg_value('-project', '-project_dir', '--project')
    if project_dir:
        return os.path.abspath(project_dir)
    project_dir = os.environ.get('SEIMS_PROJECT_DIR')
    return os.path.abspath(project_dir) if project_dir else None


def _discover_calibration_ini(project_dir):
    candidates = [
        os.path.join(project_dir, 'model_configs', 'calibration.ini'),
        os.path.join(project_dir, 'calibration.ini'),
        os.path.join(project_dir, 'configs', 'calibration.ini'),
    ]
    project_name = os.path.basename(os.path.normpath(project_dir))
    candidates.append(os.path.join(project_dir, project_name + '_longterm_model',
                                   'calibration.ini'))
    for path in candidates:
        if os.path.isfile(path):
            return path
    for root, dirs, files in os.walk(project_dir):
        rel = os.path.relpath(root, project_dir)
        if rel != '.' and rel.count(os.sep) > 2:
            dirs[:] = []
            continue
        if 'calibration.ini' in files:
            return os.path.join(root, 'calibration.ini')
    return None


def _ini_file_from_argv():
    ini_file = _arg_value('-ini')
    if ini_file:
        return ini_file
    project_dir = _project_dir_from_argv()
    if project_dir:
        return _discover_calibration_ini(project_dir)
    return os.environ.get('SEIMS_CALIBRATION_INI')


def parse_objectives_from_string(objectives_str):
    parsed = dict()
    for raw_item in objectives_str.split(','):
        raw_item = raw_item.strip()
        if not raw_item:
            continue
        parts = [part.strip() for part in raw_item.split(':')]
        if len(parts) < 4:
            raise ValueError("Invalid objective item: %s" % raw_item)
        var_name, obj_name = parts[0], parts[1]
        weight = float(parts[2])
        worse_value = float(parts[3])
        item = [obj_name, weight, worse_value]
        if len(parts) > 4 and parts[4]:
            item.append(parts[4])
        parsed.setdefault(var_name, []).append(item)
    return parsed


def parse_objectives_from_config(cf):
    """Parse optional [Calibration_Objectives] objectives from calibration.ini."""
    sec_name = 'Calibration_Objectives'
    if sec_name not in cf.sections() or not cf.has_option(sec_name, 'objectives'):
        return parse_objectives_from_string(DEFAULT_OBJECTIVES)
    objectives_str = cf.get(sec_name, 'objectives')
    return parse_objectives_from_string(objectives_str)


def load_objectives_from_config():
    objectives_str = _arg_value('-objective', '-objectives', '--objective', '--objectives') \
        or os.environ.get('SEIMS_OBJECTIVES')
    if objectives_str:
        return parse_objectives_from_string(objectives_str)
    ini_file = _ini_file_from_argv()
    if not ini_file or not os.path.isfile(ini_file):
        return parse_objectives_from_string(DEFAULT_OBJECTIVES)
    cf = ConfigParser()
    cf.read(ini_file)
    return parse_objectives_from_config(cf)


def validate_objectives(multiobj):
    if not multiobj:
        raise ValueError('Multiobjective MUST not be Empty!')
    for _, v in list(multiobj.items()):
        for item in v:
            if len(item) < 3:
                raise ValueError('Each item of objective MUST have three elements, '
                                 'i.e., object name, weight, worse value.')
            if item[0] not in accepted_objnames:
                raise ValueError('Object name %s is unsupported! Please input one of %s!' %
                                 (item[0], ','.join(accepted_objnames)))


multiobj = load_objectives_from_config()
validate_objectives(multiobj)
# Get parameters from `multiobj`
object_vars = list(multiobj.keys())
object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
worse_objects = list(l[2] for v in multiobj.values() for l in v)
conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)


def objective_labels():
    labels = list()
    for var_name, names in list(object_names.items()):
        for name in names:
            labels.append('%s-%s' % (var_name, name))
    return labels


def penalty_fitness(weights):
    return tuple(-9999.0 if w > 0 else 9999.0 for w in weights)


def assign_penalty(ind, reason, weights=None, object_names_map=None):
    weights = weights or multi_weight
    object_names_map = object_names_map or object_names
    vals = penalty_fitness(weights)
    ind.fitness.values = vals
    ind.failed = True
    ind.failure_reason = str(reason)
    ind.cali.valid = False
    ind.cali.objnames = []
    ind.cali.objvalues = []
    idx = 0
    for var, names in list(object_names_map.items()):
        for name in names:
            ind.cali.objnames.append('%s-%s' % (var, name))
            ind.cali.objvalues.append(vals[idx])
            idx += 1
    ind.runtime = getattr(ind, 'runtime', 0.)
    return ind


def objective_values_from_individual(ind):
    fitness_values = list()
    labels = list()
    for k, _ in list(multiobj.items()):
        tmpvalues, tmplabel = ind.cali.efficiency_values(k, object_names[k])
        fitness_values += tmpvalues[:]
        labels += tmplabel[:]
    if len(fitness_values) != len(multi_weight):
        raise ValueError("objective dimension mismatch: got %d, expected %d" %
                         (len(fitness_values), len(multi_weight)))
    if len(labels) != len(multi_weight):
        raise ValueError("objective labels missing: got %d, expected %d" %
                         (len(labels), len(multi_weight)))
    for val in fitness_values:
        if val is None or not math.isfinite(float(val)):
            raise ValueError("invalid objective value: %r" % val)
    return tuple(fitness_values), labels


def _individual_key(ind):
    return (getattr(ind, 'gen', -1),
            getattr(ind, 'id', -1),
            getattr(ind, 'run_id', getattr(ind, 'id', -1)))


def safe_evaluate(cali_obj, ind):
    try:
        lock_timeout = int(os.environ.get("SEIMS_EVALUATE_LOCK_TIMEOUT", "2400"))
        res = evaluate_blocking(cali_obj, ind, timeout_s=lock_timeout)
        if res is None or res == -1:
            return assign_penalty(ind, "evaluate returned missing result")
        if not getattr(res.cali, 'valid', False):
            return assign_penalty(res, "calibration result is invalid")
        objective_values_from_individual(res)
        return res
    except Exception as e:
        scoop_log(
            "[SAFE_EVALUATE_FAILED] gen=%s id=%s run_id=%s pid=%s err=%r" %
            (
                getattr(ind, "gen", -1),
                getattr(ind, "id", -1),
                getattr(ind, "run_id", -1),
                os.getpid(),
                e,
            )
        )
        return assign_penalty(ind, e)


creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# The FitnessMulti class equals to (as an example):
# class FitnessMulti(base.Fitness):
#     weights = (2., -1., -1.)
# NOTE that to maintain the compatibility with Python2 and Python3,
#      the com typecode=str('d') MUST NOT changed to typecode='d', since
#      the latter will raise TypeError that 'must be char, not unicode'!
creator.create('Individual', array.array, typecode=str('d'), fitness=creator.FitnessMulti,
               gen=-1, id=-1, run_id=-1, failed=False, failure_reason='',
               obs=TimeseriesData, sim=TimeseriesData,
               cali=ObsSimData, vali=ObsSimData,
               io_time=0., comp_time=0., simu_time=0., runtime=0.)
# The Individual class equals to:
# class Individual(array.array):
#     gen = -1  # Generation No.
#     id = -1   # Calibration index of current generation
#     def __init__(self):
#         self.fitness = FitnessMulti()

# Register NSGA-II related operations
toolbox = base.Toolbox()
toolbox.register('gene_values', initialize_calibrations)
toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
toolbox.register('evaluate', safe_evaluate)


# mate and mutate
toolbox.register('mate', tools.cxSimulatedBinaryBounded)
toolbox.register('mutate', tools.mutPolynomialBounded)
toolbox.register('select', tools.selNSGA2)



def main(cfg):
    """Main workflow of NSGA-II based Scenario analysis."""
    random.seed()
    scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))

    # Initial timespan variables
    stime = time.time()
    plot_time = 0.
    allmodels_exect = list()  # execute time of all model runs

    # create reference point for hypervolume
    ref_pt = numpy.array(worse_objects) * multi_weight * -1

    stats = tools.Statistics(lambda sind: sind.fitness.values)
    stats.register('min', numpy.min, axis=0)
    stats.register('max', numpy.max, axis=0)
    stats.register('avg', numpy.mean, axis=0)
    stats.register('std', numpy.std, axis=0)
    logbook = tools.Logbook()
    logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'

    # read observation data from MongoDB
    cali_obj = Calibration(cfg)

    # Read observation data just once
    model_cfg_dict = cali_obj.model.ConfigDict
    model_obj = MainSEIMS(args_dict=model_cfg_dict)

    model_obj.SetMongoClient()
    #obs_vars, obs_data_dict = model_obj.ReadOutletObservations(object_vars)  #读取每个变量的观测值
    obs_vars, obs_data_dict = model_obj.ReadOutletObservations_new(object_vars)  #ljj++
    #-------------------------xiaodw, if calibrate inundation extent --------------------
    if cali_inundation_extent:
        obs_inun_data_dict = ic_tool.load_monthly_tifs(
            root_dir=subbasin_flood_path,
            subbasin_ids=subbasin_ids,
            start="2010-01",
            end="2010-02",
            band=1,
            on_missing="warn",
            readonly=True
        )
        some_month = list(obs_inun_data_dict.keys())[0]
        arr_1171 = obs_inun_data_dict[some_month]["1171"]
        print(some_month, arr_1171.shape, arr_1171.dtype)
    #-------------------------xiaodw, if calibrate inundation area --------------------

    model_obj.UnsetMongoClient()

    # Initialize population
    # print(f"cfg.opt is {cfg.opt}")
    # print(f"npop is: {cfg.opt.npop}")
    param_values = cali_obj.initialize(cfg.opt.npop)
    pop = list()
    run_counter = 0
    for i in range(cfg.opt.npop):
        ind = creator.Individual(param_values[i])
        ind.gen = 0
        ind.id = i
        ind.run_id = run_counter
        run_counter += 1
        ind.obs.vars = obs_vars[:]
        ind.obs.data = deepcopy(obs_data_dict)
        if cali_inundation_extent:
            ind.obs.inun_data = deepcopy(obs_inun_data_dict)
        pop.append(ind)
        # print(f"ind: {ind} ")
    param_values = numpy.array(param_values)
    # print(f"npop: {cfg.opt.npop}")

    # print(param_values)
    # Write calibrated values to MongoDB
    # TODO, extract this function, which is same with `Sensitivity::write_param_values_to_mongodb`.
    write_param_values_to_mongodb_by_ids(cfg.model.db_name, cali_obj.ParamDefs, pop)
    # get the low and up bound of calibrated parameters
    bounds = numpy.array(cali_obj.ParamDefs['bounds'])
    low = bounds[:, 0]
    up = bounds[:, 1]
    low = low.tolist()
    up = up.tolist()
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
        """Evaluate model by SCOOP or map, and set fitness of individuals
         according to calibration step."""
        original_invalid_pops = list(invalid_pops)
        popnum = len(original_invalid_pops)
        labels = objective_labels()
        scoop_log("evaluate input population: %d" % popnum)

        try:
            from scoop import futures
            evaluated = list(futures.map(toolbox.evaluate,
                                         [cali_obj] * popnum,
                                         original_invalid_pops))
        except (ImportError, ImportWarning):
            evaluated = list(map(toolbox.evaluate,
                                 [cali_obj] * popnum,
                                 original_invalid_pops))
        except Exception as err:
            scoop_log("[EVALUATE_PARALLEL_FAILED] input_count=%d err=%r" % (popnum, err))
            evaluated = [assign_penalty(ind, "future map failed: %r" % err)
                         for ind in original_invalid_pops]

        result_map = dict()
        for ind in evaluated:
            if ind is None or ind == -1:
                continue
            result_map.setdefault(_individual_key(ind), ind)

        fixed_pops = list()
        for original in original_invalid_pops:
            key = _individual_key(original)
            tmpind = result_map.get(key)
            if tmpind is None:
                tmpind = assign_penalty(original, "future missing")
            try:
                tmpfitnessv, labels = objective_values_from_individual(tmpind)
                tmpind.fitness.values = tmpfitnessv
            except Exception as e:
                scoop_log("[MODEL_FAILED] gen=%s id=%s run_id=%s reason=%r" %
                          (getattr(tmpind, 'gen', -1),
                           getattr(tmpind, 'id', -1),
                           getattr(tmpind, 'run_id', -1),
                           e))
                tmpind = assign_penalty(tmpind, e)
            fixed_pops.append(tmpind)

        if filter_ind:
            invalid_count = 0
            for tmpind in fixed_pops:
                if not check_validation(tmpind.fitness.values):
                    assign_penalty(tmpind, "objective validation condition failed")
                    invalid_count += 1
            if invalid_count:
                scoop_log("filter_ind kept population size stable; "
                          "%d individuals failed validation and were penalized" %
                          invalid_count)

        penalty_count = sum(1 for tmpind in fixed_pops
                            if getattr(tmpind, 'failed', False)
                            or tmpind.fitness.values == penalty_fitness(multi_weight))
        valid_count = len(fixed_pops) - penalty_count
        scoop_log("evaluate output population: %d; penalty_count=%d; valid_count=%d" %
                  (len(fixed_pops), penalty_count, valid_count))
        if len(fixed_pops) != popnum:
            raise RuntimeError("evaluate_parallel output_count=%d input_count=%d" %
                               (len(fixed_pops), popnum))
        return fixed_pops, labels

    # Record the count and execute timespan of model runs during the optimization
    modelruns_count = {0: len(pop)}
    modelruns_time = {0: 0.}  # Total time counted according to evaluate_parallel()
    modelruns_time_sum = {0: 0.}  # Summarize time of every model runs according to pop

    # Generation 0 before optimization
    stime = time.time()
    # print(f"***************pop: {pop}")
    pop, plotlables = evaluate_parallel(pop)
    modelruns_time[0] = time.time() - stime
    for ind in pop:
        allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
        modelruns_time_sum[0] += ind.runtime

    # currently, len(pop) may less than pop_select_num
    pop = toolbox.select(pop, pop_select_num)
    # Output simulated data to json or pickle files for future use.
    output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)

    record = stats.compile(pop)
    logbook.record(gen=0, evals=len(pop), **record)
    # scoop_log(logbook.stream)

    # Begin the generational process
    output_str = '### Generation number: %d, Population size: %d ###\n' % (cfg.opt.ngens,
                                                                           cfg.opt.npop)
    # scoop_log(output_str)
    UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')

    modelsel_count = {0: len(pop)}  # type: Dict[int, int] # newly added Pareto fronts

    for gen in range(1, cfg.opt.ngens + 1):
        output_str = '###### Generation: %d ######\n' % gen
        # scoop_log(output_str)

        offspring = [toolbox.clone(ind) for ind in pop]
        # method1: use crowding distance (normalized as 0~1) as eta
        # tools.emo.assignCrowdingDist(offspring)
        # method2: use the index of individual at the sorted offspring list as eta
        if len(offspring) >= 2:  # when offspring size greater than 2, mate can be done
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

        # Evaluate the individuals with an invalid fitness
        invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
        valid_inds = [ind for ind in offspring if ind.fitness.valid]
        if len(invalid_inds) == 0:  # No need to continue
            scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
            break

        # Write new calibrated parameters to MongoDB
        param_values = list()
        for idx, ind in enumerate(invalid_inds):
            ind.gen = gen
            ind.id = idx
            ind.run_id = run_counter
            run_counter += 1
            param_values.append(ind[:])
        param_values = numpy.array(param_values)
        write_param_values_to_mongodb_by_ids(cfg.model.db_name, cali_obj.ParamDefs, invalid_inds)
        # Count the model runs, and execute models
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

        # Select the next generation population
        # Previous version may result in duplications of the same scenario in one Pareto front,
        #   thus, I decided to check and remove the duplications first.
        # pop = toolbox.select(pop + valid_inds + invalid_inds, pop_select_num)
        tmppop = pop + valid_inds + invalid_inds
        pop = list()
        unique_sces = set()
        for tmpind in tmppop:
            unique_key = getattr(tmpind, 'run_id', None)
            if unique_key is None or unique_key < 0:
                unique_key = (tmpind.gen, tmpind.id)
            if unique_key in unique_sces:
                continue
            unique_sces.add(unique_key)
            pop.append(tmpind)
        pop = toolbox.select(pop, pop_select_num)

        output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
        hyper_str = 'Gen: %d, New model runs: %d, ' \
                    'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
                    'Hypervolume: %.4f\n' % (gen, invalid_ind_size,
                                             curtimespan, modelruns_time_sum[gen],
                                             hypervolume(pop, ref_pt))
        # scoop_log(hyper_str)
        UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')

        record = stats.compile(pop)
        logbook.record(gen=gen, evals=len(invalid_inds), **record)
        # scoop_log(logbook.stream)

        # Count the newly generated near Pareto fronts
        new_count = 0
        for ind in pop:
            if ind.gen == gen:
                new_count += 1
        modelsel_count.setdefault(gen, new_count)

        # Plot 2D near optimal pareto front graphs,
        #   i.e., (NSE, RSR), (NSE, PBIAS), and (RSR,PBIAS)
        # And 3D near optimal pareto front graphs, i.e., (NSE, RSR, PBIAS)
        stime = time.time()
        front = numpy.array([ind.fitness.values for ind in pop])
        title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
                 'Near Pareto optimal solutions')

        if front.ndim == 2 and front.shape[1] >= 2:
            plot_pareto_front_single(front, plotlables, cfg.opt.out_dir,
                                     gen, title, plot_cfg=cali_obj.cfg.plot_cfg)
        else:
            scoop_log("Skip Pareto front plot: objective dimension < 2")
        plot_time += time.time() - stime

        # save in file
        # Header information
        output_str += 'generation\tcalibrationID\trunID\t'
        for kk, vv in list(object_names.items()):
            output_str += pop[0].cali.output_header(kk, vv, 'Cali')
        if cali_obj.cfg.calc_validation:
            for kkk, vvv in list(object_names.items()):
                output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')

        output_str += 'gene_values\n'
        for ind in pop:
            output_str += '%d\t%d\t%d\t' % (ind.gen, ind.id, getattr(ind, 'run_id', ind.id))
            for kk, vv in list(object_names.items()):
                output_str += ind.cali.output_efficiency(kk, vv)
            if cali_obj.cfg.calc_validation:
                for kkk, vvv in list(object_names.items()):
                    output_str += ind.vali.output_efficiency(kkk, vvv)
            output_str += str(ind)
            output_str += '\n'
        UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')

        # TODO: Figure out if we should terminate the evolution

    # Plot hypervolume and newly executed model count
    plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)

    # Save newly added Pareto fronts of each generations
    new_fronts_count = numpy.array(list(modelsel_count.items()))
    numpy.savetxt('%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
                  new_fronts_count, delimiter=str(','), fmt=str('%d'))

    # Save and print timespan information
    allmodels_exect = numpy.array(allmodels_exect)
    numpy.savetxt('%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
                  allmodels_exect, delimiter=str(' '), fmt=str('%.4f'))
    scoop_log('Running time of all SEIMS models:\n'
              '\tIO\tCOMP\tSIMU\tRUNTIME\n'
              'MAX\t%s\n'
              'MIN\t%s\n'
              'AVG\t%s\n'
              'SUM\t%s\n' % ('\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
                             '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
                             '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
                             '\t'.join('%.3f' % t for t in allmodels_exect.sum(0))))

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
              'Plot Pareto graphs timespan: %.4f' % (init_time, exec_time,
                                                     exec_time_sum, plot_time))

    return pop, logbook


if __name__ == "__main__":
    cf, method = get_optimization_config()
    cali_cfg = CaliConfig(cf, method=method)

    scoop_log('### START TO CALIBRATION OPTIMIZING ###')
    startT = time.time()

    fpop, fstats = main(cali_cfg)

    fpop.sort(key=lambda x: x.fitness.values)
    # scoop_log(fstats)
    with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
        # In case of 'TypeError: write() argument 1 must be unicode, not str' in Python2.7
        #   when using unicode_literals, please use '%s' to concatenate string!
        f.write('%s' % fstats.__str__())
    endT = time.time()
    scoop_log('### END OF CALIBRATION OPTIMIZING ###')
    scoop_log('Running time: %.2fs' % (endT - startT))
