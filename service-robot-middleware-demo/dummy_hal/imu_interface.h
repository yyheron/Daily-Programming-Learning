#pragma once

#include <functional>
#include <string>
#include <cstdint>
#include <vector>
#include <chrono>

namespace hal {

struct ImuMessage {
    uint64_t timestamp_ns;
    float linear_acceleration[3];
    float angular_velocity[3];
    float orientation[4];
};

} // namespace hal

extern "C" {

int rca_imu_init()
{

}

int rca_imu_read_data(ImuMessage* msg)
{
    msg->timestamp_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    float t = static_cast<float>(msg->timestamp_ns) / 1e9f;
    msg->linear_acceleration[0] = 0.1f * sin(t);
    msg->linear_acceleration[1] = 0.05f * cos(t);
    msg->linear_acceleration[2] = 9.81f + 0.02f * sin(2 * t);

    msg->angular_velocity[0] = 0.01f * sin(3 * t);
    msg->angular_velocity[1] = 0.02f * cos(2 * t);
    msg->angular_velocity[2] = 0.015f * sin(t);

    msg->orientation[0] = 1.0f;
    msg->orientation[1] = 0.0f;
    msg->orientation[2] = 0.0f;
    msg->orientation[3] = 0.0f;

    return 0;
}

int rca_imu_exit()
{

}

}