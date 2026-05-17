# DW_CH_HAND 重构梳理稿

## 1. 模块目标

`DW_CH_HAND` 的目标不是单纯做河道路由，也不是单纯做湖泊蓄泄，而是把下面三层过程统一起来：

1. 子流域总水量平衡  
   包括地表径流、壤中流、地下水、点源、降水、蒸发、渗漏、bank storage 等。

2. 子流域内部淹没状态计算  
   依赖 HAND 带结构，根据当前蓄水量反推出：
   - 哪些 HAND 单元被淹
   - 各 HAND 单元水深
   - 当前淹没面积
   - 当前淹没层级

3. 子流域之间的水动力交换  
   通过局部惯性方程，在相邻子流域控制断面之间计算交换流量和回水。

后续重构的核心不是放弃 HAND，而是把：

- 子流域整体淹没状态
- 控制断面交换状态

这两套状态明确分开。

## 2. HAND 在这里的定位

这里的 HAND 不是只给湖泊用的，而是给所有子流域用的统一淹没空间框架。

- 湖泊子流域要用 HAND
- 河道子流域也要用 HAND
- 区别不在于“谁用 HAND，谁不用 HAND”
- 区别在于“从 HAND 结果里提取哪个量进入局部惯性方程”

因此，HAND 主要负责回答：

- 当前子流域内部哪里被淹
- 各 HAND 单元局部水深是多少
- 子流域总淹没面积是多少

而局部惯性方程需要的是：

- 控制断面的局部水位
- 下游控制断面的局部水位
- 两者之间的坡降

所以不能直接把整体代表淹没量拿去当控制断面交换水头。

## 3. 当前最核心的三个问题

### 3.1 `m_chBedElev` 的物理意义不统一

当前 `m_chBedElev` 在河道子流域和湖泊子流域中，构造方式并不一致，导致其物理意义被混用了。

它后续应该统一定义为：

```text
子流域控制断面的槽底高程
```

而不再混用为：

- 整个子流域最低地面高程
- 整个湖盆底高程
- 初始湖深参考面
- 最大湖深代表面

### 3.2 把整体淹没代表量误当成了控制断面水头

当前最容易出问题的地方是：

- `m_subbasinWtrDep` 更接近整体代表淹没深度
- `m_handWtrDep[handId]` 才是具体 HAND 单元的局部水深

但是局部惯性方程真正需要的是控制断面水深和控制断面水位。

所以后续必须避免直接用：

- `m_subbasinWtrDep`
- 整体状态直接构造的平均意义 `m_sfcElv`

去代表控制断面交换水头。

### 3.3 `53/64/62` 这类短 link 子流域容易成为激荡点

这类子流域常常夹在两个大淹没单元之间，表现为：

- 上游来水一到，局部很快抬高
- 但如果控制断面水头定义不对，它又不能把水及时传给真正的交换对象
- 最终形成局部堆积和振荡

但我当前判断仍然是：

- link 问题很重要
- 但它更多是前面两个问题的放大器
- 先改正控制断面水头定义，再处理 link 稳定性，顺序更合理

## 4. 后续变量层次怎么分

### 4.1 第一层：总蓄水状态

这组变量回答“这个子流域总共有多少水”：

- `m_chSto`
- `m_chStoLastStep`
- `m_rteWtrIn`
- `m_rteWtrOut`
- `m_bankSto`
- `m_bankStoLastStep`

### 4.2 第二层：HAND 淹没状态

这组变量回答“这些水在子流域内部淹到了哪里”：

- `m_Hands`
- `m_handWtrDep`
- `m_isHandFlooded`
- `m_subbasinWtrDep`
- `m_subbasinInundationArea`
- `m_subbasinArea`

这里需要特别强调：

- `m_subbasinWtrDep` 可以保留
- 但它更适合表示整体代表淹没深度
- 不建议再直接进入局部惯性方程

### 4.3 第三层：控制断面交换状态

