#include "imu_interface.h"
#include "chassis_interface.h"
#include <atomic>


namespace hal {

static void chassis_imu_callback(const ImuData* imu) {
    if (g_imu_callback && imu) {
        ImuMessage msg;
        msg.timestamp_ns = imu->timestamp_ns;
        msg.linear_acceleration[0] = imu->linear_acceleration[0];
        msg.linear_acceleration[1] = imu->linear_acceleration[1];
        msg.linear_acceleration[2] = imu->linear_acceleration[2];
        msg.angular_velocity[0] = imu->angular_velocity[0];
        msg.angular_velocity[1] = imu->angular_velocity[1];
        msg.angular_velocity[2] = imu->angular_velocity[2];
        msg.orientation[0] = imu->orientation[0];
        msg.orientation[1] = imu->orientation[1];
        msg.orientation[2] = imu->orientation[2];
        msg.orientation[3] = imu->orientation[3];
        g_imu_callback(&msg);
    }
}

extern "C" {

int rca_imu_init() {
    return rca_chassis_init();
}

int rca_imu_start_read_data(ImuMessage* msg)
{
    g_imu_callback = [](const ImuMessage* pData) {
        *msg = *pData;
        return 0;
    };
    return rca_chassis_start(nullptr, chassis_imu_callback);
}

int rca_imu_exit() {
    g_imu_callback = nullptr;
    return rca_chassis_stop();
}

}

} // namespace hal

