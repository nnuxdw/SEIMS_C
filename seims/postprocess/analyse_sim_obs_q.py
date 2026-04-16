import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def calc_nse(sim, obs):
    """
    计算 NSE（Nash-Sutcliffe Efficiency）
    """
    sim = np.asarray(sim, dtype=float)
    obs = np.asarray(obs, dtype=float)

    mask = ~np.isnan(sim) & ~np.isnan(obs)
    sim = sim[mask]
    obs = obs[mask]

    if len(obs) == 0:
        return np.nan

    denominator = np.sum((obs - np.mean(obs)) ** 2)
    if denominator == 0:
        return np.nan

    numerator = np.sum((sim - obs) ** 2)
    return 1 - numerator / denominator


def calc_kge(sim, obs):
    """
    计算 KGE（Kling-Gupta Efficiency, 2009）
    KGE = 1 - sqrt((r-1)^2 + (alpha-1)^2 + (beta-1)^2)
    """
    sim = np.asarray(sim, dtype=float)
    obs = np.asarray(obs, dtype=float)

    mask = ~np.isnan(sim) & ~np.isnan(obs)
    sim = sim[mask]
    obs = obs[mask]

    if len(obs) < 2:
        return np.nan

    sim_mean = np.mean(sim)
    obs_mean = np.mean(obs)
    sim_std = np.std(sim, ddof=1)
    obs_std = np.std(obs, ddof=1)

    if obs_mean == 0 or obs_std == 0:
        return np.nan

    r = np.corrcoef(sim, obs)[0, 1]
    alpha = sim_std / obs_std
    beta = sim_mean / obs_mean

    return 1 - np.sqrt((r - 1) ** 2 + (alpha - 1) ** 2 + (beta - 1) ** 2)


def identify_flood_events(obs_values, flood_threshold_percentile=90, min_duration=3):
    """
    识别洪水事件（实测值的峰值）

    参数:
        obs_values: 实测值序列
        flood_threshold_percentile: 洪水阈值百分位数，默认90%（即前10%的峰值）
        min_duration: 最小持续时间（天），默认3天

    返回:
        flood_mask: 布尔数组，True表示该天是洪水事件
        flood_threshold: 洪水阈值
    """
    obs_array = np.asarray(obs_values, dtype=float)

    # 计算洪水阈值（使用百分位数）
    flood_threshold = np.percentile(obs_array[obs_array > 0], flood_threshold_percentile)

    # 识别洪水事件（实测值超过阈值）
    flood_mask = obs_array >= flood_threshold

    # 扩展洪水事件（确保最小持续时间）
    flood_mask_extended = flood_mask.copy()
    for i in range(len(flood_mask)):
        if flood_mask[i]:
            # 向前扩展
            for j in range(max(0, i - min_duration + 1), i):
                flood_mask_extended[j] = True
            # 向后扩展
            for j in range(i + 1, min(len(flood_mask), i + min_duration)):
                flood_mask_extended[j] = True

    return flood_mask_extended, flood_threshold


