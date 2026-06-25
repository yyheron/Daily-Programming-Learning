#include "perception_node.hpp"
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/vector3.hpp>



PerceptionNode::PerceptionNode(const rclcpp::NodeOptions& options)
    : Node("perception_node", options) {
    this->declare_parameter("input_topic", "/camera/front/image_raw");
    this->declare_parameter("imu_input_topic", "/imu/data");
    this->declare_parameter("output_topic", "/perception/result");
    this->declare_parameter("process_interval_ms", 33);
    this->declare_parameter("use_intra_process_comms", true);

    std::string input_topic = this->get_parameter("input_topic").as_string();
    std::string imu_input_topic = this->get_parameter("imu_input_topic").as_string();
    std::string output_topic = this->get_parameter("output_topic").as_string();
    process_interval_ms_ = this->get_parameter("process_interval_ms").as_int();

    rclcpp::SensorDataQoS qos;

    result_pub_ = this->create_publisher<
        service_robot_middleware_demo::msg::PerceptionResult>(output_topic, qos);

    image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        input_topic, qos,
        [this](sensor_msgs::msg::Image::ConstSharedPtr msg) {
            on_image_received(msg);
        });

    imu_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
        imu_input_topic, qos,
        [this](sensor_msgs::msg::Imu::ConstSharedPtr msg) {
            on_imu_received(msg);
        });

    RCLCPP_INFO(this->get_logger(),
        "PerceptionNode started: %s -> %s",
        input_topic.c_str(), output_topic.c_str());
}

void PerceptionNode::on_image_received(sensor_msgs::msg::Image::ConstSharedPtr msg) {
    auto now = this->now();
    auto elapsed = (now - last_process_time_).nanoseconds() / 1'000'000;
    if (elapsed < process_interval_ms_) {
        return;
    }
    last_process_time_ = now;

    auto result = process_image(msg);
    result_pub_->publish(std::move(result));
}

void PerceptionNode::on_imu_received(sensor_msgs::msg::Imu::ConstSharedPtr msg) {
    auto now = this->now();
    auto elapsed = (now - last_process_time_).nanoseconds() / 1'000'000;
    if (elapsed < process_interval_ms_) {
        return;
    }
    last_process_time_ = now;

    auto result = process_imu(msg);
    result_pub_->publish(std::move(result));
}

service_robot_middleware_demo::msg::PerceptionResult::UniquePtr
PerceptionNode::process_image(sensor_msgs::msg::Image::ConstSharedPtr msg) {
    auto result = std::make_unique<
        service_robot_middleware_demo::msg::PerceptionResult>();

    result->header.stamp = this->now();
    result->header.frame_id = msg->header.frame_id;
    result->image_width = msg->width;
    result->image_height = msg->height;

    auto process_start = this->now();

    size_t object_count = (msg->width * msg->height) % 5 + 1;
    for (size_t i = 0; i < object_count; ++i) {
        service_robot_middleware_demo::msg::ObjectInfo obj;
        obj.label = "object_" + std::to_string(i);
        obj.confidence = 0.7 + (i % 3) * 0.1;
        obj.track_id = static_cast<uint32_t>(i);

        obj.center.x = static_cast<double>(msg->width) / (object_count + 1) * (i + 1);
        obj.center.y = static_cast<double>(msg->height) / 2.0;
        obj.center.z = 0.0;

        obj.size.x = 0.5;
        obj.size.y = 0.3;
        obj.size.z = 0.2;

        uint32_t x1 = static_cast<uint32_t>(obj.center.x - 20);
        uint32_t y1 = static_cast<uint32_t>(obj.center.y - 15);
        uint32_t x2 = static_cast<uint32_t>(obj.center.x + 20);
        uint32_t y2 = static_cast<uint32_t>(obj.center.y + 15);
        obj.bounding_box = {
            static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF),
            static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF),
            static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2 & 0xFF),
            static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2 & 0xFF)
        };

        result->objects.push_back(std::move(obj));
    }

    result->scene_label = "indoor";
    result->scene_confidence = 0.95;

    auto process_end = this->now();
    result->processing_latency_ms =
        (process_end - process_start).nanoseconds() / 1'000'000.0;

    return result;
}

service_robot_middleware_demo::msg::PerceptionResult::UniquePtr
PerceptionNode::process_imu(sensor_msgs::msg::Imu::ConstSharedPtr msg) {
    auto result = std::make_unique<
        service_robot_middleware_demo::msg::PerceptionResult>();

    result->header.stamp = this->now();
    result->header.frame_id = msg->header.frame_id;

    auto process_start = this->now();

    size_t object_count = 3;
    for (size_t i = 0; i < object_count; ++i) {
        service_robot_middleware_demo::msg::ObjectInfo obj;
        obj.label = "imu_object_" + std::to_string(i);
        obj.confidence = 0.8;
        obj.track_id = static_cast<uint32_t>(i);
        obj.center.x = i * 10.0;
        obj.center.y = 0.0;
        obj.center.z = 0.0;
        obj.size.x = 0.5;
        obj.size.y = 0.3;
        obj.size.z = 0.2;
        result->objects.push_back(std::move(obj));
    }

    result->scene_label = "imu_scene";
    result->scene_confidence = 0.9;

    auto process_end = this->now();
    result->processing_latency_ms =
        (process_end - process_start).nanoseconds() / 1'000'000.0;

    return result;
}


