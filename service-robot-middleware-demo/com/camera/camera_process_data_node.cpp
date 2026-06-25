#include "camera_process_data_node.hpp"

CameraProcessDataNode::CameraProcessDataNode(const rclcpp::NodeOptions& options)
    : Node("camera_process_data_node", options)
{
    rclcpp::SensorDataQoS qos;

    image_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
        "/camera/front/image_raw", qos);

    hal_raw_sub_ = this->create_subscription<service_robot_middleware_demo::msg::HalImage>(
        "/camera/front/hal_image", qos,
        [this](const service_robot_middleware_demo::msg::HalImage::SharedPtr hal_msg) {
            on_hal_raw_received(hal_msg);
        });

    RCLCPP_INFO(this->get_logger(), "CameraProcessDataNode created");
}

CameraProcessDataNode::~CameraProcessDataNode() {}

void CameraProcessDataNode::on_hal_raw_received(const service_robot_middleware_demo::msg::HalImage::SharedPtr hal_msg)
{
    auto msg = std::make_unique<sensor_msgs::msg::Image>();
    msg->header = hal_msg->header;
    msg->header.frame_id = hal_msg->header.frame_id;
    msg->header.seq = hal_msg->header.seq;
    msg->width = hal_msg->width;
    msg->height = hal_msg->height;
    if (hal_msg->channels == 1) {
        msg->encoding = "mono8";
    } else if (hal_msg->channels == 3) {
        msg->encoding = "rgb8";
    } else if (hal_msg->channels == 4) {
        msg->encoding = "rgba8";
    }
    msg->is_bigendian = false;
    msg->step = hal_msg->step;
    msg->data = std::move(hal_msg->data);
    image_pub_->publish(std::move(msg));
}
