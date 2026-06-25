#include "chassis_get_data_node.hpp"

#include "dummy_hal/chassis_interface.h"

static ChassisGetDataNode* g_chassis_get_data_node = nullptr;

static void OnChassisStateReceived(const hal::ChassisState* pState)
{
    if (!g_chassis_get_data_node) return;

    auto hal_msg = std::make_unique<service_robot_middleware_demo::msg::HalChassisState>();
    hal_msg->header.stamp = rclcpp::Time(pState->timestamp_ns);
    hal_msg->header.frame_id = "base_link";
    hal_msg->x = pState->x;
    hal_msg->y = pState->y;
    hal_msg->theta = pState->theta;
    hal_msg->linear_velocity = pState->linear_velocity;
    hal_msg->angular_velocity = pState->angular_velocity;

    g_chassis_get_data_node->hal_state_pub_->publish(std::move(hal_msg));
}

ChassisGetDataNode::ChassisGetDataNode(const rclcpp::NodeOptions& options)
    : Node("chassis_get_data_node", options)
{
    rclcpp::SensorDataQoS qos;

    hal_state_pub_ = this->create_publisher<service_robot_middleware_demo::msg::HalChassisState>(
        "/chassis/hal_state", qos);

    cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "/cmd_vel", qos,
        [this](const geometry_msgs::msg::Twist::SharedPtr twist) {
            on_cmd_vel_received(twist);
        });

    RCLCPP_INFO(this->get_logger(), "ChassisGetDataNode created");

    rca_chassis_init();
}

void ChassisGetDataNode::start()
{
    g_chassis_get_data_node = this;
    rca_chassis_start(OnChassisStateReceived, nullptr);
}

void ChassisGetDataNode::stop()
{
    rca_chassis_stop();
    g_chassis_get_data_node = nullptr;
}

ChassisGetDataNode::~ChassisGetDataNode()
{
    rca_chassis_stop();
    rca_chassis_exit();
    g_chassis_get_data_node = nullptr;
}

void ChassisGetDataNode::on_cmd_vel_received(const geometry_msgs::msg::Twist::SharedPtr twist)
{
    hal::TwistMessage hal_twist;
    hal_twist.linear_x = twist->linear.x;
    hal_twist.linear_y = twist->linear.y;
    hal_twist.linear_z = twist->linear.z;
    hal_twist.angular_x = twist->angular.x;
    hal_twist.angular_y = twist->angular.y;
    hal_twist.angular_z = twist->angular.z;
    rca_chassis_set_speed(&hal_twist);
}
