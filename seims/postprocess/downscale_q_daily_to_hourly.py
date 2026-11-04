import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
from scipy.interpolate import interp1d


def generate_daily_curve_shape(curve_type='normal', n_seconds=86400, prev_flow=None, curr_flow=None, next_flow=None):
    """
    生成日内流量曲线形状（归一化，总和为1）

    参数:
        curve_type: 曲线类型
            - 'normal': 正态分布（单峰）
            - 'bimodal': 双峰分布（早晚高峰）
            - 'triangular': 三角分布（线性上升下降）
            - 'skewed': 偏态分布（洪峰偏后）
            - 'uniform': 均匀分布（作为基准）
            - 'trend': 趋势插值（基于前后天流量线性插值）
        n_seconds: 一天的秒数，默认86400
        prev_flow: 前一天的日流量（仅用于'trend'类型）
        curr_flow: 当天的日流量（仅用于'trend'类型）
        next_flow: 后一天的日流量（仅用于'trend'类型）

    返回:
        curve: 归一化的日内曲线（总和为1）
    """
    t = np.linspace(0, 1, n_seconds)

    if curve_type == 'normal':
        # 正态分布：峰值在中午（t=0.5）
        mean = 0.5
        std = 0.15
        curve = np.exp(-((t - mean) ** 2) / (2 * std ** 2))

    elif curve_type == 'bimodal':
        # 双峰分布：早晚高峰
        mean1, std1 = 0.35, 0.08
        mean2, std2 = 0.65, 0.08
        curve = 0.6 * np.exp(-((t - mean1) ** 2) / (2 * std1 ** 2)) + \
                0.4 * np.exp(-((t - mean2) ** 2) / (2 * std2 ** 2))

    elif curve_type == 'triangular':
        # 三角分布：线性上升后下降
        peak = 0.5
        curve = np.where(t < peak, t / peak, (1 - t) / (1 - peak))

    elif curve_type == 'skewed':
        # 偏态分布：洪峰偏后（模拟洪水过程）
        # 使用对数正态分布的形状
        curve = np.exp(-((np.log(t + 0.01) - (-0.5)) ** 2) / (2 * 0.5 ** 2))
        # 归一化到0-1范围
        curve = (curve - curve.min()) / (curve.max() - curve.min())

    elif curve_type == 'uniform':
        # 均匀分布
        curve = np.ones(n_seconds)

    elif curve_type == 'trend':
        # 趋势插值：基于前后天流量线性插值
        # 一天开始时（t=0）接近前一天流量，结束时（t=1）接近后一天流量
        if prev_flow is None or curr_flow is None or next_flow is None:
            # 如果缺少前后天数据，使用三角分布作为默认
            peak = 0.5
            curve = np.where(t < peak, t / peak, (1 - t) / (1 - peak))
        else:
            # 线性插值：从prev_flow到next_flow
            # 使用二次插值，使曲线更平滑
            # 三个控制点：(0, prev_flow), (0.5, curr_flow), (1, next_flow)
            # 使用拉格朗日插值
            L0 = (t - 0.5) * (t - 1) / ((0 - 0.5) * (0 - 1))
            L1 = (t - 0) * (t - 1) / ((0.5 - 0) * (0.5 - 1))
            L2 = (t - 0) * (t - 0.5) / ((1 - 0) * (1 - 0.5))
            curve = prev_flow * L0 + curr_flow * L1 + next_flow * L2
            # 归一化到0-1范围
            curve = (curve - curve.min()) / (curve.max() - curve.min() + 1e-10)

    else:
        raise ValueError(f"未知的曲线类型: {curve_type}")

    # 归一化，使总和为1
    curve = curve / curve.sum()

    return curve


def select_curve_type_based_on_flow(daily_flow, flood_threshold=None):
    """
    根据日流量大小选择合适的日内曲线类型

    参数:
        daily_flow: 日流量值
        flood_threshold: 洪水阈值，超过此值使用偏态分布

    返回:
        curve_type: 曲线类型
    """
    if flood_threshold is not None and daily_flow > flood_threshold:
        # 洪水事件：使用偏态分布（洪峰偏后）
        return 'skewed'
    elif daily_flow > 1000:
        # 大流量：使用正态分布
        return 'normal'
    elif daily_flow > 500:
        # 中等流量：使用双峰分布
        return 'bimodal'
    else:
        # 小流量：使用三角分布
        return 'triangular'


