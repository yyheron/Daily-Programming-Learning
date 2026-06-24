#pragma once

#include "srm/dal/transport_interface.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace srm {
namespace dal {

using PubListener = std::function<void(const std::string& topic,
                                       const uint8_t* data, size_t size,
                                       uint64_t timestamp)>;

class InprocTransport : public IPubSubTransport {
public:
    InprocTransport();
    ~InprocTransport() override;

    bool init() override;
    void shutdown() override;

    bool create_publisher(const std::string& topic, const std::string& type_name) override;
    bool create_subscriber(const std::string& topic, const std::string& type_name,
                           DataCallback callback) override;

    bool publish(const std::string& topic, const uint8_t* data, size_t size,
                 uint64_t timestamp = 0) override;

    void set_pub_listener(PubListener listener) {
        pub_listener_ = std::move(listener);
    }

private:
    struct TopicState {
        std::vector<DataCallback> subscribers;
    };

    std::unordered_map<std::string, std::shared_ptr<TopicState>> topics_;
    bool running_ = false;
    PubListener pub_listener_;

    std::shared_ptr<TopicState> get_or_create_topic(const std::string& topic);
};

}
}
