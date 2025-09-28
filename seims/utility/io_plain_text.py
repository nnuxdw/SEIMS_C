"""Read and write of plain text file.

    @author   : Liangjun Zhu

    @changelog:
    - 18-10-29 - lj - Extract from other packages.
"""
from __future__ import absolute_import, unicode_literals

import json
from io import open
import os
from collections import OrderedDict
from datetime import datetime

from typing import List, Dict, Union, AnyStr
from numpy import ndarray as np_array
from pygeoc.utils import StringClass, UtilClass, FileClass
from preprocess.text import DBTableNames, ModelCfgFields, FieldNames, SubbsnStatsName, \
    DataValueFields, DataType, StationFields
from pymongo import MongoClient



class SpecialJsonEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, np_array):
            return obj.tolist()
        elif isinstance(obj, datetime):
            return obj.strftime('%Y-%m-%d %H:%M:%S')
        return json.JSONEncoder.default(self, obj)


def status_output(status_msg, percent, file_name):
    # type: (AnyStr, Union[int, float], AnyStr) -> None
    """Print status and flush to file.
    Args:
        status_msg: status message
        percent: percentage rate of progress
        file_name: file name
    """
    UtilClass.writelog(file_name, "[Output] %d..., %s" % (percent, status_msg), 'a')


