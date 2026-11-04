import re
import pandas as pd
import matplotlib.pyplot as plt


def parse_lakebudget_log(log_path):
    """
    解析 SEIMS_HAND LakeBudget 调试日志，提取每个时间步的:
      - datetime       : 真实时间（从日志行 'YYYY-MM-DD HH:MM:SS' 读取）
      - rchID          : 河段/湖泊 ID
      - TotalIn        : 当天总入流量 (m3/day)
      - TotalOut       : 当天总出流量 (m3/day)
      - Storage_pre    : 当天开始时库容 (m3)
      - Storage_after  : 当天结束时库容 (m3)

    支持多年份、跨年连续模拟。
    """
    records = []

    # ===== 各种正则 =====
    # LakeBudget 调试块头
    header_pattern = re.compile(
        r"===== LakeBudget Debug:\s*rchID=(\d+)\s*Day=(\d+)"
    )
    # 日志时间行，如：2010-01-01 00:00:00
    datetime_pattern = re.compile(
        r"^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})$"
    )

    totalin_pattern = re.compile(r"TotalIn=([+-]?\d+\.?\d*(?:e[+-]?\d+)?)")
    totalout_pattern = re.compile(r"TotalOut=([+-]?\d+\.?\d*(?:e[+-]?\d+)?)")
    storage_after_pattern = re.compile(r"Storage_after=([+-]?\d+\.?\d*(?:e[+-]?\d+)?)")
    storage_pre_pattern = re.compile(r"Storage_pre=([+-]?\d+\.?\d*(?:e[+-]?\d+)?)")

    # ===== 状态变量 =====
    current_datetime = None   # 当前时间步的真实时间
    current_rchid = None
    current_day = None        # 只是记录一下 Day=，不用于时间轴
    current_total_in = None
    current_total_out = None
    current_storage_pre = None
    current_storage_after = None
    inside_block = False

    with open(log_path, "r", encoding="utf-8") as f:
        for raw_line in f:
            line = raw_line.strip()

            # 1) 捕捉时间行：YYYY-MM-DD HH:MM:SS
            m_dt = datetime_pattern.match(line)
            if m_dt:
                current_datetime = m_dt.group(1)
                continue

            # 2) LakeBudget 块头
            m_header = header_pattern.match(line)
            if m_header:
                # 如果前一个块未写入，先写入
                if inside_block and current_total_in is not None:
                    records.append({
                        "datetime": current_datetime,
                        "rchID": current_rchid,
                        "day": current_day,
                        "TotalIn": current_total_in,
                        "TotalOut": current_total_out,
                        "Storage_pre": current_storage_pre,
                        "Storage_after": current_storage_after,
                    })

                current_rchid = int(m_header.group(1))
                current_day = int(m_header.group(2))  # 仅供参考，可不用
                current_total_in = None
                current_total_out = None
                current_storage_pre = None
                current_storage_after = None
                inside_block = True
                continue

            if not inside_block:
                continue

            # 3) Inflow / Outflow / Storage 信息
            # [Initial] Storage_pre=...
            if line.startswith("[Initial]"):
                m = storage_pre_pattern.search(line)
                if m:
                    current_storage_pre = float(m.group(1))
                continue

            if line.startswith("[Inflow]"):
                m = totalin_pattern.search(line)
                if m:
                    current_total_in = float(m.group(1))
                continue

            if line.startswith("[Outflow]"):
                m = totalout_pattern.search(line)
                if m:
                    current_total_out = float(m.group(1))
                continue

            if line.startswith("[Delta Storage]"):
                m = storage_after_pattern.search(line)
                if m:
                    current_storage_after = float(m.group(1))
                continue

            # 4) 块结束分隔线
            if line.startswith("============================================="):
                if current_total_in is not None:
                    records.append({
                        "datetime": current_datetime,
                        "rchID": current_rchid,
                        "day": current_day,
                        "TotalIn": current_total_in,
                        "TotalOut": current_total_out,
                        "Storage_pre": current_storage_pre,
                        "Storage_after": current_storage_after,
                    })
                inside_block = False

    # ===== 生成 DataFrame =====
    df = pd.DataFrame(records)
    if df.empty:
        return df

    # 将字符串时间转为 pandas 的 datetime
    df["datetime"] = pd.to_datetime(df["datetime"])
    # 衍生一些方便用的列：year、day_of_year
    df["year"] = df["datetime"].dt.year
    df["doy"] = df["datetime"].dt.dayofyear

    df = df.sort_values(by=["rchID", "datetime"]).reset_index(drop=True)
    return df