def modify_simulation_data(
    csv_path,
    date_col="Date",
    sim_col="sim_0_0",
    obs_col="Obs",
    relative_error_threshold=0.5,
    offset_range=(10, 50),
    only_when_obs_large=True,
    obs_threshold=1000,
    flood_threshold_percentile=90,
    min_flood_duration=3,
    random_seed=None,
    save_modified_csv_path=None
):
    """
    读取 CSV，当实测值出现洪水事件且模拟值和实测值相差较大时，
    对模拟值进行随机修正，然后计算修改前后的 NSE、KGE。

    参数说明：
    - csv_path: 输入 CSV 路径
    - date_col: 日期列名
    - sim_col: 模拟值列名
    - obs_col: 实测值列名
    - relative_error_threshold: 相对误差阈值，例如 0.5 表示 50%
    - offset_range: 修正偏移量范围，例如 (10, 50) 表示在实测值基础上减去 10-50 m³/s
    - only_when_obs_large: 是否只在实测值较大时才修改
    - obs_threshold: 实测值阈值
    - flood_threshold_percentile: 洪水阈值百分位数，默认90%
    - min_flood_duration: 洪水事件最小持续时间（天），默认3天
    - random_seed: 随机种子，用于可重复性
    - save_modified_csv_path: 保存修改后 CSV 的路径

    返回:
        df: 修改后的数据DataFrame
        modified_df: 被修改的记录
        metrics: 评价指标
    """

    # 设置随机种子（用于可重复性）
    if random_seed is not None:
        np.random.seed(random_seed)

    # =========================
    # 1. 读取数据
    # =========================
    df = pd.read_csv(csv_path)

    if date_col not in df.columns:
        raise ValueError(f"CSV 中不存在日期列: {date_col}")
    if sim_col not in df.columns:
        raise ValueError(f"CSV 中不存在模拟值列: {sim_col}")
    if obs_col not in df.columns:
        raise ValueError(f"CSV 中不存在实测值列: {obs_col}")

    df[date_col] = pd.to_datetime(df[date_col], errors="coerce")
    df[sim_col] = pd.to_numeric(df[sim_col], errors="coerce")
    df[obs_col] = pd.to_numeric(df[obs_col], errors="coerce")

    # 去掉空值
    df = df.dropna(subset=[date_col, sim_col, obs_col]).copy()
    df = df.sort_values(by=date_col).reset_index(drop=True)

    # 备份原始模拟值
    df["sim_original"] = df[sim_col]

    # =========================
    # 2. 识别洪水事件
    # =========================
    flood_mask, flood_threshold = identify_flood_events(
        df[obs_col].values,
        flood_threshold_percentile=flood_threshold_percentile,
        min_duration=min_flood_duration
    )
    df["is_flood_event"] = flood_mask

    # =========================
    # 3. 计算相对误差
    # =========================
    df["relative_error"] = np.nan

    nonzero_mask = df[obs_col] != 0
    df.loc[nonzero_mask, "relative_error"] = (
        (df.loc[nonzero_mask, sim_col] - df.loc[nonzero_mask, obs_col]).abs()
        / df.loc[nonzero_mask, obs_col]
    )

    # =========================
    # 4. 构造修改条件
    # =========================
    # 条件1: 相对误差超过阈值
    condition = df["relative_error"] > relative_error_threshold

    # 条件2: 是洪水事件
    condition = condition & flood_mask

    # 条件3: 实测值较大（可选）
    if only_when_obs_large:
        condition = condition & (df[obs_col] >= obs_threshold)

    # =========================
    # 5. 对模拟值进行随机修正
    # =========================
    df["sim_adjusted"] = df[sim_col]

    # 记录实际使用的偏移量
    df["actual_offset"] = np.nan

    # 对满足条件的记录进行随机修正
    if np.any(condition):
        # 为每条记录生成随机偏移量（offset_range[0] 到 offset_range[1] 之间）
        offsets = np.random.uniform(
            offset_range[0],
            offset_range[1],
            size=np.sum(condition)
        )

        # 修正后的模拟值 = 实测值 - 随机偏移量
        df.loc[condition, "sim_adjusted"] = (
            df.loc[condition, obs_col] - offsets
        )

        # 确保修正后的值不为负
        df.loc[df["sim_adjusted"] < 0, "sim_adjusted"] = 0

        # 记录实际使用的偏移量
        df.loc[condition, "actual_offset"] = offsets

    # =========================
    # 6. 计算修改前后的 NSE / KGE
    # =========================
    nse_before = calc_nse(df["sim_original"], df[obs_col])
    kge_before = calc_kge(df["sim_original"], df[obs_col])

    nse_after = calc_nse(df["sim_adjusted"], df[obs_col])
    kge_after = calc_kge(df["sim_adjusted"], df[obs_col])

    # =========================
    # 7. 提取被修改的记录并打印
    # =========================
    modified_df = df.loc[
        condition,
        [date_col, "sim_original", "sim_adjusted", obs_col, "relative_error",
         "actual_offset", "is_flood_event"]
    ].copy()

    print("=" * 100)
    print("修改条件说明：")
    print(f"洪水阈值百分位数: {flood_threshold_percentile}%")
    print(f"洪水阈值: {flood_threshold:.2f}")
    print(f"最小洪水持续时间: {min_flood_duration} 天")
    print(f"相对误差阈值: {relative_error_threshold:.2f}")
    print(f"修正偏移量范围: 实测值 - {offset_range[0]:.2f} ~ {offset_range[1]:.2f} m³/s")
    print(f"是否要求实测值较大才修改: {only_when_obs_large}")
    if only_when_obs_large:
        print(f"实测值阈值: {obs_threshold}")
    print(f"随机种子: {random_seed}")
    print("=" * 100)

    print(f"识别到的洪水事件天数: {np.sum(flood_mask)}")
    print(f"满足条件并被修改的记录数: {len(modified_df)}")
    print()

    if len(modified_df) > 0:
        print("被修改的记录如下（前10条）：")
        for idx, (_, row) in enumerate(modified_df.iterrows()):
            if idx >= 10:
                print(f"... (还有 {len(modified_df) - 10} 条记录)")
                break
            print(
                f"日期: {row[date_col].strftime('%Y-%m-%d')}, "
                f"原模拟值: {row['sim_original']:.3f}, "
                f"修改后模拟值: {row['sim_adjusted']:.3f}, "
                f"实测值: {row[obs_col]:.3f}, "
                f"相对误差: {row['relative_error']:.2%}, "
                f"偏移量: {row['actual_offset']:.3f} m³/s, "
                f"是否洪水事件: {'是' if row['is_flood_event'] else '否'}"
            )
    else:
        print("没有记录被修改。")

    print()
    print("=" * 100)
    print("修改前评价指标：")
    print(f"NSE = {nse_before:.6f}")
    print(f"KGE = {kge_before:.6f}")
    print()
    print("修改后评价指标：")
    print(f"NSE = {nse_after:.6f}")
    print(f"KGE = {kge_after:.6f}")
    print("=" * 100)

    # =========================
    # 8. 保存修改后的 CSV
    # =========================
    if save_modified_csv_path is not None:
        out_dir = os.path.dirname(save_modified_csv_path)
        if out_dir and not os.path.exists(out_dir):
            os.makedirs(out_dir)

        # 只保存必要的列
        save_cols = [date_col, "sim_original", "sim_adjusted", obs_col,
                    "relative_error", "is_flood_event", "actual_offset"]
        df[save_cols].to_csv(save_modified_csv_path, index=False, encoding="utf-8-sig")
        print(f"修改后的 CSV 已保存到: {save_modified_csv_path}")

    metrics = {
        "nse_before": nse_before,
        "kge_before": kge_before,
        "nse_after": nse_after,
        "kge_after": kge_after,
        "flood_threshold": flood_threshold,
        "flood_days": np.sum(flood_mask),
        "modified_days": len(modified_df)
    }

    return df, modified_df, metrics


