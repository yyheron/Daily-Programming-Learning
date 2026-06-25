#pragma once

#include <functional>
#include <string>
#include <cstdint>
#include <vector>

namespace hal {

struct ImageMessage {
    uint64_t timestamp_ns;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    std::vector<uint8_t> data;
};

typedef void (*CameraCallback)(const ImageMessage* pData);

} // namespace hal