def downscale_daily_to_interval(
    daily_csv_path,
    output_csv_path=None,
    value_col='sim_adjusted',
    date_col='Date',
    curve_type='auto',
    flood_threshold=None,
    interval_minutes=15,
    add_noise=True,
    noise_level=0.05,
    random_seed=None
):
    """
    将日尺度径流量降尺度到指定时间间隔（默认15分钟）

    参数:
        daily_csv_path: 日尺度CSV文件路径
        output_csv_path: 输出CSV文件路径（可选）
        value_col: 流量值列名
        date_col: 日期列名
        curve_type: 日内曲线类型
            - 'auto': 自动根据流量大小选择
            - 'normal', 'bimodal', 'triangular', 'skewed', 'uniform', 'trend'
        flood_threshold: 洪水阈值（用于自动选择曲线类型）
        interval_minutes: 输出时间间隔（分钟），默认15分钟
        add_noise: 是否添加随机噪声
        noise_level: 噪声水平（标准差相对于均值）
        random_seed: 随机种子

    返回:
        df_interval: 指定间隔数据DataFrame
    """

    if random_seed is not None:
        np.random.seed(random_seed)

    # 读取日尺度数据
    df_daily = pd.read_csv(daily_csv_path)
    df_daily[date_col] = pd.to_datetime(df_daily[date_col])
    df_daily = df_daily.sort_values(date_col).reset_index(drop=True)

    # 计算每天的间隔数
    n_intervals_per_day = int(24 * 60 / interval_minutes)

    # 准备间隔数据列表
    interval_data = []

    print("=" * 80)
    print(f"开始降尺度：日尺度 → {interval_minutes}分钟间隔")
    print("=" * 80)
    print(f"输入文件: {daily_csv_path}")
    print(f"日尺度记录数: {len(df_daily)}")
    print(f"输出间隔: {interval_minutes} 分钟")
    print(f"每天间隔数: {n_intervals_per_day}")
    print(f"曲线类型: {curve_type}")
    if flood_threshold is not None:
        print(f"洪水阈值: {flood_threshold:.2f} m³/s")
    print(f"添加噪声: {add_noise}")
    if add_noise:
        print(f"噪声水平: {noise_level:.2%}")
    print("=" * 80)

    # 处理每一天
    for idx, row in df_daily.iterrows():
        date = row[date_col]
        daily_flow = row[value_col]

        # 跳过缺失值
        if pd.isna(daily_flow) or daily_flow <= 0:
            continue

        # 选择日内曲线类型
        if curve_type == 'auto':
            actual_curve_type = select_curve_type_based_on_flow(daily_flow, flood_threshold)
        else:
            actual_curve_type = curve_type

        # 生成日内曲线形状
        if actual_curve_type == 'trend':
            # 对于trend类型，暂时使用均匀分布
            # 稍后会根据日流量直线连接，计算每个时刻的实际流量
            curve_shape = np.ones(n_intervals_per_day)
        else:
            curve_shape = generate_daily_curve_shape(
                actual_curve_type,
                n_intervals_per_day,
                prev_flow=None,
                curr_flow=daily_flow,
                next_flow=None
            )

        # 计算间隔尺度流量
        if actual_curve_type == 'trend':
            # 对于trend类型，暂时设置为日流量
            # 稍后会根据日流量直线连接进行修正
            interval_flow = np.full(n_intervals_per_day, daily_flow)
        else:
            # 对于其他类型，curve_shape是归一化的分布，需要乘以总流量
            # 日流量单位：m³/s，表示该日的平均流量
            # 总流量 = 日流量 × 86400 秒
            total_flow = daily_flow * 86400
            # 间隔尺度流量 = 总流量 × 曲线形状 / (interval_minutes * 60)（转换为m³/s）
            interval_flow = total_flow * curve_shape / (interval_minutes * 60)

        # 生成时间戳
        start_time = datetime.combine(date.date(), datetime.min.time())
        timestamps = [start_time + timedelta(minutes=i * interval_minutes) for i in range(n_intervals_per_day)]

        # 添加到结果列表
        for i, (ts, flow) in enumerate(zip(timestamps, interval_flow)):
            interval_data.append({
                'datetime': ts,
                'date': date.date(),
                'hour': i * interval_minutes // 60,
                'minute': i * interval_minutes % 60,
                'interval': i,
                'daily_flow': daily_flow,
                'interval_flow': flow,
                'curve_type': actual_curve_type
            })

        # 进度提示
        if (idx + 1) % 10 == 0:
            print(f"已处理: {idx + 1}/{len(df_daily)} 天")

    # 转换为DataFrame
    df_interval = pd.DataFrame(interval_data)

    # 对trend类型进行全局插值，确保连续性
    if curve_type == 'trend' and len(df_interval) > 0:
        print("\n对trend类型进行全局线性插值...")

        # 提取所有trend类型的数据点
        trend_mask = df_interval['curve_type'] == 'trend'
        df_trend = df_interval[trend_mask].copy()

        if len(df_trend) > 2:
            # 创建时间戳（转换为数值）
            timestamps = df_trend['datetime'].values
            time_numeric = (timestamps - timestamps[0]) / np.timedelta64(1, 'h')  # 转换为小时

            # 获取每天的日流量（用于线性插值）
            # 每天的日流量在一天内是恒定的，所以取每天的第一个值
            daily_flows = df_interval[df_interval['interval'] == 0][['date', 'daily_flow']].copy()
            daily_flows['datetime'] = pd.to_datetime(daily_flows['date'])
            daily_flows = daily_flows.sort_values('datetime').reset_index(drop=True)

            # 创建日流量的线性插值函数
            daily_time_numeric = (daily_flows['datetime'].values - timestamps[0]) / np.timedelta64(1, 'h')
            from scipy.interpolate import interp1d
            linear_interp = interp1d(daily_time_numeric, daily_flows['daily_flow'].values,
                                    kind='linear', fill_value='extrapolate')

            # 对每个15分钟时刻进行线性插值
            df_trend['interval_flow'] = linear_interp(time_numeric)

            # 确保流量不为负
            df_trend['interval_flow'] = np.maximum(df_trend['interval_flow'], 0)

            # 更新原始DataFrame
            df_interval.loc[trend_mask, 'interval_flow'] = df_trend['interval_flow'].values

            print(f"全局线性插值完成，共处理 {len(df_trend)} 个数据点")

    # 计算统计信息
    print("=" * 80)
    print("降尺度完成！")
    print("=" * 80)
    print(f"间隔尺度记录数: {len(df_interval)}")
    print(f"日尺度总流量: {df_daily[value_col].sum() * 86400:.2f} m³")
    print(f"间隔尺度总流量: {df_interval['interval_flow'].sum() * (interval_minutes * 60):.2f} m³")
    print(f"流量误差: {abs(df_daily[value_col].sum() * 86400 - df_interval['interval_flow'].sum() * (interval_minutes * 60)):.2f} m³")
    print(f"相对误差: {abs(df_daily[value_col].sum() * 86400 - df_interval['interval_flow'].sum() * (interval_minutes * 60)) / (df_daily[value_col].sum() * 86400) * 100:.4f}%")
    print("=" * 80)

    # 打印曲线类型分布
    curve_type_counts = df_interval.groupby('date')['curve_type'].first().value_counts()
    print("\n日内曲线类型分布：")
    for curve_type, count in curve_type_counts.items():
        print(f"  {curve_type}: {count} 天")
    print("=" * 80)

    # 保存到CSV
    if output_csv_path is not None:
        out_dir = os.path.dirname(output_csv_path)
        if out_dir and not os.path.exists(out_dir):
            os.makedirs(out_dir)

        df_interval.to_csv(output_csv_path, index=False, encoding='utf-8-sig')
        print(f"\n结果已保存到: {output_csv_path}")

    return df_interval