这组变量回答“这个子流域通过控制断面如何和上下游交换”。

后续代码命名风格按你的要求，不再增加 `outlet` 前缀。  
真正参与局部惯性方程的主变量，继续使用当前这种简洁风格：

- `m_chBedElev`
  - 控制断面的槽底高程
- `m_chWtrDepth`
  - 控制断面的局部水深
- `m_chWtrWth`
  - 控制断面的有效水面宽度
- `m_chCrossArea`
  - 控制断面的过流面积
- `m_sfcElv`
  - 控制断面的绝对水位
- `m_dwnElv`
  - 下游控制断面的绝对水位
- `m_chLen`
  - reach 到下游的有效交换长度
- `m_chMan`
  - 控制断面的等效糙率

### 4.4 建议新增的辅助元数据变量

如果后续需要显式记录控制断面位置和来源，建议新增辅助变量，但保持简洁风格，不使用 `outlet` 前缀：

- `m_outletHandId`
  - 控制断面栅格对应的 HAND 单元编号
- `m_ChDepth`
  - 控制断面的局部槽深//可以用来与水深判断若这个值低于水深的话则认为开始漫滩；

## 5. 命名原则

这一版后续重构建议统一采用下面的命名原则：

### 5.1 主水动力变量保持当前风格

真正参与局部惯性方程的主变量，继续沿用当前代码风格：

- `m_chWtrDepth`
- `m_chWtrWth`
- `m_chCrossArea`
- `m_sfcElv`
- `m_dwnElv`
- `m_chBedElev`
- `m_chLen`
- `m_chMan`

也就是说，后续不再推荐把它们改成：

- `m_outletWtrDep`
- `m_outletWtrWth`
- `m_outletCrossArea`
- `m_outletSfcElv`
- `m_outletBedElev`

### 5.2 真正要改的是物理含义，不是变量前缀

后续要做的不是机械换名字，而是把这些变量的物理含义统一收敛为：

- `m_chWtrDepth`：控制断面局部水深
- `m_chWtrWth`：控制断面有效宽度
- `m_chCrossArea`：控制断面过流面积
- `m_sfcElv`：控制断面绝对水位
- `m_chBedElev`：控制断面槽底高程

### 5.3 整体状态和交换状态必须分开

因此后续代码里应该形成下面两套状态：

```text
整体状态
---------
m_subbasinWtrDep
m_subbasinInundationArea

交换状态
---------
m_chWtrDepth
m_chWtrWth
m_chCrossArea
m_sfcElv
m_dwnElv
m_chBedElev
```

## 6. `m_chBedElev` 应该如何计算

我当前认为，`m_chBedElev` 最合理的统一定义是：

```text
子流域控制断面的槽底高程
```

推荐计算方式：

```text
m_chBedElev(i) = Z_ctrl_dem(i) - D_ctrl_channel(i)
```

其中：

- `Z_ctrl_dem(i)` 是控制断面栅格 DEM
- `D_ctrl_channel(i)` 是控制断面的局部槽深

如果后续显式存储辅助元数据，则可以写成：

```text
m_chBedElev(i) = m_ctrlDem(i) - m_ctrlChDepth(i)
```

对于湖泊子流域尤其要注意：

- 不要再直接用 `Lake_Depini`
- 不要再直接用湖泊最大深度
- 应该使用湖口、颈口、连通河槽处的局部控制断面深度

## 7. 河道子流域与湖泊子流域进入局部惯性方程时应使用哪些变量

这里的原则是：

- 河道和湖泊都统一先用 HAND 反演淹没状态
- 然后都只提取控制断面状态进入局部惯性方程

### 7.1 河道子流域

河道子流域在局部惯性方程中建议使用：

- `m_chWtrDepth[i]`
- `m_sfcElv[i]`
- `m_chWtrWth[i]`
- `m_chCrossArea[i]`
- `m_chMan[i]`
- `m_chLen[i]`

推荐构造方式：

