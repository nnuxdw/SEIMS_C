import csv
import os
import math
from datetime import datetime


def ensure_output_dir(output_file):
    """
    确保输出文件的父目录存在，如果不存在则创建
    """
    # 提取文件的父目录路径
    output_dir = os.path.dirname(output_file)

    # 如果父目录路径非空（比如文件不是在当前目录）且不存在，则创建
    if output_dir and not os.path.exists(output_dir):
        # exist_ok=True：目录已存在时不报错
        os.makedirs(output_dir, exist_ok=True)
        print(f"创建目录：{output_dir}")
    elif output_dir:
        print(f"目录 {output_dir} 已存在，无需创建")


def mollweide_projection(lon_deg, lat_deg):
    """
    将经纬度（度）转换为Mollweide投影坐标（米）
    参数:
        lon_deg: 经度（十进制）
        lat_deg: 纬度（十进制）
    返回:
        (local_x, local_y): Mollweide投影下的x、y坐标（单位：米）
    """
    R = 6371000  # 地球平均半径（米）
    # 转换为弧度
    lon_rad = math.radians(lon_deg)
    lat_rad = math.radians(lat_deg)

    # Mollweide投影核心计算
    theta = math.asin((2 * lat_rad) / math.pi)
    local_x = R * math.sqrt(2) * lon_rad * math.cos(theta)
    local_y = R * math.sqrt(2) * math.sin(theta)

    return round(local_x, 6), round(local_y, 6)


def create_site_info(output_dir, x, y, monitoring_location_id):
    """
    在指定目录下创建SiteInfo.csv文件
    参数:
        output_dir: 输出目录路径
        x: 经度（对应Lon）
        y: 纬度（对应Lat）
        monitoring_location_id: 站点名称（对应Name）
    """
    site_info_path = os.path.join(output_dir, "SiteInfo.csv")

    # 计算Mollweide投影坐标
    local_x, local_y = mollweide_projection(float(x), float(y))

    # 写入SiteInfo.csv
    with open(site_info_path, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        # 写入表头
        writer.writerow([
            "StationID", "Name", "Type", "Lat", "Lon",
            "LocalX", "LocalY", "Unit", "Elevation", "isOutlet"
        ])
        # 写入数据行（按要求填充固定值）
        writer.writerow([
            1,  # StationID默认1
            monitoring_location_id,  # Name来自monitoring_location_id
            "Q",  # Type为Q
            round(float(y), 6),  # Lat（保留6位小数）
            round(float(x), 6),  # Lon（保留6位小数）
            local_x,  # Mollweide投影X
            local_y,  # Mollweide投影Y
            "m3/s",  # Unit为m3/s
            0,  # Elevation为0
            1  # isOutlet为1
        ])
    print(f"SiteInfo.csv已生成：{site_info_path}")


def convert_flow_data_to_csv(input_csv_path, output_csv_path):
    """
    从USGS流量CSV文件读取数据，转换单位并输出为指定的CSV格式
    同时提取站点信息并生成SiteInfo.csv
    参数:
        input_csv_path: 输入CSV文件路径
        output_csv_path: 输出CSV文件路径
    """
    # 1 ft³/s = 0.0283168466 m³/s
    CONVERSION_FACTOR = 0.0283168466

    # 初始化站点信息变量
    site_x = None
    site_y = None
    site_id = None

    with open(input_csv_path, 'r', newline='', encoding='utf-8') as infile, \
        open(output_csv_path, 'w', newline='', encoding='utf-8') as outfile:
        reader = csv.DictReader(infile)
        writer = csv.writer(outfile)

        # 写入文件头
        writer.writerow(["#UTCTIME"])
        writer.writerow(["StationID", "DATETIME", "Type", "VALUE"])

        for row in reader:
            # 提取站点基础信息（仅提取一次）
            if site_x is None and 'x' in row and row['x'].strip():
                site_x = row['x'].strip()
            if site_y is None and 'y' in row and row['y'].strip():
                site_y = row['y'].strip()
            if site_id is None and 'monitoring_location_id' in row and row['monitoring_location_id'].strip():
                site_id = row['monitoring_location_id'].strip()

            # 读取时间和流量值
            value_str = row['value'].strip() if row['value'] else ''

            # 判断value是否为空，为空则跳过当前行
            if not value_str:
                continue
            time_str = row['time']

            try:
                value_ft3s = float(value_str)
            except ValueError:
                # 如果value不是数字，也跳过当前行
                print(f"警告：文件 {input_csv_path} 行 {reader.line_num} 的value值 '{value_str}' 不是有效数字，已跳过")
                continue

            # 转换单位
            value_m3s = value_ft3s * CONVERSION_FACTOR

            # 解析日期并添加时间（适配YYYY-MM-DD格式）
            date_obj = datetime.strptime(time_str, "%Y-%m-%d")
            datetime_str = date_obj.strftime("%Y/%m/%d 0:00")

            # 写入一行数据
            writer.writerow(["1", datetime_str, "Q", f"{value_m3s:.1f}"])

    # 生成SiteInfo.csv（仅当站点信息完整时）
    output_dir = os.path.dirname(output_csv_path)
    if all([site_x, site_y, site_id]):
        create_site_info(output_dir, site_x, site_y, site_id)
    else:
        print(f"警告：文件 {input_csv_path} 中未找到完整的站点信息（x/y/monitoring_location_id），跳过SiteInfo.csv生成")


if __name__ == '__main__':
    # 示例使用
    BASINs = [
        "US_1",
        "US_2",
        "US_3",
        "US_4",
        "US_5",
        "US_6",
        "US_7",
        # "US_8",
        # "US_9",
        "US_10",
        "US_11",
        "US_12",
        # "US_13",
        "US_14",
        "US_15",
        "US_16",
        "US_17",
        "US_18",
    ]
    input_base_path = r'G:\program\seims\SEIMS_HAND\data\USA_Small_Watersheds\流量'
    input_suffix = r'discharge\20150101-20241231\daily.csv'
    output_base_path = r'G:\program\seims\SEIMS_HAND\data'
    output_suffix = r'data_prepare\observed'

    for basin in BASINs:
        input_file = os.path.join(input_base_path, basin, input_suffix)
        # 检查输入文件是否存在，避免报错
        if not os.path.exists(input_file):
            print(f"错误：输入文件不存在，跳过 {basin} -> {input_file}")
            continue

        output_parent = os.path.join(output_base_path, basin, output_suffix)
        if not os.path.exists(output_parent):
            os.makedirs(output_parent)
        output_file = os.path.join(output_parent, 'observed_1.csv')

        # 执行转换并生成SiteInfo.csv
        convert_flow_data_to_csv(input_file, output_file)
        print(f"转换完成，结果已保存到 {output_file}")
        print("-" * 50)  # 分隔线，方便查看日志