def downscale_daily_to_hourly(
    daily_csv_path,
    output_csv_path=None,
    value_col='sim_adjusted',
    date_col='Date',
    curve_type='auto',
    flood_threshold=None,
    n_hours_per_day=24,
    add_noise=True,
    noise_level=0.05,
    random_seed=None
):
    """
    将日尺度径流量降尺度到小时尺度

    参数:
        daily_csv_path: 日尺度CSV文件路径
        output_csv_path: 输出CSV文件路径（可选）
        value_col: 流量值列名
        date_col: 日期列名
        curve_type: 日内曲线类型
            - 'auto': 自动根据流量大小选择
            - 'normal', 'bimodal', 'triangular', 'skewed', 'uniform'
        flood_threshold: 洪水阈值（用于自动选择曲线类型）
        n_hours_per_day: 每天的小时数，默认24
        add_noise: 是否添加随机噪声
        noise_level: 噪声水平（标准差相对于均值）
        random_seed: 随机种子

    返回:
        df_hourly: 小时尺度数据DataFrame
    """

    if random_seed is not None:
        np.random.seed(random_seed)

    # 读取日尺度数据
    df_daily = pd.read_csv(daily_csv_path)
    df_daily[date_col] = pd.to_datetime(df_daily[date_col])
    df_daily = df_daily.sort_values(date_col).reset_index(drop=True)

    # 准备小时尺度数据列表
    hourly_data = []

    print("=" * 80)
    print("开始降尺度：日尺度 → 小时尺度")
    print("=" * 80)
    print(f"输入文件: {daily_csv_path}")
    print(f"日尺度记录数: {len(df_daily)}")
    print(f"曲线类型: {curve_type}")
    if flood_threshold is not None:
        print(f"洪水阈值: {flood_threshold:.2f} m³/s")
    print(f"添加噪声: {add_noise}")
    if add_noise:
        print(f"噪声水平: {noise_level:.2%}")
    print("=" * 80)

    # 处理每一天
    for idx, row in df_daily.iterrows():
        date = row[date_col]
        daily_flow = row[value_col]

        # 跳过缺失值
        if pd.isna(daily_flow) or daily_flow <= 0:
            continue

        # 选择日内曲线类型
        if curve_type == 'auto':
            actual_curve_type = select_curve_type_based_on_flow(daily_flow, flood_threshold)
        else:
            actual_curve_type = curve_type

        # 生成日内曲线形状（小时尺度）
        curve_shape = generate_daily_curve_shape(actual_curve_type, n_hours_per_day)

        # 计算小时尺度流量
        # 日流量单位：m³/s，表示该日的平均流量
        # 总流量 = 日流量 × 86400 秒
        total_flow = daily_flow * 86400

        # 小时尺度流量 = 总流量 × 曲线形状 / 3600（转换为m³/s）
        hourly_flow = total_flow * curve_shape / 3600

        # 添加随机噪声（使数据更真实）
        if add_noise:
            noise = np.random.normal(0, daily_flow * noise_level, n_hours_per_day)
            hourly_flow = hourly_flow + noise
            # 确保流量不为负
            hourly_flow = np.maximum(hourly_flow, 0)

        # 生成时间戳
        start_time = datetime.combine(date.date(), datetime.min.time())
        timestamps = [start_time + timedelta(hours=i) for i in range(n_hours_per_day)]

        # 添加到结果列表
        for i, (ts, flow) in enumerate(zip(timestamps, hourly_flow)):
            hourly_data.append({
                'datetime': ts,
                'date': date.date(),
                'hour': i,
                'daily_flow': daily_flow,
                'hourly_flow': flow,
                'curve_type': actual_curve_type
            })

        # 进度提示
        if (idx + 1) % 10 == 0:
            print(f"已处理: {idx + 1}/{len(df_daily)} 天")

    # 转换为DataFrame
    df_hourly = pd.DataFrame(hourly_data)

    # 计算统计信息
    print("=" * 80)
    print("降尺度完成！")
    print("=" * 80)
    print(f"小时尺度记录数: {len(df_hourly)}")
    print(f"日尺度总流量: {df_daily[value_col].sum() * 86400:.2f} m³")
    print(f"小时尺度总流量: {df_hourly['hourly_flow'].sum() * 3600:.2f} m³")
    print(f"流量误差: {abs(df_daily[value_col].sum() * 86400 - df_hourly['hourly_flow'].sum() * 3600):.2f} m³")
    print(f"相对误差: {abs(df_daily[value_col].sum() * 86400 - df_hourly['hourly_flow'].sum() * 3600) / (df_daily[value_col].sum() * 86400) * 100:.4f}%")
    print("=" * 80)

    # 打印曲线类型分布
    curve_type_counts = df_hourly.groupby('date')['curve_type'].first().value_counts()
    print("\n日内曲线类型分布：")
    for curve_type, count in curve_type_counts.items():
        print(f"  {curve_type}: {count} 天")
    print("=" * 80)

    # 保存到CSV
    if output_csv_path is not None:
        out_dir = os.path.dirname(output_csv_path)
        if out_dir and not os.path.exists(out_dir):
            os.makedirs(out_dir)

        df_hourly.to_csv(output_csv_path, index=False, encoding='utf-8-sig')
        print(f"\n结果已保存到: {output_csv_path}")

    return df_hourly