- `m_chWtrDepth[i] = m_handWtrDep[m_ctrlHandId[i]]`
- `m_sfcElv[i] = m_ctrlDem[i] + m_chWtrDepth[i]`
- `m_chBedElev[i] = m_ctrlDem[i] - m_ctrlChDepth[i]`
- `m_chWtrWth[i]` 由控制断面几何计算
- `m_chCrossArea[i]` 由控制断面几何计算

注意：

- 河道子流域也要先做 HAND 反演
- 不能因为是河道就完全退回到“整个子流域平均断面几何”

### 7.2 湖泊子流域

湖泊子流域进入局部惯性方程时，同样建议使用：

- `m_chWtrDepth[i]`
- `m_sfcElv[i]`
- `m_chWtrWth[i]`
- `m_chCrossArea[i]`
- `m_chMan[i]`
- `m_chLen[i]`

区别只在于：

- 湖泊内部整体淹没状态仍由 HAND 决定
- 湖泊对外交换时，不用整体池水位直接交换
- 只使用湖口控制断面的局部状态参与交换

也就是说：

- 湖体整体淹没状态决定湖里面“蓄了多少、淹了多大”
- 对外怎么交换，由控制断面决定

## 8. 局部惯性方程真正需要的变量

无论是河道子流域还是湖泊子流域，进入局部惯性方程后建议统一成下面这组变量。

### 8.1 几何量

- `B_i = m_chWtrWth[i]`
  - 控制断面顶宽
- `A_i = m_chCrossArea[i]`
  - 控制断面过流面积
- `h_i = m_chWtrDepth[i]`
  - 控制断面局部水深
- `z_i = m_chBedElev[i]`
  - 控制断面槽底高程

### 8.2 水位量

- `eta_i = m_sfcElv[i]`
  - 当前子流域控制断面绝对水位
- `eta_j = m_sfcElv[j]`
  - 下游子流域控制断面绝对水位
- `m_dwnElv[i] = m_sfcElv[j]`
  - 可作为诊断或缓存

### 8.3 动力学量

- `Q_i_prev = m_rivOut_pre[i]`
  - 前一子步交换流量
- `h_i_prev = m_rivDph_pre[i]`
  - 前一子步控制断面水深
- `n_i = m_chMan[i]`
  - 控制断面糙率
- `L_ij = m_chLen[i]`
  - 与下游之间的有效交换长度

### 8.4 坡降

```text
S_ij = (eta_i - eta_j) / L_ij
```

### 8.5 不建议直接进入局部惯性方程的量

下面这些量不建议直接当作交换水头使用：

- `m_subbasinWtrDep`
- `m_subbasinInundationArea`
- 由整体平均状态直接构造的代表水位

## 9. 模块整体运行思路

后续建议把模块执行逻辑理解成三层：

1. 总量层  
   更新总蓄水，保证水量平衡。

2. HAND 层  
   由总蓄水反演整个子流域内部淹没状态。

3. 交换层  
   从 HAND 结果中提取控制断面状态，进入局部惯性方程。

这三层里，真正进入相邻子流域交换的是第 3 层，不是第 2 层的整体代表量。

## 10. 建议的子步执行顺序

```text
for each substep:

    1. 汇总本地侧向来水
       - 地表径流
       - 壤中流
       - 地下水
       - 点源
       - 湖面降水

    2. 汇总上游来流

    3. 更新临时总蓄水
       - 更新 m_chSto 的临时状态

    4. 对所有 reach 统一反演 HAND 状态
       - 更新 m_handWtrDep
       - 更新 m_subbasinWtrDep
       - 更新 m_subbasinInundationArea
       - 更新当前 HAND 层级

    5. 对所有 reach 统一提取控制断面状态
       - 更新 m_chWtrDepth
       - 更新 m_sfcElv
       - 更新 m_chWtrWth
       - 更新 m_chCrossArea
       - 更新 m_dwnElv

    6. 对所有 reach 统一计算局部惯性方程通量
       - 计算 m_rivOut
       - 计算 m_rivVel
       - 处理回水与限速

    7. 对所有 reach 统一更新水量平衡
       - 更新出流
       - 更新蒸发
       - 更新渗漏
       - 更新 bank storage

    8. 刷新输出变量
```

