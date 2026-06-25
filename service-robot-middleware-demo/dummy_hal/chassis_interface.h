#pragma once

#include <cstdint>
#include <vector>

namespace hal {

struct TwistMessage {
    float linear_x;
    float linear_y;
    float linear_z;
    float angular_x;
    float angular_y;
    float angular_z;
};

struct ChassisState {
    uint64_t timestamp_ns;
    float x;
    float y;
    float theta;
    float linear_velocity;
    float angular_velocity;
};

struct ImuData {
    uint64_t timestamp_ns;
    float linear_acceleration[3];
    float angular_velocity[3];
    float orientation[4];
};

typedef void (*ChassisStateCallback)(const ChassisState* pState);
typedef void (*ImuDataCallback)(const ImuData* pData);

class IChassisDevice {
public:
    virtual ~IChassisDevice() = default;

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual bool set_speed(const TwistMessage& twist) = 0;

    virtual void set_state_callback(ChassisStateCallback callback) = 0;
    virtual void set_imu_callback(ImuDataCallback callback) = 0;

    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual void reset_position(float x, float y, float theta) = 0;
    virtual ChassisState get_state() const = 0;
};

extern "C" {
int rca_chassis_init();
int rca_chassis_set_speed(const TwistMessage* twist);
int rca_chassis_start(ChassisStateCallback state_callback, ImuDataCallback imu_callback);
int rca_chassis_stop();
int rca_chassis_exit();
}

} // namespace hal
