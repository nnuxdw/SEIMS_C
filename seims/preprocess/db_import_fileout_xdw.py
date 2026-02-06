
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

    # Example usage
    documents = [
        {
            "MODULE_CLASS": "Subsurface runoff",
            "OUTPUTID": "solmoist1",
            "DESCRIPTION": "m_soilMoist_1, amount of water stored in soil profile on current day (%)",
            "UNIT": "%",
            "TYPE": "TS",
            "STARTTIME": "2015-01-01 00:00:00",
            "ENDTIME": "2024-12-31 00:00:00",
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
            "STARTTIME": "2015-01-01 00:00:00",
            "ENDTIME": "2024-12-31 00:00:00",
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
            "STARTTIME": "2015-01-01 00:00:00",
            "ENDTIME": "2024-12-31 00:00:00",
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
            "STARTTIME": "2015-01-01 00:00:00",
            "ENDTIME": "2024-12-31 00:00:00",
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
            "STARTTIME": "2015-01-01 00:00:00",
            "ENDTIME": "2024-12-31 00:00:00",
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
            "STARTTIME": "2015-01-01 00:00:00",
            "ENDTIME": "2024-12-31 00:00:00",
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
            "STARTTIME": "2015-01-01 00:00:00",
            "ENDTIME": "2024-12-31 00:00:00",
            "INTERVAL": 1,
            "INTERVAL_UNIT": "DAY",
            "SUBBASIN": "ALL",
            "FILENAME": "solmoist200.txt",
            "USE": 1
        },
    ]
    main(documents)
    # Call the batch insert method
