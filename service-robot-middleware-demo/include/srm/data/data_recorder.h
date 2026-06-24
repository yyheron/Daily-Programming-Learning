#pragma once

#include "srm/dal/transport_interface.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace srm {
namespace data {

class DataRecorder {
public:
    DataRecorder();
    ~DataRecorder();

    bool open(const std::string& filename);
    void close();

    bool add_topic(const std::string& topic, const std::string& type_name);

    bool record_message(const std::string& topic, const uint8_t* data, size_t size, uint64_t timestamp);

    size_t message_count() const { return message_count_; }

private:
    struct TopicInfo {
        uint32_t channel_id;
        std::string type_name;
    };

    std::ofstream file_;
    std::unordered_map<std::string, TopicInfo> topics_;
    size_t message_count_ = 0;
    uint32_t next_channel_id_ = 0;
    bool is_open_ = false;

    void write_header();
    void write_channel_info(const std::string& topic, const TopicInfo& info);
};

}
}
