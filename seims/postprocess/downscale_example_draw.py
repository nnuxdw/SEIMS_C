# -*- coding: utf-8 -*-
"""
根据前一天、当天、后一天的日平均流量，在“每天正午12:00”为锚点的条件下，
采用线性插值生成分钟尺度降尺度流量曲线，并绘制示意图。

修正内容：
1. 解决 matplotlib 中文乱码问题
2. x轴仅显示“前一天 / 当天 / 后一天”
3. 圆点表示每天正午12:00的日平均流量
4. 线表示插值得到的分钟尺度流量曲线
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d
from matplotlib import font_manager as fm
import matplotlib as mpl


def set_chinese_font():
    """
    自动寻找系统中可用的中文字体，解决中文乱码问题
    Windows 常见字体：Microsoft YaHei, SimHei, SimSun
    """
    candidate_fonts = [
        "Microsoft YaHei",
        "SimHei",
        "SimSun",
        "KaiTi",
        "FangSong",
        "Noto Sans CJK SC",
        "WenQuanYi Zen Hei",
        "Arial Unicode MS"
    ]

    available_fonts = {f.name for f in fm.fontManager.ttflist}

    for font_name in candidate_fonts:
        if font_name in available_fonts:
            mpl.rcParams["font.sans-serif"] = [font_name]
            mpl.rcParams["axes.unicode_minus"] = False
            print(f"已启用中文字体: {font_name}")
            return

    # 如果一个都没找到，给出提醒
    mpl.rcParams["axes.unicode_minus"] = False
    print("警告：未找到常见中文字体，图中中文可能仍无法正常显示。")



def generate_minute_boundary_by_noon_interp2(
    prev_date,
    curr_date,
    next_date,
    prev_flow,
    curr_flow,
    next_flow,
    freq="1min",
    clip_negative=True
):
    """
    使用与原始脚本 trend 模式一致的三点二次拉格朗日插值，
    将前一天、当天、后一天的日平均流量分别布设在正午12:00，
    再生成分钟尺度流量序列。
    """

    prev_date = pd.to_datetime(prev_date)
    curr_date = pd.to_datetime(curr_date)
    next_date = pd.to_datetime(next_date)

    # 三个日中心时刻：正午12:00
    anchor_times = pd.to_datetime([
        prev_date.normalize() + pd.Timedelta(hours=12),
        curr_date.normalize() + pd.Timedelta(hours=12),
        next_date.normalize() + pd.Timedelta(hours=12),
    ])
    anchor_flows = np.array([prev_flow, curr_flow, next_flow], dtype=float)

    df_anchor = pd.DataFrame({
        "datetime": anchor_times,
        "flow": anchor_flows
    })

    # 仍然保持完整三天的分钟时间轴
    # 只绘制从前一天正午12:00到后一天正午12:00
    start_time = prev_date.normalize() + pd.Timedelta(hours=12)
    end_time = next_date.normalize() + pd.Timedelta(hours=12)
    minute_times = pd.date_range(start=start_time, end=end_time, freq=freq)

    # 以“天”为单位构造自变量：
    # 前一天正午 -> 0
    # 当天正午   -> 1
    # 后一天正午 -> 2
    x0, x1, x2 = 0.0, 1.0, 2.0

    # 每个分钟节点对应的连续自变量
    # 比如前一天00:00会落在 -0.5，后一天23:59 会接近 2.5
    x = ((minute_times.values - anchor_times.values[0]) / np.timedelta64(1, "D")).astype(float)

    # 三点二次拉格朗日插值
    L0 = (x - x1) * (x - x2) / ((x0 - x1) * (x0 - x2))
    L1 = (x - x0) * (x - x2) / ((x1 - x0) * (x1 - x2))
    L2 = (x - x0) * (x - x1) / ((x2 - x0) * (x2 - x1))

    minute_flows = prev_flow * L0 + curr_flow * L1 + next_flow * L2

    # 防止出现负值
    if clip_negative:
        minute_flows = np.maximum(minute_flows, 0.0)

    df_minute = pd.DataFrame({
        "datetime": minute_times,
        "flow": minute_flows
    })

    return df_minute, df_anchor

def plot_noon_interp_curve(
    df_minute,
    df_anchor,
    title="流量交换示例",
    output_png="noon_interp_curve.png"
):
    """
    绘制分钟尺度插值曲线：
    - 折线：分钟尺度插值结果
    - 圆点：每天正午12:00的日平均流量
    - x轴：仅显示“前一天 / 当天 / 后一天”
    """

    # 先设置中文字体
    set_chinese_font()

    fig, ax = plt.subplots(figsize=(10, 5))

    # 插值曲线
    ax.plot(
        df_minute["datetime"],
        df_minute["flow"],
        linewidth=2.5,
        label="插值曲线"
    )

    # 正午锚点
    ax.scatter(
        df_anchor["datetime"],
        df_anchor["flow"],
        s=120,
        marker="o",
        zorder=5,
        label="日平均流量（正午12:00）",
        color="red"

    )

    # 数值标注
    # for _, row in df_anchor.iterrows():
    #     ax.annotate(
    #         f"{row['flow']:.2f}",
    #         xy=(row["datetime"], row["flow"]),
    #         xytext=(0, -14),  # 往下偏移
    #         textcoords="offset points",
    #         ha="center",
    #         va="top",
    #         fontsize=12,
    #     )

    # x轴只显示“前一天 / 当天 / 后一天”
    ax.set_xticks(df_anchor["datetime"])
    ax.set_xticklabels(["前一天（12:00）", "当天（12:00）", "后一天（12:00）"], fontsize=19)
    # 收紧左右显示范围，只在前后两个正午时刻外各留少量空白
    pad = pd.Timedelta(hours=1.5)  # 你可以改成 1 小时、2 小时都行
    ax.set_xlim(df_anchor["datetime"].iloc[0] - pad,
                df_anchor["datetime"].iloc[-1] + pad)

    # 去掉 matplotlib 默认的额外横向留白
    ax.margins(x=0)

    # 添加每天的分界线（00:00）
    start_day = df_minute["datetime"].dt.normalize().min()
    end_day = df_minute["datetime"].dt.normalize().max()
    day_marks = pd.date_range(start_day, end_day + pd.Timedelta(days=1), freq="1D")
    for d in day_marks:
        ax.axvline(d, linestyle="--", linewidth=1.0, alpha=0.35)

    # 添加正午12:00辅助线
    for d in df_anchor["datetime"]:
        ax.axvline(d, linestyle=":", linewidth=1.0, alpha=0.5)

    ax.set_xlabel("时间", fontsize=19, ha="right")
    ax.set_ylabel("流量（m³/s）", fontsize=19)
    ax.set_title(title, fontsize=20)
    ax.legend(loc="best",fontsize=16)
    ax.grid(True, linestyle="--", alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_png, dpi=300, bbox_inches="tight")
    plt.show()

    print(f"图片已保存到: {output_png}")


def save_result_csv(df_minute, output_csv="noon_interp_curve.csv"):
    """
    保存分钟尺度插值结果
    """
    df_minute.to_csv(output_csv, index=False, encoding="utf-8-sig")
    print(f"分钟尺度结果已保存到: {output_csv}")


if __name__ == "__main__":
    # ===== 日期保持一致 =====
    prev_date = "2024-07-09"
    curr_date = "2024-07-10"
    next_date = "2024-07-11"

    # ===== 4组流量参数 =====
    # 每一组依次表示：[前一天, 当天, 后一天]
    flow_groups = [
        [80.0, 110.0, 140.0],   # 1. 单调递增
        [140.0, 110.0, 80.0],   # 2. 单调递减
        [80.0, 140.0, 95.0],    # 3. 先增后减
        [140.0, 80.0, 120.0],   # 4. 先减后增
    ]

    # ===== 每组对应的标题 =====
    titles = [
        "单调递增型流量插值示例",
        "单调递减型流量插值示例",
        "先增后减型流量插值示例",
        "先减后增型流量插值示例",
    ]

    # ===== 每组对应的输出图片路径 =====
    output_png_list = [
        r"D:\lzu\博士论文\毕业论文\图\流量降尺度_单调递增.png",
        r"D:\lzu\博士论文\毕业论文\图\流量降尺度_单调递减.png",
        r"D:\lzu\博士论文\毕业论文\图\流量降尺度_先增后减.png",
        r"D:\lzu\博士论文\毕业论文\图\流量降尺度_先减后增.png",
    ]

    # ===== 每组对应的CSV文件名 =====
    output_csv_list = [
        "noon_interp_curve_单调递增.csv",
        "noon_interp_curve_单调递减.csv",
        "noon_interp_curve_先增后减.csv",
        "noon_interp_curve_先减后增.csv",
    ]

    # ===== 循环生成4组结果 =====
    for i, flows in enumerate(flow_groups):
        prev_flow, curr_flow, next_flow = flows

        print(f"\n===== 正在生成第 {i+1} 组：{titles[i]} =====")
        print(f"前一天流量 = {prev_flow}")
        print(f"当天流量   = {curr_flow}")
        print(f"后一天流量 = {next_flow}")

        # 生成分钟尺度流量
        df_minute, df_anchor = generate_minute_boundary_by_noon_interp2(
            prev_date=prev_date,
            curr_date=curr_date,
            next_date=next_date,
            prev_flow=prev_flow,
            curr_flow=curr_flow,
            next_flow=next_flow,
            freq="1min",
            clip_negative=True
        )

        # 保存CSV
        # save_result_csv(df_minute, output_csv=output_csv_list[i])

        # 绘图
        plot_noon_interp_curve(
            df_minute=df_minute,
            df_anchor=df_anchor,
            title=titles[i],
            output_png=output_png_list[i]
        )

        print("\n前5行结果预览：")
        print(df_minute.head())
        print("\n后5行结果预览：")
        print(df_minute.tail())
