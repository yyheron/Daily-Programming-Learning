#include "chassis_interface.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>

namespace hal {

class DummyChassis : public IChassisDevice {
public:
    DummyChassis() : 
        x_(0.0f), y_(0.0f), theta_(0.0f),
        linear_vel_(0.0f), angular_vel_(0.0f),
        rate_(50) {
        interval_ns_ = 1000000000ULL / rate_;
    }
    
    ~DummyChassis() override { close(); }

    bool open() override { return true; }
    
    void close() override { stop(); }

    bool set_speed(const TwistMessage& twist) override {
        linear_vel_ = twist.linear_x;
        angular_vel_ = twist.angular_z;
        return true;
    }

    void set_state_callback(ChassisStateCallback callback) override {
        state_callback_ = callback;
    }

    void set_imu_callback(ImuDataCallback callback) override {
        imu_callback_ = callback;
    }

    bool start() override {
        if (running_.exchange(true)) return false;
        update_thread_ = std::thread(&DummyChassis::update_loop, this);
        return true;
    }

    void stop() override {
        if (!running_.exchange(false)) return;
        if (update_thread_.joinable()) update_thread_.join();
    }

    void reset_position(float x, float y, float theta) override {
        x_ = x;
        y_ = y;
        theta_ = theta;
    }

    ChassisState get_state() const override {
        ChassisState state;
        state.timestamp_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        state.x = x_;
        state.y = y_;
        state.theta = theta_;
        state.linear_velocity = linear_vel_;
        state.angular_velocity = angular_vel_;
        return state;
    }

private:
    void update_loop() {
        uint64_t last_ns = 0;
        while (running_) {
            auto now = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());

            if (last_ns == 0 || now - last_ns >= interval_ns_) {
                float dt = static_cast<float>(now - last_ns) / 1e9f;
                if (dt > 0.02f) dt = 0.02f;
                last_ns = now;

                update_position(dt);
                ChassisState state = get_state();
                if (state_callback_) state_callback_(&state);

                ImuData imu = generate_imu(dt);
                if (imu_callback_) imu_callback_(&imu);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    void update_position(float dt) {
        theta_ += angular_vel_ * dt;
        x_ += linear_vel_ * cos(theta_) * dt;
        y_ += linear_vel_ * sin(theta_) * dt;
    }

    ImuData generate_imu(float dt) {
        ImuData imu;
        imu.timestamp_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        float acc_x = (linear_vel_ - last_linear_vel_) / dt;
        float acc_z = angular_vel_ * linear_vel_;

        imu.linear_acceleration[0] = acc_x * cos(theta_) - acc_z * sin(theta_);
        imu.linear_acceleration[1] = acc_x * sin(theta_) + acc_z * cos(theta_);
        imu.linear_acceleration[2] = 9.81f;

        imu.angular_velocity[0] = 0.0f;
        imu.angular_velocity[1] = 0.0f;
        imu.angular_velocity[2] = angular_vel_;

        float half_theta = theta_ / 2.0f;
        imu.orientation[0] = cos(half_theta);
        imu.orientation[1] = 0.0f;
        imu.orientation[2] = 0.0f;
        imu.orientation[3] = sin(half_theta);

        last_linear_vel_ = linear_vel_;
        last_angular_vel_ = angular_vel_;

        return imu;
    }

    float x_;
    float y_;
    float theta_;
    float linear_vel_;
    float angular_vel_;
    float last_linear_vel_ = 0.0f;
    float last_angular_vel_ = 0.0f;

    uint32_t rate_;
    uint64_t interval_ns_;
    ChassisStateCallback state_callback_ = nullptr;
    ImuDataCallback imu_callback_ = nullptr;
    std::atomic<bool> running_{false};
    std::thread update_thread_;
};

DummyChassis* g_dummy_chassis = nullptr;

extern "C" {

int rca_chassis_init() {
    if (!g_dummy_chassis) {
        g_dummy_chassis = new DummyChassis();
        if (g_dummy_chassis) {
            g_dummy_chassis->open();
            return 0;
        }
    }
    return -1;
}

int rca_chassis_set_speed(const TwistMessage* twist) {
    if (g_dummy_chassis && twist) {
        g_dummy_chassis->set_speed(*twist);
        return 0;
    }
    return -1;
}

int rca_chassis_start(ChassisStateCallback state_callback, ImuDataCallback imu_callback) {
    if (g_dummy_chassis) {
        g_dummy_chassis->set_state_callback(state_callback);
        g_dummy_chassis->set_imu_callback(imu_callback);
        g_dummy_chassis->start();
        return 0;
    }
    return -1;
}

int rca_chassis_stop() {
    if (g_dummy_chassis) {
        g_dummy_chassis->stop();
        return 0;
    }
    return -1;
}

int rca_chassis_exit() {
    if (g_dummy_chassis) {
        delete g_dummy_chassis;
        g_dummy_chassis = nullptr;
        return 0;
    }
    return -1;
}

}

} // namespace hal