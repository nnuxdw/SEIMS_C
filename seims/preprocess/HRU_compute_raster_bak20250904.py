'''
Author: binjie
Date: 2021-12-30 09:54:38
LastEditTime: 2022-01-19 20:49:17
LastEditors: Please set LastEditors
Description:
'''
from osgeo.gdalconst import GDT_Int16
from rasterstats import zonal_stats
from pygeoc.raster import RasterUtilClass
import os
import sys
import gdal
import pandas as pd
from glob import glob

from osgeo.ogr import Open as ogr_Open
from osgeo import gdal_array
from osgeo.osr import SpatialReference as osr_SpatialReference
from osgeo.osr import CoordinateTransformation as osr_CoordinateTransformation
from osgeo.ogr import CreateGeometryFromWkt as ogr_CreateGeometryFromWkt
from osgeo.ogr import GetDriverByName as ogr_GetDriverByName
from pygeoc.utils import FileClass, UtilClass
from osgeo.ogr import FieldDefn as ogr_FieldDefn
from osgeo.ogr import OFTInteger
import numpy as np
import db_import_field_arrays
from seims.preprocess.db_import_interpolation_weights_field import ImportWeightData_field

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
    txt_data = pd.read_csv(field_txt, delimiter="    ")
    txt_data.to_csv(subbasin_csv_path,index=0,columns=['FID', 'subbasin'])

def field_center(field_shp_file,field_center_file):
    ds = ogr_Open(field_shp_file)
    lyr = ds.GetLayer(0)
    dist_field_center = {}
    for feat in lyr:
        id = feat.GetField('FIELDID')
        geometry = feat.GetGeometryRef().Centroid()
        pt = geometry.GetPoint()
        dist_field_center.setdefault(id,[]).append(pt[0])
        dist_field_center.setdefault(id,[]).append(pt[1])
    df_field_center = pd.DataFrame(dist_field_center)
    df_field_center.to_csv(field_center_file,index=0, header=True)

def field_param_csv(csv_file,field_shp_file, raster_para_files):
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
        for raster_para_file in glob(raster_para_file):
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
    other_param = ['CN2','dayLenMin','dem','depression','dormhr','USLE_P','slope','slope_dinf','moist_in','runoff_co','acc']

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
    if  not os.path.exists(csv_file):#如果路径不存在
        os.makedirs(csv_file)

    i = 0
    field_num = 0
    for param_list_mean in param_lists_mean :
        df=pd.DataFrame(columns=['FID'])
        for param in param_list_mean:
            param_file = raster_para_files + os.sep + param + '.tif'
            df_param = field_compute(field_shp_file,param_file,'mean')
            df[param] = df_param['mean']
        df['FID'] = df_param['FID']
        field_num = len(df)
        df.to_csv(csv_file + os.sep + param_lists_mean_file[i] + '.csv',index=0)
        i = i + 1
    i = 0
    for param_list_majority in param_lists_majority :
        df=pd.DataFrame(columns=['FID'])
        for param in param_list_majority:
            param_file = raster_para_files + os.sep + param + '.tif'
            df_param = field_compute(field_shp_file,param_file,'majority')
            df[param] = df_param['majority']
        df['FID'] = df_param['FID']
        df.to_csv(csv_file + os.sep + param_lists_majority_file[i] + '.csv',index=0)
        i = i + 1
    return field_num

