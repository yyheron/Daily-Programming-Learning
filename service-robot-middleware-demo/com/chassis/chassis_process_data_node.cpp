#include "chassis_process_data_node.hpp"

#include <cmath>

ChassisProcessDataNode::ChassisProcessDataNode(const rclcpp::NodeOptions& options)
    : Node("chassis_process_data_node", options)
{
    rclcpp::SensorDataQoS qos;

    odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>(
        "/odom", qos);

    hal_state_sub_ = this->create_subscription<service_robot_middleware_demo::msg::HalChassisState>(
        "/chassis/hal_state", qos,
        [this](const service_robot_middleware_demo::msg::HalChassisState::SharedPtr hal_msg) {
            on_hal_state_received(hal_msg);
        });

    RCLCPP_INFO(this->get_logger(), "ChassisProcessDataNode created");
}

ChassisProcessDataNode::~ChassisProcessDataNode() {}

void ChassisProcessDataNode::on_hal_state_received(const service_robot_middleware_demo::msg::HalChassisState::SharedPtr hal_msg)
{
    auto msg = std::make_unique<nav_msgs::msg::Odometry>();
    msg->header = hal_msg->header;
    msg->header.frame_id = "odom";
    msg->child_frame_id = "base_link";

    msg->pose.pose.position.x = hal_msg->x;
    msg->pose.pose.position.y = hal_msg->y;
    msg->pose.pose.position.z = 0.0;

    double half_theta = hal_msg->theta / 2.0;
    msg->pose.pose.orientation.w = cos(half_theta);
    msg->pose.pose.orientation.x = 0.0;
    msg->pose.pose.orientation.y = 0.0;
    msg->pose.pose.orientation.z = sin(half_theta);

    msg->twist.twist.linear.x = hal_msg->linear_velocity;
    msg->twist.twist.linear.y = 0.0;
    msg->twist.twist.linear.z = 0.0;
    msg->twist.twist.angular.x = 0.0;
    msg->twist.twist.angular.y = 0.0;
    msg->twist.twist.angular.z = hal_msg->angular_velocity;

    odom_pub_->publish(std::move(msg));
}