def read_usgs_data(usgs_file_path, convert_to_utc=True):
    """
    读取USGS数据文件

    参数:
        usgs_file_path: USGS数据文件路径
        convert_to_utc: 是否将时间转换为UTC（GMT），默认True

    返回:
        df_usgs: USGS数据DataFrame，包含datetime和discharge列
    """
    print("=" * 80)
    print("读取USGS数据")
    print("=" * 80)

    # 读取USGS数据，跳过注释行和列定义行
    # 列定义行通常包含's'或'd'（如5s, 20d）
    df_usgs = pd.read_csv(usgs_file_path, comment='#', sep='\t',
                       dtype=str, low_memory=False)

    # 过滤掉列定义行（包含's'或'd'的行）
    # 这些行不是真实数据，而是列宽度的定义
    if 'datetime' in df_usgs.columns:
        df_usgs = df_usgs[df_usgs['datetime'].str.match(r'^\d{4}-\d{2}-\d{2}', na=False)]

    # 重置索引
    df_usgs = df_usgs.reset_index(drop=True)

    # 查找流量列（通常以TS_ID结尾，如44588_00060）
    discharge_col = None
    for col in df_usgs.columns:
        if '00060' in col and '_cd' not in col:
            discharge_col = col
            break

    if discharge_col is None:
        raise ValueError(f"未找到流量列（包含'00060'的列），可用列: {df_usgs.columns.tolist()}")

    # 解析时间列
    df_usgs['datetime'] = pd.to_datetime(df_usgs['datetime'])

    # 时区转换（如果需要）
    if convert_to_utc and 'tz_cd' in df_usgs.columns:
        # USGS时区代码到UTC偏移量的映射
        tz_offset_map = {
            'EST': -5,  # Eastern Standard Time
            'EDT': -4,  # Eastern Daylight Time
            'CST': -6,  # Central Standard Time
            'CDT': -5,  # Central Daylight Time
            'MST': -7,  # Mountain Standard Time
            'MDT': -6,  # Mountain Daylight Time
            'PST': -8,  # Pacific Standard Time
            'PDT': -7,  # Pacific Daylight Time
            'AKST': -9,  # Alaska Standard Time
            'AKDT': -8,  # Alaska Daylight Time
            'HST': -10,  # Hawaii-Aleutian Standard Time
            'HDT': -9,   # Hawaii-Aleutian Daylight Time
        }

        # 将时区代码转换为UTC偏移量（小时）
        df_usgs['tz_offset'] = df_usgs['tz_cd'].map(tz_offset_map)

        # 记录原始时区信息
        tz_counts = df_usgs['tz_cd'].value_counts()
        print(f"检测到的时区:")
        for tz_code, count in tz_counts.items():
            offset = tz_offset_map.get(tz_code, 'Unknown')
            print(f"  {tz_code}: {count} 条记录 (UTC{offset:+d})")

        # 将时间转换为UTC
        df_usgs['datetime_utc'] = df_usgs['datetime'] - pd.to_timedelta(df_usgs['tz_offset'], unit='h')

        # 使用UTC时间
        df_usgs['datetime'] = df_usgs['datetime_utc']

        print(f"时间已转换为UTC（GMT）")

    # 提取流量值并转换为m³/s（ft³/s → m³/s）
    CONVERSION_FACTOR = 0.0283168466
    df_usgs['discharge'] = pd.to_numeric(df_usgs[discharge_col], errors='coerce') * CONVERSION_FACTOR

    # 只保留需要的列
    df_usgs = df_usgs[['datetime', 'discharge']].copy()

    # 删除缺失值
    df_usgs = df_usgs.dropna(subset=['discharge'])

    print(f"USGS数据读取完成:")
    print(f"  时间范围: {df_usgs['datetime'].min()} 至 {df_usgs['datetime'].max()}")
    print(f"  记录数: {len(df_usgs)}")
    print(f"  流量单位: m³/s（从ft³/s转换）")
    print("=" * 80)

    return df_usgs


