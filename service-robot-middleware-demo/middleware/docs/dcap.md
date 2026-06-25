# 实时录制 vs 触发录制 vs 循环缓冲（ROS2 的 rosbag2\_compressed+ max\_cache\_size)

## 一、三种录制模式概述

| 模式       | **核心逻辑**    | **存储特征**  | **典型场景**     |
| :------- | :---------- | :-------- | :----------- |
| **实时录制** | 从头到尾一直写     | 文件巨大，线性增长 | 标定、长测、算法训练   |
| **触发录制** | 满足条件才写      | 碎片化，精准    | 异常捕获、故障复现    |
| **循环缓冲** | 内存里转圈，触发才落盘 | 文件小，含前后文  | **黑匣子（EDR）** |

使用`ros2 bag record`命令持续录制：

```
┌─────────────────────────────────────────────┐
│            FA_Collection (数据采集)           │
├─────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│  │ 实时录制     │  │ 触发录制     │  │ 循环缓冲     │
│  │ Continuous  │  │ Triggered   │  │ Circular    │
│  ├─────────────┤  ├─────────────┤  ├─────────────┤
│  │ • 全量记录   │  │ • 事件驱动   │  │ • 最近N秒   │
│  │ • 压缩存储   │  │ • 按需记录   │  │ • 触发保存   │
│  │ • 定期归档   │  │ • 节省空间   │  │ • 黑匣子    │
│  └─────────────┘  └─────────────┘  └─────────────┘
│          │                │                │
│          ▼                ▼                ▼
│  ┌─────────────────────────────────────────────┐
│  │           rosbag2 + MCAP + CDR              │
│  │  • 压缩：zstd                               │
│  │  • 缓存：max_cache_size=2GB                 │
│  │  • 分割：max_bag_size=10GB                  │
│  └─────────────────────────────────────────────┘
└─────────────────────────────────────────────┘
```

<br />

## 二、实时录制（Continuous Recording）

### 2.1 基本实现

这是最基础的 `ros2 bag record`。

&#x20;

### 2.1 基础用法

```
ros2 bag record -a -o /data/bags/session_001
```

### 2.2 关键参数（你的架构必须配）&#x20;

#### ✅ `rosbag2_compressed`（压缩）&#x20;

ROS2 Humble/Jazzy 之后强烈建议开启：

```
ros2 bag record -a \
  --compression-mode file \
  --compression-format zstd \
  -o bag_name
```

<br />

| 压缩模式      | 说明              | 适用场景            |
| :-------- | :-------------- | :-------------- |
| `file`    | 整个 bag 文件压缩（默认） | 存储优化，回放慢        |
| `message` | 每条消息单独压缩        | 随机 seek 快，CPU 高 |

<br />

**实测效果**（激光雷达 10Hz + 图像 30Hz）：

- 未压缩：\~15GB / 小时
- zstd 压缩：\~4GB / 小时（3.75x 压缩比）

#### ✅ `max_cache_size`（内存缓存）&#x20;

这是**防丢帧的关键参数**，不是限制文件大小。

```
ros2 bag record -a \
  --max-cache-size 1073741824 \  # 1GB 内存缓存
  -o bag_name
```

**原理**：

```
DDS → [内存缓存 1GB] → 磁盘线程慢慢写
```

如果不设（默认 100MB），遇到点云爆发写入时，缓存满了直接丢数据。

> 📌 **工程经验**：
>
> - 车载：至少 **2\~4GB**​
> - 机器人：512MB \~ 1GB
> - 设太小 = 高速运动时丢帧

## 三、触发录制（Triggered Recording）

**不是 rosbag2 原生能力，需要你在 FA\_Collection 层自己做逻辑。**

### 3.1 实现思路

```
# 触发时启动
ros2 bag record -a \
  --compression-mode file \
  --compression-format zstd \
  --max-cache-size 2GB \
  --max-bag-size 10GB \   # 单个文件最大 10GB
  -o /data/bags/trigger_$(date +%s)
```

<br />

```
┌──────────────────────────────┐
│  异常检测 / 事件判断节点       │
│  • 碰撞传感器                 │
│  • 急刹车 (acc > 5m/s²)       │
│  • AI 异常评分                │
└──────────────┬───────────────┘
               │ trigger
┌──────────────▼───────────────┐
│  FA_Collection 控制节点        │
│  • 调用 ros2 bag record       │
│  • 或调用 rosbag2_cpp API     │
└──────────────┬───────────────┘
               ▼
┌──────────────────────────────┐
│  开始录制（带压缩 + 缓存）     │
│  录制 30 秒后自动停止         │
└──────────────────────────────┘
```

### 3.2 C++ 控制 rosbag2 的示例（推荐）&#x20;

```
#include "rosbag2_cpp/writer.hpp"

class TriggerRecorder {
public:
    void start_recording() {
        writer_ = std::make_unique<rosbag2_cpp::Writer>();
        writer_->open("trigger_bag");
        writer_->create_topic(topic_metadata_);
        // 开始订阅并写 bag
    }

    void stop_recording() {
        writer_.reset();  // 关闭 bag
    }
};
```

### 3.3 触发录制的典型配置&#x20;

