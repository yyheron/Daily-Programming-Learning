#pragma once

#include "camera_interface.h"
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>

namespace hal {

class DummyCamera {
public:
    DummyCamera(uint32_t width = 640, uint32_t height = 480, uint32_t fps = 30);
    ~DummyCamera();

    bool open();
    void close();

    void set_callback(CameraCallback callback);

    bool start_streaming();
    void stop_streaming();

private:
    void streaming_loop();
    ImageMessage generate_frame();

    uint32_t width_;
    uint32_t height_;
    uint32_t fps_;
    std::atomic<uint32_t> frame_count_{0};

    CameraCallback callback_ = nullptr;
    std::atomic<bool> streaming_{false};
    std::thread stream_thread_;
    uint64_t frame_interval_ns_ = 0;
};

extern "C" {
extern DummyCamera* g_dummy_camera;

int rca_camera_init();
int rca_camera_start_capture(CameraCallback callback);
int rca_camera_exit();
}

} // namespace hal