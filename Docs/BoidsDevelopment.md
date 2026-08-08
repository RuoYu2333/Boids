# Boids 二维鱼群系统开发记录

## 1. 项目目标

第一阶段在 Unreal Engine 5.7.4 中实现二维 Boids，以不同颜色、不同大小的球体代替鱼模型。当前重点是得到可复现、可调试的群体运动基础，不包含鱼模型、动画、进食、威胁、三维水体和 MassEntity。

项目采用以下约束：

- 模拟平面为 XY，渲染位置的 Z 固定。
- 使用单个 `ABoidsManager` 集中更新所有代理，不为每条鱼创建独立 Tick。
- 运行时状态保存在连续数组中，球体仅作为状态的可视化。
- 初始化使用固定随机种子，保证相同参数得到相同初态。
- 当前规模为 100 个代理，邻居查询先使用直接遍历，后续再引入空间哈希。

## 2. 工程与场景

工程位置：

```text
G:\boids\Boids\Boids.uproject
```

## 13. 三维水箱、行为与渲染

### 13.1 底面锚定的水箱坐标

`ABoidsManager3D` 的 Actor 局部原点定义为水箱底面中心，水箱中心不再保存为一份容易失配的独立参数，而是始终计算为：

```text
TankCenterLocal = (0, 0, abs(BoundaryHalfExtent.Z))
```

因此底面恒为局部 `Z=0`，顶部为 `Z=2×HalfExtent.Z`。改变水箱高度时只会向上伸展；移动或旋转 Manager 时，边框、代理、食物点和全部边界计算随 Actor Transform 一起变化。关卡中应把 Manager Actor 放在希望的水箱底面中心，而不是手工补偿框体中心。

### 13.2 三维邻居查询和 Boids 三力

每个固定步先把代理按三维网格坐标放入 `TMap<FIntVector, TArray<int32>>`。代理只遍历邻近网格，不再对全部代理执行两两检测。查询得到的同色邻居参与：

- Separation：近距离产生远离邻居的转向，防止重叠；
- Alignment：趋向同组邻居的平均单位方向；
- Cohesion：趋向同组邻居的位置中心。

三力、巡游或喂食转向、软边界力合并后统一限制到 `MaxAcceleration`。积分后速度限制在状态允许的速度范围，最后用六面投影和速度分量反转作安全兜底。空间哈希只改变查找方式，不改变固定 Seed 下的模拟规则。

### 13.3 巡游状态

巡游不是每帧抽取随机数，而是使用 `StableId + SimulationTime` 生成确定性的三轴正弦方向扰动。这样鱼会缓慢改变方向，不会长期沿直线运动，同时 Reset 后仍可复现。

主要参数为 `CruiseWeight` 和 `CruiseFrequency`：前者控制游荡转向强度，后者控制方向变化速度。该扰动只是一项转向力，不会替代三力和边界约束。

### 13.4 喂食状态

蓝图可调用：

```cpp
StartFeedingAtWorldLocation(WorldLocation)
StopFeeding()
```

世界坐标先通过 Manager Transform 转为局部坐标，再夹入当前水箱六面范围，因此移动、旋转或改高水箱后仍正确。喂食时代理采用 Arrival 行为：远处以较高期望速度接近，进入 `FeedingSlowRadius` 后逐渐降速，避免在目标处持续高速穿越。Separation 保持启用，防止所有鱼完全叠成一个点；绿色球体显示当前食物位置。

### 13.5 视锥和距离剔除

蓝色鱼、橙色鱼和方向指示器继续使用 ISM。UE 的 GPU Scene 会对不可见实例执行视锥剔除，`InstanceStartCullDistance` 和 `InstanceEndCullDistance` 进一步限定相机距离。这里不在 CPU 重复计算相机平面，以免模拟线程承担一套与渲染器重复的剔除工作。剔除只停止绘制，不停止 Boids 模拟，因此离开画面后重新观察不会出现状态跳回。

### 13.6 验收步骤

