import os
import re
from datetime import datetime

import rasterio
import numpy as np
from rasterio.errors import RasterioIOError

import matplotlib.pyplot as plt
from matplotlib.dates import DateFormatter


import os
import re
from datetime import datetime
import numpy as np
import rasterio
from rasterio.errors import RasterioIOError
import matplotlib.pyplot as plt
from matplotlib.dates import DateFormatter
import pandas as pd


def calc_tif_area_timeseries(folder, output_csv):
    """
    计算每个 tif 的 value=1 面积（km²），按文件名日期生成时序图，并保存结果到 CSV。
    遇到损坏文件自动跳过。
    """
    tif_files = [f for f in os.listdir(folder) if f.lower().endswith('.tif')]
    date_pattern = re.compile(r"(\d{4})-(\d{2})-(\d{2})_(\d{2})-(\d{2})-(\d{2})")

    results = []  # [(datetime, area_km2)]

    for fname in tif_files:
        fpath = os.path.join(folder, fname)

        # ⏳ 从文件名解析日期
        m = date_pattern.search(fname)
        if not m:
            print(f"[跳过] 文件名未找到日期格式：{fname}")
            continue

        y, mth, d, hh, mm, ss = map(int, m.groups())
        dt = datetime(y, mth, d, hh, mm, ss)

        try:
            with rasterio.open(fpath) as src:
                data = src.read(1)
                nodata = src.nodata

                # ⚠ WGS84 经纬度 — 你的数据分辨率 ≈ 10m，
                # 所以直接指定 cell_area = 100 m²
                cell_area = 100

                mask = np.ones_like(data, dtype=bool)
                if nodata is not None:
                    mask &= (data != nodata)

                mask &= (data == 1)
                count = int(mask.sum())
                area_km2 = count * cell_area / 1e6

        except RasterioIOError:
            print(f"[损坏，已跳过] {fname}")
            continue

        except Exception as e:
            print(f"[其他错误，已跳过] {fname} - {e}")
            continue

        results.append((dt, area_km2))
        print(f"{fname}: {area_km2:.2f} km²")

    # 排序
    results.sort(key=lambda x: x[0])

    if not results:
        print("没有可用数据")
        return []

    # 分离日期 & 面积
    dates = [r[0] for r in results]
    areas = [r[1] for r in results]

    # 📌 保存到 CSV
    df = pd.DataFrame({
        "datetime": dates,
        "area_km2": areas
    })

    df.to_csv(output_csv, index=False, encoding="utf-8")
    print(f"✅ 已保存 CSV：{output_csv}")

    # 📊 绘图
    plt.figure(figsize=(12, 5))
    plt.plot(dates, areas, marker='o')
    plt.xlabel("Date")
    plt.ylabel("Area (km²)")
    plt.title("Inundation Area Time Series")
    plt.grid(True, linestyle='--', alpha=0.5)

    ax = plt.gca()
    ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d'))
    plt.gcf().autofmt_xdate()

    plt.show()

    return results



if __name__ == '__main__':
    folder = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\鄱阳湖全天候面积逐日数据集（2014-2023年）_数据实体\2014-2023年鄱阳湖水域面积栅格数据"
    save_csv = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\鄱阳湖全天候面积逐日数据集（2014-2023年）_数据实体\area_ts.csv"
    save_fig = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖全天候面积逐日数据集（2014-2023年)\鄱阳湖全天候面积逐日数据集（2014-2023年）_数据实体\area_ts.png"

    folder = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖区域10m分辨率洪涝淹没范围数据集（2012年-2022年）\2017"
    save_csv = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖区域10m分辨率洪涝淹没范围数据集（2012年-2022年）\area.csv"
    save_fig = r"J:\G\program\seims\SEIMS_HAND\data\poyang_lake\鄱阳湖区域10m分辨率洪涝淹没范围数据集（2012年-2022年）\area.png"
    calc_tif_area_timeseries(
        folder=folder,
        output_csv=save_csv
    )
