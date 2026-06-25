# Aadapter层

&#x20;

std\_msgs/Header header&#x20;

uint32 width&#x20;

uint32 height&#x20;

uint32 channels&#x20;

uint32 step&#x20;

uint8\[] data

我想知道这样的数据如何发布，如何接收，请举个例子？

# 一、数据长什么样

假设有一张：

```
640 x 480 RGB图
```

每个像素：


```
R G B
```

3字节

因此：


```
width    = 640
height   = 480
channels = 3

step = 640 * 3
     = 1920

data.size()
     = 640 * 480 * 3
     = 921600
```

内存布局：


```
data

[R][G][B]
[R][G][B]
[R][G][B]
...
```

连续存储。

***

# 二、Publisher 示例

假设从 OpenCV 读取图片。


```
#include <rclcpp/rclcpp.hpp>

#include <opencv2/opencv.hpp>

#include "my_msgs/msg/image_lite.hpp"

class ImagePublisher : public rclcpp::Node
{
public:

    ImagePublisher()
        : Node("image_pub")
    {
        pub_ =
            create_publisher<
                my_msgs::msg::ImageLite
            >(
                "image",
                10
            );

        timer_ =
            create_wall_timer(
                std::chrono::seconds(1),
                std::bind(
                    &ImagePublisher::publishImage,
                    this
                )
            );
    }

private:

    void publishImage()
    {
        cv::Mat img =
            cv::imread("test.jpg");

        if(img.empty())
            return;

        my_msgs::msg::ImageLite msg;

        msg.header.stamp = now();
        msg.header.frame_id = "camera";

        msg.width = img.cols;
        msg.height = img.rows;

        msg.channels = img.channels();

        msg.step =
            img.cols *
            img.channels();

        size_t bytes =
            img.total() *
            img.elemSize();

        msg.data.assign(
            img.data,
            img.data + bytes
        );

        pub_->publish(msg);

        RCLCPP_INFO(
            get_logger(),
            "publish %u x %u",
            msg.width,
            msg.height
        );
    }

    rclcpp::Publisher<
        my_msgs::msg::ImageLite
    >::SharedPtr pub_;

    rclcpp::TimerBase::SharedPtr timer_;
};
```

***

# 三、Subscriber 示例

收到后恢复成 OpenCV 图像。


```
#include <rclcpp/rclcpp.hpp>

#include <opencv2/opencv.hpp>

#include "my_msgs/msg/image_lite.hpp"

class ImageSubscriber
    : public rclcpp::Node
{
public:

    ImageSubscriber()
        : Node("image_sub")
    {
        sub_ =
            create_subscription<
                my_msgs::msg::ImageLite
            >(
                "image",
                10,
                std::bind(
                    &ImageSubscriber::callback,
                    this,
                    std::placeholders::_1
                )
            );
    }

private:

    void callback(
        const my_msgs::msg::ImageLite::SharedPtr msg
    )
    {
        cv::Mat image(
            msg->height,
            msg->width,
            CV_8UC3,
            const_cast<uint8_t*>(
                msg->data.data()
            )
        );

        cv::imshow(
            "image",
            image
        );

        cv::waitKey(1);

        RCLCPP_INFO(
            get_logger(),
            "recv %u x %u",
            msg->width,
            msg->height
        );
    }

    rclcpp::Subscription<
        my_msgs::msg::ImageLite
    >::SharedPtr sub_;
};
```

***

# 四、DDS 实际发送的数据

ROS2 最终会把：


```
header
width
height
channels
step
data
```

序列化成：


```
--------------------------------
stamp
frame_id

width
height
channels
step

921600 bytes image data
--------------------------------
```

发送给 DDS。

你不用自己处理序列化。

只需要：


```
pub_->publish(msg);
```

ROS2 自动完成：


```
C++对象
  ↓
cdr序列化
  ↓
DDS
  ↓
网络
  ↓
反序列化
  ↓
C++对象
```

***

# 五、如果不使用 OpenCV

