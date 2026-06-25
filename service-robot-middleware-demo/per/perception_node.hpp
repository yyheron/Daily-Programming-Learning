#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include "service_robot_middleware_demo/msg/perception_result.hpp"



class PerceptionNode : public rclcpp::Node {
public:
    explicit PerceptionNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
    void on_image_received(sensor_msgs::msg::Image::ConstSharedPtr msg);
    void on_imu_received(sensor_msgs::msg::Imu::ConstSharedPtr msg);

    service_robot_middleware_demo::msg::PerceptionResult::UniquePtr
    process_image(sensor_msgs::msg::Image::ConstSharedPtr msg);

    service_robot_middleware_demo::msg::PerceptionResult::UniquePtr
    process_imu(sensor_msgs::msg::Imu::ConstSharedPtr msg);

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Publisher<service_robot_middleware_demo::msg::PerceptionResult>::SharedPtr result_pub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;

    rclcpp::Time last_process_time_{0, 0, RCL_ROS_TIME};
    int process_interval_ms_ = 33;
};


