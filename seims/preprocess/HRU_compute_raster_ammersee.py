'''
Author: binjie
Date: 2021-12-30 09:54:38
LastEditTime: 2022-01-19 20:49:17
LastEditors: Please set LastEditors
Description:
'''
import subprocess

from osgeo.gdalconst import GDT_Int16
from rasterstats import zonal_stats
from pygeoc.raster import RasterUtilClass
import os
import sys
import gdal
import csv
import pandas as pd
from glob import glob
import re

from osgeo.ogr import Open as ogr_Open
from osgeo import gdal_array, osr, ogr
from osgeo.osr import SpatialReference as osr_SpatialReference
from osgeo.osr import CoordinateTransformation as osr_CoordinateTransformation
from osgeo.ogr import CreateGeometryFromWkt as ogr_CreateGeometryFromWkt
from osgeo.ogr import GetDriverByName as ogr_GetDriverByName
from pygeoc.utils import FileClass, UtilClass
from osgeo.ogr import FieldDefn as ogr_FieldDefn
from osgeo.ogr import OFTInteger
import numpy as np
import db_import_field_arrays
from preprocess.db_import_interpolation_weights_field import ImportWeightData_field

from multiprocessing import Pool


def field_compute(field_path, para_path,compute_type):
    stats = zonal_stats(field_path, para_path,stats=[compute_type])
    df = pd.DataFrame(stats)
    # get sequence of field id in the shp
    # field_shp_file = field_file + os.sep + 'fields_raster.shp'
    ds = ogr_Open(field_path)
    lyr = ds.GetLayer(0)
    field_id_shp = list()
    for feat in lyr:
        id = feat.GetField('FIELDID')
        field_id_shp.append(id)
    df.insert(loc=0, column='FID', value=field_id_shp)
    df.sort_values(by=['FID'], ascending=True, inplace=True)
    return df

def raster2shp(rasterfile, vectorshp, layername=None, fieldname=None,
                   band_num=1, mask='default'):
        """Convert raster to ESRI shapefile"""
        FileClass.remove_files(vectorshp)
        FileClass.check_file_exists(rasterfile)
        # this allows GDAL to throw Python Exceptions
        gdal.UseExceptions()
        src_ds = gdal.Open(rasterfile)
        if src_ds is None:
            print('Unable to open %s' % rasterfile)
            sys.exit(1)
        try:
            srcband = src_ds.GetRasterBand(band_num)
        except RuntimeError as e:
            # for example, try GetRasterBand(10)
            print('Band ( %i ) not found, %s' % (band_num, e))
            sys.exit(1)
        
        if mask == 'default':
            maskband = srcband.GetMaskBand()
        elif mask is None or mask.upper() == 'NONE':
            maskband = None
        else:
            mask_ds = gdal.Open(mask)
            maskband = mask_ds.GetRasterBand(1)
        #  create output datasource
        if layername is None:
            layername = FileClass.get_core_name_without_suffix(rasterfile)
        drv = ogr_GetDriverByName(str('ESRI Shapefile'))
        dst_ds = drv.CreateDataSource(vectorshp)
        srs = None
        if src_ds.GetProjection() != '':
            srs = osr_SpatialReference()
            srs.ImportFromWkt(src_ds.GetProjection())
        dst_layer = dst_ds.CreateLayer(str(layername), srs=srs)
        if fieldname is None:
            fieldname = layername.upper()
        fd = ogr_FieldDefn(str(fieldname), OFTInteger)
        dst_layer.CreateField(fd)
        dst_field = 0
        result = gdal.Polygonize(srcband, maskband, dst_layer, dst_field,
                                 ['8CONNECTED=8'], callback=None)
        return result

def IUH_1Darray(iuh_csv,field_num):
    # write 1D IUH data, length, 111,111,...
    f = open(iuh_csv, 'w')
    f.write('FID,OL_IUH\n')
    f.write('0,%d\n' %field_num)
    for i in range(1, field_num*3+1):
        con = '%d,1\n' %(i)
        f.write(con)
    return

def field_subbasin_array(field_txt, subbasin_csv_path):
    txt_data = pd.read_csv(field_txt, delimiter="    ",engine='python')
    txt_data.to_csv(subbasin_csv_path,index=0,columns=['FID', 'subbasin'])

def field_center(field_shp_file,field_center_file):
    ds = ogr_Open(field_shp_file)
    lyr = ds.GetLayer(0)
    dist_field_center = {}
    id_arr = []
    for feat in lyr:
        id = feat.GetField('FIELDID')
        if id not in id_arr:
            id_arr.append(id)
            geometry = feat.GetGeometryRef().Centroid()
            pt = geometry.GetPoint()
            dist_field_center.setdefault(id,[]).append(pt[0])
            dist_field_center.setdefault(id,[]).append(pt[1])
    df_field_center = pd.DataFrame(dist_field_center)
    df_field_center.to_csv(field_center_file,index=0, header=True)


def test(i, param_list_mean, ctype,field_shp_file, param_lists_mean_file,csv_file):
    print(i)
    print(param_list_mean)
    print(ctype)
    print(field_shp_file)
    print(param_lists_mean_file)
    po = Pool()
    df1 = pd.DataFrame(columns=['FID'])
    # for param in param_list_mean:
    #     print(param)
    #     param_file = raster_para_files + os.sep + param + '.tif'
    #     result = po.apply_async(field_compute, (field_shp_file,param_file,ctype))
    #     df_param = result.get()
    #     df1[param] = df_param[ctype]

    results = [po.apply_async(field_compute, (field_shp_file,raster_para_files + os.sep + param + '.tif',ctype)) for param in param_list_mean]

    for index, result in enumerate(results):
        df_param = result.get()
        param = param_list_mean[index]
        df1[param] = df_param[ctype]
    global field_num
    field_num = len(df1)
    po.close()
    # 等待po中所有子进程执行完成，必须放在close语句之后
    po.join()
    df1['FID'] = df_param['FID']
    df1.to_csv(csv_file + os.sep + param_lists_mean_file[i] + '.csv', index=0)