```
# 触发时启动
ros2 bag record -a \
  --compression-mode file \
  --compression-format zstd \
  --max-cache-size 2GB \
  --max-bag-size 10GB \   # 单个文件最大 10GB
  -o /data/bags/trigger_$(date +%s)
```

**优点**：

- 节省 90% 存储空间
- 只录“出事的那一段”

**缺点**：

- 错过触发前的上下文（这就是为什么需要循环缓冲）

***

## 四、循环缓冲（Circular Buffer / Black Box）&#x20;

这是 **ADAS / 自动驾驶 / 高端机器人 必选方案**。

### 4.1 核心思想（最重要）&#x20;

> **数据一直在内存里转圈，只保留最近 N 秒；一旦触发，把内存里的 N 秒 + 之后 M 秒一起落盘。**

```
时间轴：
... | T-30s | T-29s | ... | T-1s | [触发!] | T+1s | ... | T+30s |
      ↑_______________________________↑
              始终保留（内存）
      ↑_________________________________________↑
                    触发后落盘
```

### 4.2 ROS2 原生支持吗？&#x20;

❌ \*\*rosbag2 原生不支持循环缓冲。\*\*​

✅ 但可以通过 \*\*`max_bag_size + 文件轮转 + 自定义逻辑`\*\*​ 模拟。

### 4.3 工程级实现方案（推荐你用）&#x20;

#### 方案 A：双 Bag 文件轮转（简单实用）&#x20;

```
# 启动两个录制进程，交替写
ros2 bag record -a -o /data/bags/buffer_1 --max-bag-size 5GB &
ros2 bag record -a -o /data/bags/buffer_2 --max-bag-size 5GB &
```

触发时：

- 停止当前写入
- 把最近 30 秒的文件重命名为 `incident_xxx.db3/.mcap`
- 重启新的 buffer

#### 方案 B：自定义内存缓冲（工业级）&#x20;

这是 **AUTOSAR / 车规级**​ 做法：

```
┌──────────────────────────────────────┐
│        FA_Collection 节点            │
│  ┌──────────────────────────────┐   │
│  │  内存 Ring Buffer (SHM)       │   │
│  │  • 最近 30 秒数据             │   │
│  │  • 无磁盘 IO                  │   │
│  └──────────────┬───────────────┘   │
│                 │ 触发              │
│  ┌──────────────▼───────────────┐   │
│  │  异步写入线程（低优先级）      │   │
│  │  • 写 MCAP + CDR             │   │
│  │  • 不阻塞实时链路            │   │
│  └──────────────────────────────┘   │
└──────────────────────────────────────┘
```

**关键参数：**

- 内存缓冲大小：根据数据量算（例如 30s × 500MB/s ≈ 15GB）
- 写入线程优先级：`SCHED_FIFO`，低于控制线程

***

## 五、rosbag2 参数组合速查表&#x20;

参数

作用

你的架构推荐值

`--compression-mode file`

文件级压缩

✅ 必开

`--compression-format zstd`

压缩算法

✅ 最佳平衡

`--max-cache-size`

内存缓存

\*\*2\~4GB（车载）\*\*​

`--max-bag-size`

单文件大小

**10\~20GB**​

`--storage-preset-profile`

存储预设

`none`（默认）或 `memory`（实验）

***

## 六、映射到你的架构图&#x20;

```
FA_Collection (数据采集)
│
├── 实时录制（长测/标定）
│   └─ rosbag2 + MCAP + zstd + 4GB cache
│
├── 触发录制（异常/故障）
│   └─ FA_Collection 控制节点 + rosbag2_cpp API
│
└── 循环缓冲（黑匣子）
    └─ 内存 Ring Buffer (SHM)
        ├─ 最近 30 秒（不落盘）
        └─ 触发 → 异步落盘 MCAP
```

***

## 七、终极建议

如果你的系统是 **车载 / 移动机器人 / 工业臂**：

1. **默认不开实时录制**（太浪费）
2. **循环缓冲常驻内存**（黑匣子）
3. **触发录制作为补充**（事故片段）
4. **MCAP + zstd + 2GB cache**​ 作为统一落盘格式

> 📌 **一句话总结**：
>
> **实时录制是“为了以后”，触发录制是“为了找错”，循环缓冲是“为了保命”。**

\*\*FA\_Collection 的 C++ 类设计（RingBuffer + TriggerLogic + AsyncWriter）\*\*​ 

<br />

## 时间戳问题 ：DREP 发布的数据时间戳是录制时的，X86 仿真时需要处理时间同步

<br />

### 方案 A：使用 ROS2 时钟同步（推荐）

关键点 ：

- ROS2 支持 clock 参数，可以使用模拟时钟
- DREP 可以发布 /clock 话题
- 节点设置 use\_sim\_time=true 即可使用 DREP 的时钟

### 方案 B：时间戳偏移

```C++
// X86 模式：计算时间偏移
rclcpp::Time playback_start_time = first_msg_time;
rclcpp::Time sim_start_time = this->now();

// 后续消息的时间戳 = 回放时间戳 - 回放起始时间 + 仿真起始时间
rclcpp::Time adjusted_time = msg->header.stamp - 
playback_start_time + sim_start_time;
```

### 方案 C：完全使用录制时间戳

```C++
// X86 模式：直接使用录制的时间戳
// 注意：下游节点可能需要开启 use_sim_time
msg->header.stamp = recorded_timestamp;
```

