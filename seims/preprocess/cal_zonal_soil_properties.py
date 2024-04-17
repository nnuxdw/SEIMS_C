# -*- coding: utf-8 -*-
# @Author: Jiaojiao Liu
# @Date:   2023-03-31 20:33:08
# @Last Modified by:   Jiaojiao Liu
# @Last Modified time: 2024-04-15 15:54:39
import os
import re
import pandas as pd
import numpy as np
from glob import glob
from rasterstats import zonal_stats
from osgeo.ogr import Open as ogr_Open
from osgeo import gdal, ogr, osr
from osgeo import gdal, gdalconst
from osgeo.gdalconst import GDT_Int16
import rasterio
from rasterio.mask import mask
import geopandas as gpd
from rasterio.warp import reproject, Resampling
from rasterio.crs import CRS
from rasterio.warp import calculate_default_transform, reproject

from osgeo import gdal, ogr, osr
import os
import geopandas as gpd


src_crs_wkt = '''PROJCS["Albers_Conic_Equal_Area",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Albers_Conic_Equal_Area"],PARAMETER["standard_parallel_1",25],PARAMETER["standard_parallel_2",47],PARAMETER["latitude_of_center",0],PARAMETER["longitude_of_center",105],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["metre",1]]'''

polygon_crs_wkt  = """
PROJCS["Asia_North_Albers_Equal_Area_Conic",
    GEOGCS["WGS 84",
        DATUM["WGS_1984",
            SPHEROID["WGS 84",6378137,298.257223563,
                AUTHORITY["EPSG","7030"]],
            AUTHORITY["EPSG","6326"]],
        PRIMEM["Greenwich",0,
            AUTHORITY["EPSG","8901"]],
        UNIT["degree",0.0174532925199433,
            AUTHORITY["EPSG","9122"]],
        AUTHORITY["EPSG","4326"]],
    PROJECTION["Albers_Conic_Equal_Area"],
    PARAMETER["latitude_of_center",30],
    PARAMETER["longitude_of_center",95],
    PARAMETER["standard_parallel_1",15],
    PARAMETER["standard_parallel_2",65],
    PARAMETER["false_easting",0],
    PARAMETER["false_northing",0],
    UNIT["metre",1,
        AUTHORITY["EPSG","9001"]],
    AXIS["Easting",EAST],
    AXIS["Northing",NORTH],
    AUTHORITY["ESRI","102025"]]
"""

def field_compute(field_path, para_path):
    stats = zonal_stats(field_path, para_path,stats=['mean'])
    #print(stats)
    df = pd.DataFrame(stats)
    ds = ogr_Open(field_path)
    lyr = ds.GetLayer(0)
    field_id_shp = list()
    for feat in lyr:
        id = feat.GetField('FIELDID')
        area = feat.GetGeometryRef().Area()
        field_id_shp.append(id)
    df.insert(loc=0, column='FID', value=field_id_shp)
    df.sort_values(by=['FID'], ascending=True, inplace=True)
    return df

def batch_clip_raster_by_polygon2(raster_folder, polygon_file, target_folder):
    # 读取多边形数据
    polygons = gpd.read_file(polygon_file)
    polygon_crs = CRS.from_wkt(polygon_crs_wkt )  # 确保你已经有了正确的WKT字符串
    src_crs = CRS.from_wkt(src_crs_wkt)

    # 确保目标文件夹存在
    os.makedirs(target_folder, exist_ok=True)

    # 遍历 raster_folder 下的所有文件
    for dirpath, dirnames, filenames in os.walk(raster_folder):
        for raster_filename in filenames:
            if raster_filename.endswith('.tif'):
                raster_path = os.path.join(dirpath, raster_filename)
                relative_path = os.path.relpath(dirpath, raster_folder)
                target_subfolder = os.path.join(target_folder, relative_path)
                os.makedirs(target_subfolder, exist_ok=True)
                target_path = os.path.join(target_subfolder, raster_filename)

                with rasterio.open(raster_path) as src:
                    transform, width, height = calculate_default_transform(
                        src_crs, polygon_crs, src.width, src.height, *src.bounds)
                    kwargs = src.meta.copy()
                    kwargs.update({
                        'crs': polygon_crs,
                        'transform': transform,
                        'width': width,
                        'height': height
                    })

                    with rasterio.open(target_path, 'w', **kwargs) as dst:
                        for i in range(1, src.count + 1):
                            reproject(
                                source=rasterio.band(src, i),
                                destination=rasterio.band(dst, i),
                                src_transform=src.transform,
                                src_crs=src_crs,
                                dst_transform=transform,
                                dst_crs=polygon_crs,
                                resampling=Resampling.nearest
                            )

                        # 确保使用有效的几何对象
                        if not polygons.geometry.is_empty and polygons.geometry.is_valid:
                            out_image, out_transform = mask(dst, [polygons.geometry], crop=True)
                            dst.write(out_image, 1)

