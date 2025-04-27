
from preprocess.cal_zonal_soil_properties import zonal_statistic_mean_by_hru
from preprocess.cal_zonal_soil_properties import batch_clip_raster_by_polygon_gdal

if __name__=='__main__':

    raster_folder = r'G:\data\土壤所90m土壤数据\soil_type'
    tar_clip_folder = r'G:\program\gannan\data\gongba\soil\soil_type'
    polygon_file = r'G:\program\gannan\data\gongba\extent\gongba_contour_proj_4soil.shp'
    hru_file = r'G:\program\gannan\data\gongba\gen_mesh\2_hand_modify\hand_modify.shp'
    # 裁剪
    batch_clip_raster_by_polygon_gdal(raster_folder, polygon_file, tar_clip_folder)
    # 投影
    # ...
    # 统计每个tif中，每个HRU的bd、om、clay、silt、sand、rock
    projected_raster_files = r'G:\program\gannan\data\gongba\soil\soil_type_albers\*\*.tif'
    tar_csv_file = r'G:\program\gannan\data\gongba\soil\soil_type_albers\sol_para_extract.csv'
    zonal_statistic_mean_by_hru(projected_raster_files, hru_file, tar_csv_file)

    # workflow()
