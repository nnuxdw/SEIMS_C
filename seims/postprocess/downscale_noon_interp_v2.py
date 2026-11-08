# -*- coding: utf-8 -*-
"""
根据前一天、当天、后一天的日平均流量，在“每天正午12:00”为锚点的条件下，
采用与原脚本一致的线性插值思想（scipy.interpolate.interp1d），
生成分钟尺度降尺度流量曲线，并绘制示意图。

特点：
1. 日平均流量作为每天正午12:00的流量值；
2. 在分钟节点上进行线性插值；
3. 图中用圆点显示三个日平均流量锚点，用线显示插值后的分钟尺度曲线；
4. x轴仅标注“前一天、当天、后一天”，不显示具体日期。

作者可直接修改 main() 中的日期和流量进行测试。
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d


def generate_minute_boundary_by_noon_interp(
    prev_date,
    curr_date,
    next_date,
    prev_flow,
    curr_flow,
    next_flow,
    freq='1min',
    clip_negative=True
):
    """
    以“前一天、当天、后一天”的日平均流量为输入，
    将三天的日平均流量分别布设在各自正午12:00，
    然后在分钟尺度上进行线性插值。
    """

    prev_date = pd.to_datetime(prev_date)
    curr_date = pd.to_datetime(curr_date)
    next_date = pd.to_datetime(next_date)

    # 三个“日中心时刻”锚点：正午12:00
    anchor_times = pd.to_datetime([
        prev_date.normalize() + pd.Timedelta(hours=12),
        curr_date.normalize() + pd.Timedelta(hours=12),
        next_date.normalize() + pd.Timedelta(hours=12),
    ])
    anchor_flows = np.array([prev_flow, curr_flow, next_flow], dtype=float)

    df_anchor = pd.DataFrame({
        'datetime': anchor_times,
        'flow': anchor_flows
    })

    # 完整三天的分钟时间轴：从前一天00:00到后一天23:59
    start_time = prev_date.normalize()
    end_time = next_date.normalize() + pd.Timedelta(days=1) - pd.Timedelta(minutes=1)
    minute_times = pd.date_range(start=start_time, end=end_time, freq=freq)

    # 转换为相对首个锚点的小时数，再做线性插值
    anchor_time_numeric = (
        (df_anchor['datetime'].values - df_anchor['datetime'].values[0]) / np.timedelta64(1, 'h')
    ).astype(float)

    minute_time_numeric = (
        (minute_times.values - df_anchor['datetime'].values[0]) / np.timedelta64(1, 'h')
    ).astype(float)

    linear_interp = interp1d(
        anchor_time_numeric,
        df_anchor['flow'].values,
        kind='linear',
        fill_value='extrapolate'
    )
    minute_flows = linear_interp(minute_time_numeric)

    if clip_negative:
        minute_flows = np.maximum(minute_flows, 0.0)

    df_minute = pd.DataFrame({
        'datetime': minute_times,
        'flow': minute_flows
    })

    return df_minute, df_anchor


def plot_noon_interp_curve(
    df_minute,
    df_anchor,
    title='基于日中心时刻线性插值的分钟尺度流量边界',
    output_png='noon_interp_curve.png'
):
    """
    绘制分钟尺度插值曲线。
    - 线：分钟尺度插值结果
    - 圆点：每天正午12:00的日平均流量
    - x轴：仅显示“前一天、当天、后一天”
    """

    fig, ax = plt.subplots(figsize=(13, 5))

    # 插值曲线
    ax.plot(
        df_minute['datetime'],
        df_minute['flow'],
        linewidth=2,
        label='插值曲线'
    )

    # 三个日中心锚点
    ax.scatter(
        df_anchor['datetime'],
        df_anchor['flow'],
        s=70,
        marker='o',
        zorder=5,
        label='日平均流量（正午12:00）'
    )

    # 标注三个点的数值
    for _, row in df_anchor.iterrows():
        ax.annotate(
            f"{row['flow']:.2f}",
            xy=(row['datetime'], row['flow']),
            xytext=(0, 10),
            textcoords='offset points',
            ha='center',
            fontsize=10
        )

    # x轴只显示“前一天 / 当天 / 后一天”
    ax.set_xticks(df_anchor['datetime'])
    ax.set_xticklabels(['前一天', '当天', '后一天'], fontsize=11)

    # 每天00:00分界线
    start_day = df_minute['datetime'].dt.normalize().min()
    end_day = df_minute['datetime'].dt.normalize().max()
    day_marks = pd.date_range(start_day, end_day + pd.Timedelta(days=1), freq='1D')
    for d in day_marks:
        ax.axvline(d, linestyle='--', linewidth=0.8, alpha=0.4)

    # 正午12:00辅助线
    for d in df_anchor['datetime']:
        ax.axvline(d, linestyle=':', linewidth=0.9, alpha=0.5)

    ax.set_xlabel('时间位置')
    ax.set_ylabel('流量')
    ax.set_title(title)
    ax.legend()
    ax.grid(True, linestyle='--', alpha=0.35)
    plt.tight_layout()

    plt.savefig(output_png, dpi=200, bbox_inches='tight')
    print(f"图片已保存到: {output_png}")
    return output_png


def save_result_csv(df_minute, output_csv='noon_interp_curve.csv'):
    """保存分钟尺度插值结果"""
    df_minute.to_csv(output_csv, index=False, encoding='utf-8-sig')
    print(f"分钟尺度结果已保存到: {output_csv}")
    return output_csv


def main():
    """
    你只需要修改这里的日期和流量即可。
    x轴显示为“前一天 / 当天 / 后一天”，与具体日期无关。
    """

    prev_date = '2024-07-09'
    curr_date = '2024-07-10'
    next_date = '2024-07-11'

    prev_flow = 80.0
    curr_flow = 140.0
    next_flow = 95.0

    df_minute, df_anchor = generate_minute_boundary_by_noon_interp(
        prev_date=prev_date,
        curr_date=curr_date,
        next_date=next_date,
        prev_flow=prev_flow,
        curr_flow=curr_flow,
        next_flow=next_flow,
        freq='1min',
        clip_negative=True
    )

    save_result_csv(df_minute, output_csv='noon_interp_curve.csv')
    plot_noon_interp_curve(
        df_minute=df_minute,
        df_anchor=df_anchor,
        title='前一天—当天—后一天的日中心流量线性插值结果',
        output_png='noon_interp_curve.png'
    )

    print("\n前5行结果预览：")
    print(df_minute.head())
    print("\n后5行结果预览：")
    print(df_minute.tail())


if __name__ == '__main__':
    main()
