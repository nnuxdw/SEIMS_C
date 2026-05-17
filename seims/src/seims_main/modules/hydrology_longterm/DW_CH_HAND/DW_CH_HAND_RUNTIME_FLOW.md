# DW_CH_HAND 模块运行思路（按当前代码实现）

本文档描述的是 `DW_CH_HAND` **当前代码版本**在运行时的真实计算流程，不是理想化方案。

对应源码：

- `DW_CH_HAND.h`
- `DW_CH_HAND.cpp`
- `api.cpp`

---

## 1. 模块定位

`DW_CH_HAND` 是一个“子流域级通道-湖库统一路由模块”，核心特征：

1. 路由方程采用局部惯性（diffusive wave / local inertial）形式。
2. 同时支持河道、湖泊、水库三类单元。
3. 使用 HAND 分层结构反演子流域内部淹没状态。
4. 在链路型短河段（link-like reach）上加入了额外稳定化处理。

---

## 2. 主要状态变量（运行时最关键）

### 2.1 水量平衡主状态

- `m_chSto[i]`：当前子步末储水量（m3）
- `m_chStoLastStep[i]`：上一步储水量（m3）
- `m_rteWtrIn[i]`：本子步入库体积（m3）
- `m_rteWtrOut[i]`：本子步出库体积（m3）

### 2.2 控制断面水动力状态

- `m_chWtrDepth[i]`：断面水深（m）
- `m_chWtrWth[i]`：断面水面宽度（m）
- `m_chCrossArea[i]`：断面过水面积（m2）
- `m_sfcElv[i]`：水面高程（m）
- `m_dwnElv[i]`：下游参考水面高程（m）
- `m_rivOut[i]`：有符号交换流量（m3/s）
- `m_rivVel[i]`：流速（m/s）

### 2.3 HAND 淹没状态

- `m_Hands`：每个子流域 HAND 分层结构
- `m_handWtrDep[handId]`：HAND 单元水深
- `m_subbasinWtrDep[i]`：子流域代表淹没深度
- `m_subbasinInundationArea[i]`：淹没面积（km2）

---

## 3. 总执行流程（`Execute()`）

`Execute()` 的真实计算顺序如下：

1. `CheckInputData()`
2. `InitialOutputs()`（仅首次分配和初始化）
3. `PointSourceLoading()`
4. 固定子步数 `subSteps = 48`，`sub_dt = m_dt / 48`
5. 预计算湖库子流域的降水/蒸散“深度速率”
6. 子步循环（48 次）：
   - 先刷新每个 reach 的几何/水位状态（`RefreshReachHydraulicState`）
   - 再按层级顺序调用 `ChannelFlow_DiffusiveWave(reachIndex, sub_dt)`
   - 累加输出量用于日均/日累计
7. 子步结束后汇总输出：
   - `m_qRchOut/m_qsRchOut/m_qiRchOut/m_qgRchOut` 取子步均值
   - `m_rteWtrOut/m_lakepcp` 保留子步累计
   - `m_rrtime` 取子步均值

---

## 4. 初始化逻辑（`InitialOutputs()`）

### 4.1 一次性初始化特点

- 若 `m_qRchOut != nullptr`，函数直接返回，不重复初始化。
- 这意味着数组分配、HAND 结构加载、初值灌入都只在第一次执行时发生。

### 4.2 主要动作

1. 分配所有 1D/2D 状态数组。
2. 调用 `LoadHandLevelsFromArrays()` 重建 HAND 分层结构。
3. 对每个 reach 设初始流量分量和初始储量：
   - 河道：基于梯形断面和 `m_Chs0_perc`
   - 湖库：基于 `m_lakeHandLevelini` 对应 HAND 累积体积
4. 计算初始 `m_chWtrDepth/m_chWtrWth/m_chCrossArea/m_sfcElv`。
5. 初始化调试 cluster（当前 root：`117` 和 `123`）及拓扑文件。

---

## 5. 子步内的“状态刷新”（`RefreshReachHydraulicState(i)`）

此函数负责把 `m_chSto` 转成当前几何状态和水位状态，分三类处理。

