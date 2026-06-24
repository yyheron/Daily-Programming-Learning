#include "srm/data/data_player.h"
#include "srm/common/logging.h"
#include <chrono>
#include <cstring>

namespace srm {
namespace data {

static const char* TAG = "DataPlayer";
static const uint32_t FILE_MAGIC = 0x53524d31;
static const uint8_t MSG_TYPE_CHANNEL = 0x01;
static const uint8_t MSG_TYPE_MESSAGE = 0x02;

DataPlayer::DataPlayer(std::shared_ptr<dal::IPubSubTransport> transport)
    : transport_(std::move(transport)) {
}

DataPlayer::~DataPlayer() {
    stop_playback();
    close();
}

bool DataPlayer::open(const std::string& filename) {
    if (!read_file(filename)) {
        return false;
    }
    SRM_LOG_INFO(TAG, "Opened data file: " + filename +
                 ", total messages: " + std::to_string(messages_.size()));
    return true;
}

bool DataPlayer::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        SRM_LOG_ERROR(TAG, "Failed to open file: " + filename);
        return false;
    }

    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != FILE_MAGIC) {
        SRM_LOG_ERROR(TAG, "Invalid file magic");
        return false;
    }

    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    uint32_t reserved = 0;
    file.read(reinterpret_cast<char*>(&reserved), sizeof(reserved));

    messages_.clear();
    channel_to_topic_.clear();

    while (file.peek() != EOF) {
        uint8_t msg_type = 0;
        file.read(reinterpret_cast<char*>(&msg_type), sizeof(msg_type));

        if (msg_type == MSG_TYPE_CHANNEL) {
            uint32_t channel_id = 0;
            file.read(reinterpret_cast<char*>(&channel_id), sizeof(channel_id));

            uint32_t topic_len = 0;
            file.read(reinterpret_cast<char*>(&topic_len), sizeof(topic_len));
            std::string topic(topic_len, '\0');
            file.read(topic.data(), topic_len);

            uint32_t type_len = 0;
            file.read(reinterpret_cast<char*>(&type_len), sizeof(type_len));
            std::string type_name(type_len, '\0');
            file.read(type_name.data(), type_len);

            channel_to_topic_[channel_id] = topic;

            if (transport_) {
                transport_->create_publisher(topic, type_name);
            }

        } else if (msg_type == MSG_TYPE_MESSAGE) {
            uint32_t channel_id = 0;
            file.read(reinterpret_cast<char*>(&channel_id), sizeof(channel_id));

            uint64_t timestamp = 0;
            file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));

            uint32_t data_size = 0;
            file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));

            std::vector<uint8_t> data(data_size);
            if (data_size > 0) {
                file.read(reinterpret_cast<char*>(data.data()), data_size);
            }

            auto it = channel_to_topic_.find(channel_id);
            if (it != channel_to_topic_.end()) {
                RecordedMessage msg;
                msg.topic = it->second;
                msg.timestamp = timestamp;
                msg.data = std::move(data);
                messages_.push_back(std::move(msg));
            }
        } else {
            SRM_LOG_WARN(TAG, "Unknown message type: " + std::to_string(msg_type));
            break;
        }
    }

    file.close();
    return !messages_.empty();
}

void DataPlayer::close() {
    messages_.clear();
    channel_to_topic_.clear();
    current_idx_ = 0;
}

bool DataPlayer::start_playback(double speed) {
    if (messages_.empty()) {
        SRM_LOG_ERROR(TAG, "No messages to play");
        return false;
    }

    if (playing_) {
        SRM_LOG_WARN(TAG, "Already playing");
        return false;
    }

    playback_speed_ = speed;
    current_idx_ = 0;
    base_timestamp_ = messages_[0].timestamp;

    start_time_ns_ = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    playing_ = true;
    SRM_LOG_INFO(TAG, "Start playback, speed: " + std::to_string(speed) + "x");
    return true;
}

void DataPlayer::stop_playback() {
    if (!playing_) return;
    playing_ = false;
    SRM_LOG_INFO(TAG, "Stop playback, played " +
                 std::to_string(current_idx_) + " messages");
}

bool DataPlayer::play_next() {
    if (!playing_ || current_idx_ >= messages_.size()) {
        return false;
    }

    const auto& msg = messages_[current_idx_];

    uint64_t now_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    uint64_t elapsed_recorded = msg.timestamp - base_timestamp_;
    uint64_t elapsed_real = now_ns - start_time_ns_;
    auto target_elapsed = static_cast<uint64_t>(
        static_cast<double>(elapsed_recorded) / playback_speed_
    );

    if (elapsed_real < target_elapsed) {
        return false;
    }

    if (transport_) {
        transport_->publish(msg.topic, msg.data.data(), msg.data.size(), msg.timestamp);
    }

    current_idx_++;

    if (current_idx_ >= messages_.size()) {
        playing_ = false;
        SRM_LOG_INFO(TAG, "Playback finished");
    }

    return true;
}

}
}
