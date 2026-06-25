#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include "service_robot_middleware_demo/msg/hal_image.hpp"

class CameraProcessDataNode : public rclcpp::Node {
public:
    CameraProcessDataNode(const rclcpp::NodeOptions& options);
    ~CameraProcessDataNode();

private:
    void on_hal_raw_received(const service_robot_middleware_demo::msg::HalImage::SharedPtr hal_msg);

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
    rclcpp::Subscription<service_robot_middleware_demo::msg::HalImage>::SharedPtr hal_raw_sub_;
};
