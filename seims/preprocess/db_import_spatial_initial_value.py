import geopandas as gpd
from pymongo import MongoClient


from pymongo import MongoClient
import geopandas as gpd

def update_parameters_from_shp(shp_file, param_map, db_name, collection_name):
    """
    从输入的shp面文件读取字段，按HRU ID排序，然后获取给定参数map的默认值并写入MongoDB。
    如果参数记录已存在，则不会插入或修改。

    参数:
    shp_file: SHP 文件路径。
    param_map: 一个字典，键是参数名称，值是参数的默认值。
    db_name: 数据库名称。
    collection_name: 集合名称（即表名）。
    """

    # 读取shp文件
    gdf = gpd.read_file(shp_file)

    # 确保SH文件中包含HRU ID字段（假设字段名是 'HRU_ID'）
    if 'HRU_ID' not in gdf.columns:
        raise ValueError("SHP文件中没有找到 'HRU_ID' 字段。")

    # 获取HRU ID并按升序排序
    sorted_hru_ids = sorted(gdf['HRU_ID'].unique())

    # 连接MongoDB
    client = MongoClient('mongodb://localhost:27017/')
    db = client[db_name]
    collection = db[collection_name]

    # 对每个参数，按HRU ID的顺序填充参数值并写入数据库
    for param, default_value in param_map.items():
        for hru_id in sorted_hru_ids:
            # 检查是否已经存在该HRU_ID和参数的记录
            existing_record = collection.find_one({"HRU_ID": hru_id, param: {"$exists": True}})
            if existing_record:
                print(f"HRU_ID: {hru_id} with {param} already exists. Skipping insert.")
                continue  # 如果记录已存在，则跳过插入

            # 如果记录不存在，插入新数据
            data = {
                "HRU_ID": hru_id,
                param: default_value
            }
            collection.insert_one(data)
            print(f"Inserted HRU_ID: {hru_id} with {param}: {default_value}")

    print("数据插入完成。")


if __name__ == '__main__':

    # 创建 HRU 参数的默认值字典
    param_map_hru = {
        "K_PET_1D": 1.0, # 已有
        "RUNOFF_CO": -9999,
        "C_SNOW12_1D": 6.5, # 已有
        "C_SNOW6_1D": 2.5, # 已有
        "LAG_SNOW_1D": 0.8,# 已有
        "T0_1D": 1.0,# 已有
        "T_SNOW_1D": 0.0,
        "KI_1D": 3.0,# 已有
        "SURLAG_1D": 1.0# 已有
    }

    # 创建 Subbasin 参数的默认值字典
    param_map_subbasin = {
        "BASE_EX_1": 1.0,# 已有
        "KG_1D": 0.005,# 已有
        "GW_DELAY_1D": 31.0,# 已有
        "LAKEB_1D": 1.0,
        # "LAKE_ALPHA": -9999,#REACHES里有
        # "CH_N": -9999,#REACHES里有
        "EP_CH_1D": 1.0,# 已有
        # "LAKE_MNLWL_1D": 0.7,
        # "RES_LC": -9999,
        # "RES_LN": -9999,
        # "RES_LF": -9999,
        # "RES_ADJUST": -9999,
        # "RES_normMult": -9999,
        # "RES_minq": -9999,
        # "RES_normq": -9999,
        # "RES_ndq": -9999,
        "GWMAX_1D": 300.0
    }

    # 导入数据库中
    from config import parse_ini_configuration
    import db_import_field_arrays

    seims_cfg = parse_ini_configuration()
    # db_name = 'TP_13_longterm_model'
    # csv_path = r'G:\program\seims\SEIMS_HAND\data\TP_13\workspace\csv3'
    db_name = 'TH_4_longterm_model'
    csv_path = r'G:\program\seims\SEIMS_HAND\data\TH_4\workspace\csv3'
    field_num = 338
    db_import_field_arrays.workflow(seims_cfg, db_name, csv_path, field_num)
