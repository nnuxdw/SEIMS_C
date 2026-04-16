#! /usr/bin/env python
# -*- coding: utf-8 -*-
"""Import spatial parameters corresponding to fields as GridFS to MongoDB
    @author   : Liangjun Zhu
    @changelog: 18-06-08  lj - first implementation version.\n
"""

from __future__ import absolute_import, unicode_literals
import os
import sys

# if os.path.abspath(os.path.join(sys.path[0], '../..')) not in sys.path:
#     sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '../..')))

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
from pygeoc.utils import FileClass, StringClass
from gridfs import GridFS
from struct import pack
import pandas as pd
from db_mongodb import ConnectMongoDB
from utility import read_data_items_from_txt
from text import DBTableNames
import re


def combine_multi_layers_array(data_dict):
    """
    Combine multi-layers array data if existed.
    Args:
        data_dict: format: {'SOL_OM_1': [1.1, 0.9, 0.4],
                            'SOL_OM_2': [1.1, 0.9, 0.4],
                            'SOL_OM_3': [1.1, 0.9, 0.4],
                            'DEM': [100, 101, 102]
                           }

    Returns: Combined array dict which contains multi-layers data.
             format: {'SOL_OM': [[1.1, 0.9, 0.4], [1.1, 0.9, 0.4], [1.1, 0.9, 0.4]],
                      'DEM': [[100, 101, 102]]
                     }
    """
    comb_data_dict = dict()
    for key, value in list(data_dict.items()):
        key_split = key.split('_')
        if len(key_split) <= 1:
            comb_data_dict[key] = [value]
            continue
        # len(key_split) >= 2:
        try:
            pot_lyr_idx = int(key_split[-1]) - 1
            corename = key[0:key.rfind('_')]
            if pot_lyr_idx < 0:
                pot_lyr_idx = 0
            if corename not in comb_data_dict:
                comb_data_dict[corename] = list()
            comb_data_dict[corename].insert(pot_lyr_idx, value)
        except ValueError:
            comb_data_dict[key] = [value]
            continue
    return comb_data_dict


def read_field_arrays_from_csv(csvf):
    data_items = read_data_items_from_txt(csvf)
    if len(data_items) < 2:
        return
    flds = data_items[0]
    flds_array = dict()
    for idx, data_item in enumerate(data_items):
        if idx == 0:
            continue
        data_item_values = StringClass.extract_numeric_values_from_string(','.join(data_item))
        for fld_idx, fld_name in enumerate(flds):
            if fld_idx == 0 or StringClass.string_match(fld_name, 'FID'):
                continue
            if fld_name not in flds_array:
                flds_array[fld_name] = list()
            flds_array[fld_name].append(data_item_values[fld_idx])
            #print (fld_name)
    # for key, value in list(flds_array.items()):
    #     print('%s: %d' % (key, len(value)))
    return combine_multi_layers_array(flds_array)


def import_array_to_mongodb(gfs, array, fname):
    """
    Import array-like spatial parameters to MongoDB as GridFs
    Args:
        gfs: GridFs object
        array: format [[1,2,3], [2,2,2], [3,3,3], means an array with three layers
        fname: file name
    """
    fname = fname.upper()
    if gfs.exists(filename=fname):
        x = gfs.get_version(filename=fname)
        gfs.delete(x._id)

    rows = len(array)
    cols = len(array[0])

    # Currently, metadata is fixed.
    meta_dict = dict()
    if 'WEIGHT' in fname:
        meta_dict['NUM_SITES'] = rows
        meta_dict['NUM_CELLS'] = cols
        meta_dict['SUBBASIN'] = 0  # Field-version
    else:
        meta_dict['TYPE'] = fname
        meta_dict['ID'] = fname
        meta_dict['DESCRIPTION'] = fname
        meta_dict['SUBBASIN'] = 0
        meta_dict['CELLSIZE'] = 1
        meta_dict['NODATA_VALUE'] = -9999
        meta_dict['NCOLS'] = cols
        meta_dict['NROWS'] = 1
        meta_dict['XLLCENTER'] = 0
        meta_dict['YLLCENTER'] = 0
        meta_dict['LAYERS'] = rows
        meta_dict['CELLSNUM'] = cols
        meta_dict['SRS'] = ''

    myfile = gfs.new_file(filename=fname, metadata=meta_dict)
    for j in range(0, cols):
        cur_col = list()
        for i in range(0, rows):
            cur_col.append(array[i][j])
        fmt = '%df' % rows
        myfile.write(pack(fmt, *cur_col))
    myfile.close()
    print('Import %s done!' % fname)

