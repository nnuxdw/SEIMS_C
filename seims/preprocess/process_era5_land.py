import os.path

import pandas as pd
import numpy as np
from datetime import datetime
import pyproj
import rasterio

from pathlib import Path
# 读取图1格式的CSV文件

def process_data(input_csv, output_csv2, output_csv3):
    # 读取输入的CSV文件
    with open(input_csv, 'r') as f:
        lines = f.readlines()

    # 提取数据：日期、温度、降水量等（假设格式正确）
    data = []
    for line in lines[1:]:  # 从第二行开始读取数据，跳过标题行
        parts = line.strip().split(',')
        if len(parts) >= 11:  # 确保数据格式正确
            data.append(parts)

    # --------------------------
    # 第一步：预计算所有月份的最终极值（整月最大/最小温度），缓存到字典
    # --------------------------
    month_extremes = {}  # 缓存格式：{"2024-01": (整月最大温, 整月最小温), ...}
    current_ym = None
    temp_tmax = []  # 存储当月所有当日最高温
    temp_tmin = []  # 存储当月所有当日最低温

    for row in data:
        date_obj = datetime.strptime(row[1], '%Y-%m-%d')
        ym = date_obj.strftime('%Y-%m')
        tmax_C = float(row[6])
        tmin_C = float(row[7])

        if ym != current_ym:
            # 切换月份，计算上一个月的最终极值并缓存
            if current_ym is not None and temp_tmax and temp_tmin:
                month_extremes[current_ym] = (max(temp_tmax), min(temp_tmin))
            # 重置当月温度列表
            current_ym = ym
            temp_tmax = []
            temp_tmin = []
        # 收集当月所有当日最高/最低温
        temp_tmax.append(tmax_C)
        temp_tmin.append(tmin_C)
    # 处理最后一个月的极值缓存
    if current_ym is not None and temp_tmax and temp_tmin:
        month_extremes[current_ym] = (max(temp_tmax), min(temp_tmin))

    # --------------------------
    # 第二步：初始化输出数据结构
    # --------------------------
    # 降水量输出（csv2）- 每日当日值
    output_data_csv2 = [["DATETIME", "0"]]
    # 气象数据输出（csv3）- 每日当日值+整月极值
    output_data_csv3 = [
        ["StationID", "DATETIME", "TMEAN", "TMAX", "TMIN", "RM", "WS", "SR", "MAXMONT", "MINMONT"]
    ]

    # --------------------------
    # 第三步：处理每日数据，匹配月度极值并写入输出
    # --------------------------
    for row in data:
        # 日期提取与格式化
        date_obj = datetime.strptime(row[1], '%Y-%m-%d')
        formatted_date = date_obj.strftime('%Y/%m/%d %H:%M:%S')
        current_ym = date_obj.strftime('%Y-%m')
        # 获取当前月份的最终极值（整月）
        month_max, month_min = month_extremes[current_ym]

        # 提取当日原始气象数据并转浮点
        tavg_C = float(row[4])  # 当日平均温度（TMEAN）
        tdew_C = float(row[5])  # 当日露点温度
        tmax_C = float(row[6])  # 当日最高温度（TMAX）
        tmin_C = float(row[7])  # 当日最低温度（TMIN）

        # 计算当日的RM/WS/SR（均为当日值，无累计）
        RM = (tavg_C + tdew_C) / 2    # 当日相对湿度
        WS = np.abs(tmax_C - tmin_C) / 10  # 当日风速
        SR = tavg_C * 0.1            # 当日太阳辐射

        # 写入csv2：降水量当日值（原逻辑不变）
        output_data_csv2.append([formatted_date, row[2]])

        # 写入csv3：【全当日值】+【整月极值】（核心需求）
        output_data_csv3.append([
            0,                  # StationID 固定0
            formatted_date,     # DATETIME 日期时间
            tavg_C,             # TMEAN 当日平均温（原tavg_C）
            tmax_C,             # TMAX 当日最高温
            tmin_C,             # TMIN 当日最低温
            RM,                 # RM 当日计算值
            WS,                 # WS 当日计算值
            SR,                 # SR 当日计算值
            month_max,          # MAXMONT 整月最终最高温（非累计）
            month_min           # MINMONT 整月最终最低温（非累计）
        ])

    # --------------------------
    # 第四步：写入输出CSV文件
    # --------------------------
    # 写入气象数据csv3（保留2位小数，优化可读性，可按需调整）
    with open(output_csv3, 'w', newline='') as f_out_csv3:
        f_out_csv3.write('#UTCTIME\n')
        for row in output_data_csv3:
            # 对数值型字段保留2位小数，避免浮点数精度问题
            row_formatted = []
            for val in row:
                if isinstance(val, float):
                    row_formatted.append(f"{val:.2f}")
                else:
                    row_formatted.append(str(val))
            f_out_csv3.write(','.join(row_formatted) + '\n')

    # 写入降水量数据csv2（原逻辑不变，保留#UTCTIME头）
    with open(output_csv2, 'w', newline='') as f_out:
        f_out.write("#UTCTIME\n")
        for row in output_data_csv2:
            f_out.write(','.join(row) + '\n')


