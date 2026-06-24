#pragma once

#include "srm/dal/transport_interface.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace srm {
namespace data {

struct RecordedMessage {
    std::string topic;
    uint64_t timestamp;
    std::vector<uint8_t> data;
};

class DataPlayer {
public:
    explicit DataPlayer(std::shared_ptr<dal::IPubSubTransport> transport);
    ~DataPlayer();

    bool open(const std::string& filename);
    void close();

    bool start_playback(double speed = 1.0);
    void stop_playback();

    bool play_next();
    bool is_finished() const { return current_idx_ >= messages_.size(); }
    bool is_playing() const { return playing_; }

    size_t total_messages() const { return messages_.size(); }
    size_t current_index() const { return current_idx_; }

private:
    bool read_file(const std::string& filename);

    std::shared_ptr<dal::IPubSubTransport> transport_;
    std::vector<RecordedMessage> messages_;

    bool playing_ = false;
    size_t current_idx_ = 0;
    double playback_speed_ = 1.0;
    uint64_t base_timestamp_ = 0;
    uint64_t start_time_ns_ = 0;

    std::unordered_map<uint32_t, std::string> channel_to_topic_;
};

}
}