例如你有一块原始 RGB Buffer：


```
uint8_t* buffer;
```

大小：


```
640*480*3
```

直接：


```
msg.width = 640;
msg.height = 480;
msg.channels = 3;
msg.step = 640 * 3;

msg.data.resize(
    640 * 480 * 3
);

memcpy(
    msg.data.data(),
    buffer,
    msg.data.size()
);
```

发布即可。

***

# 六、举个具体数值例子

假设图像：


```
2 x 2 RGB
```

像素：


```
红 绿
蓝 白
```

RGB值：


```
(255,0,0)
(0,255,0)

(0,0,255)
(255,255,255)
```

消息内容：


```
width = 2
height = 2

channels = 3

step = 6
```

data：


```
255 0 0
0 255 0
0 0 255
255 255 255
```

共：


```
2 * 2 * 3

= 12 bytes
```

Subscriber 收到后：


```
cv::Mat(
    2,
    2,
    CV_8UC3,
    msg->data.data()
);
```

立刻恢复成原图。

***

实际上，如果你的消息只是图片数据，ROS 官方的：

sensor\_msgs/Image

定义已经非常成熟：


```
header
height
width
encoding
is_bigendian
step
data
```

大多数工具（RViz、image\_transport、cv\_bridge）都直接支持它。除非有特殊原因，否则通常建议直接使用 `sensor_msgs/Image`，这样后续调试和生态兼容性会好很多。

那也就是说实际上它还是一个rclcpp对象，而不是一个普通的如下struct对象。Struct Image{uint32 width; uint32 height; uint32 channels; uint32 step;  uint8\[] data;}; 那比如说我本身代码里几乎所有的逻辑都是C/C++按照这样的类型定义来写的，那转成ros2会很难？有没有好一点的方法

其实恰恰相反，**如果你的项目已经是纯 C/C++ 的 struct 设计，那么迁移到 ROS 2 往往比很多人想象的容易。**

你现在可能有这样的代码：


```
struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;

    std::vector<uint8_t> data;
};
```

算法代码：


```
class Detector
{
public:
    Result Run(const Image& img);
};
```

整个系统：


```
Camera
   ↓
Image
   ↓
Detector
   ↓
Result
   ↓
Tracker
```

这是非常好的架构。

***

## 不要把 ROS 类型传进业务代码

很多 ROS 新手会写成：


```
Result Run(
    const sensor_msgs::msg::Image& img
);
```

然后整个工程到处都是：


```
sensor_msgs::msg::Image
geometry_msgs::msg::Pose
nav_msgs::msg::Odometry
```

最后算法层和 ROS 完全绑定。

这其实不是最好的设计。

***

## 推荐架构：业务类型与 ROS 类型分离

保留你原来的结构：


```
struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;

    std::vector<uint8_t> data;
};
```

算法：


```
class Detector
{
public:
    Result Run(const Image&);
};
```

ROS 只存在于 Adapter 层：


```
ROS2
  ↓
RosImageAdapter
  ↓
Image
  ↓
Detector
```

***

## 示例

### 业务类型


```
struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;

    std::vector<uint8_t> data;
};
```

***

### 转换函数


```
Image ToImage(
    const sensor_msgs::msg::Image& msg)
{
    Image img;

    img.width = msg.width;
    img.height = msg.height;

    img.channels = 3;

    img.step = msg.step;

    img.data = msg.data;

    return img;
}
```

***

ROS Callback：


```
void callback(
    const sensor_msgs::msg::Image::SharedPtr msg)
{
    Image image =
        ToImage(*msg);

    detector_.Run(image);
}
```

你的 Detector 根本不知道 ROS 存在。

***

## 反向发布也一样

业务层输出：


```
struct Pose
{
    double x;
    double y;
    double theta;
};
```

ROS Adapter：


```
geometry_msgs::msg::Pose2D ToRos(
    const Pose& pose)
{
    geometry_msgs::msg::Pose2D msg;

    msg.x = pose.x;
    msg.y = pose.y;
    msg.theta = pose.theta;

    return msg;
}
```