from osgeo import osr

def create_custom_projection():
    srs = osr.SpatialReference()
    wkt = """
    PROJCS["Albers",
    GEOGCS["GCS_WGS_1984",
        DATUM["WGS_1984",
            SPHEROID["WGS_84",6378137,298.257223563]],
        PRIMEM["Greenwich",0],
        UNIT["Degree",0.017453292519943295]],
    PROJECTION["Albers_Conic_Equal_Area"],
    PARAMETER["standard_parallel_1",29.5],
    PARAMETER["standard_parallel_2",45.5],
    PARAMETER["latitude_of_origin",23],
    PARAMETER["central_meridian",-96],
    PARAMETER["false_easting",0],
    PARAMETER["false_northing",0],
    UNIT["Meter",1]]
    """
    srs.ImportFromWkt(wkt)
    return srs


def batch_clip_raster_by_polygon_gdal(raster_folder, polygon_file, target_folder):
    # 读取多边形数据
    polygons = gpd.read_file(polygon_file)

    # 确保目标文件夹存在
    os.makedirs(target_folder, exist_ok=True)

    # 设置GDAL环境，使GDAL错误能够抛出异常
    gdal.UseExceptions()

    # 遍历 raster_folder 下的所有文件
    for dirpath, dirnames, filenames in os.walk(raster_folder):
        for raster_filename in filenames:
            if raster_filename.endswith('.tif'):
                raster_path = os.path.join(dirpath, raster_filename)
                relative_path = os.path.relpath(dirpath, raster_folder)
                target_subfolder = os.path.join(target_folder, relative_path)
                os.makedirs(target_subfolder, exist_ok=True)
                target_path = os.path.join(target_subfolder, raster_filename)
                # 如果目标文件已存在，则删除它
                if os.path.exists(target_path):
                    os.remove(target_path)
                try:
                    # 执行裁剪操作
                    # 注意：这里不再创建新的输出文件，而是直接在Warp函数中指定输出路径和裁剪参数
                    gdal.Warp(target_path, raster_path, format='GTiff', cutlineDSName=polygon_file, cropToCutline=True)

                except RuntimeError as e:
                    print("处理文件时发生错误:", raster_path)
                    print(e)

def organize_data(sol_para):
    # 初始化一个新的 DataFrame，包含你需要的列
    result_df = pd.DataFrame(columns=[
        'SEQN', 'SNAM', 'SOILLAYERS', 'SOL_Z', 'SOL_BD', 'SOL_OM',
        'SOL_CLAY', 'SOL_SILT', 'SOL_SAND', 'SOL_ROCK'
    ])

    # 假设已有 sol_para 包含所有必要数据
    for index, row in sol_para.iterrows():
        # 准备数据
        new_row = {
            'SEQN': row['FID'],
            'SNAM': 'NONE',
            'SOILLAYERS': 6,
            'SOL_Z': '5-15-30-60-100-200',
            'SOL_BD': '-'.join([str(row[f'bd_{d}']/1000) for d in ['05', '515', '1530', '3060', '60100','100200']]),
            # 原始数据soc单位是g/kg，要转为kg/kg的om
            'SOL_OM': '-'.join([str(row[f'soc_{d}']/0.58/1000) for d in ['05', '515', '1530', '3060', '60100','100200']]),
            'SOL_CLAY': '-'.join([str(row[f'btcly_{d}_ratio']) for d in ['05', '515', '1530', '3060', '60100','100200']]),
            'SOL_SILT': '-'.join([str(row[f'btslt_{d}_ratio']) for d in ['05', '515', '1530', '3060', '60100','100200']]),
            'SOL_SAND': '-'.join([str(row[f'btsnd_{d}_ratio']) for d in ['05', '515', '1530', '3060', '60100','100200']]),
            'SOL_ROCK': '-'.join([str(row[f'cf_{d}']) for d in ['05', '515', '1530', '3060', '60100','100200']])
        }
        # 添加到 DataFrame
        result_df = result_df.append(new_row, ignore_index=True)

    return result_df

