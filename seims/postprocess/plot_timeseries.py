"""Plot time-series variables.

    @author   : Liangjun Zhu

    @changelog:
    - 17-08-17  - lj - redesign and rewrite the plotting program.
    - 18-01-04  - lj - separate load data from MongoDB operations.
    - 18-02-01  - lj - add plot of validation period.
    - 18-02-09  - lj - compatible with Python3.
    - 19-01-09  - lj - use PlotConfig for plot settings.
"""
from __future__ import absolute_import, unicode_literals

import os
import sys
from datetime import datetime
import pandas as pd
import math
from pymongo import MongoClient


if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

import matplotlib as mpl

if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
    mpl.use('Agg', warn=False)
import matplotlib.dates as mdates
import matplotlib.pyplot as plt

from typing import List, Union
from pygeoc.utils import FileClass

from preprocess.text import DataValueFields
from preprocess.db_mongodb import ConnectMongoDB
from preprocess.db_read_model import ReadModelData
from postprocess.config import PostConfig
from run_seims import MainSEIMS
from utility import read_simulation_from_txt, match_simulation_observation, calculate_statistics
from utility import PlotConfig, save_png_eps
from typing import Union, Optional
from matplotlib.dates import AutoDateLocator, AutoDateFormatter
from typing import List, Optional, Iterable
from datetime import datetime
from collections import deque

import pandas as pd
from pymongo import MongoClient
import matplotlib.pyplot as plt

def get_upstream_subbasins(mongo_uri, target_subbasin_id,
                           db_name="poyang_lake1_longterm_model",
                           coll_name="REACHES"):
    """
    从 REACHES 集合基于 DOWNSTREAM 反向追溯，返回 target_subbasin_id 的所有上游 SUBBASINID（不含自身）
    说明：REACHES 文档里，字段 DOWNSTREAM = 该条支流/子流域的下游 SUBBASINID；-1 一般表示出湖口/无下游。
    """
    cli = MongoClient(mongo_uri)
    coll = cli[db_name][coll_name]

    visited = set()            # 已加入结果的上游节点
    frontier = deque([target_subbasin_id])  # 从目标的“下游端”开始，找所有以它为DOWNSTREAM的节点

    while frontier:
        down_id = frontier.popleft()
        # 找所有直接以上一个frontier元素为“下游”的子流域，这是它们的直接上游
        cur = coll.find(
            {"DOWNSTREAM": down_id},
            {"SUBBASINID": 1, "DOWNSTREAM": 1, "_id": 0}
        )

        for doc in cur:
            up_id = int(doc["SUBBASINID"])
            if up_id not in visited:
                visited.add(up_id)
                frontier.append(up_id)

    return sorted(visited)


def _parse_utc(ts) -> pd.Timestamp:
    """
    将 Mongo 中的 UTCDATETIME（可能是 datetime、或 ISO 字符串带Z）统一转为 pandas 的 UTC 时间戳
    """
    if isinstance(ts, datetime):
        return pd.to_datetime(ts, utc=True)
    # 字符串：可能是 '2010-01-01T00:00:00.000Z'
    s = str(ts).replace("Z", "+00:00")
    return pd.to_datetime(s, utc=True, errors="coerce")


def fetch_upstream_avg_series(mongo_uri, subbasin_ids,
                              data_db="poyang_lake1_HydroClimate",
                              data_coll="DATA_VALUES",
                              type_code="P",
                              utc_start=None,
                              utc_end=None):
    """
    在 DATA_VALUES 中查询给定 SUBBASINID 列表与 TYPE、时间范围内的记录，
    对每个 UTCDATETIME 聚合求所有上游站点 VALUE 的平均值，返回按时间排序的 pd.Series。

    参数：
    - subbasin_ids: 上游站点（SUBBASINID）列表
    - type_code: DATA_VALUES.TYPE，比如 'P'（降水）、'Q'（流量）等
    - utc_start / utc_end: UTC 起止（可为 datetime 或 ISO 字符串；若空则不限定）
    """
    cli = MongoClient(mongo_uri)
    coll = cli[data_db][data_coll]

    time_filter = {}
    if utc_start is not None or utc_end is not None:
        if utc_start is not None:
            time_filter["$gte"] = pd.to_datetime(utc_start, utc=True).to_pydatetime()
        if utc_end is not None:
            time_filter["$lt"] = pd.to_datetime(utc_end, utc=True).to_pydatetime()

    query = {
        # "STATIONID": {"$in": list(subbasin_ids)},
        "STATIONID": {"$in": [1,2]},
        "TYPE": type_code,
    }
    if time_filter:
        query["UTCDATETIME"] = time_filter

    cursor = coll.find(
        query,
        {"STATIONID": 1, "UTCDATETIME": 1, "VALUE": 1, "_id": 0}
    )

    df = pd.DataFrame(list(cursor))
    if df.empty:
        return pd.Series(dtype="float64")

    # 时间与数值清洗
    df["UTCDATETIME"] = df["UTCDATETIME"].map(_parse_utc)
    df["VALUE"] = pd.to_numeric(df["VALUE"], errors="coerce")
    df = df.dropna(subset=["UTCDATETIME", "VALUE"])

    # 对同一时间聚合求均值（忽略缺测站点）
    ser = (
        df.groupby("UTCDATETIME", as_index=True)["VALUE"]
          .mean()
          .sort_index()
    )
    return ser