1. 把 `BoidsManager3D` Actor 的位置设为水箱底面中心，运行后确认框体底边与该局部平面重合。
2. 分别改变 `BoundaryHalfExtent.Z` 为较小和较大值；底边应不动，顶边、生成范围、软边界和硬反弹同步改变。
3. 移动或旋转 Manager；鱼、框体和食物标记应保持在同一局部水箱内。
4. 巡游状态连续观察数分钟，鱼群应持续转向、结群且不长期贴边或静止。
5. 在 Level Blueprint 获取 Manager 引用，调用 `StartFeedingAtWorldLocation` 并传入一个世界坐标；鱼群应向绿色目标聚集并在附近减速。
6. 调用 `StopFeeding`；系统应恢复巡游。
7. 把相机移出 `InstanceEndCullDistance`，鱼与方向指示器应停止绘制；返回范围后应显示其持续演化后的状态。

本阶段已使用源码版 UE 完整构建 `BoidsEditor Win64 Development`，构建结果为成功。

默认地图为 `/Game/L_Boids2D`。场景使用正交相机从 Z 轴方向观察 XY 平面，默认可视区域与模拟边界对应：

```text
X = [-2000, 2000]
Y = [-1200, 1200]
```

场景中只放置一个 `BoidsManager`。项目曾出现两份同名地图，实际默认地图是 `Content/L_Boids2D.umap`，后续编辑时应先确认当前包路径，避免修改错误地图。

## 3. 集中式数据与渲染

### 3.1 代理状态

每条代理鱼使用 `FBoidAgent2D` 保存最小运行时状态：

```cpp
FVector2D Position;
FVector2D Velocity;
FVector2D Acceleration;
float Scale;
int32 GroupId;
int32 StableId;
int32 InstanceIndex;
```

- `Position`、`Velocity`、`Acceleration` 只包含二维分量。
- `GroupId` 用于限制 Alignment 和 Cohesion；Separation 对所有鱼生效，避免异色鱼互相穿插聚集。
- `StableId` 用于确定性边界处理和完全重叠处理。
- `InstanceIndex` 将模拟数据映射到对应 ISM 实例。

所有代理集中保存在：

```cpp
TArray<FBoidAgent2D> Agents;
```

数组是模拟状态的唯一来源，ISM Transform 只是每帧提交的显示结果。

### 3.2 ISM 批量显示

蓝色和橙色分别使用一个 `UInstancedStaticMeshComponent`：

```text
BlueInstances   → 蓝色组
OrangeInstances → 橙色组
```

两组共用引擎 Sphere 网格。每个实例只保存自己的位置和缩放，避免创建 100 个 Actor 和 100 个网格组件。

材质构造曾遇到序列化问题：动态材质实例不能在 Actor 构造函数中由类默认对象持有，否则关卡保存时会产生非法私有对象引用。当前做法是：

1. 构造函数只绑定基础材质。
2. `BeginPlay()` 为实际组件创建动态材质实例。
3. 分别设置蓝色与橙色参数。

### 3.3 方向与边界调试表现

二维和三维 Manager 都使用独立的 Cone ISM 显示速度方向。每条代理对应一个黄色锥体实例，Cone 的本地 Z 轴旋转到速度单位向量，并放置在球体前方。指示器只消费当前速度，不参与力计算、碰撞或邻居查询。

```text
bShowDirectionIndicators
DirectionIndicatorLength
DirectionIndicatorWidth
```

三维 Manager 额外使用一个 Cube ISM，以 12 个细长立方体实例构成长方体边框。边框尺寸直接读取 `BoundaryHalfExtent`，在生成和 Reset 时重建：

```text
bShowBoundaryFrame
BoundaryFrameThickness
```

方向指示器和边框都关闭碰撞与阴影。高规模性能测试时应关闭方向指示器，只保留边框或全部关闭，避免调试表现影响 ISM 更新基线。

## 4. 确定性初始化

初始化使用局部随机流：

```cpp
FRandomStream RandomStream(RandomSeed);
```

它依次生成代理的位置、大小、初始方向和初始速度。只要种子、参数和随机调用顺序不变，每次 Play 或 Reset 都会得到相同初态。

`SpawnAgents()` 在生成前执行：

```text
清空蓝色实例
清空橙色实例
清空代理数组
预分配数组和实例内存
重新生成全部代理
```

因此重复 Reset 不会累积旧实例。

## 5. 每帧模拟流程

当前每帧按以下顺序执行：

```text
读取帧初代理状态
→ 在一次邻居遍历中计算 Separation、Alignment 与 Cohesion
→ 计算软边界转向力
→ 合并并限制加速度
→ 统一更新速度
→ 限制最小/最大速度
→ 统一更新位置
→ 执行边界投影与反弹
→ 更新 ISM Transform
→ 通知渲染线程刷新
```

