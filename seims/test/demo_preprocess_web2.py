import os
import shutil
import sys
import numpy as np
import time

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

# windows本地配置
# os.environ['GDAL_DATA'] = r'D:\Anaconda3\envs\SEIMS\Lib\site-packages\osgeo\data\gdal'
# 服务器配置
os.environ['GDAL_DATA'] = '/datanode05/xujs/.conda/envs/SEIMS/lib/python3.6/site-packages/osgeo/data/gdal'


def project(input_file, output_file):
    # 定义输入文件路径和名称
    # input_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem.tif"
    # output_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem2.tif"

    projection = 'PROJCS["Beijing 1954 / 3-degree Gauss-Kruger zone 39",GEOGCS["Beijing 1954",DATUM["Beijing_1954",SPHEROID["Krassowsky 1940",6378245,298.2999999999998,AUTHORITY["EPSG","7024"]],AUTHORITY["EPSG","6214"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",117],PARAMETER["scale_factor",1],PARAMETER["false_easting",39500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]]]'
    # print(projection)
    # 创建目标投影对象（beijing）
    target_srs = osr.SpatialReference()
    target_srs.ImportFromWkt(projection)

    # 创建源空间参考对象(WGS1984)
    source_srs = osr.SpatialReference()
    source_srs.ImportFromEPSG(4326)

    # 设置重采样方法为双线性插值
    resample_alg = None

    # 重投影
    gdal.Warp(output_file, input_file,
              format='GTiff', resampleAlg=resample_alg, srcNodata=-9999,
              srcSRS=source_srs.ExportToWkt(),
              dstSRS=target_srs.ExportToWkt()
              )
def clip_raster(input1,input2,output):
    # 打开栅格数据集
    dataset = gdal.Open(input2, gdal.GA_ReadOnly)
    if dataset is None:
        raise ValueError("Could not open raster file")

    # 获取地理转换参数
    geo_transform = dataset.GetGeoTransform()

    # 根据地理转换参数计算边界
    x_min = geo_transform[0]
    y_min = geo_transform[3]
    x_max = geo_transform[0] + geo_transform[1] * dataset.RasterXSize
    y_max = geo_transform[3] + geo_transform[5] * dataset.RasterYSize

    # 输出边界
    print(f"Min X: {x_min}, Min Y: {y_min}")
    print(f"Max X: {x_max}, Max Y: {y_max}")

    # 清理资源
    dataset = None

    # 打开输入栅格
    input_dataset = gdal.Open(input1)
    if input_dataset is None:
        raise ValueError("无法打开输入栅格")

    # 获取栅格数据和空间参考信息
    rows, cols = input_dataset.RasterYSize, input_dataset.RasterXSize
    geotransform = input_dataset.GetGeoTransform()
    projection = input_dataset.GetProjection()

    # 计算裁剪后的栅格尺寸
    x_offset = int((x_min - geotransform[0]) / geotransform[1])
    y_offset = int((geotransform[3] - y_max) / geotransform[5])
    width = int((x_max - x_min) / geotransform[1])
    height = int((y_max - y_min) / geotransform[5])
    print(f"x_offset: {x_offset}, y_offset: {y_offset}")
    print(f"width: {width}, height: {height}")

    # 更新栅格的空间参考信息
    geotransform = (
        geotransform[0] + x_offset * geotransform[1],
        geotransform[1],
        geotransform[2],
        geotransform[3] - y_offset * geotransform[5],
        geotransform[4],
        geotransform[5]
    )

    # 创建输出栅格
    driver = gdal.GetDriverByName('GTiff')
    output_dataset = driver.Create(output, width, height, 1, gdal.GDT_Float32)
    output_dataset.SetGeoTransform(geotransform)
    output_dataset.SetProjection(projection)

    # 复制数据
    input_band = input_dataset.GetRasterBand(1)
    output_band = output_dataset.GetRasterBand(1)
    output_band.WriteArray(input_band.ReadAsArray(xoff=x_offset, yoff=y_offset, xsize=width, ysize=height))

    # 关闭数据集
    input_dataset = None
    output_dataset = None