def plot_series(ser: pd.Series, title: str = "Upstream Average"):
    """
    将时间序列画成折线图
    """
    if ser.empty:
        print("⚠️ 没有可绘制的数据。")
        return

    plt.figure(figsize=(10, 4))
    plt.plot(ser.index, ser.values)  # 折线
    plt.title(title)
    plt.xlabel("UTC Time")
    plt.ylabel("Average VALUE")
    plt.tight_layout()
    plt.show()

def divtd(td1, td2):
    us1 = td1.microseconds + 1000000 * (td1.seconds + 86400 * td1.days)
    us2 = td2.microseconds + 1000000 * (td2.seconds + 86400 * td2.days)
    return float(us1) / float(us2)


class TimeSeriesPlots(object):
    """Plot time series data, e.g., flow charge, sediment charge, etc.
    """

    def __init__(self, cfg):
        # type: (PostConfig) -> None
        """Constructor"""
        self.model = MainSEIMS(args_dict=cfg.model_cfg.ConfigDict)
        self.ws = self.model.OutputDirectory
        if not FileClass.is_dir_exists(self.ws):
            raise ValueError('The output directory %s is not existed!' % self.ws)
        self.plot_vars = cfg.plot_vars
        self.plot_cfg = cfg.plot_cfg  # type: PlotConfig
        # UTCTIME, calibration period
        self.stime = cfg.cali_stime
        self.etime = cfg.cali_etime
        self.subbsnID = cfg.plt_subbsnid
        # validation period
        self.vali_stime = cfg.vali_stime
        self.vali_etime = cfg.vali_etime

        # Read model data from MongoDB, the time period of simulation is read from FILE_IN.
        mongoclient = ConnectMongoDB(self.model.host, self.model.port).get_conn()
        self.readData = ReadModelData(mongoclient, self.model.db_name)
        self.mode = self.readData.Mode
        self.interval = self.readData.Interval
        # check start and end time of calibration
        st, et = self.readData.SimulationPeriod
        self.plot_validation = True
        if st > self.stime:
            self.stime = st
        if et < self.etime:
            self.etime = et
        if st > self.etime > self.stime:
            self.stime = st
            self.etime = et
            # in this circumstance, no validation should be calculated.
            self.vali_stime = None
            self.vali_etime = None
            self.plot_validation = False
        # check validation time period
        if self.vali_stime and self.vali_etime:
            if self.vali_stime >= self.vali_etime or st > self.vali_etime > self.vali_stime \
                or self.vali_stime >= et:
                self.vali_stime = None
                self.vali_etime = None
                self.plot_validation = False
            elif st > self.vali_stime:
                self.vali_stime = st
            elif et < self.vali_etime:
                self.vali_etime = et
        else:
            self.plot_validation = False
        # Set start time and end time of both calibration and validation periods
        start = self.stime
        end = self.etime
        if self.plot_validation:
            start = self.stime if self.stime < self.vali_stime else self.vali_stime
            end = self.etime if self.etime > self.vali_etime else self.vali_etime
        self.outletid = self.readData.OutletID
        # read precipitation
        self.pcp_date_value = self.readData.Precipitation(self.subbsnID, start, end)
        # read simulated data and update the available variables
        self.plot_vars, self.sim_data_dict = read_simulation_from_txt(self.ws, self.plot_vars,
                                                                      self.subbsnID,
                                                                      start, end)
        self.sim_data_value = list()  # type: List[List[Union[datetime, float]]]
        for d, vs in self.sim_data_dict.items():
            self.sim_data_value.append([d] + vs[:])
        # reset start time and end time
        if len(self.sim_data_value) == 0:
            raise RuntimeError('No available simulate data, please check the start and end time!')
        # read observation data from MongoDB
        self.obs_vars, self.obs_data_dict = self.readData.Observation(self.subbsnID, self.plot_vars,
                                                                      start, end)

        # Calibration period
        self.sim_obs_dict = match_simulation_observation(self.plot_vars, self.sim_data_dict,
                                                         self.obs_vars, self.obs_data_dict,
                                                         start_time=self.stime, end_time=self.etime)
        calculate_statistics(self.sim_obs_dict)
        # Validation period if existed
        self.vali_sim_obs_dict = dict()
        if self.plot_validation:
            self.vali_sim_obs_dict = match_simulation_observation(self.plot_vars,
                                                                  self.sim_data_dict,
                                                                  self.obs_vars,
                                                                  self.obs_data_dict,
                                                                  start_time=self.vali_stime,
                                                                  end_time=self.vali_etime)
            calculate_statistics(self.vali_sim_obs_dict)

    def generate_plots(self):
        """Generate hydrographs of discharge, sediment, nutrient (amount or concentrate), etc."""
        # set ticks direction, in or out
        plt.rcParams['xtick.direction'] = 'out'
        plt.rcParams['ytick.direction'] = 'out'
        plt.rcParams['font.family'] = self.plot_cfg.font_name
        plt.rcParams['mathtext.fontset'] = 'custom'
        plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
        plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'

        obs_str = 'Observation'
        sim_str = 'Simulation'
        cali_str = 'Calibration'
        vali_str = 'Validation'
        pcp_str = 'Precipitation'
        pcpaxis_str = 'Precipitation (mm)'
        xaxis_str = 'Date time'
        if self.plot_cfg.plot_cn:
            plt.rcParams['axes.unicode_minus'] = False
            obs_str = u'观测值'
            sim_str = u'模拟值'
            cali_str = u'率定期'
            vali_str = u'验证期'
            pcp_str = u'降水'
            pcpaxis_str = u'降水 (mm)'
            xaxis_str = u'时间'

        sim_date = list(self.sim_data_dict.keys())
        for i, param in enumerate(self.plot_vars):
            # plt.figure(i)
            fig, ax = plt.subplots(figsize=(12, 4))
            ylabel_str = param
            if param in ['Q', 'QI', 'QG', 'QS']:
                ylabel_str += ' (m$^3$/s)'
            elif 'CONC' in param.upper():  # Concentrate
                if 'SED' in param.upper():
                    ylabel_str += ' (g/L)'
                else:
                    ylabel_str += ' (mg/L)'
            elif 'SED' in param.upper():  # amount
                ylabel_str += ' (kg)'

            obs_dates = None  # type: List[datetime]
            obs_values = None  # type: List[float]
            if self.sim_obs_dict and param in self.sim_obs_dict:
                obs_dates = self.sim_obs_dict[param][DataValueFields.utc]
                obs_values = self.sim_obs_dict[param]['Obs']
            # append validation data
            if self.vali_sim_obs_dict and param in self.vali_sim_obs_dict:
                obs_dates += self.vali_sim_obs_dict[param][DataValueFields.utc]
                obs_values += self.vali_sim_obs_dict[param]['Obs']
            if obs_values is not None:
                # TODO: if the observed data is continuous with datetime, plot line, otherwise, bar.
                # bar graph
                p1 = ax.bar(obs_dates, obs_values, label=obs_str, color='none',
                            edgecolor='black',
                            linewidth=0.5, align='center', hatch='//')
                # # line graph
                # p1, = ax.plot(obs_dates, obs_values, label=obs_str, color='black', marker='+',
                #              markersize=2, linewidth=1)
            sim_list = [v[i + 1] for v in self.sim_data_value]
            p2, = ax.plot(sim_date, sim_list, label=sim_str, color='red',
                          marker='+', markersize=2, linewidth=0.8)
            plt.xlabel(xaxis_str, fontdict={'size': self.plot_cfg.axislabel_fsize})
            # format the ticks date axis
            date_fmt = mdates.DateFormatter('%m-%d-%y')
            # autodates = mdates.AutoDateLocator()
            # days = mdates.DayLocator(bymonthday=range(1, 32), interval=4)
            # months = mdates.MonthLocator()
            # ax.xaxis.set_major_locator(months)
            ax.xaxis.set_major_formatter(date_fmt)
            # ax.xaxis.set_minor_locator(days)
            ax.tick_params('both', length=5, width=2, which='major',
                           labelsize=self.plot_cfg.tick_fsize)
            ax.tick_params('both', length=3, width=1, which='minor',
                           labelsize=self.plot_cfg.tick_fsize)
            ax.set_xlim(left=self.sim_data_value[0][0], right=self.sim_data_value[-1][0])
            fig.autofmt_xdate(rotation=0, ha='center')

            plt.ylabel(ylabel_str, fontdict={'size': self.plot_cfg.axislabel_fsize})
            # plt.legend(bbox_to_anchor = (0.03, 0.85), loc = 2, shadow = True)
            if obs_values is not None:
                ymax = max(max(sim_list), max(obs_values)) * 1.6
                ymin = min(min(sim_list), min(obs_values)) * 0.8
            else:
                ymax = max(sim_list) * 1.8
                ymin = min(sim_list) * 0.8
            ax.set_ylim(float(ymin), float(ymax))
            ax2 = ax.twinx()
            ax.tick_params(axis='x', which='both', bottom=True, top=False,
                           labelsize=self.plot_cfg.tick_fsize)
            ax2.tick_params(axis='y', length=5, width=2, which='major',
                            labelsize=self.plot_cfg.tick_fsize)
            ax2.set_ylabel(pcpaxis_str, fontdict={'size': self.plot_cfg.axislabel_fsize})

            pcp_date = [v[0] for v in self.pcp_date_value]
            preci = [v[1] for v in self.pcp_date_value]
            p3 = ax2.bar(pcp_date, preci, label=pcp_str, color='blue', linewidth=0,
                         align='center')
            ax2.set_ylim(float(max(preci)) * 1.8, float(min(preci)) * 0.8)
            # draw a dash line to separate calibration and validation period
            delta_dt = (self.sim_data_value[-1][0] - self.sim_data_value[0][0]) // 9
            delta_dt2 = (self.sim_data_value[-1][0] - self.sim_data_value[0][0]) // 35
            # by default, separate time line is the end of calibration period
            sep_time = self.etime
            time_pos = [sep_time - delta_dt]
            ymax, ymin = ax2.get_ylim()
            yc = abs(ymax - ymin) / 4.
            order = 1  # By default, calibration period is before validation period
            if self.plot_validation:
                sep_time = self.vali_stime if self.vali_stime >= self.etime else self.stime
                cali_vali_labels = [cali_str, vali_str]
                if self.vali_stime < self.stime:
                    order = 0
                    cali_vali_labels = [vali_str, cali_str]
                time_pos = [sep_time - delta_dt, sep_time + delta_dt2]
                ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=2)
                plt.text(time_pos[0], yc, cali_vali_labels[0],
                         fontdict={'style': 'italic', 'weight': 'bold',
                                   'size': self.plot_cfg.label_fsize},
                         color='black')
                plt.text(time_pos[1], yc, cali_vali_labels[1],
                         fontdict={'style': 'italic', 'weight': 'bold',
                                   'size': self.plot_cfg.label_fsize},
                         color='black')
            # set legend and labels
            if obs_values is None or len(obs_values) < 2:
                leg = ax.legend([p3, p2], [pcp_str, sim_str], ncol=2,
                                bbox_to_anchor=(0., 1.02, 1., 0.102),
                                borderaxespad=0.2,
                                loc='lower left', fancybox=True,
                                fontsize=self.plot_cfg.legend_fsize)
            else:
                leg = ax.legend([p3, p1, p2], [pcp_str, obs_str, sim_str],
                                bbox_to_anchor=(0., 1.02, 1., 0.102),
                                borderaxespad=0.,
                                ncol=3, loc='lower left', fancybox=True,
                                fontsize=self.plot_cfg.legend_fsize)
                try:
                    nse = self.sim_obs_dict[param]['NSE']  # type: float
                    r2 = self.sim_obs_dict[param]['R-square']  # type: float
                    pbias = self.sim_obs_dict[param]['PBIAS']  # type: float
                    rsr = self.sim_obs_dict[param]['RSR']  # type: float
                    cali_txt = '$\mathit{NSE}$: %.2f\n$\mathit{RSR}$: %.2f\n' \
                               '$\mathit{PBIAS}$: %.2f%%\n$\mathit{R^2}$: %.2f' % \
                               (nse, rsr, pbias, r2)
                    print_msg_header = 'Cali-%s-NSE,Cali-%s-RSR,' \
                                       'Cali-%s-PBIAS,Cali-%s-R2,' % (param, param, param, param)
                    print_msg = '%.3f,%.3f,%.3f,%.3f,' % (nse, rsr, pbias, r2)
                    cali_pos = time_pos[0] if order else time_pos[1]
                    plt.text(cali_pos, yc * 2.5, cali_txt, color='red',
                             fontsize=self.plot_cfg.label_fsize - 1)
                    if self.plot_validation and self.vali_sim_obs_dict:
                        nse = self.vali_sim_obs_dict[param]['NSE']
                        r2 = self.vali_sim_obs_dict[param]['R-square']
                        pbias = self.vali_sim_obs_dict[param]['PBIAS']
                        rsr = self.vali_sim_obs_dict[param]['RSR']
                        vali_txt = '$\mathit{NSE}$: %.2f\n$\mathit{RSR}$: %.2f\n' \
                                   '$\mathit{PBIAS}$: %.2f%%\n$\mathit{R^2}$: %.2f' % \
                                   (nse, rsr, pbias, r2)
                        print_msg_header += 'Vali-%s-NSE,Vali-%s-RSR,' \
                                            'Vali-%s-PBIAS,' \
                                            'Vali-%s-R2' % (param, param, param, param)
                        print_msg += '%.3f,%.3f,%.3f,%.3f' % (nse, rsr, pbias, r2)
                        vali_pos = time_pos[1] if order else time_pos[0]
                        plt.text(vali_pos, yc * 2.5, vali_txt, color='red',
                                 fontsize=self.plot_cfg.label_fsize - 1)
                    print('%s\n%s\n' % (print_msg_header, print_msg))

                except ValueError or Exception:
                    pass
            plt.tight_layout()
            leg.get_frame().set_alpha(0.5)
            timerange = '%s-%s' % (self.sim_data_value[0][0].strftime('%Y-%m-%d'),
                                   self.sim_data_value[-1][0].strftime('%Y-%m-%d'))
            save_png_eps(plt, self.ws, param + '-' + timerange, self.plot_cfg)


