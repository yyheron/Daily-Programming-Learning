#pragma once

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include "service_robot_middleware_demo/msg/hal_chassis_state.hpp"

class ChassisProcess : public rclcpp::Node {
public:
    ChassisProcess(const rclcpp::NodeOptions& options);
    ~ChassisProcess();

private:
    void on_cmd_vel_received(const geometry_msgs::msg::Twist::SharedPtr twist);

    rclcpp::Publisher<service_robot_middleware_demo::msg::HalChassisState>::SharedPtr hal_state_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
};
