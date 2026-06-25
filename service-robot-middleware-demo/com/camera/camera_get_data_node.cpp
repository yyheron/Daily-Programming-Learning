#include "camera_get_data_node.hpp"

#include "dummy_camera.h"

void CameraGetDataNode::OnCameraFrameReceived(const ImageMessage* pData)
{
    static uint32_t seq = 0;
    service_robot_middleware_demo::msg::HalImage image_msg;
    image_msg.header.stamp = rclcpp::Time(pData->timestamp_ns);
    image_msg.header.frame_id = "camera_front";
    image_msg.header.seq = seq++;
    image_msg.channels = pData->channels;
    image_msg.width = pData->width;
    image_msg.height = pData->height;
    image_msg.step = pData->width * pData->channels;
    image_msg.data = std::move(pData->data);
    image_pub_->publish(image_msg);
}

CameraGetDataNode::CameraGetDataNode(const rclcpp::NodeOptions& options)
    : Node("camera_get_data_node", options)
{
    rclcpp::SensorDataQoS qos;

    image_pub_ = this->create_publisher<service_robot_middleware_demo::msg::HalImage>("/camera/front/hal_image", qos);

    RCLCPP_INFO(this->get_logger(), "CameraGetDataNode created");

}

void CameraGetDataNode::start()
{
    rca_camera_init();
    rca_camera_start_capture(OnCameraFrameReceived);
}

void CameraGetDataNode::stop()
{
    rca_camera_exit();
}

CameraGetDataNode::~CameraGetDataNode()
{
    rca_camera_exit();
}

