import csv
import pandas as pd
from datetime import datetime
import os
import csv
import pytz

def read_q_from_usgs_rbd(input_file):
    """
    从 USGS 提供的 RDB 格式流量文件中读取数据，并返回 (datetime, flow(m³/s), quality_code) 三元组列表
    """

    # 读取全部行，查找数据起始行
    with open(input_file, 'r') as f:
        lines = f.readlines()

    data_start_line = 0
    for i, line in enumerate(lines):
        if not line.startswith('#'):
            data_start_line = i
            break

    # 从数据开始行读取所需列：datetime（index=2），流量（index=3），质量码（index=4）
    df = pd.read_csv(
        input_file,
        sep='\t',
        comment='#',
        skiprows=data_start_line + 2,  # 跳过 "5s 15s 20d..." 行
        header=None,
        usecols=[2, 3, 4],
        names=['datetime', 'flow_cfs', 'qualifier'],  # 手动命名列
        dtype={'datetime': str, 'flow_cfs': str, 'qualifier': str}
    )

    # 转换时间格式为 pandas datetime
    try:
        df['datetime'] = pd.to_datetime(df['datetime'], format='%Y-%m-%d')
    except Exception as e:
        print(f"[ERROR] 无法解析时间列，样例：{df['datetime'].iloc[0]}")
        raise e

    # 转换流量为 float，并从 ft³/s → m³/s
    df['flow_cfs'] = pd.to_numeric(df['flow_cfs'], errors='coerce')
    df['flow_m3s'] = df['flow_cfs'] * 0.0283168

    # 丢弃流量为 NaN 的记录
    df = df.dropna(subset=['flow_m3s'])

    # 输出格式：(datetime, flow, qualifier)
    result_list = list(zip(df['datetime'], df['flow_m3s'], df['qualifier']))

    return result_list



def filter_data_by_time(value_list, start_time, end_time):
    # 将 start_time 和 end_time 转换为 datetime 对象
    start_time_obj = datetime.strptime(start_time, '%Y-%m-%d-%H:%M:%S')
    end_time_obj = datetime.strptime(end_time, '%Y-%m-%d-%H:%M:%S')

    filter_value_list = []
    for time, value, tz in value_list:
        # 如果 time 已经是 datetime，不需要再解析
        if isinstance(time, str):
            time_obj = datetime.strptime(time, '%Y-%m-%d %H:%M:%S')
        else:
            time_obj = time

        if start_time_obj <= time_obj <= end_time_obj:
            filter_value_list.append((time, value, tz))

    return filter_value_list
def convert_time_list_to_gmt(var_list):
    # 定义时区
    edt = pytz.timezone('US/Eastern')  # 东部夏令时 (GMT-4)
    cst = pytz.timezone('US/Central')  # 中部标准时 (GMT-6)
    cdt = pytz.timezone('US/Central')  # CDT 和 CST 使用相同的 tz 对象，但转换时用 offset 判断
    gmt = pytz.timezone('GMT')         # GMT

    gmt_var_list = []

    for time_str, flow, tz in var_list:
        # 如果本身就是 GMT，则直接保留
        if tz == 'GMT':
            gmt_var_list.append((time_str, flow, 'GMT'))
            continue

        # 解析时间字符串
        time_obj = datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')

        # 根据时区进行本地化
        if tz == 'EDT':
            time_obj = edt.localize(time_obj)
        elif tz == 'CST':
            time_obj = cst.localize(time_obj)
        elif tz == 'CDT':
            time_obj = cdt.localize(time_obj)  # CDT 夏令时（应为 -5）
            time_obj = time_obj.replace(tzinfo=pytz.FixedOffset(-300))  # 手动修正
        else:
            print(f"⚠️ 未知时区：{tz}，跳过该行")
            continue

        # 转换为 GMT
        gmt_time_obj = time_obj.astimezone(gmt)
        gmt_time_str = gmt_time_obj.strftime('%Y-%m-%d %H:%M:%S')
        gmt_var_list.append((gmt_time_str, flow, 'GMT'))

    return gmt_var_list
def convert_usgs_to_sim_csv(usgs_file_path, sim_start_time, sim_end_time, output_csv_path):
    # 第一步：读取原始 usgs 数据
    real_qout_list = read_q_from_usgs_rbd(usgs_file_path)

    # 第二步：转换为 GMT 时间
    # real_qout_list_gmt = convert_time_list_to_gmt(real_qout_list)

    # 第三步：筛选在模拟时间范围内的数据
    real_qout_list_gmt_filter = filter_data_by_time(
        real_qout_list,
        sim_start_time,
        sim_end_time
    )

    # 第四步：导出为 csv，格式为 StationID DATETIME Type VALUE
    with open(output_csv_path, mode='w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['StationID', 'DATETIME', 'Type', 'VALUE'])

        for entry in real_qout_list_gmt_filter:
            # 假设 entry = (datetime_obj, value, tz_code)
            dt_str = entry[0].strftime('%Y/%m/%d %H:%M:%S')  # 👈 这里写
            writer.writerow([1, dt_str, 'Q', round(entry[1], 4)])  # StationID固定为1，Type为Q

    print(f"导出完成：{output_csv_path}")


if __name__ == '__main__':
    usgs_file_path = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347\data_prepare\observed\06601200.txt'
    start_time = '2010-01-01-00:00:00'
    end_time = '2019-12-31-00:00:00'
    csv_file = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347\data_prepare\observed\observed_Q.csv'
    convert_usgs_to_sim_csv(usgs_file_path,start_time,end_time,csv_file)