运动使用半隐式欧拉积分：

```cpp
Velocity += Acceleration * DeltaTime;
Velocity = Velocity.GetClampedToMaxSize(MaxSpeed);
Position += Velocity * DeltaTime;
```

速度单位为 `uu/s`，乘以 `DeltaTime` 后，运动速度基本不依赖帧率。

## 6. Separation 分离

### 6.1 邻居条件

代理只感知满足以下条件的对象：

- 不是自身；
- 不限制 `GroupId`，不同颜色也必须互相避让；
- 距离小于 `SeparationRadius`。

距离初筛使用平方距离，避免对范围外代理执行开平方：

```cpp
DistanceSquared < SeparationRadius * SeparationRadius
```

### 6.2 分离贡献

远离方向为：

```cpp
AwayDirection = Normalize(Self.Position - Neighbor.Position);
```

距离越近，贡献越强：

```text
Proximity = 1 - Distance / SeparationRadius
```

所有近邻贡献取平均后乘 `SeparationWeight` 和 `MaxAcceleration`，最终再限制最大加速度。

计算采用两阶段更新：第一遍只读取帧初状态并写入临时加速度数组，第二遍才统一积分。这样可以避免“先更新的代理影响后更新的代理”的顺序偏差。

完全重叠时无法归一化距离向量。当前根据一对 `StableId` 生成确定方向，并让同一对代理获得相反方向，从而避免除零、NaN 和不可复现随机结果。

## 7. Alignment 对齐

Alignment 已与 Separation 共用同一次邻居遍历。同组且位于 `NeighborRadius` 内的邻居参与方向统计，不同颜色组之间互不对齐。

旧设计直接平均完整速度：

```text
AverageVelocity = 邻居速度之和 / 邻居数量
```

当邻居方向相反时，平均速度会接近零，Alignment 会持续把代理减速。当前改为平均单位方向：

```cpp
NeighborDirectionSum += Neighbor.Velocity.GetSafeNormal();
AverageDirection = Normalize(NeighborDirectionSum / NeighborCount);
```

然后保持代理当前速度大小，只改变期望方向：

```text
DesiredVelocity = AverageDirection × 当前速度
AlignmentSteering = DesiredVelocity - Self.Velocity
```

当前速度会被限制到安全的 `[MinSpeed, MaxSpeed]` 区间。因此 Alignment 不再因为速度抵消而让整个鱼群逐渐静止。若速度极少数情况下完全变为零，则根据 `StableId` 恢复一个确定性方向。

## 8. 软边界转向与硬反弹

仅使用硬反弹时，刚反弹的代理可能立即被邻居 Alignment 再次拉向墙外，形成“反弹—重新朝外—再次反弹”的边界锁死。当前在距离边界 `BoundaryPadding` 范围内逐渐加入向内加速度：

```text
距离边界较远 → 边界力为 0
进入 Padding  → 越接近墙，向内力越强
真正越界      → 投影并反弹兜底
```

软边界力与 Separation、Alignment、Cohesion 一起合并，然后统一限制到 `MaxAcceleration`。

边界判断计入球体半径：

```cpp
Radius = 50.0f * Scale;
MaxX = BoundaryHalfWidth - Radius;
MaxY = BoundaryHalfHeight - Radius;
```

越界后先将球心投影回合法位置，再只反转朝墙外的速度分量。例如碰到右墙：

```cpp
Position.X = MaxX;
if (Velocity.X > 0.0f)
{
    Velocity.X *= -1.0f;
}
```

这种处理能保证整个球留在区域内，并避免代理已经朝内部移动时被再次错误反转。

## 9. Cohesion 聚集

Cohesion 已复用 `NeighborRadius` 和现有同组邻居查询，不增加额外的 O(N²) 循环。遍历邻居时同时累计位置：

```cpp
NeighborPositionSum += Neighbor.Position;
NeighborCenter = NeighborPositionSum / NeighborCount;
```

代理到邻居中心的方向为：

```cpp
ToCenter = NeighborCenter - Self.Position;
```

系统根据该方向构造保持当前速度大小的期望速度，再计算转向量：

```text
DesiredVelocity = Normalize(ToCenter) × 当前速度
CohesionSteering = DesiredVelocity - Self.Velocity
```