## 11. 简化的局部惯性方程伪代码

这里只保留重构时真正需要关注的变量和流程。

```text
for each reach i:
    j = downstream(i)

    eta_i = m_sfcElv[i]
    eta_j = m_sfcElv[j]
    h_i   = m_chWtrDepth[i]
    A_i   = m_chCrossArea[i]
    B_i   = m_chWtrWth[i]
    n_i   = m_chMan[i]
    L_ij  = m_chLen[i]

    S_ij = (eta_i - eta_j) / L_ij

    h_im   = semi_implicit_depth(h_i, h_i_prev)
    q_prev = Q_i_prev / max(B_i, eps)

    q_new = local_inertial_update(q_prev, h_im, S_ij, n_i, dt)
    Q_new = B_i * q_new

    apply velocity limiter
    apply backwater limiter
    apply storage limiter
```

## 12. 整体模块简化伪代码

```text
Execute():
    CheckInputData()
    InitialOutputs()
    PointSourceLoading()

    for each substep:
        for each reach:
            gather local lateral inflow
            gather upstream inflow
            update temporary storage

        for each reach:
            update HAND state from storage
            update subbasin flood state
            update control-section state

        for each reach:
            compute local inertial flux using control-section state

        for each reach:
            apply outflow to storage
            apply seepage and evaporation
            apply bank storage exchange
            refresh diagnostic outputs

    aggregate substep outputs
    return
```

## 13. 目前最容易混淆的三组变量

### 13.1 整体淹没状态

- `m_subbasinWtrDep`
- `m_subbasinInundationArea`
- `m_handWtrDep`

用途：

- 描述子流域内部当前怎样被淹
- 不直接用于局部惯性方程

### 13.2 控制断面交换状态

- `m_chWtrDepth`
- `m_sfcElv`
- `m_chWtrWth`
- `m_chCrossArea`
- `m_chBedElev`
- `m_dwnElv`

用途：

- 只用于子流域之间的水动力交换

### 13.3 总蓄水状态

- `m_chSto`
- `m_chStoLastStep`
- `m_rteWtrIn`
- `m_rteWtrOut`

用途：

- 控制总水量平衡

## 14. 对 `53/64/62` 这类 link 的处理顺序建议

我当前的判断是：

- `53/64/62` 这些小 link 的确是数值问题高发点
- 但它们不是第一主因
- 第一主因还是控制断面水头定义错误

因此建议的重构顺序是：

1. 先统一 `m_chBedElev` 的物理意义
2. 再把 HAND 的整体状态和控制断面状态分开
3. 再把局部惯性方程统一切换到 `m_sfcElv / m_chWtrDepth / m_chCrossArea` 这组控制断面变量
4. 最后再针对 link 做额外稳定化

## 15. 后续优先准备的数据

如果后面要重新提取数据，建议优先准备：

- 每个子流域的控制断面栅格编号
- 每个控制断面栅格的 DEM
- 每个控制断面栅格对应的 HAND 单元编号
- 每个控制断面的局部槽深
- 每个控制断面的局部宽度
- 每个控制断面的等效糙率
- 每个 reach 到下游的有效交换长度

## 16. 当前阶段最重要的一句话结论

当前模型的主要问题，不是 HAND 本身，而是：

```text
把 HAND 的整体淹没代表量，误当成了控制断面的交换水头
```

所以后续重构必须围绕下面这条主线：

```text
统一 HAND
分离整体状态和控制断面状态
统一 m_chBedElev 为控制断面槽底高程
局部惯性方程只使用 m_sfcElv、m_chWtrDepth、m_chWtrWth、m_chCrossArea 这组控制断面变量
```
