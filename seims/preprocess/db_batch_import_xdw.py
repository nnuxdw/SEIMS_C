
from preprocess.text import DBTableNames, DataValueFields, DataType,ModelCfgFields
from pymongo import UpdateOne
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
    def workflow(cfg, maindb,documents):
        """Workflow"""
        print('Import Daily File Out Data... ')
        ImportFileOut.batch_upsert_documents(maindb,documents)

def main(documents):
    """TEST CODE"""
    from preprocess.config import parse_ini_configuration
    from preprocess.db_mongodb import ConnectMongoDB
    seims_cfg = parse_ini_configuration()
    client = ConnectMongoDB(seims_cfg.hostname, seims_cfg.port)
    conn = client.get_conn()
    main_db = conn[seims_cfg.spatial_db]
    import time
    st = time.time()
    ImportFileOut.workflow(seims_cfg, main_db,documents)
    et = time.time()
    print(et - st)

    client.close()

if __name__ == '__main__':
    STARTTIME = "2014-08-01 00:00:00"
    ENDTIME= "2014-08-10 00:00:00"
    # Example usage
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

    main(documents_solmoist)
    main(documents_solsat)
    main(documents_solawc)
    main(documents_runoff_percentage)
    main(documents_runoff_co)
    main(documents_ks)
    main(documents_Perco200)

    # Call the batch insert method
