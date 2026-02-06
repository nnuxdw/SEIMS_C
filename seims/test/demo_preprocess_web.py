import os
import sys

from osgeo import osr, gdal
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

def generate_lat_raster(cfg):
    """Generate latitude raster"""
    dem_file = cfg.spatials.filldem
    ds = RasterUtilClass.read_raster(dem_file)

#---------------------------
    # src_srs = ds.srs
    # if not src_srs.ExportToProj4():
    #     raise ValueError('The source raster %s has not coordinate, '
    #                      'which is required!' % dem_file)

    src_srs = osr_SpatialReference()
    src_srs.ImportFromEPSG(4326)  # WGS84
#-----------------------------

    dst_srs = osr_SpatialReference()
    dst_srs.ImportFromEPSG(4326)  # WGS84
    # dst_wkt = dst_srs.ExportToWkt()
    transform = osr_CoordinateTransformation(src_srs, dst_srs)

    point_ll = ogr_CreateGeometryFromWkt('POINT (%f %f)' % (ds.xMin, ds.yMin))
    point_ur = ogr_CreateGeometryFromWkt('POINT (%f %f)' % (ds.xMax, ds.yMax))

    point_ll.Transform(transform)
    point_ur.Transform(transform)

    lower_lat = point_ll.GetY()
    up_lat = point_ur.GetY()

    rows = ds.nRows
    cols = ds.nCols
    delta_lat = (up_lat - lower_lat) / float(rows)

    def cal_cell_lat(row, col):
        """calculate latitude of cell by row number"""
        return up_lat - (row + 0.5) * delta_lat

    data_lat = fromfunction(cal_cell_lat, (rows, cols))
    data_lat = where(ds.validZone, data_lat, ds.data)
    print(cfg.spatials.cell_lat)
    RasterUtilClass.write_gtiff_file(cfg.spatials.cell_lat, rows, cols, data_lat,
                                     ds.geotrans, ds.srs,
                                     ds.noDataValue, GDT_Float32)

def genetate_mask(input_file, output_file):
    # 1打开原始栅格数据集
    dataset_in = gdal.Open(input_file, gdal.GA_ReadOnly)
    band_in = dataset_in.GetRasterBand(1)
    # 读取原始数据为NumPy数组
    data_array = band_in.ReadAsArray()
    geo_transform = dataset_in.GetGeoTransform()
    projection = dataset_in.GetProjection()
    # 定义新的数据类型（例如 GDT_Int32）
    new_data_type = gdal.GDT_Float32
    # 创建一个新的数据集，具有相同的尺寸、地理变换和投影，但数据类型不同
    driver = gdal.GetDriverByName('GTiff')
    dataset_out = driver.Create(output_file,
                                dataset_in.RasterXSize,
                                dataset_in.RasterYSize,
                                1,  # Number of bands
                                new_data_type)
    # 设置新的数据集的地理变换和投影信息
    dataset_out.SetGeoTransform(geo_transform)
    dataset_out.SetProjection(projection)
    band_out = dataset_out.GetRasterBand(1)
    # 设置输出波段的NoData值（如果适用）
    no_data_value = -9999
    if no_data_value is not None:
        band_out.SetNoDataValue(no_data_value)

    mask = data_array != band_in.GetNoDataValue()
    out_array = np.zeros(data_array.shape, dtype=np.float)
    out_array[mask] = 1
    out_array[~mask] = no_data_value

    # no_data_value = 241
    # mask = data_array == no_data_value
    # data_array[mask] = band_out.GetNoDataValue()

    band_out.WriteArray(out_array)
    # 清理并关闭数据集
    dataset_in = None
    dataset_out = None


def project(input_file, output_file):
    # 定义输入文件路径和名称
    # input_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem.tif"
    # output_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem2.tif"

    projection = 'PROJCS["Beijing 1954 / 3-degree Gauss-Kruger zone 39",GEOGCS["Beijing 1954",DATUM["Beijing_1954",SPHEROID["Krassowsky 1940",6378245,298.2999999999998,AUTHORITY["EPSG","7024"]],AUTHORITY["EPSG","6214"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",117],PARAMETER["scale_factor",1],PARAMETER["false_easting",39500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]]]'
    # print(projection)
    # 创建目标投影对象（beijing）
    source_srs = osr.SpatialReference()
    source_srs.ImportFromWkt(projection)

    # 创建源空间参考对象(WGS1984)
    target_srs = osr.SpatialReference()
    target_srs.ImportFromEPSG(4326)

    # 设置重采样方法为双线性插值
    resample_alg = None

    # 重投影
    gdal.Warp(output_file, input_file,
              format='GTiff', resampleAlg=resample_alg, srcNodata=-9999,
              srcSRS=source_srs.ExportToWkt(),
              dstSRS=target_srs.ExportToWkt()
              )