def import_array_to_mongodb_flowinindex(gfs, array, fname):
    """
    Import array-like spatial parameters to MongoDB as GridFs
    Args:
        gfs: GridFs object
        array: format [[1,2,3], [2,2,2], [3,3,3], means an array with three layers
        fname: file name
    """
    fname = fname.upper()
    if gfs.exists(filename=fname):
        x = gfs.get_version(filename=fname)
        gfs.delete(x._id)

    rows = len(array)  #field number +1 ##第一个数是地块总数
    # cols = len(array[0])

    # Currently, metadata is fixed.
    meta_dict = dict()
    if 'FLOW' or 'ROUTING' in fname:
        meta_dict['SUBBASIN'] = 0
        meta_dict['TYPE'] = fname
        meta_dict['ID'] = fname
        meta_dict['DESCRIPTION'] = fname
        meta_dict['NUMBER'] = 2 * (rows-1)

        myfile = gfs.new_file(filename=fname, metadata=meta_dict)
        for j in range(0, rows):
            cur_col = list()
            array_int = [int(s) for s in re.findall(r'\b\d+\b', array[j])]
            for i in range(0, len(array_int)):
                cur_col.append((array_int[i]))
            fmt = '%df' %  len(array_int)
            myfile.write(pack(fmt, *cur_col))
        myfile.close()
        print('Import %s done!' % fname)
    else:
        print('%s can`t use this function' %fname)

def import_array_to_mongodb_flowoutindex(gfs, array, fname):
    """
    Import array-like spatial parameters to MongoDB as GridFs
    Args:
        gfs: GridFs object
        array: format [[1,2,3], [2,2,2], [3,3,3], means an array with three layers
        fname: file name
    """
    fname = fname.upper()
    if gfs.exists(filename=fname):
        x = gfs.get_version(filename=fname)
        gfs.delete(x._id)

    rows = len(array)  #field number +1 ##第一个数是地块总数
    # cols = len(array[0])

    # Currently, metadata is fixed.
    meta_dict = dict()
    if 'FLOW' or 'ROUTING' in fname:
        meta_dict['SUBBASIN'] = 0
        meta_dict['TYPE'] = fname
        meta_dict['ID'] = fname
        meta_dict['DESCRIPTION'] = fname
        meta_dict['NUMBER'] = 2 * (rows-1)

        myfile = gfs.new_file(filename=fname, metadata=meta_dict)
        for j in range(0, rows):
            cur_col = list()
            for i in range(0, len(array)):
                cur_col.append((array[i]))
            fmt = '%df' %  len(array)
            myfile.write(pack(fmt, *cur_col))
        myfile.close()
        print('Import %s done!' % fname)
    else:
        print('%s can`t use this function' %fname)

def workflow(cfg, db_name, csv_path,field_num):
    client = ConnectMongoDB(cfg.hostname, cfg.port)
    conn = client.get_conn()

    db_model_field = conn[db_name]
    dblist = conn.list_database_names()
    # if db_name in dblist:
    #     conn.drop_database(db_name)
    # conn.admin.command('copydb',fromdb =cfg.spatial_db,todb=db_name)  # 复制数据库
    # print("field version copy database")
    # delete SPATIAL.files and SPATIAL.chunks
    db_model_field_spatial_files = db_model_field['SPATIAL.files']
    db_model_field_spatial_chunks = db_model_field['SPATIAL.chunks']
    # x = db_model_field_spatial_files.delete_many({})
    # x = db_model_field_spatial_chunks.delete_many({})
    # print(x.deleted_count, "SPATIAL documents deleted.")

    spatial_gfs = GridFS(db_model_field, DBTableNames.gridfs_spatial)

    csv_files = FileClass.get_full_filename_by_suffixes(csv_path, ['.csv'])
    field_count = field_num  #地块数量
    prefix = 0
    # Create mask file
    mask_name = '%d_MASK' % prefix
    mask_array = [[1] * field_count]
    import_array_to_mongodb(spatial_gfs, mask_array, mask_name)  #mask输入一次即可

    # Create spatial parameters
    for csv_file in csv_files:
        print('Import %s...' % csv_file)
        if 'flowin' in csv_file:
            df = pd.read_csv(csv_file, skipinitialspace=True)
            if 'flowin' in csv_file:
                array = (df.loc[:,'flowin_index_d8'])
                import_array_to_mongodb_flowinindex(spatial_gfs, array, '%d_%s' % (prefix, 'FLOWIN_INDEX_D8'))
            # elif 'flowout' in csv_file:
            #     array = (df.loc[:,'flowout_index_d8'])
            #     import_array_to_mongodb_flowoutindex(spatial_gfs, array, '%d_%s' % (prefix, 'FLOWOUT_INDEX_D8'))

        elif 'routing' in csv_file:
            df = pd.read_csv(csv_file, skipinitialspace=True)
            array = (df.loc[:,'routing_layers_down_up'])
            import_array_to_mongodb_flowinindex(spatial_gfs, array, '%d_%s' % (prefix, 'ROUTING_LAYERS_DOWN_UP'))
        else:
            print (csv_file)
            param_arrays = read_field_arrays_from_csv(csv_file)
            for key, value in list(param_arrays.items()):
                pondVal = value
                import_array_to_mongodb(spatial_gfs, pondVal, '%d_%s' % (prefix, key))