聚集强度随代理到中心的距离增加：

```text
CenteringStrength = Clamp(DistanceToCenter / NeighborRadius, 0, 1)
```

接近中心时吸引力逐渐减弱，避免代理越过中心后反复大幅转向。最终三力与边界力的组合为：

```text
CombinedAcceleration =
    SeparationSteering × SeparationWeight
  + AlignmentSteering  × AlignmentWeight
  + CohesionSteering   × CohesionWeight
  + BoundarySteering   × BoundaryWeight
```

组合结果统一限制到 `MaxAcceleration`。其中 Separation 防止局部重叠，Alignment 统一同组方向，Cohesion 维持群体空间联系。

## 10. 三力与边界验收方法

建议按以下顺序验证：

1. 临时将 `SeparationWeight` 设为 0，只观察 Alignment。
2. 同组邻居的速度方向应逐渐接近，而不是瞬间同步。
3. 不同组之间不应互相对齐。
4. 没有邻居的代理应保持原方向。
5. 恢复 Separation 后，代理应既能避免重叠，又能逐渐形成共同方向。
6. 单独启用 Cohesion 时，同组代理应逐渐向邻居中心靠近，但不应瞬移。
7. 三力同时启用后，同组代理应形成松散群体，不长期重叠，也不无限散开。
8. 两个颜色组应分别形成群体，不参与彼此的三力计算。
9. 群体接近边界时应逐渐转向内部，真正碰撞时才发生硬反弹。
10. 连续运行时不应出现整个群体速度衰减到零或长期贴在边界的情况。
11. 30、60、120 FPS 下方向收敛速度不应出现明显差异。

## 11. 当前参数基线

```text
AgentCount          = 100
RandomSeed          = 12345
BoundaryHalfWidth   = 2000
BoundaryHalfHeight  = 1200
MinAgentScale       = 0.4
MaxAgentScale       = 0.9
MinInitialSpeed     = 80
MaxInitialSpeed     = 180
SeparationRadius    = 120
SeparationWeight    = 1.5
NeighborRadius      = 300
AlignmentWeight     = 1.0
CohesionWeight      = 0.8
MaxAcceleration     = 300
MinSpeed            = 80
MaxSpeed            = 220
BoundaryPadding     = 250
BoundaryWeight      = 1.5
```

这些值是功能验证基线，不是最终调优结果。加入 Alignment 和 Cohesion 后，应重新联合调整半径、权重、最大加速度和速度范围。

## 12. 后续路线

```text
Alignment 对齐与边界脱离（已完成）
→ Cohesion 聚集（已完成）
→ 三力联合调参与单力验证
→ 确定性和帧率独立性验证
→ 运行稳定性与性能基线
→ 空间哈希优化
→ 三维鱼群扩展
```

## 14. 自由观察相机与顶部投食

三维 Manager 默认在 BeginPlay 生成并占有 `ABoidsFreeCameraPawn`。初始位置根据当前水箱尺寸计算并朝向水箱中心，因此改变水箱尺寸或移动 Manager 后不需要重新写死相机坐标。

运行时操作：

```text
W / S       前进 / 后退
A / D       左移 / 右移
Q / E       下降 / 上升
左 Shift    加速移动
按住右键    鼠标自由观察
F           从水箱顶面投下一颗食物
```

按 F 时使用 `RandomSeed + FoodDropSequence` 创建独立随机流，在当前水箱顶面合法范围内随机选择 X/Y；食物 Z 固定为顶面减去食物半径：

```text
FoodStartZ = TankCenter.Z + HalfExtent.Z - FoodRadius
```

随机范围会从四周边缘额外内缩 `FoodSpawnEdgePadding`，避免食物贴着竖直边框出现。食物随后按 `FoodFallSpeed` 从顶面向底部下落。每条鱼只有同时满足以下条件才会产生寻食转向：食物位于 `FoodSightRadius` 内，并且鱼的速度朝向与鱼到食物方向的夹角不超过 `FoodSightHalfAngleDegrees`。这相当于每条鱼拥有独立的三维视锥；没有看到食物的鱼继续巡游。任意鱼的球形体积接触食物球体后，食物实例立即销毁，全群恢复巡游。

鱼鱼接触使用实际显示球半径 `50 × Scale`。固定步积分后通过空间哈希查询附近球体，执行两轮位置投影消除穿叠，并修正相向速度；最后再次夹入水箱六面范围。Separation 负责提前避让，接触解算负责保证即使避让失败也不能重合。

