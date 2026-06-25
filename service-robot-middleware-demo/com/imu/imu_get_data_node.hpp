#pragma once

#include <rclcpp/rclcpp.hpp>
#include "service_robot_middleware_demo/msg/hal_imu.hpp"

class ImuGetDataNode : public rclcpp::Node {
public:
    ImuGetDataNode(const rclcpp::NodeOptions& options);
    ~ImuGetDataNode();
    void start();
    void stop();

private:
    rclcpp::Publisher<service_robot_middleware_demo::msg::HalImu>::SharedPtr hal_imu_pub_;
};