def sum_cellarea_by_fid(input_csv: str, output_csv: str):
    # 读取CSV文件
    df = pd.read_csv(input_csv)

    # 按FID分组求CELLAREA之和
    summed_df = df.groupby('FID', as_index=False)['CELLAREA'].sum()

    # 写入新的CSV文件
    summed_df.to_csv(output_csv, index=False)
    print(f"结果已保存到 {output_csv}")


def gen_param_group_csv(conn, db_name: str, collection: str,
                         mapper: dict, csv_in: str, csv_in_col : str,csv_out: str):
    """
    根据 mapper 映射，从 MongoDB PARAMETERS 表读取参数 VALUE，
    如果 VALUE == -9999，则取默认值 1.0。
    然后结合 cali_param.csv 的 FID 列，生成新的 csv。

    :param conn: MongoDB connection (MongoClient 或者 Database 对象)
    :param db_name: 数据库名
    :param collection: 集合名 (表名)
    :param mapper: dict，键=NAME(数据库字段)，值=输出csv列名
    :param csv_in: 输入CSV文件路径 (包含FID列)
    :param csv_out: 输出CSV文件路径
    """
    col = conn[db_name][collection]

    # 获取参数值
    values = {}
    for name, out_col in mapper.items():
        doc = col.find_one({"NAME": name}, {"VALUE": 1})
        if doc and "VALUE" in doc:
            val = doc["VALUE"]
            if val == -9999:
                val = 1.0
        else:
            val = 1.0
        values[out_col] = val

    # 读取 cali_param.csv 或 cali_param_sub.csv  的 FID
    df = pd.read_csv(csv_in, usecols=[csv_in_col])

    # 每列赋值
    for out_col, val in values.items():
        df[out_col] = val

    # 保存新 CSV
    df.to_csv(csv_out, index=False)
    print(f"新CSV已生成: {csv_out}")


def update_reaches_from_csv(conn, db_name: str, collection: str, csv_file: str):
    """
    从 CSV 更新 MongoDB REACHES 集合
    :param conn: MongoDB connection
    :param db_name: 数据库名
    :param collection: 集合名，例如 "REACHES"
    :param csv_file: 输入的 CSV 文件路径
    """
    col = conn[db_name][collection]

    # 读取 CSV
    df = pd.read_csv(csv_file)

    # 遍历行，更新 MongoDB
    for _, row in df.iterrows():
        subbasin_id = int(row["subbasin"])
        update_fields = {}

        # 只更新 CSV 里存在的字段
        for colname in df.columns:
            if colname != "subbasin":
                val = row[colname]
                # 处理 NaN
                if pd.notna(val):
                    update_fields[colname] = float(val) if isinstance(val, (int, float)) else val

        if update_fields:
            result = col.update_one(
                {"SUBBASINID": subbasin_id},
                {"$set": update_fields}
            )
            print(f"SUBBASINID={subbasin_id} 更新 {update_fields}，匹配 {result.matched_count} 条，修改 {result.modified_count} 条")

    print("全部更新完成！")