发布：


```
publisher_->publish(
    ToRos(pose)
);
```

***

## 更进一步：完全不用 sensor\_msgs

如果你定义自己的 ROS 消息：


```
Image.msg

uint32 width
uint32 height
uint32 channels
uint32 step
uint8[] data
```

ROS 会自动生成：


```
my_msgs::msg::Image
```

这个生成的类型本质上接近：


```
struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;

    std::vector<uint8_t> data;
};
```

大概会变成：


```
namespace my_msgs::msg
{
template<class Allocator>
struct Image_
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;

    std::vector<uint8_t, Allocator> data;
};
}
```

所以 ROS 消息本身就是一个 C++ struct。

它不是某种神秘对象。

***

## 很多工业项目怎么做

实际上工业机器人项目常见是：


```
core/
    image.h
    pose.h
    imu.h

algorithm/
    detector.cpp
    slam.cpp

ros_adapter/
    image_convert.cpp
    pose_convert.cpp

ros_node/
    camera_node.cpp
    slam_node.cpp
```

依赖关系：


```
algorithm
    ↓
core types

ros_adapter
    ↓
core types

ros_node
    ↓
ros_adapter
```

而不是：


```
algorithm
    ↓
sensor_msgs
geometry_msgs
rclcpp
```

***

## 如果以后 ROS 被替换

假设未来不用 ROS2：


```
ROS2
↓
CyberRT
```

或者：


```
ROS2
↓
ZeroMQ
```

或者：


```
ROS2
↓
MQTT
```

你只需要改：


```
ros_adapter/
```

而不用动：


```
detector.cpp
slam.cpp
planner.cpp
```

***

所以对于你这种已经有大量 C/C++ 代码、数据结构清晰的项目，我反而建议：


```
struct Image;
struct Pose;
struct Imu;
struct PointCloud;
```

全部保持原样。

然后专门写一层：


```
Image ToCore(const sensor_msgs::msg::Image&);
sensor_msgs::msg::Image ToRos(const Image&);
```

这样接入 ROS 2 的工作量通常只是增加一些转换函数，而不是重写业务逻辑。很多大型机器人项目（SLAM、自动驾驶、工业机器人）实际上都是这么做的。

那岂不是有巨量的转换函数代码，这些是被称作胶水代码吗？有没有很好的方法来简化写代码的工作量，尤其是这块的适配，比如反射特性？或是其他代码生成工具？

你问到了一个非常核心的问题。

实际上，很多大型 ROS 项目最后都会遇到：


```
业务类型 <-> ROS消息
```

之间的大量转换代码。

这种代码通常确实被称为：


```
Glue Code（胶水代码）
Adapter Code（适配层）
Marshaller（编组）
Mapper（映射）
```

而且很多项目后期会发现：


```
业务代码 20万行
转换代码 5万行
```

并不少见。

***

# 先说结论

如果你的项目已经有自己的数据结构体系：


```
Image
Pose
PointCloud
Imu
Gps
DetectionResult
Track
```

我一般推荐：

### 小项目

直接手写


```
ToRos()
FromRos()
```

***

### 中大型项目

自己搞一个自动映射框架

或者

利用代码生成

***

### 超大型项目

让业务类型成为消息定义的唯一来源

自动生成：


```
C++ Struct
ROS Msg
DDS IDL
Protobuf
Json
```

全部从一个 Schema 生成。

***

# 为什么 ROS 官方没有解决这个问题

因为 ROS 的设计理念是：


```
.msg
就是唯一数据定义
```

例如：


```
Image.msg
```

生成：


```
sensor_msgs::msg::Image
```

ROS 官方认为：


```
sensor_msgs::msg::Image
```

就应该直接在业务代码里使用。

所以官方生态很少讨论：


```
Core Type
↔
ROS Type
```

转换。

但工业项目恰恰相反。

***