def changeSHPFiled(shpPath,old_field_name,new_field_name):
    # 打开矢量数据集
    driver = ogr.GetDriverByName('ESRI Shapefile')
    vector_dataset = driver.Open(shpPath, update=1)
    if vector_dataset is None:
        print('无法打开矢量数据集')
        exit(1)

        # 获取矢量数据的图层
    layer = vector_dataset.GetLayer()
    if layer is None:
        print('无法获取图层')
        exit(1)

    # 获取原始字段列表
    # field_defn_list = layer.GetFieldDefnList()

    # # 假设我们要更改的字段名从'old_field_name'到'new_field_name'
    # old_field_name = 'old_field_name'
    # new_field_name = 'new_field_name'

    # 查找要更改的字段
    old_field_defn = None
    for i in range(layer.GetLayerDefn().GetFieldCount()):
        field_defn = layer.GetLayerDefn().GetFieldDefn(i)
        if field_defn.GetName() == old_field_name:
            old_field_defn = field_defn
            break

            # 检查是否找到了字段
    if old_field_defn is None:
        print(f'字段 {old_field_name} 未找到')
        return

    # 创建一个新的字段定义，除了名称以外与原始字段定义相同
    new_field_defn = ogr.FieldDefn(new_field_name, old_field_defn.GetType())
    new_field_defn.SetWidth(old_field_defn.GetWidth())
    new_field_defn.SetPrecision(old_field_defn.GetPrecision())

    # 如果字段有默认值，也设置新的默认值
    if old_field_defn.GetDefault() is not None:
        new_field_defn.SetDefault(old_field_defn.GetDefault())

    # 如果字段不是可选的，也设置新的非可选状态
    if not old_field_defn.IsNullable():
        new_field_defn.SetNullable(False)

    # 更改字段名
    i = layer.GetLayerDefn().GetFieldIndex(old_field_name)
    layer.AlterFieldDefn(i, new_field_defn, ogr.ALTER_NAME_FLAG)

    # 清理资源
    vector_dataset = None


def project(input_file, output_file):
    # 定义输入文件路径和名称
    # input_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem.tif"
    # output_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem2.tif"

    projection = 'PROJCS["Beijing 1954 / 3-degree Gauss-Kruger zone 39",GEOGCS["Beijing 1954",DATUM["Beijing_1954",SPHEROID["Krassowsky 1940",6378245,298.2999999999998,AUTHORITY["EPSG","7024"]],AUTHORITY["EPSG","6214"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",117],PARAMETER["scale_factor",1],PARAMETER["false_easting",39500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]]]'
    # print(projection)
    # 创建目标投影对象（beijing）
    target_srs = osr.SpatialReference()
    target_srs.ImportFromWkt(projection)

    # 创建源空间参考对象(WGS1984)
    source_srs = osr.SpatialReference()
    source_srs.ImportFromEPSG(4326)

    # 设置重采样方法为双线性插值
    resample_alg = None

    # 重投影
    gdal.Warp(output_file, input_file,
              format='GTiff', resampleAlg=resample_alg, srcNodata=-9999,
              srcSRS=source_srs.ExportToWkt(),
              dstSRS=target_srs.ExportToWkt()
              )