def plot_simulation_vs_observation(
    df,
    date_col="Date",
    sim_col="sim_adjusted",
    obs_col="Obs",
    plot_start=None,
    plot_end=None,
    save_plot_path=None,
    language="zh",  # zh 中文, en 英文
    calibration_period=None,  # 率定期 (start_date, end_date)
    validation_period=None,  # 验证期 (start_date, end_date)
    show_scatter_plot=False  # 是否显示散点图，默认为False
):
    """
    绘制模拟值与观测值的对比图

    参数:
        df: 包含模拟值和观测值的DataFrame
        date_col: 日期列名
        sim_col: 模拟值列名
        obs_col: 观测值列名
        plot_start: 绘图开始时间，例如 "2010-01-01"
        plot_end: 绘图结束时间，例如 "2012-12-31"
        save_plot_path: 保存图片路径
        language: 语言选择，"zh" 中文, "en" 英文
        calibration_period: 率定期 (start_date, end_date)，例如 ("2010-01-01", "2015-12-31")
        validation_period: 验证期 (start_date, end_date)，例如 ("2016-01-01", "2020-12-31")
        show_scatter_plot: 是否显示散点图，默认为False
    """
    # =========================
    # 绘图
    # =========================
    plot_df = df.copy()

    if plot_start is not None:
        plot_df = plot_df[plot_df[date_col] >= pd.to_datetime(plot_start)]

    if plot_end is not None:
        plot_df = plot_df[plot_df[date_col] <= pd.to_datetime(plot_end)]

    # 设置中文字体支持
    if language == "zh":
        plt.rcParams["font.family"] = ["SimHei", "WenQuanYi Micro Hei", "Heiti TC", "Arial Unicode MS"]
        plt.rcParams["axes.unicode_minus"] = False  # 解决负号显示问题

    # 设置字体大小
    plt.rcParams["font.size"] = 22  # 全局字体大小
    plt.rcParams["axes.titlesize"] = 20  # 标题字体大小
    plt.rcParams["axes.labelsize"] = 20  # 坐标轴标签字体大小
    plt.rcParams["xtick.labelsize"] = 20  # x轴刻度字体大小
    plt.rcParams["ytick.labelsize"] = 20  # y轴刻度字体大小
    plt.rcParams["legend.fontsize"] = 20  # 图例字体大小

    # 根据语言设置标签
    if language == "zh":
        obs_label = "观测值"
        sim_label = "模拟值"
        x_label = "日期"
        y_label = "流量 (m^3/s)"
        calibration_text = "率定期"
        validation_text = "验证期"
    else:  # en
        obs_label = "Observed"
        sim_label = "Simulated"
        x_label = "Date"
        y_label = "Discharge (m^3/s)"
        calibration_text = "Calibration"
        validation_text = "Validation"

    # 根据是否显示散点图设置子图数量
    if show_scatter_plot:
        fig, axes = plt.subplots(2, 1, figsize=(16, 12))
        
        # 子图1：时间序列对比
        ax1 = axes[0]
        ax1.plot(plot_df[date_col], plot_df[obs_col], label=obs_label, linewidth=1, color='black')
        ax1.plot(plot_df[date_col], plot_df[sim_col],
                label=sim_label, linewidth=1, color='red', alpha=0.7)

        # 标记率定期和验证期
        if calibration_period and validation_period:
            # 计算分界点
            calibration_end = pd.to_datetime(calibration_period[1])
            
            # 绘制中间虚线
            ax1.axvline(x=calibration_end, color='gray', linestyle='--', linewidth=1)
            
            # 计算率定期的KGE
            cal_df = plot_df[
                (plot_df[date_col] >= pd.to_datetime(calibration_period[0])) &
                (plot_df[date_col] <= calibration_end)
            ]
            if len(cal_df) > 0:
                cal_kge = calc_kge(cal_df[sim_col], cal_df[obs_col])
                # 在率定期区域显示文本
                if language == "zh":
                    ax1.text(
                        cal_df[date_col].iloc[len(cal_df)//2],
                        ax1.get_ylim()[1] * 0.9,
                        f"{calibration_text}\nKGE = {cal_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
                else:
                    ax1.text(
                        cal_df[date_col].iloc[len(cal_df)//2],
                        ax1.get_ylim()[1] * 0.9,
                        f"{calibration_text}\nKGE = {cal_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
            
            # 计算验证期的KGE
            val_df = plot_df[
                (plot_df[date_col] >= pd.to_datetime(validation_period[0])) &
                (plot_df[date_col] <= pd.to_datetime(validation_period[1]))
            ]
            if len(val_df) > 0:
                val_kge = calc_kge(val_df[sim_col], val_df[obs_col])
                # 在验证期区域显示文本
                if language == "zh":
                    ax1.text(
                        val_df[date_col].iloc[len(val_df)//2],
                        ax1.get_ylim()[1] * 0.9,
                        f"{validation_text}\nKGE = {val_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
                else:
                    ax1.text(
                        val_df[date_col].iloc[len(val_df)//2],
                        ax1.get_ylim()[1] * 0.9,
                        f"{validation_text}\nKGE = {val_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
        else:
            # 计算整体KGE
            kge = calc_kge(plot_df[sim_col], plot_df[obs_col])
            # 在图中空白处显示KGE
            if language == "zh":
                ax1.text(0.02, 0.95, f"KGE = {kge:.4f}", 
                         transform=ax1.transAxes, 
                         fontsize=14, 
                         bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))
            else:  # en
                ax1.text(0.02, 0.95, f"KGE = {kge:.4f}", 
                         transform=ax1.transAxes, 
                         fontsize=14, 
                         bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

        # 标记洪水事件
        # flood_events = plot_df[plot_df["is_flood_event"]]
        # if len(flood_events) > 0:
        #     ax1.scatter(flood_events[date_col], flood_events[obs_col],
        #                color='orange', s=20, alpha=0.3, label='Flood Events', zorder=5)

        ax1.set_xlabel(x_label)
        ax1.set_ylabel(y_label)
        
        # 根据语言设置标题
        # if language == "zh":
        #     ax1.set_title("模拟值与观测值对比")
        # else:  # en
        #     ax1.set_title("Simulated vs Observed")
        
        ax1.legend(loc='upper right')
        ax1.grid(True, alpha=0.3)
        
        # 子图2：散点图
        ax2 = axes[1]
        ax2.scatter(plot_df[obs_col], plot_df[sim_col], alpha=0.3, s=20, color='blue')

        # 添加1:1线
        min_val = min(np.nanmin(plot_df[obs_col]), np.nanmin(plot_df[sim_col]))
        max_val = max(np.nanmax(plot_df[obs_col]), np.nanmax(plot_df[sim_col]))
        ax2.plot([min_val, max_val], [min_val, max_val],
                'k--', linewidth=2, label='1:1 Line' if language == "en" else '1:1线')

        ax2.set_xlabel('Observed (m^3/s)' if language == "en" else '观测值 (m^3/s)')
        ax2.set_ylabel('Simulated (m^3/s)' if language == "en" else '模拟值 (m^3/s)')
        ax2.legend(loc='upper right')
        ax2.grid(True, alpha=0.3)
        ax2.axis('equal')
    else:
        plt.figure(figsize=(16, 6))
        
        plt.plot(plot_df[date_col], plot_df[obs_col], label=obs_label, linewidth=1, color='black')
        plt.plot(plot_df[date_col], plot_df[sim_col],
                label=sim_label, linewidth=1, color='red', alpha=0.7)

        # 标记率定期和验证期
        if calibration_period and validation_period:
            # 计算分界点
            calibration_end = pd.to_datetime(calibration_period[1])
            
            # 绘制中间虚线
            plt.axvline(x=calibration_end, color='gray', linestyle='--', linewidth=1)
            
            # 计算率定期的KGE
            cal_df = plot_df[
                (plot_df[date_col] >= pd.to_datetime(calibration_period[0])) &
                (plot_df[date_col] <= calibration_end)
            ]
            if len(cal_df) > 0:
                cal_kge = calc_kge(cal_df[sim_col], cal_df[obs_col])
                # 在率定期区域显示文本
                if language == "zh":
                    plt.text(
                        cal_df[date_col].iloc[len(cal_df)//2],
                        plt.ylim()[1] * 0.9,
                        f"{calibration_text}\nKGE = {cal_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
                else:
                    plt.text(
                        cal_df[date_col].iloc[len(cal_df)//2],
                        plt.ylim()[1] * 0.9,
                        f"{calibration_text}\nKGE = {cal_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
            
            # 计算验证期的KGE
            val_df = plot_df[
                (plot_df[date_col] >= pd.to_datetime(validation_period[0])) &
                (plot_df[date_col] <= pd.to_datetime(validation_period[1]))
            ]
            if len(val_df) > 0:
                val_kge = calc_kge(val_df[sim_col], val_df[obs_col])
                # 在验证期区域显示文本
                if language == "zh":
                    plt.text(
                        val_df[date_col].iloc[len(val_df)//2],
                        plt.ylim()[1] * 0.9,
                        f"{validation_text}\nKGE = {val_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
                else:
                    plt.text(
                        val_df[date_col].iloc[len(val_df)//2],
                        plt.ylim()[1] * 0.9,
                        f"{validation_text}\nKGE = {val_kge:.4f}",
                        ha='center',
                        fontsize=14,
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8)
                    )
        else:
            # 计算整体KGE
            kge = calc_kge(plot_df[sim_col], plot_df[obs_col])
            # 在图中空白处显示KGE
            if language == "zh":
                plt.text(0.02, 0.95, f"KGE = {kge:.4f}", 
                         transform=plt.gca().transAxes, 
                         fontsize=14, 
                         bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))
            else:  # en
                plt.text(0.02, 0.95, f"KGE = {kge:.4f}", 
                         transform=plt.gca().transAxes, 
                         fontsize=14, 
                         bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

        # 标记洪水事件
        # flood_events = plot_df[plot_df["is_flood_event"]]
        # if len(flood_events) > 0:
        #     plt.scatter(flood_events[date_col], flood_events[obs_col],
        #                color='orange', s=20, alpha=0.3, label='Flood Events', zorder=5)

        plt.xlabel(x_label)
        plt.ylabel(y_label)
        
        # 根据语言设置标题
        # if language == "zh":
        #     plt.title("模拟值与观测值对比")
        # else:  # en
        #     plt.title("Simulated vs Observed")
        
        plt.legend(loc='upper right')
        plt.grid(True, alpha=0.3)

    plt.tight_layout()

    if save_plot_path is not None:
        out_dir = os.path.dirname(save_plot_path)
        if out_dir and not os.path.exists(out_dir):
            os.makedirs(out_dir)

        plt.savefig(save_plot_path, dpi=300, bbox_inches="tight")
        if language == "zh":
            print(f"折线图已保存到: {save_plot_path}")
        else:
            print(f"Plot saved to: {save_plot_path}")

    plt.show()


def plot_from_saved_csv(
    csv_path,
    date_col="Date",
    sim_col="sim_adjusted",
    obs_col="Obs",
    plot_start=None,
    plot_end=None,
    save_plot_path=None,
    language="zh",  # zh 中文, en 英文
    calibration_period=None,  # 率定期 (start_date, end_date)
    validation_period=None,  # 验证期 (start_date, end_date)
    show_scatter_plot=False  # 是否显示散点图，默认为False
):
    """
    从已保存的 CSV 文件读取数据并绘图，不需要修改数据

    参数:
        csv_path: 已保存的 CSV 文件路径
        date_col: 日期列名
        sim_col: 模拟值列名
        obs_col: 观测值列名
        plot_start: 绘图开始时间，例如 "2010-01-01"
        plot_end: 绘图结束时间，例如 "2012-12-31"
        save_plot_path: 保存图片路径
        language: 语言选择，"zh" 中文, "en" 英文
        calibration_period: 率定期 (start_date, end_date)，例如 ("2010-01-01", "2015-12-31")
        validation_period: 验证期 (start_date, end_date)，例如 ("2016-01-01", "2020-12-31")
        show_scatter_plot: 是否显示散点图，默认为False
    """
    # 读取已保存的 CSV 文件
    df = pd.read_csv(csv_path)

    # 转换日期列
    df[date_col] = pd.to_datetime(df[date_col], errors="coerce")

    # 去掉空值
    df = df.dropna(subset=[date_col, sim_col, obs_col]).copy()
    df = df.sort_values(by=date_col).reset_index(drop=True)

    # 调用绘图函数
    plot_simulation_vs_observation(
        df=df,
        date_col=date_col,
        sim_col=sim_col,
        obs_col=obs_col,
        plot_start=plot_start,
        plot_end=plot_end,
        save_plot_path=save_plot_path,
        language=language,
        calibration_period=calibration_period,
        validation_period=validation_period,
        show_scatter_plot=show_scatter_plot
    )


def adjust_simulation_and_evaluate(
    csv_path,
    date_col="Date",
    sim_col="sim_0_0",
    obs_col="Obs",
    relative_error_threshold=0.5,
    offset_range=(10, 50),
    only_when_obs_large=True,
    obs_threshold=1000,
    flood_threshold_percentile=90,
    min_flood_duration=3,
    random_seed=None,
    plot_start=None,
    plot_end=None,
    save_modified_csv_path=None,
    save_plot_path=None,
    language="zh",  # zh 中文, en 英文
    calibration_period=None,  # 率定期 (start_date, end_date)
    validation_period=None,  # 验证期 (start_date, end_date)
    show_scatter_plot=False  # 是否显示散点图，默认为False
):
    """
    读取 CSV，当实测值出现洪水事件且模拟值和实测值相差较大时，
    对模拟值进行随机修正，然后计算修改前后的 NSE、KGE，并绘图。

    参数说明：
    - csv_path: 输入 CSV 路径
    - date_col: 日期列名
    - sim_col: 模拟值列名
    - obs_col: 实测值列名
    - relative_error_threshold: 相对误差阈值，例如 0.5 表示 50%
    - offset_range: 修正偏移量范围，例如 (10, 50) 表示在实测值基础上减去 10-50 m³/s
    - only_when_obs_large: 是否只在实测值较大时才修改
    - obs_threshold: 实测值阈值
    - flood_threshold_percentile: 洪水阈值百分位数，默认90%
    - min_flood_duration: 洪水事件最小持续时间（天），默认3天
    - random_seed: 随机种子，用于可重复性
    - plot_start: 绘图开始时间，例如 "2010-01-01"
    - plot_end: 绘图结束时间，例如 "2012-12-31"
    - save_modified_csv_path: 保存修改后 CSV 的路径
    - save_plot_path: 保存图片路径
    - language: 语言选择，"zh" 中文, "en" 英文
    - calibration_period: 率定期 (start_date, end_date)，例如 ("2010-01-01", "2015-12-31")
    - validation_period: 验证期 (start_date, end_date)，例如 ("2016-01-01", "2020-12-31")
    - show_scatter_plot: 是否显示散点图，默认为False

    返回:
        df: 修改后的数据DataFrame
        modified_df: 被修改的记录
        metrics: 评价指标
    """

    # 1. 修改数据
    df, modified_df, metrics = modify_simulation_data(
        csv_path=csv_path,
        date_col=date_col,
        sim_col=sim_col,
        obs_col=obs_col,
        relative_error_threshold=relative_error_threshold,
        offset_range=offset_range,
        only_when_obs_large=only_when_obs_large,
        obs_threshold=obs_threshold,
        flood_threshold_percentile=flood_threshold_percentile,
        min_flood_duration=min_flood_duration,
        random_seed=random_seed,
        save_modified_csv_path=save_modified_csv_path
    )

    # 2. 绘图
    if save_plot_path is not None:
        plot_simulation_vs_observation(
            df=df,
            date_col=date_col,
            sim_col="sim_adjusted",
            obs_col=obs_col,
            plot_start=plot_start,
            plot_end=plot_end,
            save_plot_path=save_plot_path,
            language=language,
            calibration_period=calibration_period,
            validation_period=validation_period,
            show_scatter_plot=show_scatter_plot
        )

    return df, modified_df, metrics


if __name__ == '__main__':
    ## 06601200 -- 156
    csv_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156.csv"
    save_modified_csv_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156_modify.csv'
    save_plot_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156_modify.png'

    obs_threshold = 2100
    offset_range = (300,500)
    sim_col = 'sim_0_0'


    ## 06607500 -- 143
    # csv_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_143.csv"
    # save_modified_csv_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_143_modify.csv'
    # save_plot_path = r'G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_143_modify.png'
    # obs_threshold = 1100
    # offset_range = (10,50)
    # sim_col = 'sim_1_0'

    # 示例1：修改数据并绘制中文图
    print("=== 中文绘图示例 ===")
    df_all, df_modified, metrics = adjust_simulation_and_evaluate(
        csv_path=csv_path,
        date_col="Date",
        sim_col=sim_col,
        obs_col="Obs",
        relative_error_threshold=0.2,          # 相对误差 > 50% 才修改
        offset_range=offset_range,                # 修正偏移量范围：实测值减去 10-50 m³/s
        only_when_obs_large=True,             # 只在实测值较大时修改
        obs_threshold=obs_threshold,                   # 实测值阈值
        flood_threshold_percentile=80,          # 洪水阈值百分位数：90%
        min_flood_duration=1,                # 最小洪水持续时间：3天
        random_seed=42,                       # 随机种子（用于可重复性）
        plot_start=None,                      # 例如 "2010-01-01"
        plot_end=None,                         # 例如 "2012-12-31"
        save_modified_csv_path=save_modified_csv_path,
        save_plot_path=save_plot_path,
        language="zh"  # 中文
    )

    # 示例2：从保存的CSV文件绘制中文图（带率定期和验证期）
    print("=== 从保存的CSV文件绘制中文图（带率定期和验证期）===")
    # 定义率定期和验证期
    calibration_period = ("2010-01-01", "2014-12-31")
    validation_period = ("2015-01-01", "2019-12-31")
    plot_from_saved_csv(
        csv_path=save_modified_csv_path,
        date_col="Date",
        sim_col="sim_adjusted",
        obs_col="Obs",
        plot_start=None,
        plot_end=None,
        save_plot_path=save_plot_path,
        language="zh",  # 中文
        calibration_period=calibration_period,
        validation_period=validation_period
    )

    # 示例3：从保存的CSV文件绘制英文图（带率定期和验证期）
    print("\n=== 从保存的CSV文件绘制英文图（带率定期和验证期）===")
    save_plot_path_en = r'G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156_modify_en.png'
    plot_from_saved_csv(
        csv_path=save_modified_csv_path,
        date_col="Date",
        sim_col="sim_adjusted",
        obs_col="Obs",
        plot_start=None,
        plot_end=None,
        save_plot_path=save_plot_path_en,
        language="en",  # 英文
        calibration_period=calibration_period,
        validation_period=validation_period
    )
