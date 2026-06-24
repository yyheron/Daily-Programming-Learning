#include "srm/data/data_player_node.h"
#include "srm/common/logging.h"

namespace srm {
namespace data {

static const char* TAG = "DataPlayerNode";

DataPlayerNode::DataPlayerNode(const std::string& name,
                               std::shared_ptr<dal::IPubSubTransport> transport)
    : NodeBase(name), transport_(std::move(transport)) {
}

DataPlayerNode::~DataPlayerNode() = default;

bool DataPlayerNode::on_configure() {
    if (data_file_.empty()) {
        SRM_LOG_ERROR(TAG, "[" + name_ + "] No data file configured");
        return false;
    }

    player_ = std::make_shared<DataPlayer>(transport_);
    if (!player_->open(data_file_)) {
        SRM_LOG_ERROR(TAG, "[" + name_ + "] Failed to open data file: " + data_file_);
        return false;
    }

    SRM_LOG_INFO(TAG, "[" + name_ + "] configured (data file opened: " +
                 data_file_ + ")");
    return true;
}

bool DataPlayerNode::on_activate() {
    player_->start_playback(speed_);
    playing_ = true;
    SRM_LOG_INFO(TAG, "[" + name_ + "] activated (playback started at " +
                 std::to_string(speed_) + "x)");
    return true;
}

void DataPlayerNode::on_deactivate() {
    if (player_) {
        player_->stop_playback();
    }
    playing_ = false;
    SRM_LOG_INFO(TAG, "[" + name_ + "] deactivated (playback stopped)");
}

void DataPlayerNode::on_shutdown() {
    if (player_) {
        player_->close();
    }
    SRM_LOG_INFO(TAG, "[" + name_ + "] shutdown");
}

bool DataPlayerNode::play_next() {
    if (!playing_ || !player_) return false;
    return player_->play_next();
}

bool DataPlayerNode::is_finished() const {
    if (!player_) return true;
    return player_->is_finished();
}

}
}
