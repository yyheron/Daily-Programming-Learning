#pragma once

#include "srm/hal/camera_interface.h"
#include <string>
#include <cstdint>

namespace srm {
namespace hal {

class DummyCamera : public ICameraDevice {
public:
    DummyCamera(uint32_t width = 640, uint32_t height = 480, uint32_t fps = 30);
    ~DummyCamera() override;

    bool open() override;
    void close() override;

    void set_callback(CameraCallback callback) override;

    bool start_streaming() override;
    void stop_streaming() override;

    uint32_t get_width() const override { return width_; }
    uint32_t get_height() const override { return height_; }
    uint32_t get_fps() const override { return fps_; }

    bool read_frame();

    uint32_t frame_count() const { return frame_count_; }

private:
    ImageMessage generate_frame();

    uint32_t width_;
    uint32_t height_;
    uint32_t fps_;
    uint32_t frame_count_;

    CameraCallback callback_;
    bool streaming_ = false;
    uint64_t last_frame_ns_ = 0;
    uint64_t frame_interval_ns_ = 0;
};

}
}
