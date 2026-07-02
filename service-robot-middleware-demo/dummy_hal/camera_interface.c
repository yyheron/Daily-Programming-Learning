#include "camera_interface.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdatomic.h>

static DummyCamera* g_dummy_camera = NULL;

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void generate_frame(DummyCamera* camera) {
    camera->frame_buffer.timestamp_ns = get_timestamp_ns();
    camera->frame_buffer.width = camera->width;
    camera->frame_buffer.height = camera->height;
    camera->frame_buffer.channels = 3;
    camera->frame_buffer.data_size = camera->width * camera->height * 3;
    
    if (!camera->frame_buffer.data) {
        camera->frame_buffer.data = (uint8_t*)malloc(camera->frame_buffer.data_size);
    }
    
    uint32_t fc = atomic_fetch_add(&camera->frame_count, 1);
    uint8_t pattern_r = (uint8_t)(fc % 256);
    uint8_t pattern_g = (uint8_t)((fc * 2) % 256);
    uint8_t pattern_b = (uint8_t)((fc * 3) % 256);
    
    for (uint32_t i = 0; i < camera->frame_buffer.data_size; i += 3) {
        uint32_t pixel_idx = i / 3;
        uint32_t x = pixel_idx % camera->width;
        uint32_t y = pixel_idx / camera->width;
        uint8_t gradient = (uint8_t)((x + y + fc * 10) % 256);
        
        camera->frame_buffer.data[i]     = (uint8_t)((pattern_r + gradient) / 2);
        camera->frame_buffer.data[i + 1] = (uint8_t)((pattern_g + gradient) / 2);
        camera->frame_buffer.data[i + 2] = (uint8_t)((pattern_b + gradient) / 2);
    }
}

static void* streaming_loop(void* arg) {
    DummyCamera* camera = (DummyCamera*)arg;
    uint64_t last_frame_ns = 0;
    
    while (camera->streaming) {
        uint64_t now = get_timestamp_ns();
        
        if (last_frame_ns == 0 || now - last_frame_ns >= camera->frame_interval_ns) {
            generate_frame(camera);
            last_frame_ns = camera->frame_buffer.timestamp_ns;
            if (camera->callback) {
                camera->callback(&camera->frame_buffer);
            }
        }
        
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 100000ULL;
        nanosleep(&sleep_time, NULL);
    }
    
    return NULL;
}

static bool camera_open(DummyCamera* camera) {
    camera->frame_buffer.data = (uint8_t*)malloc(camera->width * camera->height * 3);
    return camera->frame_buffer.data != NULL;
}

static void camera_close(DummyCamera* camera) {
    if (camera->streaming) {
        camera->streaming = false;
        if (camera->stream_thread) {
            pthread_join((pthread_t)(uintptr_t)camera->stream_thread, NULL);
            camera->stream_thread = NULL;
        }
    }
    if (camera->frame_buffer.data) {
        free(camera->frame_buffer.data);
        camera->frame_buffer.data = NULL;
    }
}

int rca_camera_init() {
    if (!g_dummy_camera) {
        g_dummy_camera = (DummyCamera*)malloc(sizeof(DummyCamera));
        if (!g_dummy_camera) {
            return -1;
        }
        
        memset(g_dummy_camera, 0, sizeof(DummyCamera));
        g_dummy_camera->width = 640;
        g_dummy_camera->height = 480;
        g_dummy_camera->fps = 30;
        g_dummy_camera->frame_interval_ns = 1000000000ULL / 30;
        g_dummy_camera->streaming = false;
        atomic_init(&g_dummy_camera->frame_count, 0);
        
        if (camera_open(g_dummy_camera)) {
            return 0;
        }
        
        free(g_dummy_camera);
        g_dummy_camera = NULL;
    }
    return -1;
}

int rca_camera_start_capture(CameraCallback callback) {
    if (!g_dummy_camera || !callback) {
        return -1;
    }
    
    if (g_dummy_camera->streaming) {
        return -1;
    }
    
    g_dummy_camera->callback = callback;
    g_dummy_camera->streaming = true;
    atomic_store(&g_dummy_camera->frame_count, 0);
    
    pthread_t thread;
    if (pthread_create(&thread, NULL, streaming_loop, g_dummy_camera) != 0) {
        g_dummy_camera->streaming = false;
        return -1;
    }
    
    g_dummy_camera->stream_thread = (void*)(uintptr_t)thread;
    return 0;
}

int rca_camera_exit() {
    if (!g_dummy_camera) {
        return -1;
    }
    
    camera_close(g_dummy_camera);
    free(g_dummy_camera);
    g_dummy_camera = NULL;
    return 0;
}