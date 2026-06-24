# 服务机器人中间件 Demo

本 Demo 演示了**数据驱动仿真**的核心理念：**同一套业务处理代码，在部署模式和仿真模式下无差别运行**。

## 架构图

```
┌──────────── 部署模式 (Deploy / ARM) ─────────────┐
│                                                    │
│  ┌──────────┐    ┌───────────┐    ┌────────────┐  │
│  │ Dummy HAL│───▶│ gdc_      │───▶│ Fake VLA   │  │
│  │ (Camera) │    │ process   │    │ 决策       │  │
│  └──────────┘    └───────────┘    └────────────┘  │
│        │              │                │           │
│        └──────────────┼────────────────┘           │
│                       ▼                            │
│              ┌─────────────────┐                   │
│              │  Data Recorder  │                   │
│              │  (录制为文件)   │                   │
│              └─────────────────┘                   │
│                                                    │
└────────────────────────────────────────────────────┘

┌──────────── 仿真模式 (Sim / x86) ────────────────┐
│                                                    │
│  ┌──────────┐    ┌───────────┐    ┌────────────┐  │
│  │ Data     │───▶│ gdc_      │───▶│ Fake VLA   │  │
│  │ Player   │    │ process   │    │ 决策       │  │
│  │ (回灌)   │    │【相同代码】│    │【相同代码】│  │
│  └──────────┘    └───────────┘    └────────────┘  │
│                                                    │
└────────────────────────────────────────────────────┘
```

## 目录结构

```
service-robot-middleware-demo/
├── CMakeLists.txt
├── README.md
├── include/srm/
│   ├── common/           # 通用类型、日志、序列化
│   ├── dal/              # 通信抽象层 (DAL)
│   ├── hal/              # 硬件抽象层 (HAL)
│   ├── data/             # 数据录制与回放
│   ├── perception/       # 感知算法（业务层）
│   └── decision/         # 决策层（Fake VLA）
└── src/
    ├── dal/
    ├── hal/
    ├── data/
    ├── perception/
    ├── decision/
    ├── main_deploy.cpp   # 部署模式入口
    └── main_sim.cpp      # 仿真模式入口
```

## 核心模块说明

### 1. 通信抽象层 (DAL)
- `IPubSubTransport`：统一的发布/订阅接口
- `InprocTransport`：进程内传输（模拟共享内存，Demo 用）
- 真实系统中可替换为：共享内存 / DDS / MQTT

### 2. 硬件抽象层 (HAL)
- `ICameraDevice`：相机设备接口
- `DummyCamera`：Dummy 相机实现，生成测试图像

### 3. 数据录制与回放
- `DataRecorder`：数据录制器（类似 MCAP Writer）
- `DataPlayer`：数据回放器（类似 MCAP Reader + 时间同步）

### 4. 业务算法（核心！相同代码）
- `GdcProcessNode`：感知处理节点（部署和仿真用完全相同的代码）
- `FakeVlaNode`：Fake VLA 决策节点（部署和仿真用完全相同的代码）

**关键：业务算法完全不感知自己运行在部署模式还是仿真模式！**

## 构建与运行

### 构建

```bash
cd service-robot-middleware-demo
mkdir build && cd build
cmake ..
make -j
```

### 运行部署模式（生成测试数据）

```bash
./srm_deploy
```

运行 5 秒后退出，生成 `sample_data.mcap` 文件。

### 运行仿真模式（用录制的数据驱动）

```bash
./srm_sim sample_data.mcap
```

数据回放器会按照录制时的时间节奏，将数据回灌到中间件，业务算法照常运行。

## 核心理念

1. **代码一致性**：业务层代码完全相同，不存在"仿真一套、部署一套"
2. **数据驱动**：通过切换数据来源（HAL vs DataPlayer）切换模式
3. **中间件解耦**：业务模块只依赖 DAL 接口，不依赖具体数据源
4. **数据飞轮**：采集 → 回放调试 → 算法优化 → 部署 → 再采集

## 扩展方向

- 接入真实 ROS 2 DDS 通信
- 替换 InprocTransport 为真实共享内存（iceoryx / 自研）
- 使用真正的 MCAP 格式文件
- 接入物理仿真引擎（Gazebo / Isaac Sim）
- 接入真实 VLA 模型（RT-2 / Octo 等）