def test2(i,param_list_mean, ctype,field_shp_file, param_lists_majority_file,csv_file):
    po = Pool()
    df1 = pd.DataFrame(columns=['FID'])

    results = [po.apply_async(field_compute, (field_shp_file, raster_para_files + os.sep + param + '.tif', ctype)) for
               param in param_list_mean]

    for index, result in enumerate(results):
        df_param = result.get()
        param = param_list_mean[index]
        df1[param] = df_param[ctype]

    po.close()
    # 等待po中所有子进程执行完成，必须放在close语句之后
    po.join()
    df1['FID'] = df_param['FID']
    df1.to_csv(csv_file + os.sep + param_lists_majority_file[i] + '.csv', index=0)

def field_param_csv(csv_file,field_shp_file, raster_para_files, lakesubareaidTXT, soilgrids0idTXT):
    landuse_lookup = ['landuse','CN2A','CN2B','CN2C','CN2D','ROOTDEPTH','MANNING','INTERC_MAX',
                    'INTERC_MIN','SHC','SOIL_T10','USLE_C','PET_FR','PRC_ST1','PRC_ST2','PRC_ST3',
                    'PRC_ST4','PRC_ST5','PRC_ST6','PRC_ST7','PRC_ST8','PRC_ST9','PRC_ST10','PRC_ST11',
                    'PRC_ST12','SC_ST1','SC_ST2','SC_ST3','SC_ST4','SC_ST5','SC_ST6','SC_ST7','SC_ST8',
                    'SC_ST9','SC_ST10','SC_ST11','SC_ST12','DSC_ST1','DSC_ST2','DSC_ST3','DSC_ST4',
                    'DSC_ST5','DSC_ST6','DSC_ST7','DSC_ST8','DSC_ST9','DSC_ST10','DSC_ST11','DSC_ST12']#众数
    soil_typedata = ['SOILLAYERS', 'HYDRO_GROUP','SOIL_TEXTURE'] #众数
    # 根据土壤层数不同，获取各层的土壤参数
    soil = ['SOL_AVPOR','DET_SILT','WFSH','ESCO','SOL_AVBD','DET_SAND','SOL_SUMWP','SOL_ZMX',
            'SOL_CRK','ANION_EXCL','SOL_SUMUL','DET_CLAY','SOL_SUMAWC','SOL_ALB','DET_SMAGG','DET_LGAGG']       
    soil_paras = ['SOL_AWC_','SILT_','WILTINGPOINT_','SOL_ORGN_','SOL_SOLP_','SAND_','ROCK_','CONDUCTIVITY_','AWC_',
                'SOL_N_','SOL_NO3_','POREINDEX_','DENSITY_','CLAY_','OM_','USLE_K_','SOL_WPMM_','SOL_ORGP_','SOILTHICK_',
                'SOL_NH4_','SOL_HK_','SOL_CBN_','FIELDCAP_','VWT_','POROSITY_','SOL_UL_','SOILDEPTH_','CRDEP_']
    for soil_para in soil_paras:
        raster_para_file = raster_para_files + os.sep + soil_para + '*.tif'
        raster_para_filess = glob(raster_para_file)
        # xjs++:排序
        raster_para_files_sorted = sorted(raster_para_filess,
                                          key=lambda x: int(re.search(soil_para + r'(\d+)\.tif', x).group(1)))
        for raster_para_file in raster_para_files_sorted:
            raster_para_name = os.path.basename(raster_para_file).split('.')[0]
            soil.append(raster_para_name)
    ##
    crop = ['IDC', 'BIO_E', 'HVSTI', 'BLAI', 'FRGRW1', 'LAIMX1', 'FRGRW2',
            'LAIMX2', 'DLAI', 'CHTMX', 'RDMX', 'T_OPT', 'T_BASE', 'CNYLD',
            'CPYLD', 'BN1', 'BN2', 'BN3', 'BP1', 'BP2', 'BP3', 'WSYF',
            'USLE_C', 'GSI', 'VPDFR', 'FRGMAX', 'WAVP', 'CO2HI', 'BIOEHI',
            'RSDCO_PL', 'OV_N', 'CN2A', 'CN2B', 'CN2C', 'CN2D', 'FERTFIELD',
            'ALAI_MIN', 'BIO_LEAF', 'MAT_YRS', 'BMX_TREES', 'EXT_COEF', 'BM_DIEOFF','soiltype']#众数
    land_init = ['CURYR_INIT','BIO_INIT','RSDIN','EPCO','LAI_INIT','CHT','DORMI','PHU_PLT']
    land_init_type = ['LANDCOVER','IGRO']#众数
    other_param = ['CN2','dayLenMin','dem','depression','dormhr','USLE_P','slope','slope_dinf','moist_in','runoff_co','acc','dist2Stream']
    
    # landuse_lookup = ['landuse'] #众数
    # soil_typedata = ['SOILLAYERS', 'HYDRO_GROUP','SOIL_TEXTURE'] #众数
    # # 根据土壤层数不同，获取各层的土壤参数
    # soil = ['SOL_AVPOR']       
    # soil_paras = ['SOL_AWC_']
    # for soil_para in soil_paras: 
    #     raster_para_file = raster_para_files + os.sep + soil_para + '*.tif'
    #     for raster_para_file in glob(raster_para_file):
    #         raster_para_name = os.path.basename(raster_para_file).split('.')[0]
    #         soil.append(raster_para_name)
    # ##
    # crop = ['IDC']
    # land_init = ['CURYR_INIT']
    # land_init_type = ['LANDCOVER','IGRO']#众数
    # other_param = ['CN2']

    param_lists_mean = [soil,other_param,land_init]
    param_lists_mean_file = ['soil','other_param','land_init']
    param_lists_majority = [landuse_lookup,soil_typedata,crop,land_init_type]
    param_lists_majority_file = ['landuse_lookup','soil_typedata','crop','land_init_type']


    param_lists_mean = [soil,other_param,land_init]
    param_lists_mean_file = ['soil','other_param','land_init']
    param_lists_majority = [landuse_lookup,soil_typedata,crop,land_init_type]
    param_lists_majority_file = ['landuse_lookup','soil_typedata','crop','land_init_type']

    # add by xjs
    param_lists_max = [['dem']]
    param_lists_max_file = ['dem_max']
    param_lists_min = [['dem']]
    param_lists_min_file = ['dem_min']

    if  not os.path.exists(csv_file):#如果路径不存在
        os.makedirs(csv_file)





    i = -1

    # 定义进程池
    # po = Pool()
    for param_list_mean in param_lists_mean :
        # print("----" + str(param_list_mean))
        i = i + 1
        # po.apply_async(test, (i, param_list_mean, 'mean',field_shp_file, param_lists_mean_file,field_num))
        test(i, param_list_mean, 'mean', field_shp_file, param_lists_mean_file,csv_file)

        # for param in param_list_mean:
        #     param_file = raster_para_files + os.sep + param + '.tif'
        #
        #     # df_param = field_compute(field_shp_file,param_file,'mean')
        #     # df[param] = df_param['mean']
        #     df_param = po.apply_async(field_compute, (field_shp_file, param_file, 'mean')).get()
        #     df[param] = df_param['mean']
        # df['FID'] = df_param['FID']
        # field_num = len(df)
        # df.to_csv(csv_file + os.sep + param_lists_mean_file[i] + '.csv',index=0)

    j = -1

    for param_list_majority in param_lists_majority :
        # print("----" + str(param_list_majority))
        j = j + 1
        # po.apply_async(test2, (j, param_list_majority, 'majority',field_shp_file, param_lists_majority_file))

        test2(j, param_list_majority, 'majority', field_shp_file, param_lists_majority_file, csv_file)

        # for param in param_list_majority:
        #     param_file = raster_para_files + os.sep + param + '.tif'
        #
        #     # df_param = field_compute(field_shp_file,param_file,'majority')
        #     # df[param] = df_param['majority']
        #     df_param = po1.apply_async(field_compute, (field_shp_file, param_file, 'majority')).get()
        #     df[param] = df_param['majority']
        # df['FID'] = df_param['FID']
        # df.to_csv(csv_file + os.sep + param_lists_majority_file[j] + '.csv',index=0)

    # print("--------------------------------")
    # add by xjs
    # # 修正landuse_lookup.csv中subarea为湖泊的值为18
    # print('landuse')
    with open(csv_file + os.sep + 'landuse_lookup.csv', mode='r', newline='', encoding='utf-8') as file:
        reader = csv.DictReader(file)
        data = [row for row in reader]
    # print(data[0]['FID'])
    # print(data[0]['landuse'])
    # print(data[1]['FID'])
    # print(data[2]['FID'])
    # print(data[3]['FID'])

    lakesubareaidList = []

    # 读取湖泊subarea ID
    if os.path.exists(lakesubareaidTXT):
        with open(lakesubareaidTXT, 'r', encoding='utf-8') as file:
            lakesubareaidString = file.readline()
            lakesubareaidList = lakesubareaidString.split(' ')[:-1]

    # 读取soilgrids为0的subarea ID
    if os.path.exists(soilgrids0idTXT):
        with open(soilgrids0idTXT, 'r', encoding='utf-8') as file:
            soilgrids0idString = file.readline()
            soilgrids0idList = soilgrids0idString.split(' ')[:-1]

    # print(lakesubareaidList)
    # print(soilgrids0idList)
    for index in lakesubareaidList:
        # print(index)
        # df[param][index] = 18
        data[int(index)]['landuse'] = str(18)
        # df.loc[str(index), param] = str(18)
        # print(df.loc[str(index), param])
    for index in soilgrids0idList:
        # print(index)
        # df[param][index] = 18
        data[int(index)]['landuse'] = str(18)
        # df.loc[str(index), param] = str(18)
    fieldnames = data[0].keys()
    with open(csv_file + os.sep + 'landuse_lookup.csv', mode='w', newline='', encoding='utf-8') as file:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(data)

    # add by xjs
    i = 0
    for param_list_max in param_lists_max :
        df=pd.DataFrame(columns=['FID'])
        for param in param_list_max:
            param_file = raster_para_files + os.sep + param + '.tif'
            df_param = field_compute(field_shp_file,param_file,'max')
            df[param] = df_param['max']
        df['FID'] = df_param['FID']
        df.to_csv(csv_file + os.sep + param_lists_max_file[i] + '.csv',index=0)
        i = i + 1
    i = 0
    for param_list_min in param_lists_min :
        df=pd.DataFrame(columns=['FID'])
        for param in param_list_min:
            param_file = raster_para_files + os.sep + param + '.tif'
            df_param = field_compute(field_shp_file,param_file,'min')
            df[param] = df_param['min']
        df['FID'] = df_param['FID']
        df.to_csv(csv_file + os.sep + param_lists_min_file[i] + '.csv',index=0)
        i = i + 1

    return field_num