def main(pre_cfg_file, flow_code_type):
    # wtsd_name = get_watershed_name('Specify watershed name to run preprocess.')
    # if wtsd_name not in list(DEMO_MODELS.keys()):
        # print('%s is not one of the available demo watershed: %s' %
              # (wtsd_name, ','.join(list(DEMO_MODELS.keys()))))
        # exit(-1)
    # cur_path = UtilClass.current_path(lambda: 0)
    # SEIMS_path = os.path.abspath(cur_path + '../../..')

    # print(SEIMS_path) # E:\Code\SEIMS-hulugou
    # print(wtsd_name) # hulugou
    # print(DEMO_MODELS[wtsd_name]) # demo_hulugou_longterm_model
    # model_paths = ModelPaths(SEIMS_path, wtsd_name, DEMO_MODELS[wtsd_name])
    # seims_cfg = write_preprocess_config_file(model_paths, 'preprocess.ini')
    
    cf = ConfigParser()
    # print(pre_cfg_file)
    cf.read(pre_cfg_file)
    seims_cfg = PreprocessConfig(cf)

    # 1投影spatial_raster中的acc.tif
    # accPath = seims_cfg.workspace + r'\spatial_raster\acc.tif'
    # outPath = seims_cfg.workspace + r'\spatial_raster\acc1.tif'
    # project(accPath,outPath)
    # os.remove(accPath)
    # shutil.copy2(outPath, accPath)
    # os.remove(outPath)


    # # slope.tif裁剪spatial_raster中的acc.tif
    # slopPath = seims_cfg.workspace + r'\spatial_raster\slope.tif'
    # clip_raster(accPath,slopPath,outPath)
    # os.remove(accPath)
    # shutil.copy2(outPath, accPath)
    # os.remove(outPath)

    #




    #2设置slope和acc文件有值的地方相同
    # 打开第一个栅格数据集
    slopPath = seims_cfg.workspace + r'/spatial_raster/slope.tif'
    accPath = seims_cfg.workspace + r'/spatial_raster/acc.tif'
    dataset1 = gdal.Open(slopPath, gdal.GA_ReadOnly)
    band1 = dataset1.GetRasterBand(1)
    no_data_value1 = band1.GetNoDataValue()
    # 打开第二个栅格数据集
    dataset2 = gdal.Open(accPath, gdal.GA_Update)
    band2 = dataset2.GetRasterBand(1)
    no_data_value2 = band2.GetNoDataValue()
    # 确保两个栅格具有相同的尺寸
    if dataset1.RasterXSize != dataset2.RasterXSize or dataset1.RasterYSize != dataset2.RasterYSize:
        raise ValueError("The two rasters must have the same dimensions")
     # 读取第一个栅格为NumPy数组
    data_array1 = band1.ReadAsArray()
    # 创建一个掩膜来标记NoData区域
    mask = data_array1 != no_data_value1
    # 将NoData区域赋值为0
    data_array1[~mask] = 0
    # 读取第二个栅格的数组
    data_array2 = band2.ReadAsArray()
    # 使用相同的掩膜将第二个栅格的NoData对应区域也赋值为0
    data_array2[~mask] = no_data_value2
    # 如果你想要将更新后的数组写回到第二个栅格，你可以这样做：
    band2.WriteArray(data_array2)
    band2.SetNoDataValue(no_data_value2)  # 如果需要的话，重新设置NoData值
    band2.FlushCache()  # 确保数据被写入磁盘
    # 清理资源
    dataset1 = None
    dataset2 = None


    # 3更改 landuse.tif的type
    # landuse = seims_cfg.workspace + r'\spatial_raster\landuse.tif'
    # landuse1 = seims_cfg.workspace + r'\spatial_raster\landuse1.tif'
    # # 打开原始栅格数据集
    # dataset_in = gdal.Open(landuse, gdal.GA_ReadOnly)
    # band_in = dataset_in.GetRasterBand(1)
    # # 读取原始数据为NumPy数组
    # data_array = band_in.ReadAsArray()
    # # new_data_array = data_array.astype(np.float64)
    # # no_data_value = band_in.GetNoDataValue()
    # # mask = data_array != no_data_value
    # # # 将NoData区域赋值为0
    # # data_array[~mask] = -9999
    # # band_in.SetNoDataValue(-9999)
    # # band_in.WriteArray(new_data_array)
    # # band_in.bandType = GDT_Float32
    # # 获取原始数据的地理变换参数和投影信息
    # geo_transform = dataset_in.GetGeoTransform()
    # projection = dataset_in.GetProjection()
    # # 定义新的数据类型（例如 GDT_Int32）
    # new_data_type = gdal.GDT_Float32
    # # 创建一个新的数据集，具有相同的尺寸、地理变换和投影，但数据类型不同
    # driver = gdal.GetDriverByName('GTiff')
    # dataset_out = driver.Create(landuse1,
    #                             dataset_in.RasterXSize,
    #                             dataset_in.RasterYSize,
    #                             1,  # Number of bands
    #                             new_data_type)
    # # 设置新的数据集的地理变换和投影信息
    # dataset_out.SetGeoTransform(geo_transform)
    # dataset_out.SetProjection(projection)
    # band_out = dataset_out.GetRasterBand(1)
    # # 设置输出波段的NoData值（如果适用）
    # no_data_value = -9999
    # if no_data_value is not None:
    #     band_out.SetNoDataValue(no_data_value)
    # no_data_value = 65535
    # # print("band_in" + str(no_data_value))
    #
    # mask = data_array == no_data_value
    # # print(mask)
    # # 将NoData区域赋值为0
    # # print("band_out" + str(band_out.GetNoDataValue()))
    # data_array[mask] = band_out.GetNoDataValue()
    # band_out.WriteArray(data_array)
    # # 清理并关闭数据集
    # dataset_in = None
    # dataset_out = None
    # os.remove(landuse)
    # os.rename(landuse1,landuse)

    # 2更改subbasin.shp和basin.shp的字段名称
    subbainPath = seims_cfg.workspace + r'/spatial_shp/subbasin.shp'
    bainPath = seims_cfg.workspace + r'/spatial_shp/basin.shp'
    changeSHPFiled(subbainPath,'VALUE','SUBBASINID')
    changeSHPFiled(bainPath, 'code', 'BASIN')
    changeSHPFiled(bainPath, 'VALUE', 'BASIN')

    # 3入库及其他参数计算
    ImportMongodbClass.workflow(seims_cfg, flow_code_type)  # Import to MongoDB database
      


if __name__ == "__main__":
    start_time = time.time()
    pre_cfg_file = sys.argv[1]
    flow_code_type = sys.argv[2]
    main(pre_cfg_file, flow_code_type)
    end_time = time.time()
    range_time = end_time - start_time
    print("demo preprocess2 run time:", range_time)