def read_runoff_file(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    runoff_data = {}
    current_subbasin = None
    current_data = []

    for line in lines:
        line = line.strip()
        if not line:
            continue

        if line.startswith("Subbasin:"):
            # 保存上一个subbasin的数据
            if current_subbasin is not None and current_data:
                df = pd.DataFrame(current_data, columns=["Date", "Value"])
                df["Date"] = pd.to_datetime(df["Date"])
                df.set_index("Date", inplace=True)
                runoff_data[current_subbasin] = df
                current_data = []

            # 新subbasin编号
            current_subbasin = int(line.split(":")[1].strip())
        else:
            # 正常的时间+数值行
            parts = line.split()
            if len(parts) >= 2:
                date = parts[0]
                value = float(parts[-1])
                current_data.append((date, value))

    # 收尾处理最后一个subbasin
    if current_subbasin is not None and current_data:
        df = pd.DataFrame(current_data, columns=["Date", "Value"])
        df["Date"] = pd.to_datetime(df["Date"])
        df.set_index("Date", inplace=True)
        runoff_data[current_subbasin] = df

    return runoff_data  # dict[subbasin] = DataFrame

"""xiaodw, plot Q,QI,QS,QG into one chart"""
def plot_runoff_components(basedir, file_dict, subbasin_id):
    """
    file_dict: dict，如 {
        "Surface Runoff": "surf.txt",
        "Interflow": "interflow.txt",
        "Groundwater": "gw.txt",
        "Total Runoff": "total.txt"
    }
    basedir: 文件所在目录 + 图像保存目录
    subbasin_id: 要提取的子流域编号（整数）
    """
    plt.figure(figsize=(12, 6))

    for label, file_name in file_dict.items():
        file_path = os.path.join(basedir, file_name)
        runoff_dict = read_runoff_file(file_path)

        if subbasin_id not in runoff_dict:
            print(f"跳过 {file_path}，未找到 Subbasin {subbasin_id}")
            continue

        df = runoff_dict[subbasin_id]
        plt.plot(df.index, df['Value'], label=label)

    plt.title(f"Runoff Components for Subbasin {subbasin_id}")
    plt.xlabel("Date")
    plt.ylabel("Runoff (unit based on input)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    # 保存图像
    output_path = os.path.join(basedir, "Q_all.png")
    plt.savefig(output_path, dpi=300)
    print(f"图已保存到: {output_path}")

    # 显示图像
    plt.show()

"""xiaodw, plot Q and observe into one chart"""
def nash_sutcliffe_efficiency(obs: pd.Series, sim: pd.Series) -> float:
    """
    NSE = 1 - sum((sim - obs)^2) / sum((obs - mean(obs))^2)
    要求 obs 和 sim 的索引一致（同一时间戳），且为 float。
    """
    # 对齐（防御式编程）
    s = sim.loc[obs.index].astype(float)
    o = obs.astype(float)

    # 只保留两者同时非空/有限的值
    valid = (~o.isna()) & (~s.isna()) & o.apply(math.isfinite) & s.apply(math.isfinite)
    o = o[valid]
    s = s[valid]

    if len(o) == 0:
        raise ValueError("NSE计算失败：没有可用的重叠观测/模拟数据点。")

    denom = ((o - o.mean()) ** 2).sum()
    if denom == 0:
        # 所有观测值相同，NSE不定义，通常返回很差的值或 NaN
        return float("nan")

    return 1.0 - (((s - o) ** 2).sum() / denom)


def load_obs_from_mongo(
    mongo_uri: str,
    db_name: str,
    collection: str,
    *,
    station_field,
    station_id,
    time_field,
    value_field,
    invalid_values=("NONE", None, "", "NaN"),
    tz: Optional[str] = None  # 等价于 str | None
) -> pd.DataFrame:
    """
    从 MongoDB 读取观测数据，返回 DataFrame，index 为时间戳，只有一列 'obs'。
    可用 station_id 作为筛选条件。
    文档字段默认：
      - 时间：time_field (默认 "DateTime")
      - 数值：value_field (默认 "Value")
    """


    client = MongoClient(mongo_uri)
    coll = client[db_name][collection]

    query = {}

    if station_id is not None and station_field:
        query[station_field] = station_id

    cursor = coll.find(query, {time_field: 1, value_field: 1, "_id": 0})
    rows = list(cursor)

    if not rows:
        raise ValueError("未查询到任何观测记录，请检查查询条件/字段名。")

    # 构建 DataFrame
    df = pd.DataFrame(rows)

    # 解析时间
    df[time_field] = pd.to_datetime(df[time_field], errors="coerce", utc=True)
    if tz:
        # 转换到指定时区（如果需要）
        df[time_field] = df[time_field].dt.tz_convert(tz)

    # 清洗值：去除无效值、不可转换为数值的行
    df[value_field] = df[value_field].replace(invalid_values, pd.NA)
    df[value_field] = pd.to_numeric(df[value_field], errors="coerce")

    df = df.dropna(subset=[time_field, value_field])

    # 按时间排序并去重
    df = df.sort_values(time_field)
    df = df.drop_duplicates(subset=[time_field], keep="last")

    # 设置索引
    df = df.set_index(time_field)
    df = df.rename(columns={value_field: "obs"})

    return df[["obs"]]

def plot_obs_vs_sim(merged_df,
                            out_path=None,
                            title=None,
                            ylabel="Discharge",
                            nse=None,
                            show=True,
                            figsize=(12, 6),
                            markersize=15,
                            alpha=0.7):
    """
    将对齐后的观测和模拟时间序列画成点图。

    参数
    ----
    merged_df : pd.DataFrame
        需包含 'obs' 与 'sim' 两列，索引为 DatetimeIndex。
    out_path : str or None
        若给定则保存到该路径。
    title : str or None
        图标题；若传入 nse，则会在标题后追加 " | NSE=..."。
    ylabel : str
        纵轴名称。
    nse : float or None
        若提供，则在标题中显示 NSE。
    show : bool
        是否 plt.show()。
    figsize : tuple
        画布大小。
    markersize : int
        点的大小。
    alpha : float
        点的透明度。
    """
    if merged_df.empty:
        raise ValueError("merged_df 为空，无法绘图。")

    if title is None:
        title = "Observed vs Simulated"

    if nse is not None and nse == nse:  # 避免 NaN
        title = "{} | NSE={:.3f}".format(title, nse)

    fig, ax = plt.subplots(figsize=figsize)

    # 点图
    ax.scatter(merged_df.index, merged_df["obs"],
               label="Observed", s=10, alpha=0.8, marker="o", color="blue", zorder=2)

    ax.scatter(merged_df.index, merged_df["sim"],
               label="Simulated", s=15, alpha=0.6, marker="o", color="red", zorder=1)

    # 轴与网格
    ax.set_title(title)
    ax.set_xlabel("Date")
    ax.set_ylabel(ylabel)
    ax.grid(True, alpha=0.3)

    # 时间刻度自动格式化
    locator = AutoDateLocator()
    formatter = AutoDateFormatter(locator)
    ax.xaxis.set_major_locator(locator)
    ax.xaxis.set_major_formatter(formatter)

    ax.legend()
    fig.tight_layout()

    if out_path:
        fig.savefig(out_path, dpi=300)
        print("图已保存到:", out_path)

    if show:
        plt.show()
    else:
        plt.close(fig)

def plot_obs_vs_sim_scatter_xy(merged_df,
                               out_path=None,
                               title="Observed vs Simulated Scatter",
                               xlabel="Observed",
                               ylabel="Simulated",
                               nse=None,
                               show=True,
                               figsize=(6, 6),
                               markersize=15,
                               alpha=0.6):
    """
    画散点图：横轴是观测值，纵轴是模拟值，并加 1:1 参考线。

    参数
    ----
    merged_df : pd.DataFrame
        需包含 'obs' 与 'sim' 两列。
    out_path : str or None
        若给定则保存到该路径。
    title : str
        图标题；若传入 nse，则会在标题后追加 " | NSE=..."。
    xlabel, ylabel : str
        坐标轴标签。
    nse : float or None
        若提供，则在标题中显示 NSE。
    show : bool
        是否 plt.show()。
    figsize : tuple
        画布大小。
    markersize : int
        点的大小。
    alpha : float
        点的透明度。
    """
    if merged_df.empty:
        raise ValueError("merged_df 为空，无法绘图。")

    if nse is not None and nse == nse:
        title = "{} | NSE={:.3f}".format(title, nse)

    fig, ax = plt.subplots(figsize=figsize)

    # 散点
    ax.scatter(merged_df["obs"], merged_df["sim"],
               s=markersize, alpha=alpha, color="blue", edgecolors="none")

    # 加 1:1 参考线
    maxv = max(merged_df["obs"].max(), merged_df["sim"].max())
    ax.plot([0, maxv], [0, maxv], "k--", lw=1, label="1:1 line")

    ax.set_title(title)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()

    if out_path:
        fig.savefig(out_path, dpi=300)
        print("图已保存到:", out_path)

    if show:
        plt.show()
    else:
        plt.close(fig)

def compute_nse_with_sim_and_obs(
    basedir: str,
    file_dict: dict,
    mongo_uri: str,
    db_name: str,
    collection: str,
    *,
    station_field: Optional[str] = None,
    station_id,
    time_field: str = "UTCDATETIME",
    value_field: str = "VALUE",
    invalid_values = ("NONE", None, "", "NaN"),
    tz: Optional[str] = None,
    # 模拟数据标签选择：用于对齐时选择哪条曲线作为“模拟总流量”
    sim_label_for_nse,
    # 你的 read_runoff_file 函数
    read_runoff_file_func=None,
):
    """
    组合：从 Mongo 取观测、从文件取模拟 -> 时间对齐 -> 计算 NSE。
    返回 (nse, merged_df)，其中 merged_df 含 'obs' 与 'sim' 两列。
    """
    if read_runoff_file_func is None:
        raise ValueError("请通过 read_runoff_file_func 参数传入你已有的 read_runoff_file 函数。")

    # 1) 读取观测
    obs_df = load_obs_from_mongo(
        mongo_uri=mongo_uri,
        db_name=db_name,
        collection=collection,
        station_field=station_field,
        station_id=station_id,
        time_field=time_field,
        value_field=value_field,
        invalid_values=invalid_values,
        tz=tz,
    )

    # 2) 读取模拟（用你给的 read_runoff_file 与 file_dict）
    #    file_dict 结构示例：
    #    {
    #        "Surface Runoff": "surf.txt",
    #        "Interflow": "interflow.txt",
    #        "Groundwater": "gw.txt",
    #        "Total Runoff": "total.txt"
    #    }
    sim_series = None

    for label, file_name in file_dict.items():
        if label != sim_label_for_nse:
            continue
        file_path = os.path.join(basedir, file_name)
        sim_dict = read_runoff_file_func(file_path)
        if station_id not in sim_dict:
            raise ValueError(f"模拟文件 {file_path} 中未找到 station_id {station_id}")
        df_sim = sim_dict[station_id].copy()

        # 期望 df_sim.index 是 DatetimeIndex，且有列 'Value'
        if not isinstance(df_sim.index, pd.DatetimeIndex):
            df_sim.index = pd.to_datetime(df_sim.index, errors="coerce", utc=True)

        df_sim = df_sim.sort_index()
        sim_series = df_sim["Value"].rename("sim")
        break

    if sim_series is None:
        raise ValueError(f"未在 file_dict 中找到用于 NSE 的曲线标签：{sim_label_for_nse}")

    # 3) 根据观测时间过滤模拟（并做 inner join 对齐）
    def to_utc_naive(index_like) -> pd.DatetimeIndex:
        """
        统一把各种时间索引/数组转成：先按 UTC 解析 -> 再去掉时区（naive）。
        """
        idx = pd.to_datetime(index_like, utc=True, errors="coerce")
        return idx.tz_localize(None)

    # —— 在拼接前做统一 ——
    obs_df = obs_df.copy()
    obs_df.index = to_utc_naive(obs_df.index)

    sim_series = sim_series.copy()
    sim_series.index = to_utc_naive(sim_series.index)
    merged = pd.concat([obs_df["obs"], sim_series.rename("sim")], axis=1).dropna()

    if merged.empty:
        # 尝试限制模拟到观测时间范围再 join（更严格的窗口）
        start, end = obs_df.index.min(), obs_df.index.max()
        sim_cut = sim_series.loc[(sim_series.index >= start) & (sim_series.index <= end)]
        merged = obs_df.join(sim_cut, how="inner")

    if merged.empty:
        raise ValueError("时间对齐后没有重叠的数据点，请检查观测与模拟的时间轴/时区。")

    # 4) 计算 NSE
    nse = nash_sutcliffe_efficiency(merged["obs"], merged["sim"])

    return nse, merged


"""xiaodw, plot QI+QS+QG-Q into one chart"""
def plot_runoff_difference(basedir, file_dict, subbasin_id):
    """
    对 file_dict 中前 len-1 项求和，与最后一个总项比较，绘制差值序列
    """
    runoff_data = {}
    for label, file_name in file_dict.items():
        file_path = os.path.join(basedir, file_name)
        runoff_dict = read_runoff_file(file_path)

        if subbasin_id not in runoff_dict:
            print(f"{label} 跳过（未包含 Subbasin {subbasin_id}）")
            return

        runoff_data[label] = runoff_dict[subbasin_id]["Value"]

    # 确保时间索引对齐
    all_series = list(runoff_data.values())
    for s in all_series[1:]:
        if not s.index.equals(all_series[0].index):
            raise ValueError("时间索引不一致，不能进行逐点相加")

    component_labels = list(file_dict.keys())
    sum_series = sum(runoff_data[label] for label in component_labels[:-1])
    total_series = runoff_data[component_labels[-1]]
    diff_series = sum_series - total_series

    # 画图
    plt.figure(figsize=(12, 4))
    plt.plot(diff_series.index, diff_series.values, label="(QS + QI + QG) - Q")
    plt.axhline(0, color='gray', linestyle='--')
    plt.title(f"Difference Between Component Sum and Total Runoff (Subbasin {subbasin_id})")
    plt.xlabel("Date")
    plt.ylabel("Difference")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()

    output_path = os.path.join(basedir, "Q_diff.png")
    plt.savefig(output_path, dpi=300)
    print(f"误差图已保存到: {output_path}")
    plt.show()


def read_soil_layers(file_path, subbasin_id):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    capturing = False
    data_rows = []

    for line in lines:
        line = line.strip()
        if not line:
            continue

        if line.startswith("Subbasin:"):
            current_id = int(line.split(":")[1].strip())
            capturing = (current_id == subbasin_id)
            continue

        if capturing:
            if line.startswith("Time"):
                continue
            parts = line.split()
            if len(parts) < 2:
                continue
            timestamp = parts[0] + " " + parts[1]
            values = [float(val) for val in parts[2:]]
            data_rows.append([timestamp] + values)

        # 可选停止读取逻辑
        if capturing and line.startswith("Subbasin:") and int(line.split(":")[1]) != subbasin_id:
            break

    df = pd.DataFrame(data_rows)
    df.columns = ["Time"] + [f"Layer{i}" for i in range(len(df.columns)-1)]
    df["Time"] = pd.to_datetime(df["Time"])
    df.set_index("Time", inplace=True)

    return df

def plot_multi_source_soil_layers(basedir, file_dict, subbasin_id):
    # 读取所有文件，构造成 {label: dataframe}
    data_by_label = {}
    for label, filename in file_dict.items():
        path = os.path.join(basedir, filename)
        try:
            df = read_soil_layers(path, subbasin_id)
            data_by_label[label] = df
        except Exception as e:
            print(f"读取 {label} 失败：{e}")

    if not data_by_label:
        print("没有有效数据可绘制")
        return

    # 假设所有文件的土层数一致，取第一个 DataFrame 的列数
    first_df = next(iter(data_by_label.values()))
    num_layers = first_df.shape[1]
    layer_names = first_df.columns

    fig, axes = plt.subplots(num_layers, 1, figsize=(12, 2.5 * num_layers), sharex=True)

    if num_layers == 1:
        axes = [axes]

    for i, layer_name in enumerate(layer_names):
        ax = axes[i]
        for label, df in data_by_label.items():
            if layer_name in df.columns:
                ax.plot(df.index, df[layer_name], label=label)
        ax.set_ylabel(f"{layer_name} (mm)")
        ax.set_title(f"{layer_name} - HRU {subbasin_id}")
        ax.grid(True)
        ax.legend()

    axes[-1].set_xlabel("Date")
    plt.tight_layout()

    # 自动构造保存路径
    file_stem = "_".join([key.replace(" ", "") for key in file_dict.keys()])
    filename = f"{file_stem}_HRU{subbasin_id}.png"
    save_path = os.path.join(basedir, filename)

    plt.savefig(save_path, dpi=300)
    print(f"图已保存到: {save_path}")

    plt.show()