def align_sim_obs(df_sim, df_obs, sim_time_col='datetime', obs_time_col='datetime',
                sim_value_col='interval_flow', obs_value_col='discharge'):
    """
    对齐模拟值和观测值的时间序列

    参数:
        df_sim: 模拟值DataFrame
        df_obs: 观测值DataFrame
        sim_time_col: 模拟值时间列名
        obs_time_col: 观测值时间列名
        sim_value_col: 模拟值列名
        obs_value_col: 观测值列名

    返回:
        df_aligned: 对齐后的DataFrame
    """
    print("=" * 80)
    print("对齐模拟值和观测值")
    print("=" * 80)

    # 确保时间列为datetime类型
    df_sim[sim_time_col] = pd.to_datetime(df_sim[sim_time_col])
    df_obs[obs_time_col] = pd.to_datetime(df_obs[obs_time_col])

    # 找到时间交集
    time_min = max(df_sim[sim_time_col].min(), df_obs[obs_time_col].min())
    time_max = min(df_sim[sim_time_col].max(), df_obs[obs_time_col].max())

    print(f"模拟值时间范围: {df_sim[sim_time_col].min()} 至 {df_sim[sim_time_col].max()}")
    print(f"观测值时间范围: {df_obs[obs_time_col].min()} 至 {df_obs[obs_time_col].max()}")
    print(f"交集时间范围: {time_min} 至 {time_max}")

    # 筛选时间交集内的数据
    df_sim_filtered = df_sim[(df_sim[sim_time_col] >= time_min) &
                           (df_sim[sim_time_col] <= time_max)].copy()
    df_obs_filtered = df_obs[(df_obs[obs_time_col] >= time_min) &
                           (df_obs[obs_time_col] <= time_max)].copy()

    # 使用merge对齐时间（内连接）
    df_aligned = pd.merge(
        df_sim_filtered[[sim_time_col, sim_value_col]],
        df_obs_filtered[[obs_time_col, obs_value_col]],
        left_on=sim_time_col,
        right_on=obs_time_col,
        how='inner'
    )

    # 重命名列
    df_aligned = df_aligned.rename(columns={
        sim_time_col: 'datetime',
        sim_value_col: 'simulated',
        obs_value_col: 'observed'
    })

    # 删除重复的时间列
    if f'{obs_time_col}_y' in df_aligned.columns:
        df_aligned = df_aligned.drop(columns=[f'{obs_time_col}_y'])

    print(f"对齐后记录数: {len(df_aligned)}")
    print(f"模拟值记录数（交集内）: {len(df_sim_filtered)}")
    print(f"观测值记录数（交集内）: {len(df_obs_filtered)}")
    print("=" * 80)

    return df_aligned