相机输入使用 Pawn 的正式 `SetupPlayerInputComponent` 键绑定，而不是在 Tick 中轮询 PlayerController。项目没有 Enhanced Input Action/Mapping Context，因此 `DefaultInput.ini` 统一使用传统 `PlayerInput + InputComponent`，并配置 `BoidsMoveForward/Right/Vertical` 三条轴映射。只有成功占有自由 Pawn 后，按键事件才会进入该 Pawn。

三维关卡中已经调好的 `CameraActor` 是实际 View Target：自由 Pawn 以该 CameraActor 的位置和旋转生成，再将 CameraActor 挂到 Pawn 上。WASDQE 移动 Pawn 时相机同步移动，同时保留 CameraActor 自己的 FOV、后处理和镜头参数；如果关卡不存在 CameraActor，才回退到 Pawn 内置 CameraComponent。

## 15. 原定路线完成度审计

已完成：

- 二维回归基线保持独立；
- 独立三维 Manager 和三维连续代理数据；
- 固定 Seed、固定步长和最大子步；
- 三维确定性初始化与球面均匀初始方向；
- 六面软边界、硬投影和反弹；
- 三维 Separation、Alignment、Cohesion；
- 三维空间哈希；
- 巡游和顶部落食行为；
- 两组 ISM、方向指示器、水箱框体、GPU 视锥及距离剔除；
- 可自由移动的运行时观察相机；
- `L_Boids3D` 三维验证地图。

仍需完成的核心验证：

1. 建立自动化测试，比较相同 Seed、相同步数的代理状态，并测试 X/Y/Z 六面反弹。
2. 分别以不同渲染帧率运行相同模拟时长，量化固定步长结果是否一致。
3. 关闭方向调试实例，将代理数从 100 提升到 2000，记录 Game Thread、Render Thread 和 GPU 基线。
4. 根据 2000 代理结果联合调整网格尺寸、邻居半径和三力参数。

后续表现或扩展，不属于当前核心 Boids 未完成项：

- 用鱼模型和游动动画替代球体；
- 水体材质、透明水箱和水下光照；
- 威胁、逃逸、障碍物避让及多种食物；
- 规模继续扩大后再评估 MassEntity 或计算着色器。

本轮新增源码已经使用源码版 UE 完整构建 `BoidsEditor Win64 Development`，结果为成功。

## 16. 五种鱼与种群级分流

三维验证默认使用 50 条鱼。种类采用固定的非均匀配额，保证固定 Seed 下数量也可复现：

```text
蓝色 17（34%）  体型倍率 1.12
橙色 13（26%）  体型倍率 0.90
绿色  9（18%）  体型倍率 1.28
紫色  7（14%）  体型倍率 0.72
白色  4（ 8%）  体型倍率 1.00
```

每条鱼仍在所属种类倍率基础上随机取大小，因此同种内部也不是完全等大。五种鱼分别使用独立 ISM，但继续共享一个 Manager 固定步模拟。

力的分层规则：

- 所有鱼之间执行个体 Separation 和球形接触解算；
- 蓝色参与的个体对使用 `BluePersonalSpaceMultiplier` 扩大 Separation 距离；
- Alignment 与 Cohesion 只读取同种邻居，使每种鱼形成自己的集群；
- 每步计算五个种群质心。异种质心距离小于 `SpeciesAvoidanceRadius` 时，同一物种所有成员共享一个远离其他质心的转向力，强度由 `SpeciesAvoidanceWeight` 控制。

种群级力解决的是“两群并排向同一方向运动”的问题；个体 Separation 解决局部穿插；接触投影是最后的不可重合约束。三层不能互相替代。

## 17. 体型相关运动与寻食优先级

鱼的显示缩放 `Scale` 现在同时代表体型。以配置的最小和最大缩放中点作为参考体型，运动倍率为反幂函数：

```text
SpeedMultiplier        = (ReferenceScale / Scale) ^ SizeSpeedExponent
AccelerationMultiplier = (ReferenceScale / Scale) ^ SizeAccelerationExponent
```

