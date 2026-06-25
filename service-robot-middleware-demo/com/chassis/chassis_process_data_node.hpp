#pragma once

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include "service_robot_middleware_demo/msg/hal_chassis_state.hpp"

class ChassisProcessDataNode : public rclcpp::Node {
public:
    ChassisProcessDataNode(const rclcpp::NodeOptions& options);
    ~ChassisProcessDataNode();

private:
    void on_hal_state_received(const service_robot_middleware_demo::msg::HalChassisState::SharedPtr hal_msg);

    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Subscription<service_robot_middleware_demo::msg::HalChassisState>::SharedPtr hal_state_sub_;
};
