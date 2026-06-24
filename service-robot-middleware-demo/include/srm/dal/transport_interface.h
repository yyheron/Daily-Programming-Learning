#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

namespace srm {
namespace dal {

using DataCallback = std::function<void(const uint8_t* data, size_t size, uint64_t timestamp)>;

enum class TransportType {
    INPROC,
    SHARED_MEMORY,
    DDS,
    MQTT
};

class IPubSubTransport {
public:
    virtual ~IPubSubTransport() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual bool create_publisher(const std::string& topic, const std::string& type_name) = 0;
    virtual bool create_subscriber(const std::string& topic, const std::string& type_name,
                                   DataCallback callback) = 0;

    virtual bool publish(const std::string& topic, const uint8_t* data, size_t size,
                         uint64_t timestamp = 0) = 0;
};

std::shared_ptr<IPubSubTransport> create_transport(TransportType type);

}
}