结果限制在 `MinSizeMovementMultiplier` 与 `MaxSizeMovementMultiplier` 之间。较小鱼拥有更高的初始速度、最低/最高速度、寻食速度和最大加速度；较大鱼速度较低且转向更迟钝。指数控制体型差异的敏感程度，倍率上下限防止极端缩放破坏固定步稳定性。

鱼与食物之间没有 Separation 或排斥力。此前食物靠近顶面时，软边界向内力和异种质心排斥可能压过食物转向，视觉上类似被食物推开。现在一条鱼通过视锥确认食物后，这两项竞争力降为原来的 20%，食物 Steering 成为主要行为；鱼鱼 Separation 和六面硬边界继续生效，接触食物后仍立即销毁。

## 18. 多食物目标与固定对象池

食物由 Manager 内的单一全局池管理，默认预分配 20 个 `FBoidsFood3D` 槽位和 20 个一一对应的 ISM 实例。投食时激活第一个空闲槽；达到 `MaxSimultaneousFood` 后拒绝继续生成。被吃掉时只将槽位置为未激活，并把对应 ISM 实例缩放为零，下次投食直接复用，不执行 Actor Spawn/Destroy，也不因数组删除改变索引。

每个固定步中，每条鱼独立遍历激活食物：

1. 排除感知距离外的食物；
2. 排除速度朝向视锥外的食物；
3. 在剩余食物中选择距离最近者；
4. 距离近似相同时选择 `StableId` 较小者，保证确定性。

因此同时看到多个食物时不会把转向力简单相加，也不会在相邻目标之间剧烈摇摆。不同鱼可以选择不同的最近目标。任意鱼接触某个食物后只停用该槽位；池中没有激活食物时才恢复全局 Cruising 状态。Reset 和 StopFeeding 会显式停用全部槽位。

鱼本身已经使用对象池式连续数据：一个 `TArray<FBoidAgent3D>` 保存全部状态，五个 ISM 保存显示实例，没有逐鱼 Actor 和逐鱼 Tick。当前鱼不会在运行中出生或死亡，因此固定 `AgentCount` 下直接连续遍历比增加 `bActive` 分支更简单高效；如果后续加入捕食、死亡或动态补鱼，再扩展为固定容量激活槽池。

## 19. 种群级大范围巡游

仅靠每条鱼独立的正弦巡游扰动时，同种成员的随机转向会在 Alignment 中互相抵消，Cohesion 容易让集群形成局部稳定环流。为推动整个种群穿越水箱，五个种群现在各自拥有一个共享巡游航点。

巡游不再使用水箱两侧的直线对穿航点，因为多条直线路径会同时经过中心。当前为五个种群规划不同相位、不同半径且交替正反方向的闭合椭圆航线。每步在航线上计算前方动态引导点，同种鱼共同趋向本组引导点，因此：

- Cohesion 保持集群结构；
- Alignment 统一集群朝向；
- 个体 Cruise 保留局部自然摆动；
- Group Cruise 推动种群质心大范围迁移；
- Species Avoidance 让不同种群在相遇时分流。

初始化时每个物种也生成在自己航线附近，避免先随机铺满水箱再向中心收拢。`GroupCruisePathPeriod` 控制一圈所需时间，`GroupCruiseLaneSpacing` 控制五条航线的半径间隔，Z 方向幅度小于 X/Y。默认参数为 `GroupCruiseWeight=2.4`、`GroupCruisePathPeriod=55`、`GroupCruiseRangeFraction=0.9`、`GroupCruiseLaneSpacing=0.055`。鱼看见食物时暂停航线转向，优先追逐自己选中的食物。

## 20. 运行计时 HUD

Manager 在获得 PlayerController 后将 HUD 设置为 `ABoidsHUD`。HUD 记录本次 Play 开始时间，并在每帧 Canvas 绘制阶段把 `Time MM:SS` 放在视口右上角。计时只用于观察运行稳定性，不参与固定步模拟，也不会影响 Reset 的确定性状态。

## 21. 原计划补齐：转向限制与邻域调试

速度积分后不再允许方向任意瞬变。系统比较上一固定步方向与本步期望方向，将旋转角限制为：

```text
MaxTurnAngle = MaxTurnRateDegrees × FixedDeltaTime
```

超出时通过四元数球面插值只完成允许的部分旋转，同时保留速度大小。默认 `MaxTurnRateDegrees=180`，用于减少折线和瞬间掉头。

