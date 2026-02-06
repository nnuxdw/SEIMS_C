"""Entrance of Postprocess for SEIMS.

    @author   : Liangjun Zhu, Huiran Gao

    @changelog:
    - 17-08-17  - lj - redesign and rewrite the plotting program.
    - 18-02-09  - lj - compatible with Python3.
"""
from __future__ import absolute_import, unicode_literals

import os
import sys

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from postprocess.config import parse_ini_configuration
from postprocess.plot_timeseries import TimeSeriesPlots
from postprocess.plot_timeseries import read_runoff_file
from postprocess.plot_timeseries import read_simulation_only,plot_sim_only


def main():
    ###--------------------xiaodw, plot Q,QI,QS,QG into one chart
    subbasin_id = 2
    # subbasin_id = 1171
    file_dict = {
        "Surface Runoff": "QS.txt",
        "Interflow": "QI.txt",
        "Groundwater": "QG.txt",
        "Total Runoff": "Q.txt"
    }
    # basedir = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347\-90_124556_38_819347_longterm_model\OUTPUT0_base'
    basedir = r'G:\program\seims\SEIMS_HAND\data\US_2\US_2_longterm_model\OUTPUT0-0'
    # plot_runoff_components(basedir,file_dict,subbasin_id)
    # plot_runoff_difference(basedir,file_dict,subbasin_id)

    ###--------------------xiaodw, plot Q and observation into one chart and calculate nse

    merged = read_simulation_only(
        basedir=basedir,
        file_dict=file_dict,
        station_id=subbasin_id,
        sim_label_for_nse="Total Runoff",
        read_runoff_file_func=read_runoff_file,
    )
    # 时间分布图
    plot_sim_only(
        merged_df=merged,
        out_path=os.path.join(basedir, "Total Runoff.png"),
        title="Hydrograph",
        ylabel="Total Runoff",
        show=True
    )

    merged = read_simulation_only(
        basedir=basedir,
        file_dict=file_dict,
        station_id=subbasin_id,
        sim_label_for_nse="Interflow",
        read_runoff_file_func=read_runoff_file,
    )
    # 时间分布图
    plot_sim_only(
        merged_df=merged,
        out_path=os.path.join(basedir, "Interflow.png"),
        title="Hydrograph",
        ylabel="Interflow",
        show=True
    )
    # 时间分布图

    merged = read_simulation_only(
        basedir=basedir,
        file_dict=file_dict,
        station_id=subbasin_id,
        sim_label_for_nse="Groundwater",
        read_runoff_file_func=read_runoff_file,
    )
    plot_sim_only(
        merged_df=merged,
        out_path=os.path.join(basedir, "Groundwater.png"),
        title="Hydrograph",
        ylabel="Groundwater",
        show=True
    )
    merged = read_simulation_only(
        basedir=basedir,
        file_dict=file_dict,
        station_id=subbasin_id,
        sim_label_for_nse="Surface Runoff",
        read_runoff_file_func=read_runoff_file,
    )
    plot_sim_only(
        merged_df=merged,
        out_path=os.path.join(basedir, "Surface Runoff.png"),
        title="Hydrograph",
        ylabel="Surface Runoff",
        show=True
    )


    ###--------------------xiaodw, plot all upstream rainfall of one subbasinid
    # # 1) 找指定 SUBBASINID 的全部上游
    # upstream_ids = get_upstream_subbasins(mongo_uri, target_subbasin_id=subbasin_id)
    # print("上游 SUBBASINID：", upstream_ids)
    #
    # # 2) 取这些上游在 DATA_VALUES 里的时间序列，按时间求平均
    # avg_series = fetch_upstream_avg_series(
    #     mongo_uri,
    #     upstream_ids,
    #     type_code="P",  # 指定 TYPE，比如降水P、流量Q等
    #     utc_start="2014-01-01T00:00:00Z",  # 可换成需要的时间范围
    #     utc_end="2019-12-31T23:59:59Z",
    # )

    # # 3) 画折线图
    # plot_series(avg_series, title=f"Avg {'P'} of upstream to {subbasin_id}")

    ###--------------------xiaodw, plot 2DArray into one chart
    HRU_IDs = [2562,2563,2564,2565,2566,2567,2568,2569,2570,2571,2572,2573,2574,2575,2576,2577,2578,2579,2580,2581,2582]
    file_dict = {
        "Soil Moisture(mm)": "SOLST.txt",
        # "Porosity Depth(mm)": "PorosityDepth.txt",
        # "FieldCap Depth(mm)": "FieldCapDepth.txt",
        "Perco(mm)": "Perco.txt",
        "Awc(mm)": "Awc.txt",
        "Ul(mm)":"Ul.txt"
    }
    # for hru_id in HRU_IDs:
    #     plot_multi_source_soil_layers(basedir,file_dict, hru_id)

    """Main workflow."""
    cfg = parse_ini_configuration()

    TimeSeriesPlots(cfg).generate_plots()




if __name__ == "__main__":
    main()
