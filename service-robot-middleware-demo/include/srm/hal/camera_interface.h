#pragma once

#include <functional>
#include <string>
#include <cstdint>
#include "srm/common/types.h"

namespace srm {
namespace hal {

using CameraCallback = std::function<void(const ImageMessage& image)>;

class ICameraDevice {
public:
    virtual ~ICameraDevice() = default;

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual void set_callback(CameraCallback callback) = 0;

    virtual bool start_streaming() = 0;
    virtual void stop_streaming() = 0;

    virtual uint32_t get_width() const = 0;
    virtual uint32_t get_height() const = 0;
    virtual uint32_t get_fps() const = 0;
};

}
}
