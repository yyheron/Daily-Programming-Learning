#include "chassis_hal_if.hpp"

ChassisHalIf::ChassisHalIf(const rclcpp::NodeOptions& options)
    : Node("chassis_hal_if", options)
{
    rclcpp::SensorDataQoS qos;

    hal_cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "/chassis/cmd_vel", qos,
        [this](const geometry_msgs::msg::Twist::ConstSharedPtr msg) {
            on_hal_cmd_vel_received(msg);
        });

    update_timer_ = this->create_wall_timer(
        std::chrono::milliseconds(100),
        std::bind(&ChassisHalIf::update_timer_callback, this));

    RCLCPP_INFO(this->get_logger(), "ChassisHalIf created");
}

void ChassisHalIf::start()
{
    RCLCPP_INFO(this->get_logger(), "ChassisHalIf started");
}

void ChassisHalIf::stop()
{
    RCLCPP_INFO(this->get_logger(), "ChassisHalIf stopped");
}

ChassisHalIf::~ChassisHalIf()
{
    stop();
}

void ChassisHalIf::on_hal_cmd_vel_received(const geometry_msgs::msg::Twist::ConstSharedPtr msg)
{
    
    RCLCPP_INFO(this->get_logger(), 
        "ChassisHalIf received hal_cmd_vel: linear_x=%.2f, linear_y=%.2f, linear_z=%.2f, angular_x=%.2f, angular_y=%.2f, angular_z=%.2f",
        msg->linear.x, msg->linear.y, msg->linear.z, msg->angular.x, msg->angular.y, msg->angular.z);
        
    rca_chassis_set_speed(msg);
}

void ChassisHalIf::update_timer_callback()
{
    while (true)
    {
        if (!rca_chassis_get_speed(msg)) continue;
        RCLCPP_INFO(this->get_logger(), 
        "ChassisHalIf get_speed: position_x=%.2f, position_y=%.2f, position_z=%.2f, linear_x=%.2f, linear_y=%.2f, linear_z=%.2f, angular_x=%.2f, angular_y=%.2f, angular_z=%.2f",
        msg->position.x, msg->position.y, msg->position.z, msg->velocity.linear.x, msg->velocity.linear.y, msg->velocity.linear.z, msg->velocity.angular.x, msg->velocity.angular.y, msg->velocity.angular.z);
    }
    
}
