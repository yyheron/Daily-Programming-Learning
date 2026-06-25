#include "dummy_camera.h"
#include <chrono>
#include <cstring>

namespace hal {

DummyCamera* g_dummy_camera = nullptr;

DummyCamera::DummyCamera(uint32_t width, uint32_t height, uint32_t fps)
    : width_(width), height_(height), fps_(fps) {
    frame_interval_ns_ = 1000000000ULL / fps_;
}

DummyCamera::~DummyCamera() {
    close();
}

bool DummyCamera::open() {
    return true;
}

void DummyCamera::close() {
    stop_streaming();
}

void DummyCamera::set_callback(CameraCallback callback) {
    callback_ = callback;
}

bool DummyCamera::start_streaming() {
    if (streaming_.exchange(true)) {
        return false;
    }
    frame_count_ = 0;
    stream_thread_ = std::thread(&DummyCamera::streaming_loop, this);
    return true;
}

void DummyCamera::stop_streaming() {
    if (!streaming_.exchange(false)) {
        return;
    }
    if (stream_thread_.joinable()) {
        stream_thread_.join();
    }
}

void DummyCamera::streaming_loop() {
    uint64_t last_frame_ns = 0;
    while (streaming_) {
        auto now = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );

        if (last_frame_ns == 0 || now - last_frame_ns >= frame_interval_ns_) {
            auto img = generate_frame();
            last_frame_ns = img.timestamp_ns;
            if (callback_) {
                callback_(&img);
            }
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

ImageMessage DummyCamera::generate_frame() {
    ImageMessage img;
    img.timestamp_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    img.width = width_;
    img.height = height_;
    img.channels = 3;
    img.data.resize(width_ * height_ * 3);

    uint32_t fc = frame_count_.fetch_add(1);
    uint8_t pattern_r = static_cast<uint8_t>(fc % 256);
    uint8_t pattern_g = static_cast<uint8_t>((fc * 2) % 256);
    uint8_t pattern_b = static_cast<uint8_t>((fc * 3) % 256);

    for (size_t i = 0; i < img.data.size(); i += 3) {
        size_t pixel_idx = i / 3;
        uint32_t x = static_cast<uint32_t>(pixel_idx % width_);
        uint32_t y = static_cast<uint32_t>(pixel_idx / width_);
        uint8_t gradient = static_cast<uint8_t>((x + y + fc * 10) % 256);

        img.data[i]     = static_cast<uint8_t>((pattern_r + gradient) / 2);
        img.data[i + 1] = static_cast<uint8_t>((pattern_g + gradient) / 2);
        img.data[i + 2] = static_cast<uint8_t>((pattern_b + gradient) / 2);
    }

    return img;
}

extern "C" {

int rca_camera_init() {
    if (!g_dummy_camera) {
        g_dummy_camera = new DummyCamera(640, 480, 30);
        if (g_dummy_camera) {
            g_dummy_camera->open();
            return 0;
        }
    }
    return -1;
}

int rca_camera_start_capture(CameraCallback callback) {
    if (g_dummy_camera && callback) {
        g_dummy_camera->set_callback(callback);
        g_dummy_camera->start_streaming();
        return 0;
    }
    return -1;
}

int rca_camera_exit() {
    if (g_dummy_camera) {
        delete g_dummy_camera;
        g_dummy_camera = nullptr;
        return 0;
    }
    return -1;
}

}

} // namespace hal