`bShowNeighborhoodDebug` 启用后，只为 `DebugAgentStableId` 指定的单条鱼绘制调试信息：绿色球表示 NeighborRadius，红色球表示 SeparationRadius，黄色线表示合并后的加速度。只画一条鱼可以验证参数含义，同时避免 50 条鱼全部 DrawDebug 造成噪声和性能干扰。

此前尚未完成的双状态领航编队试验已撤回，没有混入当前基础 Boids 回归版本。开局位置恢复为固定 Seed 下的全水箱随机打散。

同种基础 `SeparationRadius` 提高为 170，使集群保持更松散的个体间距。异种个体额外乘 `CrossSpeciesPersonalSpaceMultiplier=1.45`，在进入接触范围前更早互相避让；种群质心排斥调整为 `SpeciesAvoidanceRadius=1800`、`SpeciesAvoidanceWeight=1.5`，用于让接近的不同种群整体分流。蓝色仍叠加自己的 `BluePersonalSpaceMultiplier`。

## 22. 确定性独游与混合鱼群

原路线只把“独游 / 群游 / 混合模式”列为后续方向，基础版本没有脱群概率。现在每隔 `SoloDecisionInterval`，每条鱼使用 `RandomSeed + StableId + 时间段` 的独立随机流判定是否暂时独游。默认约 18% 的鱼进入独游，因此相同 Seed 和固定步数仍可复现。

独游鱼继续执行所有鱼之间的 Separation、球形接触、边界和食物感知，但 Alignment 降为 30%、Cohesion 降为 12%、共享巡游目标影响降为 20%，个体巡游扰动提高到 1.8 倍。下一个判定周期可以重新加入鱼群。基础 Cohesion 默认值也从 0.8 降为 0.6，避免非独游成员形成过紧团块。

## 23. 非对称种群冲突与让行

异种质心接近时不再让双方承受完全相同的群体排斥。系统使用 `RandomSeed + 种群对 + PrioritySegment` 为每次冲突选择一个让行种群：非让行群保持主要方向，让行群受到 `SpeciesAvoidanceWeight=2.4` 的整体远离力。让行群中每条鱼还根据 StableId 获得不同的侧向散开方向，强度由 `SpeciesYieldScatterWeight=1.35` 控制，因此阵型会暂时被冲散，而不是整团刚性平移。

`SpeciesPriorityInterval=18` 会改变后续冲突的确定性优先级，避免同一种鱼永久让行。食物已被看见时，这两项力仍降为 20%，防止群体冲突压过寻食目标。近距离个体 Separation 和球形碰撞保持双向生效。

## 24. Quaternius 免费鱼模型接入

显示资源采用 Quaternius 的 **Animated Fish Pack**：

- 原始页面：<https://quaternius.com/packs/animatedfish.html>
- 许可：CC0 1.0，可用于个人和商业项目；项目内保留原始 `License.txt`。
- 原始文件归档：`ThirdParty/QuaterniusAnimatedFish/`。
- UE 资源路径：`/Game/Fish/Quaternius/`。

当前五个种群依次使用 `Fish1`、`Fish2`、`Fish3`、`Manta_ray` 和 `Shark`。导入脚本位于 `Tools/ImportQuaterniusFish.py`。五种网格仍由五个 ISM 组件批量绘制，因此没有给单条鱼增加 Actor、组件或 Tick。

UE 的 OBJ 导入器不会可靠采用 `FbxImportUI.import_uniform_scale`；实测这五个网格的包围球半径只有约 4–8 cm。Manager 在 `BeginPlay` 读取每种 Static Mesh 的真实包围球半径，并缓存显示倍率：

```text
MeshScaleMultiplier = FishDisplayRadius / SourceMeshSphereRadius
InstanceScale        = Agent.Scale * MeshScaleMultiplier
```

默认 `FishDisplayRadius=50`，与原球体代理使用的 `50 × Agent.Scale` 碰撞半径一致。该归一化基于资源实际尺寸，不依赖水箱高度、相机距离或某一组硬编码导入倍率；以后替换网格也会自动适配。

Quaternius 模型的鱼头朝本地 +Z。每次实例更新时使用四元数把本地 +Z 对齐到代理速度方向：

```text
FishRotation = FindBetweenNormals(LocalUp, Normalize(Velocity))
```

这样朝向完全来自已有模拟状态，不参与力计算，也不会破坏固定步长的确定性。食物继续使用 Engine Sphere，方向指示器继续使用 Cone，水箱边框继续使用 Cube。

