#include "navigation_node.hpp"
#include <cmath>

using namespace std::chrono_literals;

NavigationNode::NavigationNode(const rclcpp::NodeOptions& options)
    : Node("navigation_node", options),
      target_distance_(10.0),
      target_turn_angle_(M_PI / 2.0),
      speed_(0.5),
      angular_speed_(0.5),
      state_(State::FORWARD),
      current_distance_(0.0),
      current_turn_angle_(0.0),
      last_x_(0.0),
      last_y_(0.0),
      last_theta_(0.0),
      start_x_(0.0),
      start_y_(0.0) {
    this->declare_parameter("cmd_vel_topic", "/cmd_vel");
    this->declare_parameter("odom_topic", "/odom");

    std::string cmd_vel_topic = this->get_parameter("cmd_vel_topic").as_string();
    std::string odom_topic = this->get_parameter("odom_topic").as_string();

    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(cmd_vel_topic, 10);

    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
        odom_topic, 10, std::bind(&NavigationNode::odom_callback, this, std::placeholders::_1));

    control_timer_ = this->create_wall_timer(
        100ms, std::bind(&NavigationNode::control_loop, this));

    RCLCPP_INFO(this->get_logger(), "NavigationNode started");
    RCLCPP_INFO(this->get_logger(), "  Publishing cmd_vel to: %s", cmd_vel_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "  Subscribing to odom: %s", odom_topic.c_str());
}

void NavigationNode::odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
    float x = msg->pose.pose.position.x;
    float y = msg->pose.pose.position.y;
    float q_w = msg->pose.pose.orientation.w;
    float q_z = msg->pose.pose.orientation.z;
    float theta = 2.0f * atan2(q_z, q_w);

    RCLCPP_INFO(this->get_logger(), 
        "Navigation received odom: x=%.2f, y=%.2f, theta=%.2f, linear_vel=%.2f, angular_vel=%.2f",
        x, y, theta, 
        msg->twist.twist.linear.x, 
        msg->twist.twist.angular.z);

    switch (state_) {
        case State::FORWARD: {
            float dx = x - start_x_;
            float dy = y - start_y_;
            current_distance_ = sqrt(dx * dx + dy * dy);
            break;
        }
        case State::TURNING: {
            float dtheta = theta - last_theta_;
            while (dtheta < -M_PI) dtheta += 2 * M_PI;
            while (dtheta > M_PI) dtheta -= 2 * M_PI;
            current_turn_angle_ += dtheta;
            break;
        }
        default:
            break;
    }

    last_x_ = x;
    last_y_ = y;
    last_theta_ = theta;
}

void NavigationNode::control_loop() {
    geometry_msgs::msg::Twist cmd_vel;
    cmd_vel.linear.x = 0.0;
    cmd_vel.angular.z = 0.0;

    switch (state_) {
        case State::FORWARD: {
            if (current_distance_ < target_distance_) {
                cmd_vel.linear.x = speed_;
            } else {
                cmd_vel.linear.x = 0.0;
                state_ = State::TURNING;
                current_turn_angle_ = 0.0;
                RCLCPP_INFO(this->get_logger(), "Reached target distance: %.2fm, starting turn", current_distance_);
            }
            break;
        }
        case State::TURNING: {
            float target_deg = target_turn_angle_ * 180 / M_PI;
            float current_deg = current_turn_angle_ * 180 / M_PI;
            if (current_deg < target_deg) {
                cmd_vel.angular.z = angular_speed_;
            } else {
                cmd_vel.angular.z = 0.0;
                state_ = State::FORWARD;
                start_x_ = last_x_;
                start_y_ = last_y_;
                current_distance_ = 0.0;
                RCLCPP_INFO(this->get_logger(), "Turn completed: %.2f degrees, starting forward", current_deg);
            }
            break;
        }
        default:
            break;
    }

    cmd_vel_pub_->publish(cmd_vel);
    RCLCPP_INFO(this->get_logger(), 
        "Navigation publishing cmd_vel: linear_x=%.2f, angular_z=%.2f",
        cmd_vel.linear.x, cmd_vel.angular.z);
}
