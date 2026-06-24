#include "srm/data/data_recorder.h"
#include "srm/common/logging.h"
#include <cstring>

namespace srm {
namespace data {

static const char* TAG = "DataRecorder";
static const uint32_t FILE_MAGIC = 0x53524d31;
static const uint32_t FILE_VERSION = 1;
static const uint8_t MSG_TYPE_CHANNEL = 0x01;
static const uint8_t MSG_TYPE_MESSAGE = 0x02;

DataRecorder::DataRecorder() = default;

DataRecorder::~DataRecorder() {
    close();
}

bool DataRecorder::open(const std::string& filename) {
    file_.open(filename, std::ios::binary | std::ios::trunc);
    if (!file_.is_open()) {
        SRM_LOG_ERROR(TAG, "Failed to open file: " + filename);
        return false;
    }

    write_header();
    is_open_ = true;
    message_count_ = 0;
    SRM_LOG_INFO(TAG, "Opened recording file: " + filename);
    return true;
}

void DataRecorder::close() {
    if (!is_open_) return;

    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }

    is_open_ = false;
    SRM_LOG_INFO(TAG, "Closed recording file, total messages: " +
                 std::to_string(message_count_));
}

void DataRecorder::write_header() {
    file_.write(reinterpret_cast<const char*>(&FILE_MAGIC), sizeof(FILE_MAGIC));
    file_.write(reinterpret_cast<const char*>(&FILE_VERSION), sizeof(FILE_VERSION));
    uint32_t placeholder = 0;
    file_.write(reinterpret_cast<const char*>(&placeholder), sizeof(placeholder));
}

bool DataRecorder::add_topic(const std::string& topic, const std::string& type_name) {
    if (topics_.find(topic) != topics_.end()) {
        return true;
    }

    TopicInfo info;
    info.channel_id = next_channel_id_++;
    info.type_name = type_name;
    topics_[topic] = info;

    write_channel_info(topic, info);
    SRM_LOG_INFO(TAG, "Added topic: " + topic + " (" + type_name +
                 "), channel_id: " + std::to_string(info.channel_id));
    return true;
}

void DataRecorder::write_channel_info(const std::string& topic, const TopicInfo& info) {
    file_.write(reinterpret_cast<const char*>(&MSG_TYPE_CHANNEL), sizeof(MSG_TYPE_CHANNEL));
    file_.write(reinterpret_cast<const char*>(&info.channel_id), sizeof(info.channel_id));

    uint32_t topic_len = static_cast<uint32_t>(topic.size());
    file_.write(reinterpret_cast<const char*>(&topic_len), sizeof(topic_len));
    file_.write(topic.data(), topic_len);

    uint32_t type_len = static_cast<uint32_t>(info.type_name.size());
    file_.write(reinterpret_cast<const char*>(&type_len), sizeof(type_len));
    file_.write(info.type_name.data(), type_len);
}

bool DataRecorder::record_message(const std::string& topic, const uint8_t* data,
                              size_t size, uint64_t timestamp) {
    if (!is_open_) return false;

    auto it = topics_.find(topic);
    if (it == topics_.end()) {
        SRM_LOG_WARN(TAG, "Topic not registered: " + topic);
        return false;
    }

    file_.write(reinterpret_cast<const char*>(&MSG_TYPE_MESSAGE), sizeof(MSG_TYPE_MESSAGE));
    file_.write(reinterpret_cast<const char*>(&it->second.channel_id), sizeof(uint32_t));
    file_.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    uint32_t data_size = static_cast<uint32_t>(size);
    file_.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    file_.write(reinterpret_cast<const char*>(data), size);

    message_count_++;
    return true;
}

}
}
