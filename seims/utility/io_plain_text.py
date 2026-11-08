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
from collections import defaultdict

f_type = 'lixiyue'

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
            if os.name == 'nt':  # Windows
                client = MongoClient("mongodb://172.21.124.127:27019/")
            else:
                client = MongoClient("mongodb://localhost:27019/")
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
            site_set = set(site_ids)
            # 预先把时间范围转成ISO字符串，便于用字符串比较（速度远快于解析datetime）
            st_str = stime.strftime('%Y-%m-%d %H:%M:%S')
            en_str = etime.strftime('%Y-%m-%d %H:%M:%S')

            out = defaultdict(list)
            cur_station = None
            keep = False  # 当前块是否需要
            for rec in data_items:
                s = rec[0] if isinstance(rec, (list, tuple)) else rec
                # 最多分成3段：快
                parts = s.split(None, 2)
                n = len(parts)

                if n == 2:
                    # 站点头行：... <stationid>
                    # 第二段是stationid（你的原逻辑如此）
                    try:
                        cur_station = int(parts[1])
                        keep = (cur_station in site_set)
                    except ValueError:
                        keep = False
                    continue

                if not keep:
                    continue

                if n != 3:
                    continue

                # 数据行：<date> <time> <value>
                # 组合成 'YYYY-MM-DD HH:MM:SS'
                dt_str = parts[0] + ' ' + parts[1]

                # 用字符串范围比较过滤时间（ISO串可直接比较大小）
                if dt_str < st_str or dt_str > en_str:
                    continue

                try:
                    val = float(parts[2])
                except ValueError:
                    continue

                out[dt_str].append(val)

            # 若需要datetime键，再统一转换一次（次数更少）
            from datetime import datetime
            out_dt = defaultdict(list)
            for k_str, vals in out.items():
                out_dt[datetime.strptime(k_str, "%Y-%m-%d %H:%M:%S")].extend(vals)
            if f_type == 'longdi':
                # 1) 先计算每天的总和，并按(年,月)分桶
                daily_sum = {}
                month_bucket = defaultdict(list)

                for key_dt, values in out_dt.items():
                    s = float(sum(values))  # 当天所有 subbasin 的总和
                    daily_sum[key_dt] = s
                    ym = (key_dt.year, key_dt.month)
                    month_bucket[ym].append(s)

                # 按月平均（基于“每日总和”的均值）
                month_avg = {ym: (sum(vals) / len(vals)) for ym, vals in month_bucket.items()}

                # 写回 sim_data_dict：先写每日总和，再写该月平均
                for (y, m), avg in month_avg.items():
                    key_dt = datetime(y, m, 1, 0, 0, 0)  # 该月第一天
                    if key_dt not in sim_data_dict:
                        sim_data_dict[key_dt] = []
                    sim_data_dict[key_dt].append(float(avg))
                    data_available = True
                if data_available:
                    plot_vars_existed.append(v)
            elif f_type == 'lixiyue':
                # 1) 直接按“每日”处理，不再收集到 month_bucket
                daily_sum = {}

                for key_dt, values in out_dt.items():
                    s = float(sum(values))  # 当天所有 subbasin 的总和
                    daily_sum[key_dt] = s

                # 2) 逐日写回 sim_data_dict
                for key_dt, s in daily_sum.items():
                    if key_dt not in sim_data_dict:
                        sim_data_dict[key_dt] = []
                    sim_data_dict[key_dt].append(s)
                    data_available = True

                # 3) 记录该变量已存在
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