def read_data_items_from_txt(txt_file):
    # type: (AnyStr) -> List[List[AnyStr]]
    """Read data items include title from text file, each data element are split by TAB or COMMA.
       Be aware, the separator for each line can only be TAB or COMMA, and COMMA is the recommended.
    Args:
        txt_file: full path of text data file
    Returns:
        2D data array
    """
    data_items = list()
    with open(txt_file, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            str_line = line.strip()
            if str_line != '' and str_line.find('#') < 0:
                line_list = StringClass.split_string(str_line, ['\t'])
                if len(line_list) <= 1:
                    line_list = StringClass.split_string(str_line, [','])
                data_items.append(line_list)
    return data_items


def read_simulation_from_txt(ws,  # type: AnyStr
                             plot_vars,  # type: List[AnyStr]
                             subbsnID,  # type: int
                             stime,  # type: datetime
                             etime  # type: datetime
                             ):
    # type: (...) -> (List[AnyStr], Dict[datetime, List[float]])
    """
    Read simulation data from text file according to subbasin ID.
    Returns:
        1. Matched variable names, [var1, var2, ...]
        2. Simulation data dict of all plotted variables, with UTCDATETIME.
           {Datetime: [value_of_var1, value_of_var2, ...], ...}
    """
    plot_vars_existed = list()
    sim_data_dict = OrderedDict()
    for i, v in enumerate(plot_vars):
        txtfile = ws + os.path.sep + v + '.txt'
        if not FileClass.is_file_exists(txtfile):
            print('WARNING: Simulation variable file: %s is not existed!' % txtfile)
            continue
        data_items = read_data_items_from_txt(txtfile)
        found = False
        data_available = False
        for item in data_items:
            item_vs = StringClass.split_string(item[0], ' ', elim_empty=True)
            if len(item_vs) == 2:
                if int(item_vs[1]) == subbsnID and not found:
                    found = True
                elif int(item_vs[1]) != subbsnID and found:
                    break
            if not found:
                continue
            if len(item_vs) != 3:
                continue
            date_str = '%s %s' % (item_vs[0], item_vs[1])
            sim_datetime = StringClass.get_datetime(date_str, "%Y-%m-%d %H:%M:%S")

            if stime <= sim_datetime <= etime:
                if sim_datetime not in sim_data_dict:
                    sim_data_dict[sim_datetime] = list()
                sim_data_dict[sim_datetime].append(float(item_vs[2]))
                data_available = True
        if data_available:
            plot_vars_existed.append(v)

    print('Read simulation from %s to %s done.' % (stime.strftime('%c'),
                                                   etime.strftime('%c')))
    return plot_vars_existed, sim_data_dict
#ljj
def read_simulation_from_txt_new(ws,  # type: AnyStr
                             plot_vars,  # type: List[AnyStr]
                             subbsnID,  # type: int
                             stime,  # type: datetime
                             etime  # type: datetime
                             ):
    # type: (...) -> (List[AnyStr], Dict[datetime, List[float]])
    """
    Read simulation data from text file according to subbasin ID.
    Returns:
        1. Matched variable names, [var1, var2, ...]
        2. Simulation data dict of all plotted variables, with UTCDATETIME.
           {Datetime: [value_of_var1, value_of_var2, ...], ...}
    """
    def get_subbasinid(name):
            """To avoid the prefix of subbasin number."""
            if '_' in name:
                name = name.split('_')[1]
            return name
    def get_observed_name_new(name):
            """To avoid the prefix of subbasin number."""
            if '_' in name:
                name = name.split('_')[0]
            return name
    plot_vars_existed = list()
    sim_data_dict = OrderedDict()
    for i, v in enumerate(plot_vars):
        var = get_observed_name_new(v)
        txtfile = ws + os.path.sep + var + '.txt'
        if not FileClass.is_file_exists(txtfile):
            print('WARNING: Simulation variable file: %s is not existed!' % txtfile)
            continue
        if var == 'F':
            # sites = siteTbl.find({StationFields.type: get_observed_name_new(param_name),
            #                       StationFields.outlet: float(is_outlets(param_name, isoutlet)),
            #                       StationFields.base_subbasin: subbasin_id
            #                       }).sort([(StationFields.id, 1)])  # 根据其所属下游subabsinid查
            client = MongoClient("mongodb://localhost:27017/")
            db = client["poyang_lake1_HydroClimate"]
            sites_collection = db[DBTableNames.sites]
            subbasin_id = int(get_subbasinid(v))
            query = {
                StationFields.type: var,
                StationFields.base_subbasin: subbasin_id
            }
            sites = sites_collection.find(query).sort(StationFields.id, 1)
            # 如果需要转成 list
            site_ids = list()
            found = {}
            data_available = False
            tmp_dict = OrderedDict()
            for site in sites:
                stationid = site[StationFields.id]
                site_ids.append(stationid)
                found[stationid] = False
            data_items = read_data_items_from_txt(txtfile)
            stationid = -1
            for item in data_items:
                item_vs = StringClass.split_string(item[0], ' ', elim_empty=True)

                if len(item_vs) == 2:
                    stationid = int(item_vs[1])
                    if stationid in site_ids:
                        found[stationid] = True
                if not found[stationid]:
                    continue
                if len(item_vs) != 3:
                    continue
                date_str = '%s %s' % (item_vs[0], item_vs[1])
                sim_datetime = StringClass.get_datetime(date_str, "%Y-%m-%d %H:%M:%S")
                if stime <= sim_datetime <= etime:
                    if sim_datetime not in tmp_dict:
                        tmp_dict[sim_datetime] = list()
                    tmp_dict[sim_datetime].append(float(item_vs[2]))
            for key, values in tmp_dict.items():
                if key not in sim_data_dict:
                    sim_data_dict[key] = []
                sum_inundation_area_of_subbasin = sum(values)
                sim_data_dict[key].append(float(sum_inundation_area_of_subbasin))
                data_available = True
            if data_available:
                plot_vars_existed.append(v)
        else:

            data_items = read_data_items_from_txt(txtfile)
            found = False
            data_available = False
            for item in data_items:
                item_vs = StringClass.split_string(item[0], ' ', elim_empty=True)

                if len(item_vs) == 2:
                    if int(item_vs[1]) == int(get_subbasinid(v)) and not found:
                        found = True
                    elif int(item_vs[1]) != int(get_subbasinid(v)) and found:
                        break
                if not found:
                    continue
                if len(item_vs) != 3:
                    continue
                date_str = '%s %s' % (item_vs[0], item_vs[1])
                sim_datetime = StringClass.get_datetime(date_str, "%Y-%m-%d %H:%M:%S")

                if stime <= sim_datetime <= etime:
                    if sim_datetime not in sim_data_dict:
                        sim_data_dict[sim_datetime] = list()
                    sim_data_dict[sim_datetime].append(float(item_vs[2]))
                    data_available = True
            if data_available:
                plot_vars_existed.append(v)
    print('Read simulation from %s to %s done.' % (stime.strftime('%c'),
                                                   etime.strftime('%c')))
    return plot_vars_existed, sim_data_dict