def calculate_box_centers(coords, decimals=None):
    """
    计算每个矩形坐标框的经纬度中心点
    :param coords: 原始坐标数组，每个元素为[西经min, 西经max, 北纬min, 北纬max]
    :param decimals: 可选，保留的小数位数，如6/8，为None则不保留
    :return: 中心点数组，每个元素为[经度中心, 纬度中心]
    """
    center_points = []
    for box in coords:
        lon_min, lon_max, lat_min, lat_max = box  # 解构赋值提取四至坐标
        # 计算中心点：经度=(西经min+西经max)/2，纬度=(北纬min+北纬max)/2
        center_lon = (lon_min + lon_max) / 2
        center_lat = (lat_min + lat_max) / 2

        # 保留指定小数位数（可选）
        if decimals is not None:
            center_lon = round(center_lon, decimals)
            center_lat = round(center_lat, decimals)

        center_points.append([center_lon, center_lat])
    return center_points


def calculate_box_centers(coords, decimals=None):
    """
    计算每个矩形坐标框的经纬度中心点
    :param coords: 原始坐标数组，每个元素为[西经min, 西经max, 北纬min, 北纬max]
    :param decimals: 可选，保留的小数位数，如6/8，为None则不保留
    :return: 中心点数组，每个元素为[经度中心, 纬度中心]
    """
    center_points = []
    for box in coords:
        lon_min, lon_max, lat_min, lat_max = box  # 解构赋值提取四至坐标
        # 计算中心点：经度=(西经min+西经max)/2，纬度=(北纬min+北纬max)/2
        center_lon = (lon_min + lon_max) / 2
        center_lat = (lat_min + lat_max) / 2

        # 保留指定小数位数（可选）
        if decimals is not None:
            center_lon = round(center_lon, decimals)
            center_lat = round(center_lat, decimals)

        center_points.append([center_lon, center_lat])
    return center_points

