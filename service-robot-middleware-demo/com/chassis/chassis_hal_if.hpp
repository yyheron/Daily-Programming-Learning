#pragma once

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>
#include "dummy_hal/chassis_interface.h"

class ChassisHalIf : public rclcpp::Node {
public:
    ChassisHalIf(const rclcpp::NodeOptions& options);
    ~ChassisHalIf();
    void start();
    void stop();

private:
    void on_hal_cmd_vel_received(const geometry_msgs::msg::Twist::ConstSharedPtr msg);
    void update_timer_callback();

    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr hal_cmd_vel_sub_;
    rclcpp::TimerBase::SharedPtr update_timer_;
};
