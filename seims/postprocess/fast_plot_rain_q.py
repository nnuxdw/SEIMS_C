# -*- coding: utf-8 -*-

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


def plot_mass_file(mass_file, output_png=None):
    """
    读取 LISFLOOD-FP mass 文件并绘制 Qout 和 Rain(mm/h)。

    Parameters
    ----------
    mass_file : str
        mass 文件路径。
    output_png : str or None
        图片输出路径；为 None 时不保存图片。
    """

    # 模拟起始时间
    start_time = pd.Timestamp("2026-09-17 00:00:00")

    # mass 文件以一个或多个空格分隔
    data = pd.read_csv(
        mass_file,
        sep=r"\s+",
        engine="python"
    )

    # 去除列名两侧可能存在的空格
    data.columns = data.columns.str.strip()

    required_columns = ["Time", "Qout", "Rain(mm/h)"]
    missing_columns = [
        column for column in required_columns
        if column not in data.columns
    ]

    if missing_columns:
        raise ValueError(
            "mass文件缺少以下字段：{}".format(", ".join(missing_columns))
        )

    # 转换为数值，无法解析的数据设为 NaN
    for column in required_columns:
        data[column] = pd.to_numeric(data[column], errors="coerce")

    data = data.dropna(subset=required_columns)

    # Time 为距离模拟开始的秒数
    data["Datetime"] = (
        start_time
        + pd.to_timedelta(data["Time"], unit="s")
    )

    # 创建图形
    fig, ax1 = plt.subplots(figsize=(12, 6))

    # 左轴：流量
    line1 = ax1.plot(
        data["Datetime"],
        data["Qout"],
        color="#1565C0",
        linewidth=1.8,
        label="Qout"
    )

    ax1.set_xlabel("Time")
    ax1.set_ylabel("Qout (m³/s)", color="#1565C0")
    ax1.tick_params(axis="y", labelcolor="#1565C0")
    ax1.grid(
        axis="both",
        linestyle="--",
        linewidth=0.6,
        alpha=0.4
    )

    # 右轴：降雨强度
    ax2 = ax1.twinx()

    line2 = ax2.plot(
        data["Datetime"],
        data["Rain(mm/h)"],
        color="#E53935",
        linewidth=1.5,
        label="Rain"
    )

    ax2.set_ylabel("Rainfall intensity (mm/h)", color="#E53935")
    ax2.tick_params(axis="y", labelcolor="#E53935")

    # 时间坐标格式
    ax1.xaxis.set_major_formatter(
        mdates.DateFormatter("%m-%d\n%H:%M")
    )
    ax1.xaxis.set_major_locator(
        mdates.AutoDateLocator()
    )

    # 合并图例
    lines = line1 + line2
    labels = [line.get_label() for line in lines]
    ax1.legend(lines, labels, loc="upper right", frameon=False)

    ax1.set_title("LISFLOOD-FP Discharge and Rainfall")
    fig.tight_layout()

    if output_png:
        plt.savefig(
            output_png,
            dpi=300,
            bbox_inches="tight"
        )
        print("图片已保存：{}".format(output_png))

    plt.show()


if __name__ == "__main__":
    mass_file = r"F:\BasinFloodData\LTH30M\result_precipitation_20260921_20260927_BJT\test.mass"
    output_png = r"F:\BasinFloodData\LTH30M\result_precipitation_20260921_20260927_BJT\Qout_Rain.png"

    plot_mass_file(mass_file, output_png)