def center2csv(centers, dem_file_list, output_dir_list):
    """
    将每个中心点转换为独立CSV文件（每个CSV保存在专属输出目录），匹配指定格式
    实现：centers[i] → dem_file_list[i] → output_dir_list[i] → 专属目录下的CSV
    :param centers: 中心点坐标数组，每个元素[经度, 纬度]
    :param dem_file_list: DEM文件路径列表，与centers索引一一对应
    :param output_dir_list: 输出目录路径数组，与centers索引一一对应，每个中心点对应专属输出目录
    :param epsg_code: 投影标识，默认54009（摩尔魏德），3857为Web墨卡托（已做兼容处理）
    :return: 无返回值，按映射关系生成CSV
    """
    # 1. 前置校验：三个数组长度必须一致，避免映射错位
    if not (len(centers) == len(dem_file_list) == len(output_dir_list)):
        raise ValueError("错误：centers、dem_file_list、output_dir_list 长度必须完全一致！")

    # 2. 初始化投影转换器：WGS84经纬度 → 投影（核心修改：PROJ4字符串定义摩尔魏德，兼容所有pyproj）
    wgs84 = pyproj.CRS("EPSG:4326")  # 原始WGS84经纬度坐标系，不变
    # 默认54009用摩尔魏德PROJ4字符串
    proj_crs = pyproj.CRS('+proj=moll +lon_0=0 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m no_defs')
    # 初始化转换器，always_xy=True保证经纬度顺序（lon,lat）
    transformer = pyproj.Transformer.from_crs(wgs84, proj_crs, always_xy=True)

    # 3. 遍历映射：每个中心点→对应DEM→对应输出目录→生成两个独立CSV
    for idx, (center, dem_file, out_dir) in enumerate(zip(centers, dem_file_list, output_dir_list), 1):
        lon, lat = center  # 提取中心点经纬度
        dem_path = Path(dem_file)
        out_path = Path(out_dir)
        # 定义两个CSV文件名（保留你原代码的命名）
        csv_p_filename = "SITES_P_ERA5.csv"
        csv_p_save_path = out_path / csv_p_filename
        csv_m_filename = "SITES_M_ERA5.csv"
        csv_m_save_path = out_path / csv_m_filename

        try:
            # 步骤1：计算投影的LocalX、LocalY（米单位，保留4位小数）
            local_x, local_y = transformer.transform(lon, lat)
            local_x = round(local_x, 4)
            local_y = round(local_y, 4)

            # 步骤2：从对应DEM中提取高程Elevation（处理边界+无效值，保留你原逻辑）
            if not dem_path.exists():
                raise FileNotFoundError(f"DEM文件不存在：{dem_file}")
            with rasterio.open(dem_path) as dem:
                # 经纬度转DEM栅格行列号
                row, col = dem.index(lon, lat)
                # 校验行列号在DEM范围内
                if 0 <= row < dem.height and 0 <= col < dem.width:
                    elevation = dem.read(1)[row, col].astype(np.float64)
                    # 处理DEM无效值（保留你原定义的无效值列表）
                    invalid_values = [-9999, 0, np.nan, -32768]
                    elevation = np.nan if elevation in invalid_values else round(elevation, 4)
                else:
                    raise ValueError(f"经纬度({lon:.6f},{lat:.6f})超出DEM栅格范围")

            # 步骤3：构造CSV数据，列顺序严格匹配要求（保留你原逻辑）
            csv_data = {
                "StationID": [0],  # 固定为0
                "Name": ["Station 1"],  # 固定为Station 1
                "LocalX": [local_x],  # 投影X（米）
                "LocalY": [local_y],  # 投影Y（米）
                "Lon": [round(lon, 8)],  # 经度保留8位小数
                "Lat": [round(lat, 8)],  # 纬度保留8位小数
                "Elevation": [elevation]  # 高程保留4位小数
            }
            # 强制指定列顺序，与要求完全一致
            col_order = ["StationID", "Name", "LocalX", "LocalY", "Lon", "Lat", "Elevation"]
            df = pd.DataFrame(csv_data)[col_order]

            # 步骤4：创建专属输出目录+生成两个CSV（无索引，utf-8编码，保留你原逻辑）
            out_path.mkdir(parents=True, exist_ok=True)  # 递归创建目录（含父目录）
            df.to_csv(csv_p_save_path, index=False, encoding="utf-8")
            df.to_csv(csv_m_save_path, index=False, encoding="utf-8")
            print(f"✅ 第{idx}个中心点处理完成：")
            print(f"   经纬度：{lon:.6f},{lat:.6f} | DEM：{dem_file} | P-CSV保存：{csv_p_save_path}\n")
            print(f"   经纬度：{lon:.6f},{lat:.6f} | DEM：{dem_file} | M-CSV保存：{csv_m_save_path}\n")

        except Exception as e:
            # 单个点失败不影响其他点，控制台打印详细错误
            print(f"❌ 第{idx}个中心点处理失败（{lon:.6f},{lat:.6f}）：")
            print(f"   错误原因：{str(e)}\n")
            continue