def mask(input_file, consult_file, output_file):
    # 打开目标栅格数据集，它将提供所需的行列数和分辨率
    target_ds = gdal.Open(consult_file, gdal.GA_ReadOnly)
    if target_ds is None:
        print('无法打开目标栅格数据集')
        exit(1)

        # 获取目标栅格的行列数和分辨率
    target_cols = target_ds.RasterXSize
    target_rows = target_ds.RasterYSize
    target_geotransform = target_ds.GetGeoTransform()
    target_x_res = target_geotransform[1]
    target_y_res = -target_geotransform[-1]  # 注意：y_res通常是负的

    # 打开原始栅格数据集
    src_ds = gdal.Open(input_file, gdal.GA_ReadOnly)
    if src_ds is None:
        print('无法打开原始栅格数据集')
        exit(1)

    # 使用Warp函数进行裁剪，使其具有与目标栅格相同的行列数和分辨率
    gdal.Warp(output_file, src_ds,
              width=target_cols, height=target_rows,
              outputBounds=[target_geotransform[0], target_geotransform[3] - target_rows * target_y_res,
                            target_geotransform[0] + target_cols * target_x_res, target_geotransform[3]],
              resampleAlg=gdal.GRA_NearestNeighbour)

    # 清理资源
    src_ds = None
    target_ds = None

def resetNodata(file):
    dataset_in = gdal.Open(file, gdal.GA_Update)
    band_in = dataset_in.GetRasterBand(1)
    # 读取原始数据为NumPy数组
    data_array = band_in.ReadAsArray()
    band_out = dataset_in.GetRasterBand(1)
    # 设置输出波段的NoData值（如果适用）
    no_data_value = -9999
    if no_data_value is not None:
        band_out.SetNoDataValue(no_data_value)
    no_data_value = -1
    # print("band_in" + str(no_data_value))

    mask = data_array == no_data_value
    # print(mask)
    # print("band_out" + str(band_out.GetNoDataValue()))
    data_array[mask] = band_out.GetNoDataValue()
    band_out.WriteArray(data_array)
    # 清理并关闭数据集
    dataset_in = None
    dataset_out = None