def plot_comparison(df_aligned, output_plot_path=None, plot_start=None, plot_end=None, language="zh", show_scatter_plot=False):
    """
    绘制模拟值与观测值的对比图

    参数:
        df_aligned: 对齐后的DataFrame
        output_plot_path: 图片保存路径（可选）
        plot_start: 绘图开始时间（可选）
        plot_end: 绘图结束时间（可选）
        language: 语言选择，"zh" 中文, "en" 英文
        show_scatter_plot: 是否显示散点图，默认为True
    """
    print("=" * 80)
    if language == "zh":
        print("绘制对比图")
    else:
        print("Generating comparison plot")
    print("=" * 80)

    # 筛选绘图时间范围
    df_plot = df_aligned.copy()
    if plot_start is not None:
        df_plot = df_plot[df_plot['datetime'] >= pd.to_datetime(plot_start)]
    if plot_end is not None:
        df_plot = df_plot[df_plot['datetime'] <= pd.to_datetime(plot_end)]

    # 计算评价指标
    sim = df_plot['simulated'].values
    obs = df_plot['observed'].values

    mask = ~np.isnan(sim) & ~np.isnan(obs)
    sim = sim[mask]
    obs = obs[mask]

    if len(obs) == 0:
        if language == "zh":
            print("警告：没有有效的数据点用于计算评价指标")
        else:
            print("Warning: No valid data points for calculating evaluation metrics")
        nse = np.nan
        kge = np.nan
        rmse = np.nan
        pbias = np.nan
    else:
        # NSE
        denominator = np.sum((obs - np.mean(obs)) ** 2)
        if denominator == 0:
            nse = np.nan
        else:
            numerator = np.sum((sim - obs) ** 2)
            nse = 1 - numerator / denominator

        # RMSE
        rmse = np.sqrt(np.mean((sim - obs) ** 2))

        # PBIAS
        pbias = np.sum(sim - obs) / np.sum(obs) * 100

        # KGE
        if len(obs) >= 2:
            sim_mean = np.mean(sim)
            obs_mean = np.mean(obs)
            sim_std = np.std(sim, ddof=1)
            obs_std = np.std(obs, ddof=1)

            if obs_mean == 0 or obs_std == 0:
                kge = np.nan
            else:
                r = np.corrcoef(sim, obs)[0, 1]
                alpha = sim_std / obs_std
                beta = sim_mean / obs_mean
                kge = 1 - np.sqrt((r - 1) ** 2 + (alpha - 1) ** 2 + (beta - 1) ** 2)
        else:
            kge = np.nan

    # 设置中文字体支持
    if language == "zh":
        plt.rcParams["font.family"] = ["SimHei", "WenQuanYi Micro Hei", "Heiti TC", "Arial Unicode MS"]
        plt.rcParams["axes.unicode_minus"] = False  # 解决负号显示问题

    # 统一字体大小为20
    plt.rcParams["font.size"] = 20  # 全局字体大小
    plt.rcParams["axes.labelsize"] = 20  # 坐标轴标签字体大小
    plt.rcParams["xtick.labelsize"] = 18  # x轴刻度字体大小
    plt.rcParams["ytick.labelsize"] = 18  # y轴刻度字体大小
    plt.rcParams["legend.fontsize"] = 18  # 图例字体大小

    # 根据是否显示散点图设置子图数量
    if show_scatter_plot:
        # 绘制对比图
        fig, axes = plt.subplots(2, 1, figsize=(16, 12))

        # 子图1：时间序列对比
        ax1 = axes[0]
        ax1.plot(df_plot['datetime'], df_plot['observed'],
                label='Observed (USGS)' if language == "en" else '观测值 (USGS)',
                linewidth=1.5, color='black', alpha=0.8)
        ax1.plot(df_plot['datetime'], df_plot['simulated'],
                label='Simulated' if language == "en" else '模拟值',
                linewidth=1.5, color='red', alpha=0.7)

        # 设置坐标轴标签
        ax1.set_xlabel('Date' if language == "en" else '日期')
        ax1.set_ylabel('Discharge (m^3/s)' if language == "en" else '流量 (立方米/秒)')

        # 移除title
        # ax1.set_title('Observed vs Simulated Discharge')

        # 右上角图例
        ax1.legend(loc='upper right')
        ax1.grid(True, alpha=0.3, linestyle='--')

        # 子图2：散点图
        ax2 = axes[1]
        ax2.scatter(obs, sim, alpha=0.3, s=20, color='blue')

        # 添加1:1线
        min_val = min(np.nanmin(obs), np.nanmin(sim))
        max_val = max(np.nanmax(obs), np.nanmax(sim))
        ax2.plot([min_val, max_val], [min_val, max_val],
                'k--', linewidth=2, label='1:1 Line' if language == "en" else '1:1线')

        ax2.set_xlabel('Observed (m^3/s)' if language == "en" else '观测值 (立方米/秒)')
        ax2.set_ylabel('Simulated (m^3/s)' if language == "en" else '模拟值 (立方米/秒)')

        # 移除title
        # ax2.set_title('Scatter Plot')

        ax2.legend(loc='upper right')
        ax2.grid(True, alpha=0.3, linestyle='--')
        ax2.axis('equal')

        # 在散点图上添加评价指标
        if not np.isnan(nse) and not np.isnan(kge):
            if language == "zh":
                metrics_text = f"NSE = {nse:.4f}\nKGE = {kge:.4f}\nRMSE = {rmse:.2f}\nPBIAS = {pbias:.2f}%"
            else:
                metrics_text = f"NSE = {nse:.4f}\nKGE = {kge:.4f}\nRMSE = {rmse:.2f}\nPBIAS = {pbias:.2f}%"

            # 添加文本到散点图
            ax2.text(0.05, 0.95, metrics_text,
                     transform=ax2.transAxes,
                     verticalalignment='top',
                     bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))
    else:
        # 只绘制时间序列图
        fig, ax1 = plt.subplots(1, 1, figsize=(16, 6))
        ax1.plot(df_plot['datetime'], df_plot['observed'],
                label='Observed (USGS)' if language == "en" else '观测值 (USGS)',
                linewidth=1.5, color='black', alpha=0.8)
        ax1.plot(df_plot['datetime'], df_plot['simulated'],
                label='Simulated' if language == "en" else '模拟值',
                linewidth=1.5, color='red', alpha=0.7)

        # 设置坐标轴标签
        ax1.set_xlabel('Date' if language == "en" else '日期')
        ax1.set_ylabel('Discharge (m^3/s)' if language == "en" else '流量 (立方米/秒)')

        # 右上角图例
        ax1.legend(loc='upper right')
        ax1.grid(True, alpha=0.3, linestyle='--')

        # 在时间序列图上添加评价指标
        if not np.isnan(nse) and not np.isnan(kge):
            if language == "zh":
                metrics_text = f"NSE = {nse:.4f}\nKGE = {kge:.4f}\nRMSE = {rmse:.2f}\nPBIAS = {pbias:.2f}%"
            else:
                metrics_text = f"NSE = {nse:.4f}\nKGE = {kge:.4f}\nRMSE = {rmse:.2f}\nPBIAS = {pbias:.2f}%"

            # 添加文本到时间序列图
            ax1.text(0.02, 0.95, metrics_text,
                     transform=ax1.transAxes,
                     verticalalignment='top',
                     bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

    # 调整布局
    plt.tight_layout()

    # 保存图片
    if output_plot_path is not None:
        out_dir = os.path.dirname(output_plot_path)
        if out_dir and not os.path.exists(out_dir):
            os.makedirs(out_dir)

        plt.savefig(output_plot_path, dpi=300, bbox_inches="tight")
        if language == "zh":
            print(f"对比图已保存到: {output_plot_path}")
        else:
            print(f"Comparison plot saved to: {output_plot_path}")

    plt.show()

    # 打印评价指标
    print("=" * 80)
    if language == "zh":
        print("评价指标")
    else:
        print("Evaluation Metrics")
    print("=" * 80)
    if language == "zh":
        print(f"  NSE  = {nse:.6f}")
        print(f"  KGE  = {kge:.6f}")
        print(f"  RMSE = {rmse:.6f} m^3/s")
        print(f"  PBIAS = {pbias:.6f}%")
    else:
        print(f"  NSE  = {nse:.6f}")
        print(f"  KGE  = {kge:.6f}")
        print(f"  RMSE = {rmse:.6f} m^3/s")
        print(f"  PBIAS = {pbias:.6f}%")
    print("=" * 80)

    return {
        'nse': nse,
        'kge': kge,
        'rmse': rmse,
        'pbias': pbias
    }


def convert_to_lisflood_bdy(df_15min, output_bdy_path, site_id='06601200'):
    """
    将15分钟间隔数据转换为LISFLOOD-FP的.bdy格式

    参数:
        df_15min: 15分钟间隔数据DataFrame，包含datetime和interval_flow列
        output_bdy_path: 输出.bdy文件路径
        site_id: 站点ID，默认'06601200'

    .bdy格式说明:
        第一列: 径流量（m³/s）
        第二列: 从模拟开始时间算起的秒数
    """
    print("=" * 80)
    print("转换为LISFLOOD-FP .bdy格式")
    print("=" * 80)

    # 确保数据按时间排序
    df_15min = df_15min.sort_values('datetime').reset_index(drop=True)

    # 计算从开始时间开始的秒数
    start_time = df_15min['datetime'].iloc[0]
    df_15min['seconds'] = (df_15min['datetime'] - start_time).dt.total_seconds()

    # 写入.bdy文件
    with open(output_bdy_path, 'w') as f:
        # 写入头部信息
        f.write(f"# {site_id} Boundary Conditions\n")
        f.write(f"{site_id}\n")
        f.write(f"{df_15min['seconds'].iloc[-1]:.0f} seconds\n")

        # 写入数据（径流量 秒数）
        for idx, row in df_15min.iterrows():
            f.write(f"{row['interval_flow']:.4f} {row['seconds']:.1f}\n")

    print(f"转换完成！")
    print(f"  站点ID: {site_id}")
    print(f"  记录数: {len(df_15min)}")
    print(f"  时间范围: {start_time} 至 {df_15min['datetime'].iloc[-1]}")
    print(f"  总时长: {df_15min['seconds'].iloc[-1]:.0f} 秒")
    print(f"  输出文件: {output_bdy_path}")
    print("=" * 80)


if __name__ == '__main__':
    # 示例：降尺度到15分钟间隔并与USGS数据对比
    ## 06601200 -- subbasin 156
    # daily_csv_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156_modify_test.csv"
    # output_15min_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156_15min.csv"
    # usgs_file_path = r"F:\BasinFloodData\BasinFloodData1729687482509\upstream\06601200.txt"
    # output_plot_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156_comparison.png"
    # output_bdy_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_156.bdy"

    ## 06607500 -- subbasin 143
    daily_csv_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_143_modify_test.csv"
    output_15min_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_143_15min.csv"
    usgs_file_path = r"F:\BasinFloodData\BasinFloodData1729687482509\upstream\06607500.txt"
    output_plot_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_143_comparison.png"
    output_bdy_path = r"G:\program\seims\SEIMS_HAND\data\MSL_1\MSL_1_longterm_model\CALI_NSGA2_Gen_100_Pop_20\Q_143.bdy"

    # 步骤1：降尺度
    print("\n" + "=" * 80)
    print("步骤1：降尺度")
    print("=" * 80)
    df_15min = downscale_daily_to_interval(
        daily_csv_path=daily_csv_path,
        output_csv_path=output_15min_path,
        value_col='sim_adjusted',
        date_col='Date',
        curve_type='trend',
        interval_minutes=15,
        flood_threshold=2100,
        add_noise=True,
        noise_level=0.01,
        random_seed=42
    )

    # 步骤2：读取USGS数据
    print("\n" + "=" * 80)
    print("步骤2：读取USGS数据")
    print("=" * 80)
    df_usgs = read_usgs_data(usgs_file_path)

    # 步骤3：对齐模拟值和观测值
    print("\n" + "=" * 80)
    print("步骤3：对齐模拟值和观测值")
    print("=" * 80)
    df_aligned = align_sim_obs(df_15min, df_usgs)

    # 步骤4：绘制对比图
    print("\n" + "=" * 80)
    print("步骤4：绘制对比图")
    print("=" * 80)
    metrics = plot_comparison(df_aligned, output_plot_path=output_plot_path)

    # 步骤5：转换为LISFLOOD-FP .bdy格式
    print("\n" + "=" * 80)
    print("步骤5：转换为LISFLOOD-FP .bdy格式")
    print("=" * 80)
    # convert_to_lisflood_bdy(df_15min, output_bdy_path, site_id='06601200')

    print("\n" + "=" * 80)
    print("处理完成！")
    print("=" * 80)
