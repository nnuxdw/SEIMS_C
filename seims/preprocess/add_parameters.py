import os
import os.path
import numpy as np
import pandas as pd
from pymongo import MongoClient
# import db_import_field_arrays

# subbasin_param_csv = './subbasin.csv'

# db_import_field_arrays.import_array_to_mongodb(spatial_gfs, mask_array, mask_name)
# 
conn = MongoClient('127.0.0.1', 27017)
db = conn.hlg_hband_longterm_model
is_permafrost = [0,1,1,1,1,0,0,0,0,1,0,1]
CH_BNK_K = [0.0001,0.001,0.001,0.001,0.001,0.0001,0.0001,0.0001,0.0001,0.001,0.0001,0.001]
GW_SPYLD = [0.0048,0.012,0.012,0.012,0.012,0.0048,0.0048,0.0048,0.0048,0.012,0.0048,0.012]
Is_Lake = [0,0,0,0,0,0,0,0,0,0,0,0]
Lake_Area = [100000000,0,0,0,0,0,0,0,0,0,0,0]
Lake_Vol = [200000000,0,0,0,0,0,0,0,0,0,0,0]
Lake_Depini = [1,0,0,0,0,0,0,0,0,0,0,0]
Lake_Alpha = [1,0,0,0,0,0,0,0,0,0,0,0]
Is_Res = [0,0,0,0,0,0,0,0,0,0,0,0]
Nature_Flow = [5,0,0,0,0,0,0,0,0,0,0,0]
RES_LC = [0.10,0,0,0,0,0,0,0,0,0,0,0]
RES_LN = [0.30,0,0,0,0,0,0,0,0,0,0,0]
RES_LF = [0.97,0,0,0,0,0,0,0,0,0,0,0]
RES_ADJUST = [0.7,0,0,0,0,0,0,0,0,0,0,0]
RES_NSED = [12,0,0,0,0,0,0,0,0,0,0,0]

for i in range(1,13):
	db["REACHES"].update({'SUBBASINID': i}, {'$set': {"is_permafrost": is_permafrost[i-1]}},False,True)
	db["REACHES"].update({'SUBBASINID': i}, {'$set': {"GW_SPYLD": GW_SPYLD[i-1]}},False,True)
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'CH_BNK_K': CH_BNK_K[i-1]}})#0.95
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'Is_Lake': Is_Lake[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'Lake_Area': Lake_Area[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'Lake_Vol': Lake_Vol[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'Lake_Depini': Lake_Depini[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'Lake_Alpha': Lake_Alpha[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'Is_Res': Is_Res[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'Nature_Flow': Nature_Flow[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'RES_LC': RES_LC[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'RES_LN': RES_LN[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'RES_LF': RES_LF[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'RES_ADJUST': RES_ADJUST[i-1]}})
	db["REACHES"].find_one_and_update({'SUBBASINID': i},{'$set': {'RES_NSED': RES_NSED[i-1]}})
