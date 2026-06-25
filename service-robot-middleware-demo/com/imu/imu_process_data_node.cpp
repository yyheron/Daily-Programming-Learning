#include "imu_process_data_node.hpp"

ImuProcessDataNode::ImuProcessDataNode(const rclcpp::NodeOptions& options)
    : Node("imu_process_data_node", options)
{
    rclcpp::SensorDataQoS qos;

    imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>(
        "/imu/data", qos);

    hal_imu_sub_ = this->create_subscription<service_robot_middleware_demo::msg::HalImu>(
        "/imu/front/hal_imu", qos,
        [this](const service_robot_middleware_demo::msg::HalImu::SharedPtr hal_msg) {
            on_hal_imu_received(hal_msg);
        });

    RCLCPP_INFO(this->get_logger(), "ImuProcessDataNode created");
}

ImuProcessDataNode::~ImuProcessDataNode() {}

void ImuProcessDataNode::on_hal_imu_received(const service_robot_middleware_demo::msg::HalImu::SharedPtr hal_msg)
{
    auto msg = std::make_unique<sensor_msgs::msg::Imu>();
    msg->header = hal_msg->header;

    msg->linear_acceleration.x = hal_msg->linear_acceleration[0];
    msg->linear_acceleration.y = hal_msg->linear_acceleration[1];
    msg->linear_acceleration.z = hal_msg->linear_acceleration[2];

    msg->angular_velocity.x = hal_msg->angular_velocity[0];
    msg->angular_velocity.y = hal_msg->angular_velocity[1];
    msg->angular_velocity.z = hal_msg->angular_velocity[2];

    msg->orientation.w = hal_msg->orientation[0];
    msg->orientation.x = hal_msg->orientation[1];
    msg->orientation.y = hal_msg->orientation[2];
    msg->orientation.z = hal_msg->orientation[3];

    imu_pub_->publish(std::move(msg));
}