### 5.1 湖泊/水库 reach

1. 用 `HandInundation_BinarySearch(i, m_chSto[i])` 反演淹没层级。
2. 取 `m_subbasinWtrDep[i]` 作为当前水深。
3. `m_lakearea[i]` 由 `m_subbasinInundationArea[i]` 转为 m2。
4. 几何简化为：
   - `m_chWtrWth[i] = max(m_chWth[i], 1)`
   - `m_chCrossArea[i] = width * depth`

### 5.2 link-like 短河段

触发条件由 `IsHydraulicLinkReach()` 判断（短长度、单上游、邻接湖库或颈缩特征）。

核心处理：

1. 不使用 HAND 漫滩映射（先 `ClearHandStateForReach`）。
2. 用等效长度 `lenEq` 反推储量对应深度 `depByStorage`。
3. 用上下游端水位平均给出 `depByEnds`。
4. 两者混合：

```text
hydraulicDepth = (1 - alpha) * depByStorage + alpha * depByEnds
alpha = DW_LINK_STAGE_BLEND
```

5. 再按梯形断面更新宽度和面积。

### 5.3 普通河道 reach

1. 由当前储量反解水深（`SolveRiverDepthFromStorage`）。
2. 若超过 bankfull 储量，则用 excess storage 调 HAND 淹没；否则清空 HAND 状态。
3. 用复合断面公式更新 `m_chWtrWth/m_chCrossArea`。

最后统一更新：

- `m_chWtrDepth[i]`
- `m_sfcElv[i] = m_chBedElev[i] + m_chWtrDepth[i]`

---

## 6. 核心通量方程（`ChannelFlow_DiffusiveWave(i, sub_dt)`）

此函数是模块核心。按代码顺序可以理解为 8 步。

### 6.1 组装入流 `qIn`

由以下部分叠加：

1. 本地侧向流：`m_olQ2Rch + m_ifluQ2Rch + m_gndQ2Rch + m_ptSub`
2. 上游来流：`qsUp + qiUp + qgUp`（来自上游 reach 出流分量）
3. 湖库降水体积项（仅 lake/res）

并在此处做上游 NaN 防护。

### 6.2 河道 bank storage 回补

仅非湖库单元执行：

1. `bankOut` 回到河道流量
2. `bankOutGw` 回到地下水库容（`m_gwSto`）

### 6.3 更新储量并刷新本断面状态

1. `m_chSto += qIn * dt`
2. 立即调用一次 `RefreshReachHydraulicState(i)`，更新断面水位与几何。

### 6.4 计算坡降与局部惯性离散通量

1. 下游 reach `jseq` 与是否有下游判定。
2. 识别是否 link-like、是否涉湖（决定坡度上限）：
   - 涉湖：`DW_MAX_ABS_SLOPE_LAKE_LINK = 0.01`
   - 纯河：`DW_MAX_ABS_SLOPE_RIVER_LINK = 0.03`
3. 计算有效距离 `dist`：
   - 湖库取 `max(chLen, sqrt(lakearea))`
   - link 至少 `DW_LINK_MIN_LEN_FOR_SLOPE`
4. link 可用“端点约束水位”修正本端水位再算坡降。
5. 通量离散（代码中的半隐式形式）：

```text
dout = B * (q_prev + g*dt*h_im*S) / (1 + g*dt*n^2*|q_prev|*h_im^(-7/3))
```

并施加最大流速限制（link 更保守）。

### 6.5 回水（负流）处理

当 `total_out_raw < 0`：

1. 计算需求回水体积 `reqBackVol = -Q*dt`
2. 最多从下游可用储量扣除 `actBackVol`
3. 若可供体积不足，反向流量按比例缩减或置零

### 6.6 湖库蒸发与水库调度

1. 湖泊蒸发：`m_evlake * m_pet * dt`
2. 水库蒸发：基于 `m_petSubbsn`、`m_petFactor`、`m_lakearea`
3. 水库调度上限 `ComputeResScheduledOutflow()`：
   - 按蓄满度分段给最大放流
   - 受最小生态流量约束

