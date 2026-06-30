#pragma once

#include <stdint.h>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double x;  /* 或 float — 看你对精度的需求 */
    double y;
    double z;
} Vector3;

/* 等价于 geometry_msgs/msg/Twist */
typedef struct {
    Vector3 linear;   /* 线速度 m/s */
    Vector3 angular;  /* 角速度 rad/s */
} Twist;

typedef struct {
    uint64_t timestamp_ns;
    Vector3 position;
    Twist velocity;
} CmdVelocity;

int rca_chassis_set_speed(const geometry_msgs::msg::Twist::ConstSharedPtr& msg);
bool rca_chassis_get_speed(const geometry_msgs::msg::Twist::MutableSharedPtr& msg);

#ifdef __cplusplus
}
#endif
