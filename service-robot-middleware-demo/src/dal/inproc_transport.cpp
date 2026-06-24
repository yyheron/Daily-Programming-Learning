#include "srm/dal/inproc_transport.h"
#include "srm/common/logging.h"
#include <chrono>

namespace srm {
namespace dal {

static const char* TAG = "InprocTransport";

InprocTransport::InprocTransport() = default;

InprocTransport::~InprocTransport() {
    shutdown();
}

bool InprocTransport::init() {
    running_ = true;
    SRM_LOG_INFO(TAG, "Inproc transport initialized (simulates shared memory)");
    return true;
}

void InprocTransport::shutdown() {
    running_ = false;
    SRM_LOG_INFO(TAG, "Inproc transport shutdown");
}

std::shared_ptr<InprocTransport::TopicState>
InprocTransport::get_or_create_topic(const std::string& topic) {
    auto it = topics_.find(topic);
    if (it != topics_.end()) {
        return it->second;
    }
    auto state = std::make_shared<TopicState>();
    topics_[topic] = state;
    return state;
}

bool InprocTransport::create_publisher(const std::string& topic, const std::string& type_name) {
    get_or_create_topic(topic);
    SRM_LOG_DEBUG(TAG, "Publisher created: " + topic + " (" + type_name + ")");
    return true;
}

bool InprocTransport::create_subscriber(const std::string& topic, const std::string& type_name,
                                         DataCallback callback) {
    auto state = get_or_create_topic(topic);
    state->subscribers.push_back(std::move(callback));
    SRM_LOG_DEBUG(TAG, "Subscriber created: " + topic + " (" + type_name + ")");
    return true;
}

bool InprocTransport::publish(const std::string& topic, const uint8_t* data, size_t size,
                               uint64_t timestamp) {
    if (!running_) return false;

    if (timestamp == 0) {
        timestamp = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
    }

    if (pub_listener_) {
        pub_listener_(topic, data, size, timestamp);
    }

    auto state = get_or_create_topic(topic);
    for (const auto& cb : state->subscribers) {
        cb(data, size, timestamp);
    }

    return true;
}

std::shared_ptr<IPubSubTransport> create_transport(TransportType type) {
    switch (type) {
        case TransportType::INPROC:
            return std::make_shared<InprocTransport>();
        default:
            SRM_LOG_WARN("TransportFactory", "Unsupported transport type, fallback to INPROC");
            return std::make_shared<InprocTransport>();
    }
}

}
}
