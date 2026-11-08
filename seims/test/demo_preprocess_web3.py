import os
import shutil
import sys
import numpy as np

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from pygeoc.utils import UtilClass

from preprocess.db_build_mongodb import ImportMongodbClass
from preprocess.sd_delineation import SpatialDelineation
from demo_config import ModelPaths, write_preprocess_config_file
from demo_config import DEMO_MODELS, get_watershed_name
from configparser import ConfigParser
from preprocess.config import PreprocessConfig
from osgeo import gdal,ogr,osr

from pygeoc.raster import RasterUtilClass
from osgeo.osr import SpatialReference as osr_SpatialReference
from osgeo.ogr import CreateGeometryFromWkt as ogr_CreateGeometryFromWkt
from osgeo.osr import CoordinateTransformation as osr_CoordinateTransformation
from numpy import where, fromfunction
from osgeo.gdal import GDT_Int32, GDT_Float32

from pygeoc.raster import RasterUtilClass
# from pygeoc.TauDEM import TauDEM
# from pygeoc import TauDEM
import pygeoc.TauDEM as TauDEM
import time
# windows本地配置
# os.environ['GDAL_DATA'] = r'D:\Anaconda3\envs\SEIMS\Lib\site-packages\osgeo\data\gdal'
# 服务器配置
# os.environ['GDAL_DATA'] = '/datanode05/xujs/.conda/envs/SEIMS/lib/python3.6/site-packages/osgeo/data/gdal'



def main(pre_cfg_file, flow_code_type):

    start_time = time.time()
    cf = ConfigParser()
    # print(pre_cfg_file)
    cf.read(pre_cfg_file)
    seims_cfg = PreprocessConfig(cf)

    # 气象数据,土壤入库
    ImportMongodbClass.workflow2(seims_cfg, flow_code_type)  # Import to MongoDB database
    end_time = time.time()
    range_time = end_time - start_time
    print("demo preprocess3 run time:", range_time)


if __name__ == "__main__":
    pre_cfg_file = sys.argv[1]
    flow_code_type = sys.argv[2]
    main(pre_cfg_file, flow_code_type)
