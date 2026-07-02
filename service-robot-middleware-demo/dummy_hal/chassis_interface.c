#include "chassis_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

static CmdVelocity g_cmd_vel;
static bool g_data_updated = false;

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}


int rca_chassis_set_speed(const geometry_msgs::msg::Twist::ConstSharedPtr& msg) {
    
    g_cmd_vel.timestamp_ns = msg->header.stamp.nanosec;
    g_cmd_vel.position.x = msg->linear.x;
    g_cmd_vel.position.y = msg->linear.y;
    g_cmd_vel.position.z = msg->linear.z;
    g_cmd_vel.velocity.angular.x = msg->angular.x;
    g_cmd_vel.velocity.angular.y = msg->angular.y;
    g_cmd_vel.velocity.angular.z = msg->angular.z;
    g_data_updated = true;
    
    // printf("[HAL] Chassis set_speed: position_x=%.2f, position_y=%.2f, position_z=%.2f, angular_x=%.2f, angular_y=%.2f, angular_z=%.2f\n", 
    //        g_cmd_vel.position.x, g_cmd_vel.position.y, g_cmd_vel.position.z, g_cmd_vel.velocity.angular.x, g_cmd_vel.velocity.angular.y, g_cmd_vel.velocity.angular.z);
    return 0;
}

bool rca_chassis_get_speed(const geometry_msgs::msg::Twist::MutableSharedPtr& msg) {
    if (!g_data_updated) return false;
    
    msg->header.stamp.nanosec = g_cmd_vel.timestamp_ns;
    msg->position.x = g_cmd_vel.position.x;
    msg->position.y = g_cmd_vel.position.y;
    msg->position.z = g_cmd_vel.position.z;
    msg->velocity.linear.x = g_cmd_vel.velocity.linear.x;
    msg->velocity.linear.y = g_cmd_vel.velocity.linear.y;
    msg->velocity.angular.z = g_cmd_vel.velocity.angular.z;
    g_data_updated = false;
    
    // printf("[HAL] Chassis get_speed: position_x=%.2f, position_y=%.2f, position_z=%.2f, linear_x=%.2f, linear_y=%.2f, linear_z=%.2f, angular_x=%.2f, angular_y=%.2f, angular_z=%.2f\n",
    //        msg->position.x, msg->position.y, msg->position.z, msg->velocity.linear.x, msg->velocity.linear.y, msg->velocity.linear.z, msg->velocity.angular.x, msg->velocity.angular.z);
    
    return true;
}