# 方案1：模板自动映射（最实用）

例如：


```
struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;
    std::vector<uint8_t> data;
};
```

ROS：


```
my_msgs::msg::Image
```

字段完全一样。

那么可以写：


```
template<typename Src, typename Dst>
Dst Convert(const Src&);
```

特化：


```
template<>
my_msgs::msg::Image
Convert<Image,my_msgs::msg::Image>(
    const Image& src)
{
    my_msgs::msg::Image dst;

    dst.width = src.width;
    dst.height = src.height;
    dst.channels = src.channels;
    dst.step = src.step;
    dst.data = src.data;

    return dst;
}
```

这种方式最常见。

***

# 方案2：宏注册字段

很多团队会这么干。

定义：


```
struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;

    std::vector<uint8_t> data;
};
```

注册：


```
REFLECT(
    Image,
    width,
    height,
    channels,
    step,
    data
)
```

然后：


```
auto ros_msg =
    auto_convert<
        Image,
        my_msgs::msg::Image
    >(img);
```

自动遍历字段。

***

这其实类似：

- &#x20;Boost.PFR&#x20;
- &#x20;magic\_get&#x20;
- &#x20;visit\_struct&#x20;

的玩法。

***

# 方案3：Boost.PFR（非常推荐）

如果你用 C++17。

例如：


```
struct Image
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t step;

    std::vector<uint8_t> data;
};
```

Boost PFR 可以：


```
boost::pfr::for_each_field(...)
```

无需宏。

于是可以写：


```
template<typename Src,typename Dst>
Dst AutoConvert(const Src& src)
{
    Dst dst;

    boost::pfr::for_each_field(
        src,
        [&](const auto& field, std::size_t idx)
        {
            boost::pfr::get<idx>(dst)
                = field;
        });

    return dst;
}
```

只要字段顺序一致：


```
Image
```

↓


```
my_msgs::msg::Image
```

自动完成。

***

很多自动驾驶项目这么干。

***

# 方案4：利用 ROS IDL 反向生成 Struct

这是我最喜欢的方案。

例如：


```
Image.idl
```


```
struct Image
{
    uint32 width;
    uint32 height;
    uint32 channels;
    uint32 step;

    sequence<uint8> data;
};
```

然后生成：


```
ROS Message

C++ Struct

DDS Type
```

统一来源。

***

这样：


```
Image.idl
    ↓
生成器
    ↓
Core::Image

my_msgs::msg::Image
```

无需转换。

***

很多 DDS 项目本来就是这样。

***

# 方案5：直接共享内存对象（高性能）

图像尤其特殊。

例如：


```
1920x1080 RGB

6MB
```

如果：


```
Core::Image
→ ROS Image
```

复制一次。

DDS：


```
ROS Image
→ DDS Buffer
```

再复制一次。

很浪费。

***

很多视觉系统会这样：


```
struct ImageView
{
    uint32_t width;
    uint32_t height;

    uint8_t* data;
};
```

ROS Message只传：


```
shared_ptr<ImageBuffer>
```

或者：


```
LoanedMessage
```

实现零拷贝。

ROS2 本身支持：


```
rclcpp::LoanedMessage
```

***

# 如果是你的情况

从你的描述看：


```
已有大量C/C++
已有自己的Struct
未来还要上ARM
不希望业务层依赖ROS
```

我会推荐：

## 第一阶段

保持：


```
struct Image;
struct Pose;
struct Imu;
```

不变。

写：


```
ToRos()
FromRos()
```

先把系统跑起来。

***

## 第二阶段

引入：


```
Boost.PFR
```

做自动字段映射。

把大量：


```
dst.a = src.a;
dst.b = src.b;
dst.c = src.c;
```

消掉。

***

## 第三阶段（长期）

建立统一 Schema：


```
IDL
YAML
Proto
```

然后自动生成：


```
Core Struct
ROS Msg
DDS Type
Json Serializer
```

这其实就是很多自动驾驶和机器人公司的最终形态。