if __name__ == "__main__":
    from config import parse_ini_configuration
    base_path = r'G:\program\seims\SEIMS_HAND\data'
    # basin = "poyang_lake1"
    basin = 'MSL_1'
    seims_cfg = parse_ini_configuration()
    client = ConnectMongoDB(seims_cfg.hostname, seims_cfg.port)
    db_name = f'{basin}_longterm_model'
    conn = client.get_conn()
    db_model_field = conn[db_name]
    spatial_gfs = GridFS(db_model_field, DBTableNames.gridfs_spatial)

    caliparam_csv_file = os.path.join(base_path,basin,f'{basin}_longterm_model','caliparam.csv')
    param_group1_csv_file = os.path.join(base_path, basin, f'{basin}_longterm_model', 'param_group1.csv')
    prefix = 0
    ############# 根据PARAMETER表中的VALUE生成param_group1.csv,并导入spatial.file ############
    mapper1 = {
        "K_pet": "K_pet_1d",
        "Runoff_co": "Runoff_co",
        "c_snow12": "c_snow12_1d",
        "c_snow6": "c_snow6_1d",
        "lag_snow": "lag_snow_1d",
        "T0_1d": "T0_1d",
        "T_snow": "T_snow_1d",
        "Ki": "Ki_1d",
        "SURLAG": "SURLAG_1D",

    }

    # gen_param_group_csv(
    #     conn=conn,
    #     db_name=db_name,
    #     collection="PARAMETERS",
    #     mapper=mapper1,
    #     csv_in=caliparam_csv_file,
    #     csv_in_col='FID',
    #     csv_out=param_group1_csv_file
    # )


    param_arrays = read_field_arrays_from_csv(param_group1_csv_file)
    for key, value in list(param_arrays.items()):
        pondVal = value
        import_array_to_mongodb(spatial_gfs, pondVal, '%d_%s' % (prefix, key))

    ############# 根据PARAMETER表中的VALUE生成param_group2.csv,并导入spatial.file ############

    caliparam_sub_csv_file = os.path.join(base_path,basin,f'{basin}_longterm_model','caliparam_sub.csv')
    param_group2_csv_file = os.path.join(base_path, basin, f'{basin}_longterm_model', 'param_group2.csv')
    mapper2 = {
        "Base_ex": "BASE_EX_1D",
        "Kg": "KG_1D",
        "gw_delay": "GW_DELAY_1D",
        "ep_ch":"EP_CH_1D",
        "GWMAX_1D":"GWMAX_1D",
        "LAKE_EVP":"LAKE_EVP",
        "LAKE_SEEP":"LAKE_SEEP"
    }


    # gen_param_group_csv(
    #     conn=conn,
    #     db_name=db_name,
    #     collection="PARAMETERS",
    #     mapper=mapper2,
    #     csv_in=caliparam_sub_csv_file,
    #     csv_in_col='subbasin',
    #     csv_out=param_group2_csv_file
    # )
    param_arrays = read_field_arrays_from_csv(param_group2_csv_file)
    prefix = 0
    for key, value in list(param_arrays.items()):
        pondVal = value
        import_array_to_mongodb(spatial_gfs, pondVal, '%d_%s' % (prefix, key))

    ############# 根据PARAMETER表中的VALUE生成param_group2_ch.csv,并导入REACHES ############

    caliparam_sub_csv_file = os.path.join(base_path,basin,f'{basin}_longterm_model','caliparam_sub.csv')
    param_group2_ch_csv_file = os.path.join(base_path, basin, f'{basin}_longterm_model', 'param_group2_ch.csv')
    mapper3 = {
        "LAKEB": "LAKEB_1D",
        "LAKE_ALPHA": "LAKE_ALPHA",
        "CH_N": "CH_N",
        ## 考虑水库
        "RES_LC":"RES_LC",
        "RES_LN":"RES_LN",
        "RES_LF":"RES_LF",
        "RES_ADJUST":"RES_ADJUST",
        "RES_normMult":"RES_normMult",
        "LAKE_MNLWL_1D": "LAKE_MNLWL_1D",
    }
    # gen_param_group_csv(
    #     conn=conn,
    #     db_name=db_name,
    #     collection="PARAMETERS",
    #     mapper=mapper3,
    #     csv_in=caliparam_sub_csv_file,
    #     csv_in_col='subbasin',
    #     csv_out=param_group2_ch_csv_file
    # )

    update_reaches_from_csv(
        conn=conn,
        db_name=db_name,
        collection='REACHES',
        csv_file=param_group2_ch_csv_file
    )


    bug_type = 2

    if bug_type == 0:
        ##--------------------- 重新导入celllat.csv，修复经纬度和投影坐标互换的问题 -----------------------

        csv_file = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347\workspace\csv\celllat.csv'
        prefix = 0
        param_arrays = read_field_arrays_from_csv(csv_file)
        for key, value in list(param_arrays.items()):
            pondVal = value
            import_array_to_mongodb(spatial_gfs, pondVal, '%d_%s' % (prefix, key))
    elif bug_type == 1:
        ##--------------------- 重新导入cellarea.csv，修复面积入库和读取错误的问题 -----------------------
        cellarea_csv = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\csv\cellarea.csv'
        modify_cellarea_csv = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\csv\cellarea_modify.csv'
        sum_cellarea_by_fid(cellarea_csv, modify_cellarea_csv)
        prefix = 0
        param_arrays = read_field_arrays_from_csv(modify_cellarea_csv)
        for key, value in list(param_arrays.items()):
            pondVal = value
            import_array_to_mongodb(spatial_gfs, pondVal, '%d_%s' % (prefix, key))
    elif bug_type == 2:
        ##--------------------- 直接重新导入cellarea.csv，修复spatial.file里的面积和cellarea.csv里不一致的问题--------
        cellarea_csv = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\csv\cellarea.csv'
        # cellarea_csv = r'/data/user/xiaodw/software/WISE/data/poyang_lake1/workspace/csv/cellarea.csv'
        param_arrays = read_field_arrays_from_csv(cellarea_csv)
        for key, value in list(param_arrays.items()):
            pondVal = value
            import_array_to_mongodb(spatial_gfs, pondVal, '%d_%s' % (prefix, key))

