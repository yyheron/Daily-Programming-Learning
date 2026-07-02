#include "chassis_process.hpp"
#include <cmath>

ChassisProcess::ChassisProcess(const rclcpp::NodeOptions& options)
    : Node("chassis_process", options)
{
    rclcpp::SensorDataQoS qos;

    hal_state_pub_ = this->create_publisher<service_robot_middleware_demo::msg::HalChassisState>(
        "/chassis/hal_state", qos);

    cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "/cmd_vel", qos,
        [this](const geometry_msgs::msg::Twist::SharedPtr twist) {
            on_cmd_vel_received(twist);
        });

    RCLCPP_INFO(this->get_logger(), "ChassisProcess created");
}

ChassisProcess::~ChassisProcess() {}

void ChassisProcess::on_cmd_vel_received(const geometry_msgs::msg::Twist::SharedPtr twist)
{
    RCLCPP_INFO(this->get_logger(), 
        "ChassisProcess received cmd_vel: linear_x=%.2f, angular_z=%.2f",
        twist->linear.x, twist->angular.z);

    auto hal_msg = std::make_unique<service_robot_middleware_demo::msg::HalChassisState>();
    hal_msg->header.stamp = this->now();
    hal_msg->header.frame_id = "base_link";
    
    hal_msg->x = 0.0;
    hal_msg->y = 0.0;
    hal_msg->theta = 0.0;
    hal_msg->linear_velocity = twist->linear.x;
    hal_msg->angular_velocity = twist->angular.z;

    hal_state_pub_->publish(std::move(hal_msg));
    
    RCLCPP_INFO(this->get_logger(), "ChassisProcess published hal_state");
}
