#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include "service_robot_middleware_demo/msg/hal_imu.hpp"

class ImuProcessDataNode : public rclcpp::Node {
public:
    ImuProcessDataNode(const rclcpp::NodeOptions& options);
    ~ImuProcessDataNode();

private:
    void on_hal_imu_received(const service_robot_middleware_demo::msg::HalImu::SharedPtr hal_msg);

    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Subscription<service_robot_middleware_demo::msg::HalImu>::SharedPtr hal_imu_sub_;
};
