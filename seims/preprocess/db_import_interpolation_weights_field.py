"""Generate weight data for interpolate of hydroclimate data

    @author   : Liangjun Zhu, Junzhi Liu

    @changelog:
    - 16-12-07  - lj - rewrite for version 2.0
    - 17-06-26  - lj - reorganize according to pylint and google style
    - 18-02-08  - lj - compatible with Python3.
"""
from __future__ import absolute_import, unicode_literals

import os
import sys
from io import open
import pandas as pd
if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from math import sqrt, pow
from struct import pack, unpack
import copy

from gridfs import GridFS
from numpy import zeros as np_zeros

from preprocess.db_mongodb import MongoQuery
from preprocess.text import DBTableNames, RasterMetadata, FieldNames, \
    DataType, StationFields, DataValueFields, SubbsnStatsName
from utility import UTIL_ZERO


class ImportWeightData_field(object):
    """Spatial weight and its related data"""

    @staticmethod
    def cal_dis(x1, y1, x2, y2):
        """calculate distance between two points"""
        dx = x2 - x1
        dy = y2 - y1
        return sqrt(dx * dx + dy * dy)

    @staticmethod
    def idw(x, y, loc_list):
        """IDW method for weight
        This function is not used currently"""
        ex = 2
        coef_list = list()
        sum_dist = 0
        for pt in loc_list:
            dis = ImportWeightData_field.cal_dis(x, y, pt[0], pt[1])
            coef = pow(dis, -ex)
            coef_list.append(coef)
            sum_dist += coef
        weight_list = []
        for coef in coef_list:
            weight_list.append(coef / sum_dist)
        fmt = '%df' % (len(weight_list))
        s = pack(fmt, *weight_list)
        return s

    @staticmethod
    def thiessen(x, y, loc_list):
        """Thiessen polygon method for weights"""
        i_min = 0
        coef_list = list()
        if len(loc_list) <= 1:
            coef_list.append(1)
            fmt = '%df' % 1
            return pack(fmt, *coef_list), i_min

        dis_min = ImportWeightData_field.cal_dis(x, y, loc_list[0][0], loc_list[0][1])

        coef_list.append(0)

        for i in range(1, len(loc_list)):
            coef_list.append(0)
            dis = ImportWeightData_field.cal_dis(x, y, loc_list[i][0], loc_list[i][1])
            print(x, ",   ",y, ",   ", loc_list[i][0], ",   ", loc_list[i][1], ",   ", dis)
            if dis < dis_min:
                i_min = i
                dis_min = dis
        coef_list[i_min] = 1
        fmt = '%df' % (len(coef_list))

        s = pack(fmt, *coef_list)
        return s, i_min

    @staticmethod
    def generate_weight_dependent_parameters(conn, maindb, subbsn_id):
        """Generate some parameters dependent on weight data and only should be calculated once.
            Such as PHU0 (annual average total potential heat units)
                TMEAN0 (annual average temperature)
            added by Liangjun, 2016-6-17
        """
        spatial_gfs = GridFS(maindb, DBTableNames.gridfs_spatial)
        # read mask file from mongodb
        mask_name = '%d_MASK' % subbsn_id
        # is MASK existed in Database?
        if not spatial_gfs.exists(filename=mask_name):
            raise RuntimeError('%s is not existed in MongoDB!' % mask_name)
        # read WEIGHT_M file from mongodb
        weight_m_name = '%d_WEIGHT_M' % subbsn_id
        mask = maindb[DBTableNames.gridfs_spatial].files.find({'filename': mask_name})[0]
        weight_m = maindb[DBTableNames.gridfs_spatial].files.find({'filename': weight_m_name})[0]
        num_cells = int(weight_m['metadata'][RasterMetadata.cellnum])
        num_sites = int(weight_m['metadata'][RasterMetadata.site_num])
        # read meteorology sites
        site_lists = maindb[DBTableNames.main_sitelist].find({FieldNames.subbasin_id: subbsn_id})
        site_list = next(site_lists)
        db_name = site_list[FieldNames.db]
        m_list = site_list.get(FieldNames.site_m)
        hydro_clim_db = conn[db_name]

        site_list = m_list.split(',')
        site_list = [int(item) for item in site_list]

        q_dic = {StationFields.id: {'$in': site_list},
                 StationFields.type: DataType.phu0}
        cursor = hydro_clim_db[DBTableNames.annual_stats].find(q_dic).sort(StationFields.id, 1)

        q_dic2 = {StationFields.id: {'$in': site_list},
                  StationFields.type: DataType.mean_tmp0}
        cursor2 = hydro_clim_db[DBTableNames.annual_stats].find(q_dic2).sort(StationFields.id, 1)

        id_list = list()
        phu_list = list()
        for site in cursor:
            id_list.append(site[StationFields.id])
            phu_list.append(site[DataValueFields.value])

        id_list2 = list()
        tmean_list = list()
        for site in cursor2:
            id_list2.append(site[StationFields.id])
            tmean_list.append(site[DataValueFields.value])

        weight_m_data = spatial_gfs.get(weight_m['_id'])
        total_len = num_cells * num_sites
        # print(total_len)
        fmt = '%df' % (total_len,)
        weight_m_data = unpack(fmt, weight_m_data.read())

        # calculate PHU0
        phu0_data = np_zeros(num_cells)
        # calculate TMEAN0
        tmean0_data = np_zeros(num_cells)

        for i in range(num_cells):
            for j in range(num_sites):
                phu0_data[i] += phu_list[j] * weight_m_data[i * num_sites + j]
                tmean0_data[i] += tmean_list[j] * weight_m_data[i * num_sites + j]
        ysize = int(mask['metadata'][RasterMetadata.nrows])
        xsize = int(mask['metadata'][RasterMetadata.ncols])
        nodata_value = mask['metadata'][RasterMetadata.nodata]
        mask_data = spatial_gfs.get(mask['_id'])
        total_len = xsize * ysize
        fmt = '%df' % (total_len,)
        mask_data = unpack(fmt, mask_data.read())
        fname = '%d_%s' % (subbsn_id, DataType.phu0)
        fname2 = '%d_%s' % (subbsn_id, DataType.mean_tmp0)
        if spatial_gfs.exists(filename=fname):
            x = spatial_gfs.get_version(filename=fname)
            spatial_gfs.delete(x._id)
        if spatial_gfs.exists(filename=fname2):
            x = spatial_gfs.get_version(filename=fname2)
            spatial_gfs.delete(x._id)
        meta_dic = copy.deepcopy(mask['metadata'])
        meta_dic['TYPE'] = DataType.phu0
        meta_dic['ID'] = fname
        meta_dic['DESCRIPTION'] = DataType.phu0

        meta_dic2 = copy.deepcopy(mask['metadata'])
        meta_dic2['TYPE'] = DataType.mean_tmp0
        meta_dic2['ID'] = fname2
        meta_dic2['DESCRIPTION'] = DataType.mean_tmp0

        myfile = spatial_gfs.new_file(filename=fname, metadata=meta_dic)
        myfile2 = spatial_gfs.new_file(filename=fname2, metadata=meta_dic2)
        vaild_count = 0
        for i in range(0, ysize):
            cur_row = list()
            cur_row2 = list()
            for j in range(0, xsize):
                index = i * xsize + j
                if abs(mask_data[index] - nodata_value) > UTIL_ZERO:
                    cur_row.append(phu0_data[vaild_count])
                    cur_row2.append(tmean0_data[vaild_count])
                    vaild_count += 1
                else:
                    cur_row.append(nodata_value)
                    cur_row2.append(nodata_value)
            fmt = '%df' % xsize
            myfile.write(pack(fmt, *cur_row))
            myfile2.write(pack(fmt, *cur_row2))
        myfile.close()
        myfile2.close()
        print('Valid Cell Number of subbasin %d is: %d' % (subbsn_id, vaild_count))
        return True

    @staticmethod
    def climate_itp_weight_thiessen(conn, db_model, subbsn_id, geodata2dbdir):
        """Generate and import weight information using Thiessen polygon method.

        Args:
            conn:
            db_model: workflow database object
            subbsn_id: subbasin id
            geodata2dbdir: directory to store weight data as txt file
        """
        spatial_gfs = GridFS(db_model, DBTableNames.gridfs_spatial)
        # read mask file from mongodb
        mask_name = str(subbsn_id) + '_MASK'
        if not spatial_gfs.exists(filename=mask_name):
            raise RuntimeError('%s is not existed in MongoDB!' % mask_name)
        mask = db_model[DBTableNames.gridfs_spatial].files.find({'filename': mask_name})[0]
        ysize = int(mask['metadata'][RasterMetadata.nrows])
        xsize = int(mask['metadata'][RasterMetadata.ncols])
        nodata_value = mask['metadata'][RasterMetadata.nodata]
        dx = mask['metadata'][RasterMetadata.cellsize]
        xll = mask['metadata'][RasterMetadata.xll]
        yll = mask['metadata'][RasterMetadata.yll]

        data = spatial_gfs.get(mask['_id'])

        total_len = xsize * ysize
        fmt = '%df' % (total_len,)
        data = unpack(fmt, data.read())
        # print(data[0], len(data), type(data))

        # count number of valid cells
        num = 0
        for type_i in range(0, total_len):
            if abs(data[type_i] - nodata_value) > UTIL_ZERO:
                num += 1

        # read stations information from database
        metadic = {RasterMetadata.subbasin: subbsn_id,
                   RasterMetadata.cellnum: num}
        site_lists = db_model[DBTableNames.main_sitelist].find({FieldNames.subbasin_id: subbsn_id})
        site_list = next(site_lists)
        clim_db_name = site_list[FieldNames.db]
        p_list = site_list.get(FieldNames.site_p)
        m_list = site_list.get(FieldNames.site_m)
        pet_list = site_list.get(FieldNames.site_pet)
        # print(p_list)
        # print(m_list)
        hydro_clim_db = conn[clim_db_name]

        type_list = [DataType.m, DataType.p, DataType.pet]
        site_lists = [m_list, p_list, pet_list]
        if pet_list is None:
            del type_list[2]
            del site_lists[2]

        # if storm_mode:  # todo: Do some compatible work for storm and longterm models.
        #     type_list = [DataType.p]
        #     site_lists = [p_list]

        for type_i, type_name in enumerate(type_list):
            fname = '%d_WEIGHT_%s' % (subbsn_id, type_name)
            if spatial_gfs.exists(filename=fname):
                x = spatial_gfs.get_version(filename=fname)
                spatial_gfs.delete(x._id)
            site_list = site_lists[type_i]
            if site_list is not None:
                site_list = site_list.split(',')
                # print(site_list)
                site_list = [int(item) for item in site_list]
                metadic[RasterMetadata.site_num] = len(site_list)
                # print(site_list)
                q_dic = {StationFields.id: {'$in': site_list},
                         StationFields.type: type_list[type_i]}
                cursor = hydro_clim_db[DBTableNames.sites].find(q_dic).sort(StationFields.id, 1)

                # meteorology station can also be used as precipitation station
                if cursor.count() == 0 and type_list[type_i] == DataType.p:
                    q_dic = {StationFields.id.upper(): {'$in': site_list},
                             StationFields.type.upper(): DataType.m}
                    cursor = hydro_clim_db[DBTableNames.sites].find(q_dic).sort(StationFields.id, 1)

                # get site locations
                id_list = list()
                loc_list = list()
                for site in cursor:
                    if site[StationFields.id] in site_list:
                        id_list.append(site[StationFields.id])
                        loc_list.append([site[StationFields.x], site[StationFields.y]])
                # print('loclist', locList)
                # interpolate using the locations
                myfile = spatial_gfs.new_file(filename=fname, metadata=metadic)
                txtfile = '%s/weight_%d_%s.txt' % (geodata2dbdir, subbsn_id, type_list[type_i])
                with open(txtfile, 'w', encoding='utf-8') as f_test:
                    for y in range(0, ysize):
                        for x in range(0, xsize):
                            index = int(y * xsize + x)
                            if abs(data[index] - nodata_value) > UTIL_ZERO:
                                x_coor = xll + x * dx
                                y_coor = yll + (ysize - y - 1) * dx
                                line, near_index = ImportWeightData_field.thiessen(x_coor, y_coor,
                                                                             loc_list)
                                myfile.write(line)
                                fmt = '%df' % (len(loc_list))
                                f_test.write('%f %f %s\n' % (x, y, unpack(fmt, line).__str__()))
                myfile.close()

    @staticmethod
    def generate_weight_dependent_parameters_field(conn, maindb, subbsn_id, field_center_list):
        """Generate some parameters dependent on weight data and only should be calculated once.
            Such as PHU0 (annual average total potential heat units)
                TMEAN0 (annual average temperature)
            added by Liangjun, 2016-6-17
        """
        row, num = field_center_list.shape
        spatial_gfs = GridFS(maindb, DBTableNames.gridfs_spatial)
        # read mask file from mongodb
        mask_name = '%d_MASK' % subbsn_id
        # is MASK existed in Database?
        # if not spatial_gfs.exists(filename=mask_name):
        #     raise RuntimeError('%s is not existed in MongoDB!' % mask_name)
        # read WEIGHT_M file from mongodb
        weight_m_name = '%d_WEIGHT_M' % subbsn_id
        weight_id_name = '%d_WEIGHT_ID_M' % subbsn_id# wanghaocheng
        mask = maindb[DBTableNames.gridfs_spatial].files.find({'filename': mask_name})[0]
        weight_m = maindb[DBTableNames.gridfs_spatial].files.find({'filename': weight_m_name})[0]
        weight_m_id = maindb[DBTableNames.gridfs_spatial].files.find({'filename': weight_id_name})[0] # wanghaocheng
        num_cells = int(weight_m['metadata'][RasterMetadata.cellnum])
        num_sites = int(weight_m['metadata'][RasterMetadata.site_num])
        temp_C = int(weight_m['metadata']['NUM_SITE_WEIGHT'])
        # read meteorology sites
        site_lists = maindb[DBTableNames.main_sitelist].find({FieldNames.subbasin_id: subbsn_id})
        site_list = next(site_lists)
        db_name = site_list[FieldNames.db]
        m_list = site_list.get(FieldNames.site_m)
        hydro_clim_db = conn[db_name]

        site_list = m_list.split(',')
        site_list = [int(item) for item in site_list]

        q_dic = {StationFields.id: {'$in': site_list},
                 StationFields.type: DataType.phu0}
        cursor = hydro_clim_db[DBTableNames.annual_stats].find(q_dic).sort(StationFields.id, 1)

        q_dic2 = {StationFields.id: {'$in': site_list},
                  StationFields.type: DataType.mean_tmp0}
        cursor2 = hydro_clim_db[DBTableNames.annual_stats].find(q_dic2).sort(StationFields.id, 1)

        id_list = list()
        phu_list = list()
        for site in cursor:
            id_list.append(site[StationFields.id])
            phu_list.append(site[DataValueFields.value])

        id_list2 = list()
        tmean_list = list()
        for site in cursor2:
            id_list2.append(site[StationFields.id])
            tmean_list.append(site[DataValueFields.value])

        weight_m_data = spatial_gfs.get(weight_m['_id'])
        weight_m_id_data = spatial_gfs.get(weight_m_id['_id'])
        total_len = num_cells * temp_C
        # print(total_len)
        fmt = '%df' % (total_len,)
        weight_m_data = unpack(fmt, weight_m_data.read())
        weight_m_id_data = unpack(fmt, weight_m_id_data.read())
        #wanghaocheng
        # calculate PHU0
        phu0_data = np_zeros(num_cells)
        # calculate TMEAN0
        tmean0_data = np_zeros(num_cells)
        for i in range(num_cells):
            for j in range(temp_C):
                k = int(weight_m_id_data[i * temp_C + j])        #wanghaocheng
                # print(k)
                phu0_data[i] += phu_list[k] * weight_m_data[i * temp_C + j]        #wanghaocheng
                tmean0_data[i] += tmean_list[k] * weight_m_data[i * temp_C + j]        #wanghaocheng
        # ysize = int(mask['metadata'][RasterMetadata.nrows])
        # xsize = int(mask['metadata'][RasterMetadata.ncols])
        # nodata_value = mask['metadata'][RasterMetadata.nodata]
        # mask_data = spatial_gfs.get(mask['_id'])
        # total_len = xsize * ysize
        # fmt = '%df' % (total_len,)
        # mask_data = unpack(fmt, mask_data.read())
        fname = '%d_%s' % (subbsn_id, DataType.phu0)
        fname2 = '%d_%s' % (subbsn_id, DataType.mean_tmp0)
        if spatial_gfs.exists(filename=fname):
            x = spatial_gfs.get_version(filename=fname)
            spatial_gfs.delete(x._id)
        if spatial_gfs.exists(filename=fname2):
            x = spatial_gfs.get_version(filename=fname2)
            spatial_gfs.delete(x._id)
        meta_dic = copy.deepcopy(mask['metadata'])
        meta_dic['TYPE'] = DataType.phu0
        meta_dic['ID'] = fname
        meta_dic['DESCRIPTION'] = DataType.phu0

        meta_dic2 = copy.deepcopy(mask['metadata'])
        meta_dic2['TYPE'] = DataType.mean_tmp0
        meta_dic2['ID'] = fname2
        meta_dic2['DESCRIPTION'] = DataType.mean_tmp0

        myfile = spatial_gfs.new_file(filename=fname, metadata=meta_dic)
        myfile2 = spatial_gfs.new_file(filename=fname2, metadata=meta_dic2)
        vaild_count = 0
        for i in range(0, num):
            cur_row = list()
            cur_row2 = list()
            cur_row.append(phu0_data[vaild_count])
            cur_row2.append(tmean0_data[vaild_count])
            vaild_count += 1
            fmt = '%df' % 1
            myfile.write(pack(fmt, *cur_row))
            myfile2.write(pack(fmt, *cur_row2))
        myfile.close()
        myfile2.close()
        print('Valid Cell Number of subbasin %d is: %d' % (subbsn_id, vaild_count))
        return True

    @staticmethod
    def climate_itp_weight_thiessen_field(conn, db_model, geodata2dbdir, field_center_list):
        """Generate and import weight information using Thiessen polygon method.

        Args:
            conn:
            db_model: workflow database object
            subbsn_id: subbasin id  (subbasin id is zero in field-version)
            geodata2dbdir: directory to store weight data as txt file
        """
        subbsn_id = 0
        spatial_gfs = GridFS(db_model, DBTableNames.gridfs_spatial)
        # read mask file from mongodb
        # mask_name = str(subbsn_id) + '_MASK'
        # if not spatial_gfs.exists(filename=mask_name):
            # raise RuntimeError('%s is not existed in MongoDB!' % mask_name)
        # mask = db_model[DBTableNames.gridfs_spatial].files.find({'filename': mask_name})[0]
        # ysize = int(mask['metadata'][RasterMetadata.nrows])
        # xsize = int(mask['metadata'][RasterMetadata.ncols])
        # nodata_value = mask['metadata'][RasterMetadata.nodata]
        # dx = mask['metadata'][RasterMetadata.cellsize]
        # xll = mask['metadata'][RasterMetadata.xll]
        # yll = mask['metadata'][RasterMetadata.yll]

        # data = spatial_gfs.get(mask['_id'])

        # total_len = xsize * ysize
        # fmt = '%df' % (total_len,)
        # data = unpack(fmt, data.read())
        # print(data[0], len(data), type(data))

        # count number of valid cells  field number
        row, num = field_center_list.shape

        # read stations information from database
        metadic = {RasterMetadata.subbasin: subbsn_id,
                   RasterMetadata.cellnum: num}
        site_lists = db_model[DBTableNames.main_sitelist].find({FieldNames.subbasin_id: subbsn_id})
        site_list = next(site_lists)
        clim_db_name = site_list[FieldNames.db]
        p_list = site_list.get(FieldNames.site_p)
        m_list = site_list.get(FieldNames.site_m)
        pet_list = site_list.get(FieldNames.site_pet)
        # print(p_list)
        # print(m_list)
        hydro_clim_db = conn[clim_db_name]

        type_list = [DataType.m, DataType.p, DataType.pet]
        site_lists = [m_list, p_list, pet_list]
        if pet_list is None:
            del type_list[2]
            del site_lists[2]

        # if storm_mode:  # todo: Do some compatible work for storm and longterm models.
        #     type_list = [DataType.p]
        #     site_lists = [p_list]

        for type_i, type_name in enumerate(type_list):
            fname = '%d_WEIGHT_%s' % (subbsn_id, type_name)
            # wanghaocheng
            fname1 = '%d_WEIGHT_ID_%s' % (subbsn_id, type_name)
            # wanghaocheng
            if spatial_gfs.exists(filename=fname):
                x = spatial_gfs.get_version(filename=fname)
                spatial_gfs.delete(x._id)
            # wanghaocheng
            if spatial_gfs.exists(filename=fname1):
                x = spatial_gfs.get_version(filename=fname1)
                spatial_gfs.delete(x._id)
            # wanghaocheng
            site_list = site_lists[type_i]
            if site_list is not None:
                site_list = site_list.split(',')
                # print(site_list)
                site_list = [int(item) for item in site_list]
                metadic[RasterMetadata.site_num] = len(site_list)
                #wang haocheng
                # import argparse
                # arg= argparse.ArgumentParser()
                # arg.add_argument('-name', type=str, help='Name of demo watershed')
                # arg.add_argument('-num_site_weight', type=int, help='Number of site weights')
                # args = arg.parse_args()
                temp_C = 3
                # if args.num_site_weight is None:
                #     temp_C = 1
                # else:
                #     temp_C = args.num_site_weight
                if temp_C>len(site_list):
                    metadic['NUM_SITE_WEIGHT'] = len(site_list)
                else:
                    metadic['NUM_SITE_WEIGHT'] = temp_C
                #wang haocheng

                q_dic = {StationFields.id: {'$in': site_list},
                         StationFields.type: type_list[type_i]}
                cursor = hydro_clim_db[DBTableNames.sites].find(q_dic).sort(StationFields.id, 1)

                # meteorology station can also be used as precipitation station
                if cursor.count() == 0 and type_list[type_i] == DataType.p:
                    q_dic = {StationFields.id.upper(): {'$in': site_list},
                             StationFields.type.upper(): DataType.m}
                    cursor = hydro_clim_db[DBTableNames.sites].find(q_dic).sort(StationFields.id, 1)

                # get site locations
                id_list = list()
                loc_list = list()
                for site in cursor:
                    if site[StationFields.id] in site_list:
                        id_list.append(site[StationFields.id])
                        loc_list.append([site[StationFields.x], site[StationFields.y]])
                # print('loclist', locList)
                # interpolate using the locations
                myfile = spatial_gfs.new_file(filename=fname, metadata=metadic)
                txtfile = '%s/weight_%d_%s_field.txt' % (geodata2dbdir, subbsn_id, type_list[type_i])
                #wanghaocheng
                myfile1 = spatial_gfs.new_file(filename=fname1, metadata=metadic)
                txtfile1 = '%s/weight_id_%d_%s.txt' % (geodata2dbdir, subbsn_id, type_list[type_i])
                f_test1=open(txtfile1, 'w', encoding='utf-8')
                # wanghaocheng
                with open(txtfile, 'w', encoding='utf-8') as f_test:
                    for key in range(num):
                        #print(type_name,num)
                        # print(field_center_list[num])
                        # print(key)
                        x_coor, y_coor = field_center_list.loc[:,str(key)]
                        # wanghaocheng
                        line, near_index, num1 = ImportWeightData_field.inverseDistanceWeighting(x_coor, y_coor,
                                                                                            loc_list, temp_C)
                        myfile.write(line)
                        # fmt = '%df' % (len(loc_list))
                        # f_test.write('%f %s\n' % (key, unpack(fmt, line).__str__()))
                        # wanghaocheng
                        myfile1.write(near_index)
                        fmt = '%df' % (num1)
                        f_test.write('%f %s\n' % (key, unpack(fmt, line).__str__()))
                        f_test1.write('%f %s\n' % (key, unpack(fmt, near_index).__str__()))
                        # wanghaocheng
                myfile.close()
                # wanghaocheng
                myfile1.close()
                f_test1.close()
                # wanghaocheng

    @staticmethod
    def inverseDistanceWeighting(x, y, loc_list,C=None):
        """在Thiessen方法基础上改的，用于反距离计算权重"""
        i_min = list()
        coef_list = list()
        if len(loc_list) <= 1 :
            coef_list.append(1)
            i_min.append(0)
            fmt = '%df' % 1
            fmt1 = '%df' % 1
            return pack(fmt, *coef_list), pack(fmt1, *i_min),1
        #使用气象站点的数量
        if C is None or len(loc_list) <= C:
            C = len(loc_list)
        dis_min = ImportWeightData_field.cal_dis(x, y, loc_list[0][0], loc_list[0][1])
        distance=list()
        distance.append([0,dis_min])
        for i in range(1, len(loc_list)):
            dis = ImportWeightData_field.cal_dis(x, y, loc_list[i][0], loc_list[i][1])
            distance.append([i, dis])
        distance.sort(key=lambda x: x[1])#, reverse=True
        first_C_items = distance[:C]

        coef=list()
        index = list()
        for i in range(C):
            first_C_items[i][1]=1 / first_C_items[i][1]
        sum_weight = sum(sublist[1] for sublist in first_C_items)
        for i in range(C):
            first_C_items[i][1]= first_C_items[i][1]/sum_weight
            coef.append( first_C_items[i][1])
            index.append(first_C_items[i][0])
        num=len(coef)
        fmt = '%df' % (len(coef))
        s = pack(fmt, *coef)
        index1 = pack(fmt, *index)
        return s, index1,num

    @staticmethod
    def workflow(cfg, field_center,db_name):
        """Workflow"""
        from preprocess.db_mongodb import ConnectMongoDB
        client = ConnectMongoDB(cfg.hostname, cfg.port)
        conn = client.get_conn()
        db_model_field = conn[db_name]
        # db_model = conn[seims_cfg.spatial_db]
        geodata2dbdir = cfg.dirs.geodata2db
        # field_center = r'G:\codes\SEIMS-master\data\youwuzhen\workspace\temp1\fields_center.csv'
        field_center_list = pd.read_csv(field_center)
        #修改sitelist

        site_lists = db_model_field[DBTableNames.main_sitelist].find({FieldNames.subbasin_id: 0})
        site_list = next(site_lists)
        clim_db_name = site_list[FieldNames.db]
        # print(clim_db_name)
        hydro_clim_db = conn[clim_db_name]
        q_dic1 = {StationFields.type: 'M'}
        cursor1 = hydro_clim_db[DBTableNames.sites].find(q_dic1).sort(StationFields.id, 1)
        q_dic2 = {StationFields.type: 'P'}
        cursor2 = hydro_clim_db[DBTableNames.sites].find(q_dic2).sort(StationFields.id, 1)
        SITELISTM = []
        SITELISTP = []
        for site in cursor1:
            SITELISTM.append(site[StationFields.id])
        for site in cursor2:
            SITELISTP.append(site[StationFields.id])
        slist1 = [str(item) for item in SITELISTM]
        site_list_str1 = ','.join(slist1)
        slist2 = [str(item) for item in SITELISTP]
        site_list_str2 = ','.join(slist2)
        db_model_field[DBTableNames.main_sitelist].find_one_and_update({'SUBBASINID': 0},{'$set': {'SITELISTM': site_list_str1}})
        db_model_field[DBTableNames.main_sitelist].find_one_and_update({'SUBBASINID': 0},{'$set': {'SITELISTP': site_list_str2}})

        ImportWeightData_field.climate_itp_weight_thiessen_field(conn, db_model_field, geodata2dbdir, field_center_list)
        ImportWeightData_field.generate_weight_dependent_parameters_field(conn, db_model_field, 0, field_center_list)
        print('Import WEIGHT done!')


def main():
    """TEST CODE"""
    from preprocess.config import parse_ini_configuration
    from preprocess.db_mongodb import ConnectMongoDB
    seims_cfg = parse_ini_configuration()
    client = ConnectMongoDB(seims_cfg.hostname, seims_cfg.port)
    conn = client.get_conn()

    # base_dir = r'/data/user/xiaodw/software/WISE/data/poyang_lake1'
    base_dir = r'G:\program\seims\SEIMS_HAND\data\US_6'
    csv_path = base_dir + os.sep + 'workspace/csv'
    field_center_file = csv_path + os.sep + 'fields_center.csv'
    ImportWeightData_field.workflow(seims_cfg, field_center_file, 'US_6_longterm_model')

    client.close()

if __name__ == "__main__":
    main()

