#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

class PerceptionNode : public rclcpp::Node {
public:
    PerceptionNode(const rclcpp::NodeOptions& options);

private:
    void on_image_received(sensor_msgs::msg::Image::ConstSharedPtr msg);

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
};