def plot_lake_timeseries(
        df,
        rchid=None,
        start=None,
        end=None,
        vars_to_plot=None
    ):
    """
    使用双 y 轴绘制点状图（scatter），x 轴为真实日期，可跨年连续。

    参数：
        rchid : 指定湖泊/河段 ID
        start, end : 时间过滤
        vars_to_plot : list[str] 要绘制的变量，例如：
              ["TotalIn", "TotalOut", "Storage_pre", "Storage_after"]
              若为 None，则绘制全部四个
    """

    if df.empty:
        print("DataFrame 为空，无法绘图")
        return

    data = df.copy()

    # --- 1) rchID 过滤 ---
    if rchid is not None:
        data = data[data["rchID"] == rchid]
        if data.empty:
            print(f"rchID={rchid} 无数据")
            return

    # --- 2) 时间过滤 ---
    if start is not None:
        data = data[data["datetime"] >= pd.to_datetime(start)]
    if end is not None:
        data = data[data["datetime"] <= pd.to_datetime(end)]
    if data.empty:
        print("指定时间范围无数据")
        return

    # --- 3) 要绘制的变量 ---
    all_vars = ["TotalIn", "TotalOut", "Storage_pre", "Storage_after"]

    if vars_to_plot is None:
        vars_to_plot = all_vars
    else:
        # 过滤掉不存在的变量
        vars_to_plot = [v for v in vars_to_plot if v in all_vars]

    if not vars_to_plot:
        print("没有有效的变量可绘制")
        return

    # --- 颜色方案（可按需替换）---
    color_map = {
        "TotalIn": "black",
        "TotalOut": "red",
        "Storage_after": "blue",
        "Storage_pre": "green",
    }

    # --- 左右轴变量分组 ---
    left_vars = [v for v in vars_to_plot if v in ["TotalIn", "TotalOut"]]
    right_vars = [v for v in vars_to_plot if v in ["Storage_pre", "Storage_after"]]

    x = data["datetime"]

    fig, ax1 = plt.subplots(figsize=(12, 6))

    # ========== 左轴 ==========
    for v in left_vars:
        ax1.scatter(
            x, data[v],
            s=10, alpha=0.7,
            color=color_map[v],
            label=v
        )

    ax1.set_ylabel("In/Out (m³/day)")
    ax1.grid(True, linestyle="--", alpha=0.4)

    # ========== 右轴 ==========
    ax2 = ax1.twinx()
    for v in right_vars:
        ax2.scatter(
            x, data[v],
            s=10, alpha=0.7,
            color=color_map[v],
            label=v
        )
    ax2.set_ylabel("Storage (m³)")

    # --- 合并图例 ---
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc="upper right")

    title = "LakeBudget (scatter, real datetime)"
    if rchid is not None:
        title += f"  rchID={rchid}"
    plt.title(title)

    fig.autofmt_xdate()
    plt.tight_layout()
    plt.show()



# ================= 使用示例 =================
if __name__ == "__main__":
    log_file = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171\output_test_1171_bak.txt"

    df_lake = parse_lakebudget_log(log_file)
    print(df_lake.head())
    print(df_lake[["datetime", "rchID", "TotalIn", "TotalOut",
                   "Storage_pre", "Storage_after"]].head())

    # 示例：只看 2010 年
    plot_lake_timeseries(df_lake, rchid=1171,
                         start="2010-01-01", end="2010-12-31",
                         vars_to_plot=["TotalIn","TotalOut", "Storage_after"])
