#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include "service_robot_middleware_demo/msg/hal_image.hpp"

class CameraGetDataNode : public rclcpp::Node {

public:
    CameraGetDataNode(const rclcpp::NodeOptions& options);
    ~CameraGetDataNode();
    void start();
    void stop();

private:
    rclcpp::Publisher<service_robot_middleware_demo::msg::HalImage>::SharedPtr image_pub_;
};