def field_param_csv_test(csv_file, field_shp_file, raster_para_files, lakesubareaidTXT, soilgrids0idTXT):
    landuse_lookup = ['landuse']  # 众数

    param_lists_majority = [landuse_lookup]
    param_lists_majority_file = ['landuse_lookup']


    if not os.path.exists(csv_file):  # 如果路径不存在
        os.makedirs(csv_file)

    i = 0
    for param_list_majority in param_lists_majority:
        df = pd.DataFrame(columns=['FID'])
        for param in param_list_majority:

            param_file = raster_para_files + os.sep + param + '.tif'
            df_param = field_compute(field_shp_file, param_file, 'majority')
            df[param] = df_param['majority']



        df['FID'] = df_param['FID']
        df.to_csv(csv_file + os.sep + param_lists_majority_file[i] + '.csv', index=0)
        i = i + 1

        # add by xjs
        with open(csv_file + os.sep  + 'landuse_lookup.csv', mode='r', newline='', encoding='utf-8') as file:
            reader = csv.DictReader(file)
            data = [row for row in reader]
        # 若为landusea，则修正df[param]中subarea为湖泊的值为18
        if param == 'landuse':
            print('landuse')
            print(data[0]['FID'])
            print(data[0]['landuse'])
            print(data[1]['FID'])
            print(data[2]['FID'])
            print(data[3]['FID'])

            # 读取湖泊subarea ID
            if os.path.exists(lakesubareaidTXT):
                with open(lakesubareaidTXT, 'r', encoding='utf-8') as file:
                    lakesubareaidString = file.readline()
                    lakesubareaidList = lakesubareaidString.split(' ')[:-1]

            # 读取soilgrids为0的subarea ID
            if os.path.exists(soilgrids0idTXT):
                with open(soilgrids0idTXT, 'r', encoding='utf-8') as file:
                    soilgrids0idString = file.readline()
                    soilgrids0idList = soilgrids0idString.split(' ')[:-1]

            print(lakesubareaidList)
            print(soilgrids0idList)
            for index in lakesubareaidList:
                print(index)
                # df[param][index] = 18
                data[int(index)]['landuse'] = str(18)
                # df.loc[str(index), param] = str(18)
                # print(df.loc[str(index), param])
            for index in soilgrids0idList:
                print(index)
                # df[param][index] = 18
                data[int(index)]['landuse'] = str(18)
                # df.loc[str(index), param] = str(18)
            fieldnames = data[0].keys()
            with open(csv_file + os.sep  + 'landuse_lookup.csv', mode='w', newline='', encoding='utf-8') as file:
                writer = csv.DictWriter(file, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(data)

    return None


def get_coord_field_point(field_center_file_84, field_center, csv_file):
    # ds = RasterUtilClass.read_raster(celllat_tif)
    # src_srs = ds.srs
    # if not src_srs.ExportToProj4():
    #     raise ValueError('The source raster %s has not coordinate, '
    #                         'which is required!' % celllat_tif)
    #
    # dst_srs = osr_SpatialReference()
    # dst_srs.ImportFromEPSG(4326)  # WGS84
    # # dst_wkt = dst_srs.ExportToWkt()
    # transform = osr_CoordinateTransformation(src_srs, dst_srs)

    field_center_list = pd.read_csv(field_center)
    row, fiald_num = field_center_list.shape

    field_center_list_84 = pd.read_csv(field_center_file_84)

    fcsv = open(csv_file, 'w')
    fcsv.write('FID,prjX,prjY,celllong,celllat\n')
    for fiald_id in range(fiald_num):
        # 原来：
        # field_x, field_y = field_center_list.loc[:,str(fiald_id)]
        # point = ogr_CreateGeometryFromWkt('POINT (%f %f)' % (field_x, field_y))
        # point.Transform(transform)

        # 现在
        field_x, field_y = field_center_list_84.loc[:, str(fiald_id)]
        field_lon, field_lat = field_center_list.loc[:, str(fiald_id)]

        con = '%s,%s,%s,%s,%s\n' % (str(fiald_id), str(field_x), str(field_y), str(field_lon), str(field_lat))
        fcsv.write(con)
    fcsv.close()

def cell_area(field_shp_file, csv_file):
    ds = ogr_Open(field_shp_file)
    lyr = ds.GetLayer(0)
    field_id = list()
    field_area = list()
    df = pd.DataFrame()
    for feat in lyr:
        id = feat.GetField('FIELDID')
        field_id.append(id)
        area = feat.GetGeometryRef().Area()
        field_area.append(area)
        # print('ID: ',id, 'AREA: ', area)
    df.insert(loc=0, column='FID', value=field_id)
    df.insert(loc=1,column='CELLAREA', value=field_area)
    df.sort_values(by=['FID'], ascending=True, inplace=True)
    df.to_csv(csv_file ,index=0)

def stream_link_csv(field_txt, csv_file):
    txt_data = pd.read_csv(field_txt, delimiter="    ", skipinitialspace=True)
    df = pd.DataFrame()
    field_id = list()
    stream_link = list()
    
    for id in range(len(txt_data.loc[:,'FID'])):
        field_id.append(id)
        if txt_data.loc[id, 'downstreamFID'] < 0:
            stream_link.append(txt_data.loc[id,'subbasin'])
        else:
            stream_link.append(-1)
    # print(stream_link)
    df.insert(loc=0, column='FID', value=field_id)
    df.insert(loc=1, column='stream_link', value=stream_link)
    df.to_csv(csv_file ,index=0)

def flowin_index(field_txt,csv_file):
    txt_data = pd.read_csv(field_txt, delimiter="    ", skipinitialspace=True)
    txt_data.loc[: ,'downstreamFID'] == 0
    upstram_id = txt_data.loc[txt_data['downstreamFID']==0]
    df = pd.DataFrame()
    # UpstreamID = list()
    flowin_index = list()
    flowin_index.append(len(txt_data.loc[:,'FID']))
    # print(list(upstram_id.loc[:, 'FID']))
    for id in range(len(txt_data.loc[:,'FID'])):
        flowin = list()
        upstram_id = txt_data.loc[txt_data['downstreamFID'] == id]
        flowin.append(len(upstram_id))
        flowin.extend(list(upstram_id.loc[:, 'FID']))
        flowin_index.append(flowin)
    df.insert(loc=0,column='flowin_index_d8', value=flowin_index)
    df.to_csv(csv_file ,index=0)

def Reference(input,ouput):
    from osgeo import ogr, osr
    import os
    os.environ['SHAPE_ENCODING'] = "utf-8"

    src_ds = ogr.Open(input)
    src_layer = src_ds.GetLayer(0)
    src_srs = src_layer.GetSpatialRef()  # 输入数据投影

    # 输出数据投影定义，参考资料：http://spatialreference.org/ref/sr-org/8657
    srs_def = """+proj=aea +lat_0=30 +lon_0=95 +lat_1=15 +lat_2=65 +x_0=0 +y_0=0 +datum=WGS84 +units=m +no_defs """
    dst_srs = osr.SpatialReference()
    dst_srs.ImportFromProj4(srs_def)

    # 创建转换对象
    ctx = osr.CoordinateTransformation(src_srs, dst_srs)

    # 创建输出文件
    driver = ogr.GetDriverByName('ESRI Shapefile')
    dst_ds = driver.CreateDataSource(ouput)
    dst_layer = dst_ds.CreateLayer('FIELDID', dst_srs, ogr.wkbPolygon)

    # 给输出文件图层添加属性定义
    layer_def = src_layer.GetLayerDefn()
    for i in range(layer_def.GetFieldCount()):
        field_def = layer_def.GetFieldDefn(i)
        dst_layer.CreateField(field_def)

    # 循环遍历源Shapefile中的几何体添加到目标文件中
    src_feature = src_layer.GetNextFeature()
    while src_feature:
        geometry = src_feature.GetGeometryRef()
        geometry.Transform(ctx)
        dst_feature = ogr.Feature(layer_def)
        dst_feature.SetGeometry(geometry)  # 设置Geometry
        # 依次设置属性值
        for i in range(layer_def.GetFieldCount()):
            field_def = layer_def.GetFieldDefn(i)
            field_name = field_def.GetName()
            dst_feature.SetField(field_name, src_feature.GetField(field_name))
        dst_layer.CreateFeature(dst_feature)
        dst_feature = None
        src_feature = None
        src_feature = src_layer.GetNextFeature()
    dst_ds.FlushCache()

    del src_ds
    del dst_ds

def flowout_index(field_shp_file,stream_file,field_raster_file,field_file,csv_file1,csv_file2):
    df = pd.DataFrame()
    txt_data = pd.read_csv(field_file, delimiter="    ", skipinitialspace=True)
    df.insert(loc=0, column='FID', value=list(txt_data.loc[:,'FID']))
    txt_data.loc[txt_data['downstreamFID']==-9999] = -1
    df.insert(loc=1, column='flowout_index_d8', value=list(txt_data.loc[:,'downstreamFID']))
    df.to_csv(csv_file1 ,index=0)

    ds = ogr_Open(field_shp_file)
    lyr = ds.GetLayer(0)
    ftdic = {}
    length = list()
    for feat in lyr:
        id = feat.GetField('FIELDID')
        geom = feat.GetGeometryRef() 
        ftdic[id] = [feat,id]
    ds2 = ogr_Open(stream_file)
    lyr2 = ds2.GetLayer(0)
    ftdic2 = {}
    length = list()
    for feat in lyr2:
        id = feat.GetField('FIELDID')
        geom = feat.GetGeometryRef() 
        ftdic2[id] = [feat,id]
    dataset = gdal.Open(field_raster_file)
    adfGeoTransform = dataset.GetGeoTransform()

    # print(txt_data.loc[:,'FID'])
    for id in range(len(txt_data.loc[:,'FID'])):
        poly1 = (ftdic[id][0].GetGeometryRef() )
        if (txt_data.loc[id,'downstreamFID'] > -1):
            #不临近河流的计算共边长
            poly2 = (ftdic[txt_data.loc[id,'downstreamFID']][0].GetGeometryRef() )
            boundary = poly1.Intersection(poly2)
            if(boundary == None):
                poly1 = poly1.Union(poly1)
                boundary = poly1.Intersection(poly2)

            if (boundary == None):
                length.append(poly1.Length() / 3)

            if(boundary.Length()>0): 
                length.append(round(boundary.Length(),2))#公共边界长度
            else: 
                length.append(adfGeoTransform[1]) #取分辨率
        else:
            #临近河流的计算河流的共边
            # print(id)
            # print(txt_data.loc[id, 'subbasin'])
            length.append(round((pow(ftdic[id][0].GetGeometryRef().GetArea(), 0.5)), 2))
            # poly3 = (ftdic2[txt_data.loc[id,'subbasin']][0].GetGeometryRef() )
            # boundary = poly1.Intersection(poly3)
            # if(boundary.Length()>0):
            #     length.append(round(boundary.Length(),2))#公共边界长度
            # else:
            #     length.append(adfGeoTransform[1])
    #print(length)
    df = pd.DataFrame()
    txt_data = pd.read_csv(field_txt, delimiter="    ", skipinitialspace=True)
    df.insert(loc=0, column='FID', value=list(txt_data.loc[:,'FID']))
    df.insert(loc=1, column='flowout_length', value=list(length))
    df.to_csv(csv_file2 ,index=0)

def routing_layer( massif_downstream):
    
    massif_num = len(massif_downstream)
    result = np.zeros(massif_num, dtype=np.int32)
    block = np.zeros(massif_num, dtype=np.int32)
    block_num = 0
    insert_idx = 0


    # 先计算每个地块的直接上游地块数量
    upstream_num = np.zeros(massif_num, dtype=np.int32)
    for downstream_massif_id in massif_downstream:
        if downstream_massif_id > 0:
            upstream_num[downstream_massif_id] += 1
    
    queue = []
    # 寻找没有上游的地块：    
    for i in range(massif_num):
        if upstream_num[i] == 0:
            queue.append(i)

    queue_num = len(queue)
    while queue_num > 0:
        for i in range(queue_num):
            single_massif_id = queue.pop(0)
            result[insert_idx] = single_massif_id
            insert_idx += 1
            downstream_massif_id = massif_downstream[single_massif_id]
            if downstream_massif_id > 0:
                upstream_num[downstream_massif_id] -= 1
                if upstream_num[downstream_massif_id] == 0:
                    queue.append(downstream_massif_id)

        block[block_num] = insert_idx
        block_num += 1
        queue_num = len(queue)


    return result, block, block_num

def routing_layer_csv(field_txt,csv_file):
    downstream_txt = np.loadtxt(field_txt,delimiter='    ',skiprows=1,usecols=[1],dtype=np.int32 )
    # print(downstream_txt)
    result, block, block_num = routing_layer(downstream_txt)
    df = pd.DataFrame()
    layer_data = list()
    # df.insert(loc=0,column='routing_layers_down_up', value=block_num)
    layer_data.append([block_num])
    for i in range(block_num):
        if i==0 :
            num = block[0]-0
            data = result[0:block[0]]
        else:
            data = result[block[i-1]:block[i]]
            num = block[i]-block[i-1]
        layer_data.append(num)
        layer_data.extend(list(data))
    df.insert(loc=0,column='routing_layers_down_up', value=layer_data)
    df.to_csv(csv_file ,index=0)

def datatype(datatype_csv):
    df = pd.DataFrame()
    layer_data = list()
    layer_data = [0]
    df.insert(loc=0,column='datatypes', value=layer_data)
    df.to_csv(datatype_csv ,index=0)

def project(input_file, output_file):
    # 定义输入文件路径和名称
    # input_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem.tif"
    # output_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem2.tif"

    #北京54
    # projection = 'PROJCS["Beijing 1954 / 3-degree Gauss-Kruger zone 39",GEOGCS["Beijing 1954",DATUM["Beijing_1954",SPHEROID["Krassowsky 1940",6378245,298.2999999999998,AUTHORITY["EPSG","7024"]],AUTHORITY["EPSG","6214"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",117],PARAMETER["scale_factor",1],PARAMETER["false_easting",39500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]]]'
    # 北美投影坐标系
    projection = 'PROJCS["NAD_1983_UTM_Zone_17N",GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.257222101004,AUTHORITY["EPSG","7019"]],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4269"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",-81],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH],AUTHORITY["EPSG","26917"]]'

    # print(projection)
    # 创建目标投影对象（beijing）
    target_srs = osr.SpatialReference()
    # target_srs.ImportFromWkt(projection)
    target_srs.ImportFromEPSG(3857)

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

# Solve shapefile self-intersection
def s_shp_si(shpFile,out_shp):
    # 打开数据
    ds = ogr.Open(shpFile, 0)
    if ds is None:
        print("打开文件 %s 失败！" % shpFile)
        return
    print("打开文件%s成功！" % shpFile)
    # 获取该数据源中的图层个数，一般shp数据图层只有一个，如果是mdb、dxf等图层就会有多个
    m_layer_count = ds.GetLayerCount()
    m_layer = ds.GetLayerByIndex(0)
    if m_layer is None:
        print("获取第%d个图层失败！\n", 0)
        return

    # 创建输出文件
    driver = ogr.GetDriverByName('ESRI Shapefile')
    if os.path.exists(out_shp):
        driver.DeleteDataSource(out_shp)
    outds = driver.CreateDataSource(out_shp)
    outlayer = outds.CreateLayer(out_shp[:-4], m_layer.GetSpatialRef(),geom_type=m_layer.GetGeomType())
    # 获取输出层的要素定义
    outLayerDefn = outlayer.GetLayerDefn()
    # 对图层进行初始化，如果对图层进行了过滤操作，执行这句后，之前的过滤全部清空
    j = 0
    m_layer.ResetReading()
    # 获取投影
    prosrs = m_layer.GetSpatialRef()
    # 添加字段
    inLayerDefn = m_layer.GetLayerDefn()
    for i in range(0, inLayerDefn.GetFieldCount()):
        fieldDefn = inLayerDefn.GetFieldDefn(i)
        outlayer.CreateField(fieldDefn)

    # loop through the input features
    m_feature = m_layer.GetNextFeature()
    while m_feature:
        j = j + 1;
        # print(j)
        o_geometry = m_feature.GetGeometryRef()
        # 关键，合并几何
        o_geometry = o_geometry.Union(o_geometry)
        outfeature = ogr.Feature(outLayerDefn)
        outfeature.SetGeometry(o_geometry)
        # 遍历每个要素的字段，并设置字段属性
        for i in range(0, outLayerDefn.GetFieldCount()):
            # print(outLayerDefn.GetFieldDefn(i).GetNameRef())
            outfeature.SetField(outLayerDefn.GetFieldDefn(i).GetNameRef(), m_feature.GetField(i))
        outlayer.CreateFeature(outfeature)
        # dereference the features and get the next input feature
        outfeature = None
        m_feature = m_layer.GetNextFeature()


    outds.Destroy()

def Check_self_intersect(field_file,HRU_shp):
    """
    检查矢量文件是否存在空间自相交的问题，并解决
    :param Basin_shp_venu:
    :return:
    """
    save_path = os.path.join(os.path.dirname(field_file),"temp_1.shp")
    print(save_path)
    s_shp_si(HRU_shp,save_path)

    os.remove(HRU_shp)
    os.rename(save_path,HRU_shp)


def check_column_value(file_path, column_index):
    """
    读取CSV文件，检查指定列的每一行值是否为目标值。

    :param file_path: CSV文件的路径
    :param column_index: 要检查的列的索引（从0开始）
    :return: 包含匹配行的列表
    """
    matching_rows = []

    with open(file_path, mode='r', newline='', encoding='utf-8') as file:
        csv_reader = csv.reader(file)
        header = next(csv_reader)  # 跳过标题行

        for row_index, row in enumerate(csv_reader):
            # print("当前行:", row_index)
            have_all_0 = True
            try:
                for column in column_index:
                    # print("当前列:", column)
                    column_value = str(row[column])  # 假设列值是整数
                    column_value_list = column_value.split('-')
                    for value in column_value_list:
                        # print("当前值", float(value))
                        print(float(value) == 0.0)
                        if not float(value) == 0:
                            have_all_0 = False
                            break
                    if not have_all_0:
                        break
            except ValueError:
                #
                print(
                    f"Warning: Row {row_index + 1}, column {column_index} has a non-integer value: {row[column_index]}")
            if not have_all_0:
                continue
            matching_rows.append(row_index)

    return matching_rows

if __name__ == "__main__":
    from preprocess.db_mongodb import ConnectMongoDB
    from preprocess.config import parse_ini_configuration

    field_num = 0
    seims_cfg = parse_ini_configuration()

    base_dir = seims_cfg.base_dir
    db_name = seims_cfg.spatial_db
    para_files = base_dir + os.sep + 'workspace\spatial_raster\*.tif'
    raster_para_files = base_dir + os.sep + 'workspace\spatial_raster'
    shp_files = base_dir + os.sep + 'workspace\spatial_shp'
    field_file = base_dir + os.sep + 'workspace\HRU_file'
    model_dir = base_dir + os.sep + db_name
    csv_path = base_dir + os.sep + 'workspace\csv'
    HRU_raster = field_file+ os.sep + 'ALL_HRU_final.tif'
    HRU_raster1 = field_file + os.sep + 'HLU1.tif' # project
    field_txt = field_file + os.sep + 'ALL_HRU_fields.txt'
    subbadin_raster = base_dir + os.sep + 'workspace\spatial_raster\subbasin.tif'
    lakebasin_txt = base_dir + os.sep + 'data_prepare\spatial\Lakebasin.txt'
    lakesubareaidTXT = base_dir + os.sep + 'data_prepare\spatial\Lakesubarea.txt'
    soilgrids0idTXT = base_dir + os.sep + 'data_prepare\spatial\soilgrid0id.txt'
    soil_hand_csv = base_dir + os.sep + 'data_prepare\lookup\soil_90m_properties_hband.csv'

    # soilgrids0idTXT = 'E:/WEB/basins/test/lhzt/1559' + os.sep + 'data_prepare\spatial\soilgrid0id.txt'
    # soil_hand_csv = 'E:/WEB/basins/test/lhzt/1559' + os.sep + 'data_prepare\lookup\soil_90m_properties_hband.csv'

    # add by xjs
    column_index = [5, 7, 8, 9]  # 替换为你要检查的列的索引（从0开始）
    matching_rows = check_column_value(soil_hand_csv, column_index)
    with open(soilgrids0idTXT, 'w', encoding='utf-8') as file:
        for row_num in matching_rows:
            file.write(str(row_num))
            file.write(str(' '))
            print(f"Row {row_num}")


    #删除目录中文件
    # UtilClass.rmmkdir(csv_path)

    # HRU_shp = field_file+ os.sep + 'HRU.shp'
    HRU_shp = field_file + os.sep + 'HRU.shp'

    raster2shp(HRU_raster, HRU_shp, 'field', 'FIELDID')
    # Check_self_intersect(field_file,HRU_shp)


    project(HRU_raster,HRU_raster1)

    HRU_shp1 = field_file + os.sep + 'HRU1.shp'
    raster2shp(HRU_raster1, HRU_shp1, 'field', 'FIELDID')
    # HRU_Albers_shp = field_file+ os.sep + 'HRU_Albers.shp'
    # Reference(HRU_shp,HRU_Albers_shp)

    # 统计各个流域地块中的数据
    # 计算时区分取众数和平均数的数据，
    field_num = field_param_csv(csv_path, HRU_shp, raster_para_files, lakesubareaidTXT, soilgrids0idTXT)
    print(field_num)

    # subarea空间不连续，fieldnum更新
    with open(field_txt, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    field_num = len(lines) - 1
    print(field_num)
    print('cumpute soil & landuse data done!')

    # 每个HRU在哪个subbasin
    subbasin_csv = csv_path + os.sep + 'subbasin.csv'
    field_subbasin_array(field_txt, subbasin_csv)

    # IUH csv
    iuh_csv = csv_path + os.sep + 'iuh.csv'
    IUH_1Darray(iuh_csv, field_num)
    print('cumpute IUH done!')

    # 计算地块中心
    field_center_file = csv_path + os.sep + 'fields_center.csv'
    field_center(HRU_shp,field_center_file)

    field_center_file_84 = csv_path + os.sep + 'fields_center_wgs84.csv'
    field_center(HRU_shp, field_center_file_84)

    # fields_center.csv lonlat2xy :add by xjs
    df = pd.read_csv(field_center_file)
    num_columns = df.shape[1]
    print(num_columns)
    row_index = 0  # 列
    lon_col_index = 0
    lat_col_index = 1

    wgs84 = osr.SpatialReference()
    wgs84.ImportFromEPSG(4326)  # 设置WGS 84坐标系
    Pseudo_Mercator = osr.SpatialReference()
    Pseudo_Mercator.ImportFromEPSG(3857)
    transform = osr.CoordinateTransformation(wgs84, Pseudo_Mercator)

    for row in range(num_columns):
        point = ogr.Geometry(ogr.wkbPoint)
        # print(df.iloc[lat_col_index, row], df.iloc[lon_col_index, row])
        # 服务器：
        # point.AddPoint(df.iloc[lat_col_index, row], df.iloc[lon_col_index, row])
        # 本地：
        point.AddPoint(df.iloc[lon_col_index, row],df.iloc[lat_col_index, row])
        point.Transform(transform)  # 转换为投影坐标
        projected_coord = (point.GetX(), point.GetY())
        # print(projected_coord)
        df.iloc[lat_col_index, row] = point.GetY()
        df.iloc[lon_col_index, row] = point.GetX()

    df.to_csv(field_center_file, index=False)

    # 计算celllat
    celllat_csv = csv_path + os.sep + 'celllat.csv'
    # get_coord_field_point(subbadin_raster,field_center_file,celllat_csv)
    get_coord_field_point(field_center_file_84, field_center_file, celllat_csv)
    print('cumpute HRU center coor done!')

    # 计算cellarea
    # 这里需要对HRU_shp进行投影
    csv_file = csv_path + os.sep + 'cellarea.csv'
    cell_area(HRU_shp1, csv_file)
    print('cumpute HRU area done!')

    # stream_link计算
    csv_file = csv_path + os.sep + 'stream_link.csv'
    stream_link_csv(field_txt, csv_file)
    print('cumpute stream_link done!')

    #flowin_index计算
    csv_file = csv_path + os.sep + 'flowin_index.csv'
    stream_file = raster_para_files + os.sep + 'stream_link.tif'
    stream_shp = csv_path+ os.sep + 'streamlink.shp'
    raster2shp(stream_file, stream_shp, 'field', 'FIELDID')

    flowin_index(field_txt,csv_file)
    #flowdown_index计算
    csv_file1 = csv_path + os.sep + 'flowout_index.csv'
    csv_file2 = csv_path + os.sep + 'flowout_length.csv'
    flowout_index(HRU_shp,stream_shp,HRU_raster,field_txt,csv_file1,csv_file2)
    print('cumpute flow in-out done!')

    # routing_layers_down_up
    csv_file = csv_path + os.sep + 'routing_layer.csv'
    routing_layer_csv(field_txt,csv_file)

    #其他参数
    datatype_csv = csv_path + os.sep + 'datatypes.csv'
    datatype(datatype_csv)

    # field_num = 25
    # field_center_file = csv_path + os.sep + 'fields_center.csv'

    # field_num = 287
    # field_center_file = csv_path + os.sep + 'fields_center.csv'

    # 导入数据库中
    # Load configuration file
    from preprocess.config import parse_ini_configuration
    seims_cfg = parse_ini_configuration()
    db_import_field_arrays.workflow(seims_cfg, db_name,csv_path,field_num)

    # 权重修改
    ImportWeightData_field.workflow(seims_cfg, field_center_file,db_name)

    #导入子流域数据
    if not os.path.exists(lakebasin_txt):

        client = ConnectMongoDB(seims_cfg.hostname, seims_cfg.port)
        conn = client.get_conn()
        db = conn[db_name]
        txt_data = pd.read_csv(field_file+os.sep+'reach_param.csv',  skipinitialspace=True)
        dict_target = {}
        sub_num = 0
        for index, row in txt_data.iteritems():
            #dict_target = dict(zip(index,row.to_list()))
            dict_target[index]=row.to_list()
            sub_num = len(row.to_list())
        for key in dict_target:
            if(key!="FID"):
                for i in range(1,sub_num+1) :
                    db["REACHES"].update({'SUBBASINID': i}, {'$set': {key:dict_target[key][i-1]}},False,True)

    else:
        # 生成reach_param参数，并直接导入数据库，不生成csv
        stream_shp = shp_files + os.sep + 'reach.shp'
        ds = ogr_Open(stream_shp)
        lyr = ds.GetLayer(0)
        subbasin_id = []
        for feat in lyr:
            id = feat.GetField('LINKNO')
            subbasin_id.append(id)

        ftdic = {}

        # 打开文件并读取第一行
        import json
        with open(lakebasin_txt, 'r') as file:
            first_line = file.readline()
        lakeidDic = eval(first_line)
        reach_id = list(lakeidDic.keys())
        lake_id = [float(value) for key, value in lakeidDic.items()]

        ftdic['FID'] = reach_id
        ftdic['Is_permafrost'] = len(reach_id) * [0]
        ftdic['Is_Lake'] = len(reach_id) * [0]
        ftdic['Is_Res'] = len(reach_id) * [0]
        ftdic['Nature_Flow'] = len(reach_id) * [0]
        # replace!
        ftdic['LAKE_ID'] = lake_id
        # print(ftdic)

        GLOBathy_path = 'C:/WEB/GLOBathy.csv'
        GLOBathy = pd.read_csv(GLOBathy_path)

        A_a = []
        A_b = []
        A_Va = []
        A_Vb = []
        lake_A = []
        lake_V = []
        lake_H = []
        for lakeid in ftdic['LAKE_ID']:
            A_a.append(float(GLOBathy[GLOBathy['lake_id'] == lakeid]['f_hA_a']))
            A_b.append(float(GLOBathy[GLOBathy['lake_id'] == lakeid]['f_hA_b']))
            A_Va.append(float(GLOBathy[GLOBathy['lake_id'] == lakeid]['f_hA_Va']))
            A_Vb.append(float(GLOBathy[GLOBathy['lake_id'] == lakeid]['f_hA_Vb']))
            lake_A.append(float(GLOBathy[GLOBathy['lake_id'] == lakeid]['A']))
            lake_V.append(float(GLOBathy[GLOBathy['lake_id'] == lakeid]['V']))
            lake_H.append(float(GLOBathy[GLOBathy['lake_id'] == lakeid]['H']))

        ftdic['A_a'] = A_a
        ftdic['A_b'] = A_b
        ftdic['A_Va'] = A_Va
        ftdic['A_Vb'] = A_Vb

        ftdic['Lake_Area'] = [i * 1e6 for i in lake_A]
        ftdic['Lake_Vol'] = [i * 1e9 for i in lake_V]
        ftdic['Lake_Depini'] = lake_H

        ftdic['RES_LC'] = len(reach_id) * [0.1]
        ftdic['RES_LN'] = len(reach_id) * [0.3]
        ftdic['RES_LF'] = len(reach_id) * [0.97]
        ftdic['RES_ADJUST'] = len(reach_id) * [0.7]
        ftdic['RES_NSED'] = len(reach_id) * [1]
        ftdic['CH_BNK_K'] = len(reach_id) * [0.0001]
        ftdic['LAKE_ALPHA'] = len(reach_id) * [0.04]
        ftdic['RES_minq'] = len(reach_id) * [0]
        ftdic['RES_normq'] = len(reach_id) * [0]
        ftdic['RES_ndq'] = len(reach_id) * [0]
        ftdic['RES_normMult'] = len(reach_id) * [1]
        # ftdic['GW_SPYLD'] = len(reach_id) * [0.04]

        from osgeo import ogr, osr, gdal

        reach_shp = "C:/WEB/HydroLAKES_polys_v10_shp/China_Lakes.shp"
        ds = ogr.Open(reach_shp, 0)
        layer = ds.GetLayer()
        feature = layer.GetNextFeature()
        # 获取图层属性表定义
        featuredefn = layer.GetLayerDefn()
        # 获取属性表中字段数
        fieldcount = featuredefn.GetFieldCount()
        # 获取字段属性名
        name = []
        for attr in range(fieldcount):
            fielddefn = featuredefn.GetFieldDefn(attr)
            # print("%s: %s" % (fielddefn.GetNameRef(), fielddefn.GetFieldTypeName(fielddefn.GetType())))
            name.append(fielddefn.GetNameRef())
        flow = []
        islake = []
        while feature:
            ID = feature.GetFieldAsInteger('Hylak_id')
            if (ID in ftdic['LAKE_ID']):
                dis = feature.GetFieldAsDouble('Dis_avg')
                if (feature.GetFieldAsDouble('Lake_type') == 1):
                    islake.append(1)
                else:
                    islake.append(2)
                flow.append(dis)

            # 清除缓存并获取下一个要素
            feature.Destroy()
            feature = layer.GetNextFeature()

        reach_param = pd.DataFrame(ftdic)

        for index, ii in enumerate(ftdic['LAKE_ID']):
            x_index = reach_param[reach_param['LAKE_ID'] == ii].index.values[0]
            if (islake[index]) == 1:
                d_index = list(reach_param.columns).index('Is_Lake')
                reach_param.iloc[x_index, d_index] = 1
            else:
                d_index = list(reach_param.columns).index('Is_Res')
                reach_param.iloc[x_index, d_index] = 0
                d_index = list(reach_param.columns).index('Is_Lake')
                reach_param.iloc[x_index, d_index] = 0
            dd_index = list(reach_param.columns).index('Nature_Flow')
            reach_param.iloc[x_index, dd_index] = flow[index]
        # print(reach_param)
        ftdicnew = reach_param.to_dict('list')
        from preprocess.db_mongodb import ConnectMongoDB
        from preprocess.config import parse_ini_configuration

        seims_cfg = parse_ini_configuration()
        client = ConnectMongoDB(seims_cfg.hostname, seims_cfg.port)
        conn = client.get_conn()
        db = conn[db_name]
        for key in ftdicnew:
            # print(key)
            if (key != "FID"):
                # for i in range(1, len(reach_id) + 1):
                #     print(reach_id[i - 1])
                #     print(ftdic[key][i - 1])
                #     db["REACHES"].update({'SUBBASINID': reach_id[i - 1]}, {'$set': {key: ftdicnew[key][i - 1]}}, False, True)

                for i in range(1, len(subbasin_id) + 1):
                    # print(reach_id[i - 1])
                    # print(ftdic[key][i - 1])
                    if i in reach_id:
                        # print(i)
                        # print(ftdicnew[key][reach_id.index(i)])
                        db["REACHES"].update({'SUBBASINID': i}, {'$set': {key: ftdicnew[key][reach_id.index(i)]}}, False, True)
                    else:
                        db["REACHES"].update({'SUBBASINID': i}, {'$set': {key: 0}}, False, True)


    




