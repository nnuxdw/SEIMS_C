
from preprocess.text import DBTableNames, DataValueFields, DataType,ModelCfgFields
from pymongo import UpdateOne,UpdateMany
import os
import sys
if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))
class ImportFileOut(object):

    def batch_upsert_documents(maindb, documents):
        # Select the collection
        collection = maindb[DBTableNames.main_fileout]

        # Create a list of update operations
        operations = []

        for document in documents:
            # Assume 'OUTPUTID' is the unique field to check for existence
            filter_query = {ModelCfgFields.output_id: document[ModelCfgFields.output_id]}  # Adjust field name as needed
            update_query = {
                "$set": document  # Update the document with the new data
            }
            operations.append(UpdateOne(filter_query, update_query, upsert=True))

        # Perform the batch upsert operation
        result = collection.bulk_write(operations)

        # Print out the number of upserts
        print(f"Upserted {result.upserted_count} documents")
        print(f"Modified {result.modified_count} documents")
        return result
    @staticmethod
    def batch_upsert_fileout_workflow( maindb, documents):
        """Workflow"""
        print('Import Daily File Out Data... ')
        ImportFileOut.batch_upsert_documents(maindb,documents)

    @staticmethod
    def batch_update_documents(maindb, filter_queries, update_data):
        # 选择数据库集合
        collection = maindb[DBTableNames.main_fileout]

        # 创建更新操作的列表
        operations = []

        for filter_query, update_query in zip(filter_queries, update_data):
            operations.append(UpdateMany(filter_query, update_query))

        # 执行批量更新操作
        result = collection.bulk_write(operations)

        # 输出已修改的文档数
        print(f"修改了 {result.modified_count} 个文档")
        return result
    @staticmethod
    def batch_update_fileout_workflow( maindb,query,update):
        """执行批量更新的工作流"""
        print('执行批量更新：文件输出数据... ')
        ImportFileOut.batch_update_documents(maindb, query, update)


def upsert_fileout_main(spatial_db,hostname,port,documents):
    """TEST CODE"""
    from preprocess.db_mongodb import ConnectMongoDB

    client = ConnectMongoDB(hostname, port)
    conn = client.get_conn()
    main_db = conn[spatial_db]
    import time
    st = time.time()
    ImportFileOut.batch_upsert_fileout_workflow( main_db, documents)
    et = time.time()
    print(et - st)
    client.close()

def update_fileout_main(spatial_db,hostname,port,query,update):
    """TEST CODE"""
    from preprocess.db_mongodb import ConnectMongoDB

    client = ConnectMongoDB(hostname, port)
    conn = client.get_conn()
    main_db = conn[spatial_db]
    import time
    st = time.time()
    ImportFileOut.batch_update_fileout_workflow(main_db,query,update)
    et = time.time()
    print(et - st)
    client.close()

if __name__ == '__main__':
    # host = '127.0.0.1'
    # port = 27017
    host = '172.21.124.127'
    port = 27019
    spatial_dbs = ['US_2_longterm_model', 'US_3_longterm_model','US_4_longterm_model', 'US_5_longterm_model', 'US_6_longterm_model',
                'US_7_longterm_model', 'US_10_longterm_model', 'US_11_longterm_model',
                'US_12_longterm_model', 'US_14_longterm_model', 'US_15_longterm_model',
                'US_16_longterm_model', 'US_17_longterm_model', 'US_18_longterm_model']
    STARTTIME = "2010-01-01 00:00:00"
    ENDTIME= "2024-12-31 00:00:00"
    # 插入File.out
    documents_solmoist = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist1",
            "DESCRIPTION": "m_soilMoist_1, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist1.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist5",
            "DESCRIPTION": "m_soilMoist_5, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist5.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist15",
            "DESCRIPTION": "m_soilMoist_15, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist15.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist30",
            "DESCRIPTION": "m_soilMoist_30, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist30.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist60",
            "DESCRIPTION": "m_soilMoist_60, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist60.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist100",
            "DESCRIPTION": "m_soilMoist_100, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist100.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist200",
            "DESCRIPTION": "m_soilMoist_200, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist200.txt",
            "USE": 1
        },
    ]

    documents_solsat = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solsat1",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solsat1.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solsat5",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solsat5.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solsat15",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solsat15.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solsat30",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solsat30.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solsat60",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solsat60.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solsat100",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solsat100.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solsat200",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solsat200.txt",
            "USE": 1
        },
    ]

    documents_solawc = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solawc1",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solawc1.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solawc5",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solawc5.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solawc15",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solawc15.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solawc30",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solawc30.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solawc60",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solawc60.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solawc100",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solawc100.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solawc200",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solawc200.txt",
            "USE": 1
        },
    ]

    documents_ks = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "ks1",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "ks1.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "ks5",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "ks5.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "ks15",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "ks15.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "ks30",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "ks30.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "ks60",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "ks60.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "ks100",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "ks100.txt",
            "USE": 1
        },
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "ks200",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "ks200.txt",
            "USE": 1
        },
    ]

    documents_runoff_percentage = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "RUNOFF_PERCENTAGE",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "RUNOFF_PERCENTAGE.txt",
            "USE": 1
        },
    ]

    documents_runoff_co = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "Runoff_co",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "Runoff_co.txt",
            "USE": 1
        },
    ]

    documents_Perco200 = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "Perco200",
            "DESCRIPTION": "todo",
            "UNIT": "mm",
            "TYPE": "TS",
            "STARTTIME": STARTTIME,
            "ENDTIME": ENDTIME,
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "Perco200.txt",
            "USE": 1
        },
    ]

    # 把所有USE为1的先设为0
    filter_queries1 = [
        {"USE": {"$in": [1, "1"]}}
    ]

    update_data1 = [
        {
            "$set": {
                "USE": 0
            }
        }
    ]

    filter_queries2 = [
        # {"OUTPUTID": {"$in": [
        #     "QRECH", "SBGS", "GWWB", "QS", "QI", "QG", "CHWTRDEPTH", "CHWTRWIDTH",
        #     "solst", "FieldCapDepth", "PorosityDepth", "Perco", "sol_awc", "solmoist1",
        #     "solmoist5", "solmoist15", "solmoist30", "solmoist60", "solmoist100", "solmoist200",
        #     "RUNOFF_PERCENTAGE", "Runoff_co",
        # ]}},
        {"OUTPUTID": {"$in": [
            "QRECH","SBGS",
            # "SBGS","GWWB", "QS", "QI", "QG",
            "solmoist1","solmoist5", "solmoist15", "solmoist30", "solmoist60", "solmoist100", "solmoist200",

        ]}},
    ]

    update_data2 = [
        {
            "$set": {
                "USE": "1",
                "STARTTIME": STARTTIME,
                "ENDTIME": ENDTIME
            }
        },
    ]

    for spatial_db in spatial_dbs:
        upsert_fileout_main(spatial_db,host,port,documents_solmoist)
        upsert_fileout_main(spatial_db,host,port,documents_solsat)
        upsert_fileout_main(spatial_db,host,port,documents_solawc)
        upsert_fileout_main(spatial_db,host,port,documents_runoff_percentage)
        upsert_fileout_main(spatial_db,host,port,documents_runoff_co)
        upsert_fileout_main(spatial_db,host,port,documents_ks)
        upsert_fileout_main(spatial_db,host,port,documents_Perco200)

        # 更新File.out
        update_fileout_main(spatial_db,host,port,filter_queries1,update_data1)
        update_fileout_main(spatial_db,host,port,filter_queries2,update_data2)
