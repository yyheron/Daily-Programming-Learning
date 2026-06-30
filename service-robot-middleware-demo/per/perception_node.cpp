#include "perception_node.hpp"

PerceptionNode::PerceptionNode(const rclcpp::NodeOptions& options)
    : Node("perception_node", options) {
    this->declare_parameter("input_topic", "/camera/front/image_raw");

    std::string input_topic = this->get_parameter("input_topic").as_string();

    rclcpp::SensorDataQoS qos;

    image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        input_topic, qos,
        [this](sensor_msgs::msg::Image::ConstSharedPtr msg) {
            on_image_received(msg);
        });

    RCLCPP_INFO(this->get_logger(), "PerceptionNode started, subscribing to: %s", input_topic.c_str());
}

void PerceptionNode::on_image_received(sensor_msgs::msg::Image::ConstSharedPtr msg) {
    RCLCPP_INFO(this->get_logger(), 
        "Perception received image: width=%d, height=%d, encoding=%s, seq=%d",
        msg->width, msg->height, msg->encoding.c_str(), msg->header.seq);
}
