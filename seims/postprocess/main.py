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
from postprocess.plot_timeseries import plot_runoff_components
from postprocess.plot_timeseries import plot_runoff_difference
from postprocess.plot_timeseries import plot_multi_source_soil_layers



def main():
    ###--------------------xiaodw, plot Q,QI,QS,QG into one chart
    file_dict = {
        "Surface Runoff": "QS.txt",
        "Interflow": "QI.txt",
        "Groundwater": "QG.txt",
        "Total Runoff": "Q.txt"
    }
    basedir = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347\-90_124556_38_819347_longterm_model\OUTPUT'
    # plot_runoff_components(basedir,file_dict,173)
    ###--------------------xiaodw, plot QI+QS+QG-Q into one chart
    # plot_runoff_difference(basedir,file_dict,173)

    ###--------------------xiaodw, plot 2DArray into one chart
    subbasin_id = 173
    solst_file = os.path.join(basedir,'SOLST.txt')
    file_dict = {
        "Soil Moisture(mm)": "SOLST.txt",
        "Porosity Depth(mm)": "PorosityDepth.txt",
        "FieldCap Depth(mm)": "FieldCapDepth.txt"
    }
    plot_multi_source_soil_layers(basedir,file_dict, subbasin_id)

    """Main workflow."""
    cfg = parse_ini_configuration()

    TimeSeriesPlots(cfg).generate_plots()




if __name__ == "__main__":
    main()