def main(pre_cfg_file):
    cf = ConfigParser()
    cf.read(pre_cfg_file)
    seims_cfg = PreprocessConfig(cf)

    #1生成celllat.tif
    generate_lat_raster(seims_cfg)

    # 使用Taudem生成slope，slope_dir---------------------------------
    # workspace = seims_cfg.workspace + r'\projectFile'
    # namecfg = TauDEM.TauDEMFilesUtils(workspace)
    # demPath = workspace + r'/dem.tif'
    # consult_file = seims_cfg.workspace + r'\spatial_raster\landuse.tif'
    # slop_path = seims_cfg.workspace + r'\spatial_raster\slope.tif'
    # slopdinf_path = seims_cfg.workspace + r'\spatial_raster\slope_dinf.tif'
    # slop_path1 = seims_cfg.workspace + r'\spatial_raster\slope1.tif'
    # slopdinf_path1 = seims_cfg.workspace + r'\spatial_raster\slope_dinf1.tif'
    # slop_path2 = seims_cfg.workspace + r'\spatial_raster\slope2.tif'
    # slopdinf_path2 = seims_cfg.workspace + r'\spatial_raster\slope_dinf2.tif'
    # TauDEM.TauDEM.d8flowdir(seims_cfg.np, demPath, namecfg.d8flow, slop_path1, workspace,
    #                          seims_cfg.mpi_bin, seims_cfg.seims_bin, log_file=seims_cfg.logs.delineation,
    #                          runtime_file= None, hostfile= None)
    # TauDEM.TauDEM.dinfflowdir(seims_cfg.np, demPath, namecfg.dinf, slopdinf_path1, workspace,
    #                            seims_cfg.mpi_bin, seims_cfg.seims_bin, log_file=seims_cfg.logs.delineation,
    #                            runtime_file= None, hostfile= None)
    # # 重投影为地理坐标系
    # project(slop_path1, slop_path2)
    # project(slopdinf_path1, slopdinf_path2)
    # # 掩膜裁剪
    # mask(slop_path2,consult_file,slop_path)
    # mask(slopdinf_path2,consult_file,slopdinf_path)
    # # 重新设置nodata值
    # resetNodata(slop_path)
    # resetNodata(slopdinf_path)
    # os.remove(slop_path1)
    # os.remove(slopdinf_path1)
    # os.remove(slop_path2)
    # os.remove(slopdinf_path2)
    # 使用Taudem生成slope，slope_dir---------------------------------

    # 使用白盒生成slope
    # 写在java中，调用命令行

    # # 2设置acc和dem文件有值的地方相同：扩大acc，扩大的部分赋值为0
    # # 打开第一个栅格数据集
    # accPath = seims_cfg.workspace + r'\spatial_raster\acc.tif'
    # demPath = seims_cfg.workspace + r'\spatial_raster\dem.tif'
    # dataset1 = gdal.Open(demPath, gdal.GA_ReadOnly)
    # band1 = dataset1.GetRasterBand(1)
    # no_data_value1 = band1.GetNoDataValue()
    # # 打开第二个栅格数据集
    # dataset2 = gdal.Open(accPath, gdal.GA_Update)
    # band2 = dataset2.GetRasterBand(1)
    # no_data_value2 = band2.GetNoDataValue()
    # # 确保两个栅格具有相同的尺寸
    # if dataset1.RasterXSize != dataset2.RasterXSize or dataset1.RasterYSize != dataset2.RasterYSize:
    #     raise ValueError("The two rasters must have the same dimensions")
    # # 读取第一个栅格为NumPy数组
    # data_array1 = band1.ReadAsArray()
    # # 创建一个掩膜来标记有值区域
    # mask_big = data_array1 != no_data_value1
    #
    # # 读取第二个栅格的数组
    # data_array2 = band2.ReadAsArray()
    # mask_small = data_array2 != no_data_value2
    #
    # tempArray = np.zeros_like(data_array1)
    # tempArray[~mask_big] = no_data_value2
    #
    # tempArray[mask_small] = data_array2
    #
    # # 如果你想要将更新后的数组写回到第二个栅格，你可以这样做：
    # band2.WriteArray(tempArray)
    # band2.SetNoDataValue(no_data_value2)  # 如果需要的话，重新设置NoData值
    # band2.FlushCache()  # 确保数据被写入磁盘

    # # 2更新acc
    # # 打开第一个栅格数据集
    accPath = seims_cfg.workspace + r'/spatial_raster/acc.tif'
    # acc2Path = seims_cfg.workspace + r'/spatial_raster/acc2.tif'
    # dataset1 = gdal.Open(accPath, gdal.GA_Update)
    # band1 = dataset1.GetRasterBand(1)
    # no_data_value1 = band1.GetNoDataValue()
    # # 打开第二个栅格数据集
    # dataset2 = gdal.Open(acc2Path, gdal.GA_ReadOnly)
    # band2 = dataset2.GetRasterBand(1)
    # no_data_value2 = band2.GetNoDataValue()
    # # 确保两个栅格具有相同的尺寸
    # if dataset1.RasterXSize != dataset2.RasterXSize or dataset1.RasterYSize != dataset2.RasterYSize:
        # raise ValueError("The two rasters must have the same dimensions")
    # # 读取第一个栅格为NumPy数组
    # data_array1 = band1.ReadAsArray()
    # # 读取第二个栅格的数组
    # data_array2 = band2.ReadAsArray()
    # mask_small = data_array2 != no_data_value2
    # data_array1[mask_small] = data_array2[mask_small]

    # # 如果你想要将更新后的数组写回到第二个栅格，你可以这样做：
    # band1.WriteArray(data_array1)
    # band1.SetNoDataValue(no_data_value1)  # 如果需要的话，重新设置NoData值
    # band1.FlushCache()  # 确保数据被写入磁盘

    # dataset2 = None
    # dataset1 = None

    # os.remove(acc2Path)

    # 3生成mask.tif
    mask_path = seims_cfg.workspace + r'/spatial_raster/mask.tif'
    genetate_mask(accPath,mask_path)


if __name__ == "__main__":
    pre_cfg_file = sys.argv[1]
    main(pre_cfg_file)