def zonal_statistic_mean_by_hru(raster_file,hru_file,tar_csv):
    param_file_list = glob(raster_file)
    sol_para = pd.DataFrame()
    # depth_dict = {'05','515','1530','3060','60100','100200'}
    depth_dict = {}
    for param_file in param_file_list:
        # print(param_file)
        # output_file = os.path.basename(param_file).split('\\')[0].split('.')[0]
        # df = field_compute(hru_file,param_file)
        # df = df.drop(df[(df['FID']==-9999)].index)
        # df.rename(columns={'mean': output_file}, inplace=True)
        # if sol_para.empty :
        #     sol_para = pd.DataFrame(df)
        #
        # else:
        #     sol_para = pd.concat([sol_para,df],axis=1,join='inner')

        basename = os.path.basename(param_file)
        output_file = basename.split('.')[0]  # 去掉文件扩展名
        # 使用正则表达式提取prefix和depth
        match = re.match(r"([a-zA-Z]+)(\d+)_", output_file)
        if match:
            prefix = match.group(1)  # 英文部分
            depth = match.group(2)  # 数字部分

            # 计算每个面的平均值
            df = field_compute(hru_file, param_file)
            df = df.drop(df[df['FID'] == -9999].index)  # 删除无效数据
            mean_column = f'{prefix}_{depth}'  # 创建列名称，如 bd_05
            df.rename(columns={'mean': mean_column}, inplace=True)

            # 存储各深度的数据以备后续计算比例
            if prefix in ['btcly', 'btslt', 'btsnd']:
                if depth not in depth_dict:
                    depth_dict[depth] = {}
                depth_dict[depth][prefix] = df[mean_column]

            # 对于 soc 开头的文件，计算值后除以 0.58
            if prefix == 'soc':
                df[mean_column] /= 0.58

            # 将数据合并到总的 DataFrame
            if sol_para.empty:
                sol_para = df
            else:
                sol_para = sol_para.merge(df, on='FID', how='inner')

    # 计算 btcly, btslt, btsnd 的比例
    for depth, layers in depth_dict.items():
        total = layers['btcly'] + layers['btslt'] + layers['btsnd']
        for prefix in ['btcly', 'btslt', 'btsnd']:
            sol_para[f'{prefix}_{depth}_ratio'] = layers[prefix] * 100 / total
    sol_para=sol_para.T.drop_duplicates().T

    result_df = organize_data(sol_para)

    result_df.to_csv(tar_csv,index=0, header=True)



def workflow():
    hband_shp_file = r'G:\program\gannan\data\gongba\gen_mesh\2_hand_modify\hand_modify.shp'
    param_file_list = glob(r'G:\program\gannan\data\gongba\soil\soil_type_albers\*\*.tif')
    tar_csv = r'G:\program\gannan\data\gongba\soil\soil_type_albers\sol_para_extract.csv'

    sol_para = pd.DataFrame()
    for param_file in param_file_list:
        print(param_file)
        output_file = os.path.basename(param_file).split('\\')[0].split('.')[0]
        df = field_compute(hband_shp_file,param_file)
        df = df.drop(df[(df['FID']==-9999)].index)
        df.rename(columns={'mean': output_file}, inplace=True)
        if sol_para.empty :
            sol_para = pd.DataFrame(df)

        else:
            sol_para = pd.concat([sol_para,df],axis=1,join='inner')
    sol_para=sol_para.T.drop_duplicates().T
    sol_para.to_csv(tar_csv,index=0, header=True)



if __name__=='__main__':

    raster_folder = r'G:\data\土壤所90m土壤数据\soil_type'
    tar_clip_folder = r'G:\program\gannan\data\gongba\soil\soil_type'
    polygon_file = r'G:\program\gannan\data\gongba\extent\gongba_contour_proj_4soil.shp'
    hru_file = r'G:\program\gannan\data\gongba\gen_mesh\2_hand_modify\hand_modify.shp'
    # 裁剪
    # batch_clip_raster_by_polygon_gdal(raster_folder, polygon_file, tar_clip_folder)
    # 投影
    # ...
    # 统计每个tif中，每个HRU的bd、om、clay、silt、sand、rock
    projected_raster_files = r'G:\program\gannan\data\gongba\soil\soil_type_albers\*\*.tif'
    tar_csv_file = r'G:\program\gannan\data\gongba\soil\soil_type_albers\sol_para_extract.csv'
    zonal_statistic_mean_by_hru(projected_raster_files, hru_file, tar_csv_file)

    # workflow()