if __name__ == '__main__':
    csv_from_era5_land_base = r'G:\program\seims\SEIMS_HAND\data\drive-download-20260203T125553Z-3-001'
    csv_wise_data_base = r'G:\program\seims\SEIMS_HAND\data'
    csv_from_era5_lands = [
        os.path.join(csv_from_era5_land_base,'ERA5Land_Daily_1_LittleRiver.csv'),
        os.path.join(csv_from_era5_land_base, 'ERA5Land_Daily_2_MadRiver.csv'),
        os.path.join(csv_from_era5_land_base, 'ERA5Land_Daily_3_WilliamsRiver.csv'),
        os.path.join(csv_from_era5_land_base, 'ERA5Land_Daily_4_DryBeaver.csv'),
        os.path.join(csv_from_era5_land_base, 'ERA5Land_Daily_5_SantaPaula.csv'),
        os.path.join(csv_from_era5_land_base, 'ERA5Land_Daily_6_SabinoCreek.csv'),
    ]
    ### 存入之前建的项目
    # csv_pcps = [
    #     os.path.join(csv_wise_data_base, '1_Little_River\data_prepare\climate\pcp_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '2_Mad_River\data_prepare\climate\pcp_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '3_Williams_River\data_prepare\climate\pcp_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '4_Dry_Beaver\data_prepare\climate\pcp_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '5_Santa_Paula\data_prepare\climate\pcp_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '6_Sabino_Creek\data_prepare\climate\pcp_daily_era5.csv'),
    # ]
    #
    # csv_meteos = [
    #     os.path.join(csv_wise_data_base, '1_Little_River\data_prepare\climate\meteo_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '2_Mad_River\data_prepare\climate\meteo_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '3_Williams_River\data_prepare\climate\meteo_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '4_Dry_Beaver\data_prepare\climate\meteo_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '5_Santa_Paula\data_prepare\climate\meteo_daily_era5.csv'),
    #     os.path.join(csv_wise_data_base, '6_Sabino_Creek\data_prepare\climate\meteo_daily_era5.csv'),
    # ]

    ### 存入之龙平代码建的新项目
    csv_pcps = [
        os.path.join(csv_wise_data_base, 'US_1\data_prepare\climate\pcp_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_2\data_prepare\climate\pcp_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_3\data_prepare\climate\pcp_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_4\data_prepare\climate\pcp_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_5\data_prepare\climate\pcp_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_6\data_prepare\climate\pcp_daily_ERA5.csv'),
    ]

    csv_meteos = [
        os.path.join(csv_wise_data_base, 'US_1\data_prepare\climate\meteo_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_2\data_prepare\climate\meteo_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_3\data_prepare\climate\meteo_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_4\data_prepare\climate\meteo_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_5\data_prepare\climate\meteo_daily_ERA5.csv'),
        os.path.join(csv_wise_data_base, 'US_6\data_prepare\climate\meteo_daily_ERA5.csv'),
    ]

    ## 1.生成降雨和气象csv数据
    for i in range(0,len(csv_meteos)):
        process_data(csv_from_era5_lands[i], csv_pcps[i], csv_meteos[i])

    ## 2.生成SITES csv数据
    coords = [
        [-83.76958333, -83.47208333, 35.56208333, 35.70041667],  # 第一组
        [-72.93791667, -72.70458333, 44.02791667, 44.29791667],  # 第二组
        [-80.49458333, -80.12958333, 38.20125, 38.41041667],  # 第三组
        [-111.8004167, -111.4395833, 34.70458333, 34.92875],  # 第四组
        [-119.1445833, -119.0054167, 34.39875, 34.51625],  # 第五组
        [-110.84375, -110.6970833, 32.3020833, 32.45041666]  # 第六组
    ]

    # 计算中心点
    centers = calculate_box_centers(coords, decimals=6)
    dem_file_list = [
        os.path.join(csv_wise_data_base, 'US_1\workspace\spatial_raster\dem.tif'),
        os.path.join(csv_wise_data_base, 'US_2\workspace\spatial_raster\dem.tif'),
        os.path.join(csv_wise_data_base, 'US_3\workspace\spatial_raster\dem.tif'),
        os.path.join(csv_wise_data_base, 'US_4\workspace\spatial_raster\dem.tif'),
        os.path.join(csv_wise_data_base, 'US_5\workspace\spatial_raster\dem.tif'),
        os.path.join(csv_wise_data_base, 'US_6\workspace\spatial_raster\dem.tif'),
    ]
    output_dir_list = [
        os.path.join(csv_wise_data_base, 'US_1\data_prepare\climate'),
        os.path.join(csv_wise_data_base, 'US_2\data_prepare\climate'),
        os.path.join(csv_wise_data_base, 'US_3\data_prepare\climate'),
        os.path.join(csv_wise_data_base, 'US_4\data_prepare\climate'),
        os.path.join(csv_wise_data_base, 'US_5\data_prepare\climate'),
        os.path.join(csv_wise_data_base, 'US_6\data_prepare\climate'),
    ]
    center2csv(centers, dem_file_list, output_dir_list)





