#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ImageMessage {
    uint64_t timestamp_ns;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint8_t* data;
    uint32_t data_size;
};

typedef void (*CameraCallback)(const struct ImageMessage* pData);

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint64_t frame_interval_ns;
    
    uint32_t frame_count;
    CameraCallback callback;
    volatile bool streaming;
    void* stream_thread;
    
    struct ImageMessage frame_buffer;
} DummyCamera;

int rca_camera_init();
int rca_camera_start_capture(CameraCallback callback);
int rca_camera_exit();

#ifdef __cplusplus
}
#endif