#pragma once

#include <stdint.h>
#include <string>
#include <cstdint>

#pragma pack(push, 1)

enum BinType: uint16_t
{
    BinType_LidarPointCloud = 310,
    BinType_LidarImu = 311,
};

struct BinHeader
{
    uint64_t timestamp;
    BinType type;
    uint32_t length;
};

static_assert(sizeof(BinHeader) == 14);

struct BinLidarPoint {
    float x;
    float y;
    float z;
    uint8_t intensity;
    uint16_t ring;
    float timestamp_offset;
    uint8_t tag;
};

struct BinLidarPointCloudData {
    uint64_t ap_timestamp;
    uint32_t point_num;
    uint32_t height;
    uint32_t width;
    bool is_dense;
    double lidar_timestamp;
    uint32_t seq;
    char frame_id[64];
    BinLidarPoint points[0];
};

struct BinLidarImuData {
    float linear_acceleration_x;
    float linear_acceleration_y;
    float linear_acceleration_z;
    float angular_velocity_x;
    float angular_velocity_y;
    float angular_velocity_z;
    float orientation_x;
    float orientation_y;
    float orientation_z;
    float orientation_w;
};