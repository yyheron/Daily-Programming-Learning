#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include "service_robot_middleware_demo/msg/hal_image.hpp"

class CameraHalIfNode : public rclcpp::Node {

public:
    CameraHalIfNode(const rclcpp::NodeOptions& options);
    ~CameraHalIfNode();
    void start();
    void stop();

private:
    rclcpp::Publisher<service_robot_middleware_demo::msg::HalImage>::SharedPtr image_pub_;
};
