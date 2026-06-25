#include "imu_get_data_node.hpp"

#include "dummy_hal/imu_interface.h"

static ImuGetDataNode* g_imu_get_data_node = nullptr;

static void OnImuDataReceived(const hal::ImuMessage* pData)
{
    if (!g_imu_get_data_node) return;

    auto hal_msg = std::make_unique<service_robot_middleware_demo::msg::HalImu>();
    hal_msg->header.stamp = rclcpp::Time(pData->timestamp_ns);
    hal_msg->header.frame_id = "imu_front";

    for (int i = 0; i < 3; ++i) {
        hal_msg->linear_acceleration[i] = pData->linear_acceleration[i];
        hal_msg->angular_velocity[i] = pData->angular_velocity[i];
    }
    for (int i = 0; i < 4; ++i) {
        hal_msg->orientation[i] = pData->orientation[i];
    }

    g_imu_get_data_node->hal_imu_pub_->publish(std::move(hal_msg));
}

ImuGetDataNode::ImuGetDataNode(const rclcpp::NodeOptions& options)
    : Node("imu_get_data_node", options)
{
    rclcpp::SensorDataQoS qos;

    hal_imu_pub_ = this->create_publisher<service_robot_middleware_demo::msg::HalImu>(
        "/imu/front/hal_imu", qos);

    RCLCPP_INFO(this->get_logger(), "ImuGetDataNode created");

    rca_imu_init();
}

void ImuGetDataNode::start()
{
    g_imu_get_data_node = this;
    rca_imu_start_read_data(ImuMessage* msg);
}

void ImuGetDataNode::stop()
{
    rca_imu_exit();
    g_imu_get_data_node = nullptr;
}

ImuGetDataNode::~ImuGetDataNode()
{
    rca_imu_exit();
    g_imu_get_data_node = nullptr;
}
