#pragma once

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>

class NavigationNode : public rclcpp::Node {
public:
    explicit NavigationNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
    enum class State {
        FORWARD,
        TURNING,
        STOPPED
    };

    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg);
    void control_loop();

    const double target_distance_;
    const double target_turn_angle_;
    const double speed_;
    const double angular_speed_;

    State state_;
    double current_distance_;
    double current_turn_angle_;
    double last_x_;
    double last_y_;
    double last_theta_;
    double start_x_;
    double start_y_;

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    rclcpp::TimerBase::SharedPtr control_timer_;
};