def get_coord_field_point(celllat_tif, field_center,csv_file):
    ds = RasterUtilClass.read_raster(celllat_tif)
    src_srs = ds.srs
    if not src_srs.ExportToProj4():
        raise ValueError('The source raster %s has not coordinate, '
                            'which is required!' % celllat_tif)

    dst_srs = osr_SpatialReference()
    dst_srs.ImportFromEPSG(4326)  # WGS84
    # dst_wkt = dst_srs.ExportToWkt()
    transform = osr_CoordinateTransformation(src_srs, dst_srs)

    field_center_list = pd.read_csv(field_center)
    row, fiald_num = field_center_list.shape

    fcsv = open(csv_file, 'w')
    fcsv.write('FID,prjX,prjY,celllong,celllat\n')
    for fiald_id in range(fiald_num):
        field_x, field_y = field_center_list.loc[:,str(fiald_id)]
        # print(field_x, field_y)
        point = ogr_CreateGeometryFromWkt('POINT (%f %f)' % (field_x, field_y))
        point.Transform(transform)
        # print ('WGS84XY:', point.GetY(), point.GetX())
        con = '%s,%s,%s,%s,%s\n' %(str(fiald_id),str(field_x),str(field_y),str(point.GetY()),str(point.GetX()))
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
        # 该HRU上游HRU的数量
        flowin.append(len(upstram_id))
        # 该HRU上游HRU的FID
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


    for id in range(len(txt_data.loc[:,'FID'])):
        poly1 = (ftdic[id][0].GetGeometryRef() )
        if (txt_data.loc[id,'downstreamFID'] > -1):
            #不临近河流的计算共边长
            poly2 = (ftdic[txt_data.loc[id,'downstreamFID']][0].GetGeometryRef() )
            boundary = poly1.Intersection(poly2)
            if(boundary.Length()>0):
                length.append(round(boundary.Length(),2))#公共边界长度
            else:
                length.append(adfGeoTransform[1]) #取分辨率
        else:
            #临近河流的计算河流的共边
            poly3 = (ftdic2[txt_data.loc[id,'subbasin']][0].GetGeometryRef() )
            boundary = poly1.Intersection(poly3)
            if(boundary.Length()>0):
                length.append(round(boundary.Length(),2))#公共边界长度
            else:
                length.append(adfGeoTransform[1])
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

def reflect_fid_and_subbasinid(subbasin_file):
    # 打开shapefile
    ds = ogr_Open(subbasin_file)
    if ds is None:
        raise ValueError("Could not open the shapefile")

    # 获取第一个图层
    layer = ds.GetLayer(0)

    # 创建字典存储FID和SUBBASINID的对应关系
    fid_subbasinid_map = {}

    # 遍历图层中的每个要素
    for feature in layer:
        # 获取FID和SUBBASINID属性
        if feature.GetFID() is not None and feature.GetField('SUBBASINID') is not None:
            fid = feature.GetFID()
            subbasinid = feature.GetField('SUBBASINID')
            fid_subbasinid_map[fid] = subbasinid

    # 关闭数据源
    ds = None

    return fid_subbasinid_map

def over_write_hru_txt(all_hru_fields_txt,hru_txt, fid_subbasinid_map):
    txt_data = pd.read_csv(all_hru_fields_txt, delimiter="\s+", engine='python')
    # 创建一个新的 DataFrame，以便替换 'subbasin' 列的值
    new_df = txt_data.copy()
    for index, row in new_df.iterrows():
        # 获取当前行的 'subbasin' 值
        subbasin = row['subbasin']
        # 通过映射找到相应的 'subbasin_id'
        subbasin_id = fid_subbasinid_map.get(subbasin, subbasin)  # 使用 get 方法避免键不存在的错误
        # 替换 'subbasin' 列的值
        new_df.at[index, 'subbasin'] = subbasin_id
    # 将更新后的 DataFrame 写入到新的 txt 文件
    with open(hru_txt, 'w') as file:
        # 写入列标题
        file.write("    ".join(new_df.columns) + "\n")
        # 写入数据行
        for index, row in new_df.iterrows():
            file.write("    ".join(row.astype(str)) + "\n")


def datatype(datatype_csv):
    df = pd.DataFrame()
    layer_data = list()
    layer_data = [0]
    df.insert(loc=0,column='datatypes', value=layer_data)
    df.to_csv(datatype_csv ,index=0)

