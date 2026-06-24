#include "srm/hal/dummy_camera.h"
#include "srm/common/logging.h"
#include <chrono>
#include <cstring>

namespace srm {
namespace hal {

static const char* TAG = "DummyCamera";

DummyCamera::DummyCamera(uint32_t width, uint32_t height, uint32_t fps)
    : width_(width), height_(height), fps_(fps), frame_count_(0) {
    frame_interval_ns_ = 1000000000ULL / fps_;
}

DummyCamera::~DummyCamera() {
    close();
}

bool DummyCamera::open() {
    SRM_LOG_INFO(TAG, "Dummy camera opened: " +
              std::to_string(width_) + "x" + std::to_string(height_) +
              "@" + std::to_string(fps_) + "fps");
    return true;
}

void DummyCamera::close() {
    stop_streaming();
    SRM_LOG_INFO(TAG, "Dummy camera closed");
}

void DummyCamera::set_callback(CameraCallback callback) {
    callback_ = std::move(callback);
}

bool DummyCamera::start_streaming() {
    if (streaming_) {
        SRM_LOG_WARN(TAG, "Already streaming");
        return false;
    }
    streaming_ = true;
    frame_count_ = 0;
    last_frame_ns_ = 0;
    SRM_LOG_INFO(TAG, "Start streaming");
    return true;
}

void DummyCamera::stop_streaming() {
    if (!streaming_) return;
    streaming_ = false;
    SRM_LOG_INFO(TAG, "Stop streaming, captured " + std::to_string(frame_count_) + " frames");
}

ImageMessage DummyCamera::generate_frame() {
    ImageMessage img;
    img.timestamp = now_ns();
    img.width = width_;
    img.height = height_;
    img.channels = 3;
    img.data.resize(width_ * height_ * 3);

    uint8_t pattern_r = static_cast<uint8_t>(frame_count_ % 256);
    uint8_t pattern_g = static_cast<uint8_t>((frame_count_ * 2) % 256);
    uint8_t pattern_b = static_cast<uint8_t>((frame_count_ * 3) % 256);

    for (size_t i = 0; i < img.data.size(); i += 3) {
        size_t pixel_idx = i / 3;
        uint32_t x = static_cast<uint32_t>(pixel_idx % width_);
        uint32_t y = static_cast<uint32_t>(pixel_idx / width_);

        uint8_t gradient = static_cast<uint8_t>((x + y + frame_count_ * 10) % 256);

        img.data[i]     = static_cast<uint8_t>((pattern_r + gradient) / 2);
        img.data[i + 1] = static_cast<uint8_t>((pattern_g + gradient) / 2);
        img.data[i + 2] = static_cast<uint8_t>((pattern_b + gradient) / 2);
    }

    frame_count_++;
    return img;
}

bool DummyCamera::read_frame() {
    if (!streaming_) return false;

    auto now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    if (last_frame_ns_ == 0 || now - last_frame_ns_ >= frame_interval_ns_) {
        auto img = generate_frame();
        last_frame_ns_ = img.timestamp;
        if (callback_) {
            callback_(img);
        }
        return true;
    }
    return false;
}

}
}