### 6.7 出流扣减、渗漏蒸发、分量拆分

1. 扣减本步出流体积，避免超抽干。
2. 非湖库计算河床渗漏和河道蒸发。
3. 再次 `RefreshReachHydraulicState(i)`，确保状态一致。
4. 计算 `m_qRchOut` 并按来源比例拆分到 `m_qsRchOut/m_qiRchOut/m_qgRchOut`。
5. 更新 `m_rrtime`。

### 6.8 湖库水量平衡矩阵与调试日志

1. 对湖库更新 `m_T_LKWB[i][0..6]`。
2. 若在 debug cluster 且满足时间窗口，输出详细 CSV 日志。

---

## 7. HAND 反演流程

### 7.1 `HandInundation_BinarySearch(reachId, sto)`

1. 在累积体积 `m_levelAccVol` 上二分定位目标层级。
2. 计算该层剩余体积 `remaining`。
3. 以该层面积换算局部附加深度 `partial_depth`。
4. 回填各层 `m_levelWtrDep`，并计算 `excessWtrVol`。
5. 调 `updateAllHandsWtrDep` 回写 cell 和子流域汇总量。

### 7.2 `updateAllHandsWtrDep(reachId)`

1. 把每层 `m_levelWtrDep` 写到 `m_handWtrDep[handId]`。
2. 按阈值 `FLOOD_DEPTH_THRESH` 判定淹没并累加面积。
3. 更新：
   - `m_subbasinInundationArea`
   - `m_subbasinArea`
   - `m_subbasinWtrDep`（当前取 level-1 水深）

---

## 8. 调试机制（当前代码已内置）

### 8.1 cluster 追踪范围

- root reaches：`117` 与 `123`
- 邻域深度：`4`
- 自动输出拓扑关系 CSV

### 8.2 时间过滤

仅写以下窗口：

- `year_idx in [0, 2]`
- `month == 1`

### 8.3 输出文件

- `DW_CH_HAND_debug_cluster_117_123_jan_first3years.csv`
- `DW_CH_HAND_debug_cluster_117_123_jan_first3years_topology.csv`

---

## 9. 当前实现的关键特征与注意点

1. 子步内是“分 reach 顺序更新”，不是全网同时隐式求解。
2. 每个 reach 在 `ChannelFlow_DiffusiveWave` 中会再次调用 `RefreshReachHydraulicState`，保证储量变化后几何状态同步。
3. link-like reach 有额外稳定化，核心是：
   - 等效长度
   - 端点水位混合
   - 更保守流速上限
4. 湖库与河道在同一框架求解，但湖库额外有：
   - 湖面降水/蒸发
   - 水库调度上限
5. `InitialOutputs()` 只执行一次，后续运行依赖持续状态推进。

---

## 10. 一页版伪代码

```text
Execute():
    CheckInputData()
    InitialOutputs()             # 仅首次
    PointSourceLoading()
    precompute_lake_pcp_pet_rate()

    for step in 1..48:
        for each reach i:
            RefreshReachHydraulicState(i)
            update_prec_pet_for_lake(i)

        for each routing layer:
            for each reach i in layer:
                ChannelFlow_DiffusiveWave(i, sub_dt)

        accumulate_substep_outputs()

    average_or_sum_outputs()
    return
```

```text
ChannelFlow_DiffusiveWave(i):
    qIn = local lateral + upstream + lake precip + bank return
    m_chSto += qIn * dt
    RefreshReachHydraulicState(i)

    compute slope and flux with local inertial form
    apply slope cap / velocity cap / backwater cap
    apply lake-res evaporation and reservoir release rule
    update storage and losses
    RefreshReachHydraulicState(i)
    split routed flow into qs/qi/qg
    write diagnostics and debug logs
```

---

## 11. 文档用途说明

这份文档的目的不是替代算法论文，而是帮助后续重构时快速回答三个问题：

1. 代码现在到底按什么顺序在算？
2. 哪些变量是“总量状态”、哪些是“交换状态”？
3. 在不破坏守恒和接口的前提下，下一步该从哪里改？

