import os
from collections import defaultdict, OrderedDict
from datetime import datetime
import csv

import matplotlib.pyplot as plt
from matplotlib.dates import DateFormatter
from pygeoc.utils import FileClass
from utility import read_data_items_from_txt


def plot_total_flood_area_from_txt(ws,
                                   var='F',
                                   stime=None,
                                   etime=None,
                                   save_csv=True):
    """
    读取 ws 目录下的 F.txt，按天汇总所有 subbasin 的淹没面积之和，
    绘制折线图，并将 daily total 写入 CSV。

    参数
    ----
    ws : str
        工作目录，里面有 F.txt
    var : str
        变量名，默认 'F'，也就是 F.txt
    stime, etime : datetime or None
        起止时间；为 None 则不限制
    save_csv : bool
        是否将 daily total 写入 CSV
    """
    txtfile = os.path.join(ws, var + '.txt')
    if not FileClass.is_file_exists(txtfile):
        print('WARNING: 文件不存在: %s' % txtfile)
        return OrderedDict()

    data_items = read_data_items_from_txt(txtfile)

    # 每天总面积（所有 subbasin 之和）
    daily_total = defaultdict(float)

    for rec in data_items:
        s = rec[0] if isinstance(rec, (list, tuple)) else rec
        s = s.strip()
        if not s:
            continue

        parts = s.split()

        # 头行：Subbasin: 7
        if len(parts) == 2:
            continue

        if len(parts) != 3:
            continue

        dt_str = parts[0] + ' ' + parts[1]
        try:
            dt = datetime.strptime(dt_str, '%Y-%m-%d %H:%M:%S')
            val = float(parts[2])
        except ValueError:
            continue

        # 时间范围过滤
        if stime is not None and dt < stime:
            continue
        if etime is not None and dt > etime:
            continue

        # 按日聚合
        day = datetime(dt.year, dt.month, dt.day)
        daily_total[day] += val

    if not daily_total:
        print("没有找到任何数据。")
        return OrderedDict()

    # 排序
    dates_sorted = sorted(daily_total.keys())
    total_series = OrderedDict((d, daily_total[d]) for d in dates_sorted)

    # ========= 保存 CSV =========
    if save_csv:
        csv_path = os.path.join(ws, "total_inundation_area.csv")
        with open(csv_path, "w", newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(["date", "total_area"])
            for d, a in total_series.items():
                writer.writerow([d.strftime("%Y-%m-%d"), a])
        print(f"CSV 已保存: {csv_path}")

    # ========= 绘图 =========
    x = list(total_series.keys())
    y = list(total_series.values())

    plt.figure(figsize=(12, 5))
    plt.plot(x, y, linewidth=1.5)
    plt.xlabel("Date")
    plt.ylabel("Total inundation area (km²)")
    plt.title("Daily total inundation area of all subbasins")
    plt.grid(True, linestyle='--', alpha=0.5)

    ax = plt.gca()
    ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d'))
    plt.gcf().autofmt_xdate()

    plt.tight_layout()
    plt.show()

    return total_series


if __name__ == '__main__':
    ws = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171\OUTPUT0-9"
    stime = datetime(2010, 1, 1)
    etime = datetime(2019, 12, 31)

    total_series = plot_total_flood_area_from_txt(ws, var='F',
                                                  stime=stime,
                                                  etime=etime,
                                                  save_csv=True)