if __name__ == "__main__":
    # base_dir = r'G:\program\seims\SEIMS_C\data\hulugou'
    # db_name = 'hulugou_longterm_model'
    # base_dir = r'G:\program\seims\SEIMS_C\data\gongba'
    base_dir = r'G:\program\seims\SEIMS_C\data\gongba_subbasin'
    db_name = 'gongba_subbasin_longterm_model'
    para_files = base_dir + os.sep + 'workspace\spatial_raster\*.tif'
    raster_para_files = base_dir + os.sep + 'workspace\spatial_raster'
    shp_files = base_dir + os.sep + 'workspace\spatial_shp'
    field_file = base_dir + os.sep + 'workspace\\HRU_file'
    model_dir = base_dir + os.sep + db_name
    csv_path = base_dir + os.sep + 'workspace\csv'
    HRU_raster = field_file+ os.sep + 'ALL_HRU_final.tif'
    ori_field_txt = field_file+ os.sep + 'ALL_HRU_fields.txt'
    field_txt = field_file + os.sep + 'HRU_fields.txt'
    subbadin_raster = base_dir + os.sep + 'workspace\spatial_raster\subbasin.tif'
    # xiaodw add, 获取subbasin.shp中的FID和SUBBASINID的对应关系
    subbasin_shp = base_dir + os.sep + 'workspace\spatial_shp\subbasin.shp'
    subbasin_id_map = reflect_fid_and_subbasinid(subbasin_shp)
    # 将subbasin信息写入HRU.txt
    over_write_hru_txt(ori_field_txt,field_txt,subbasin_id_map)

    #删除目录中文件
    UtilClass.rmmkdir(csv_path)
    HRU_shp = field_file+ os.sep + 'HRU.shp'
    raster2shp(HRU_raster, HRU_shp, 'field', 'FIELDID')
    HRU_Albers_shp = field_file+ os.sep + 'HRU_Albers.shp'
    Reference(HRU_shp,HRU_Albers_shp)

    # 统计各个流域地块中的数据
    # 计算时区分取众数和平均数的数据，
    field_num = field_param_csv(csv_path, HRU_shp, raster_para_files)
    print('compute soil & landuse data done!')

    # 每个HRU在哪个subbasin
    subbasin_csv = csv_path + os.sep + 'subbasin.csv'
    field_subbasin_array(field_txt, subbasin_csv)

    # IUH csv
    iuh_csv = csv_path + os.sep + 'iuh.csv'
    IUH_1Darray(iuh_csv, field_num)
    print('compute IUH done!')

    # 计算地块中心
    field_center_file = csv_path + os.sep + 'fields_center.csv'
    field_center(HRU_shp,field_center_file)

    # 计算celllat
    celllat_csv = csv_path + os.sep + 'celllat.csv'
    get_coord_field_point(subbadin_raster,field_center_file,celllat_csv)
    print('compute HRU center coor done!')

    # 计算cellarea
    csv_file = csv_path + os.sep + 'cellarea.csv'
    cell_area(HRU_shp, csv_file)
    print('compute HRU area done!')

    # stream_link计算
    csv_file = csv_path + os.sep + 'stream_link.csv'
    stream_link_csv(field_txt, csv_file)
    print('compute stream_link done!')

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
    print('compute flow in-out done!')

    # routing_layers_down_up
    csv_file = csv_path + os.sep + 'routing_layer.csv'
    routing_layer_csv(field_txt,csv_file)

    #其他参数
    datatype_csv = csv_path + os.sep + 'datatypes.csv'
    datatype(datatype_csv)


    #field_num = 53
    #field_center_file = csv_path + os.sep + 'fields_center.csv'
    #导入数据库中
    #Load configuration file
    from config import parse_ini_configuration
    seims_cfg = parse_ini_configuration()
    db_import_field_arrays.workflow(seims_cfg, db_name,csv_path,field_num)

    # 权重修改
    ImportWeightData_field.workflow(seims_cfg, field_center_file,db_name)

    #导入子流域数据
    from db_mongodb import ConnectMongoDB
    from config import parse_ini_configuration
    seims_cfg = parse_ini_configuration()
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







