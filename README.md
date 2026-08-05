# Boids Aquarium

基于 Unreal Engine C++ 实现的确定性三维 Boids 鱼群模拟。项目同时保留二维回归关卡，并使用单一 Manager、连续代理数据和 Instanced Static Mesh 批量渲染鱼群。

## 当前功能

- Separation、Alignment、Cohesion 三条基础 Boids 规则
- 固定随机种子、`1/60 s` 固定步长和最多 4 次子步，便于复现
- 三维空间哈希邻域查询
- 五种非均匀数量、大小和运动能力的鱼
- 同种聚群、异种群体避让、确定性独游和冲突让行
- 大范围椭圆巡游航线、转向角速度限制和六面水箱边界
- 鱼体球形防重叠约束
- 顶部随机投食、最多 20 个食物槽位和对象池复用
- 鱼在视锥内发现食物后选择最近目标，接触后停用对应食物
- 自由观察相机、水箱边框、运动方向指示器和运行计时 HUD
- 五种 CC0 鱼模型，通过 ISM 保持集中式渲染

## 环境

- Unreal Engine 5.7 源码版或兼容版本
- Visual Studio 2022，安装“使用 C++ 的游戏开发”工作负载
- Windows 10/11

本项目当前在 UE 5.7.4 源码版上完成编译验证。生成目录、IDE 缓存和本地日志已由 `.gitignore` 排除。

## 运行

1. 克隆仓库。
2. 右键 `Boids/Boids.uproject`，选择对应 Unreal Engine 版本。
3. 生成 Visual Studio 项目文件。
4. 使用 `Development Editor | Win64` 编译 `BoidsEditor`。
5. 打开 `Boids/Boids.uproject`。
6. 加载 `/Game/Maps/L_Boids3D` 并点击播放。

源码版引擎也可以直接执行：

```powershell
Engine\Build\BatchFiles\Build.bat BoidsEditor Win64 Development -Project="<仓库路径>\Boids\Boids.uproject" -WaitMutex -FromMsBuild
```

二维回归关卡位于 `/Game/Maps/L_Boids2D`。

## 操作

| 输入 | 功能 |
| --- | --- |
| `W / S` | 前进 / 后退 |
| `A / D` | 左移 / 右移 |
| `Q / E` | 下降 / 上升 |
| `左 Shift` | 加速移动 |
| 按住鼠标右键并移动 | 自由观察 |
| `F` | 从水箱顶部随机投放一个食物 |

## 技术结构

- `ABoidsManager`：二维回归实现。
- `ABoidsManager3D`：三维固定步模拟、空间哈希、行为状态、食物池和实例渲染。
- `FBoidAgent3D`：连续保存位置、速度、加速度、种群、大小和实例索引。
- `ABoidsFreeCameraPawn`：运行时相机输入与投食入口。
- `ABoidsHUD`：右上角运行计时器。

每条鱼不是独立 Actor，也没有独立 Tick。所有状态集中保存在 Manager 的数组中，五种鱼分别由五个 ISM 组件绘制。导入模型的源尺寸不同，Manager 会读取 Static Mesh 的实际包围球并归一化到 `FishDisplayRadius`，使显示尺寸与碰撞尺度保持一致。

更详细的阶段设计、算法原理和参数记录见 [开发文档](Docs/BoidsDevelopment.md)。

## 美术资源与许可

鱼模型来自 [Quaternius Animated Fish Pack](https://quaternius.com/packs/animatedfish.html)，采用 CC0 1.0 公共领域贡献许可。原始许可证保存在 [License.txt](ThirdParty/QuaterniusAnimatedFish/License.txt)。

当前阶段使用静态网格和 ISM；资源包中的骨骼动画尚未接入。后续可考虑 AnimToTexture 或 VAT，在保留 GPU 实例化的前提下增加游动动画。

## 当前提交说明

- `f789812`：建立确定性二维/三维 Boids、水箱、相机、食物系统、五种鱼群行为，并导入 CC0 鱼模型。
- `244dd73`：修复 OBJ 导入尺寸过小导致模型不可见的问题，按真实网格包围盒自动归一化显示尺寸。
- 本次 README 提交：补充仓库首页文档、运行方法、控制说明、架构和许可证信息。

## 后续方向

- 自动化验证固定 Seed、固定步数和六面反弹
- 在 2000 条代理规模下记录 Game Thread、Render Thread 和 GPU 基线
- GPU 实例鱼体动画
- 水体材质、水下光照、障碍物与捕食行为