本阶段有意使用静态 ISM，只替换几何外形。资源包虽然带骨骼动画，但逐实例播放骨骼动画需要 AnimToTexture、VAT 或其他 GPU 实例动画方案；在确定性能预算前不改成逐鱼 SkeletalMeshComponent。

## 25. P2 性能优化复现：稳定分桶受力

原资料 P2 4.9 的核心方案是“所有受力计算分帧、缓存公共结果、按设备分级”。当前版本先复现其中对 Boids CPU 开销最直接的一项：使用稳定 ID 把完整行为受力分配到多个固定步。

```text
ActiveBucket = ForceEvaluationStep % ForceUpdateInterval
FishBucket   = StableId % ForceUpdateInterval
```

第一个固定步会为所有鱼预热加速度缓存。之后只有 `FishBucket == ActiveBucket` 的鱼重新计算 Separation、Alignment、Cohesion、种群避让、巡游、食物感知和软边界力；其他鱼复用上次加速度。速度/位置积分、转向限制、六面硬边界、鱼体碰撞和 ISM 提交仍以 60 Hz 执行，因此优化不会直接降低视觉更新频率。

`ForceUpdateInterval` 默认值为 2：

- `1`：基线，每个固定步为所有鱼重算受力。
- `2`：高配分帧档，每步更新一半鱼的受力。
- `4`：低配分帧档，每步更新四分之一鱼的受力。

固定分桶不使用逐帧随机选择，因此同一 Seed、相同配置和相同步数仍可复现。加速度与寻食状态直接缓存于 `FBoidAgent3D`，逐鱼受力循环不创建临时结果数组。

### 25.1 基准方法

2026-08-08 使用源码版 UE 5.7.4、Development Editor、NullRHI，在同一台 Ryzen 5 7500F 机器上测试。每档均为 200 条鱼、固定 Seed、120 步预热、3000 个正式固定步，独立运行三次并取 `avg_step_ms` 中位数。UE 启动时间不计入测量区间。

命令行基准入口只在传入 `-BoidsBenchmark` 时运行，例如：

```text
UnrealEditor-Cmd.exe Boids.uproject /Game/Maps/L_Boids3D -game -NullRHI -Unattended
  -BoidsBenchmark -BoidsAgentCount=200 -BoidsForceInterval=2
  -BoidsWarmupSteps=120 -BoidsBenchmarkSteps=3000
```

### 25.2 实测结果

| ForceUpdateInterval | 三次平均步耗时（ms） | 中位数（ms） | 相对基线 | 加速比 |
| ---: | --- | ---: | ---: | ---: |
| 1 | 0.459510 / 0.464108 / 0.468265 | 0.464108 | 基线 | 1.00× |
| 2 | 0.354931 / 0.349416 / 0.349363 | 0.349416 | -24.7% | 1.33× |
| 4 | 0.298884 / 0.297256 / 0.296976 | 0.297256 | -36.0% | 1.56× |

课程中“约 200 条鱼从接近 10 ms 降至 3 ms 以下”是其移动端样例综合采用分帧、公共缓存、射线替换和表现 LOD 后的数据，不能直接套用到本项目。本轮只复现受力稳定分桶；空间哈希重建、硬碰撞和 ISM 提交仍每步执行，因此收益不会随间隔线性达到 2× 或 4×。后续应分别采样这些阶段，再决定是否缓存种群统计、降低碰撞频率或接入 VAT 表现分级。

### 25.3 UE 关卡内可视化验证

运行 `L_Boids3D` 后按 `B`，Manager 会在同一关卡内自动依次测试 `ForceUpdateInterval=1/2/4`。每档开始时都使用相同 Seed 重新生成相同初始鱼群，先执行 `VisualBenchmarkWarmupSteps`，再采样 `VisualBenchmarkMeasureSteps`。测试期间鱼群保持可见和运动，HUD 左上角显示当前档位、预热/测量阶段和进度。

测试完成后 HUD 同时保留三档平均固定步耗时，并以全量重算为基线计算耗时下降百分比和加速比。右上角原运行计时器保持不变。关卡内测试默认每档预热 120 步、测量 600 步，约 36 秒完成；它用于直观 A/B 对比，正式性能报告仍建议使用无界面命令行三次取中位数。
