#include "srm/hal/camera_node.h"
#include "srm/common/logging.h"
#include <cstring>

namespace srm {
namespace hal {

static const char* TAG = "CameraNode";

CameraNode::CameraNode(const std::string& name,
                       std::shared_ptr<dal::IPubSubTransport> transport)
    : NodeBase(name), transport_(std::move(transport)) {
}

CameraNode::~CameraNode() = default;

bool CameraNode::on_configure() {
    camera_ = std::make_shared<DummyCamera>(640, 480, 30);
    if (!camera_->open()) {
        SRM_LOG_ERROR(TAG, "[" + name_ + "] Failed to open camera");
        return false;
    }

    camera_->set_callback([this](const ImageMessage& frame) {
        auto serialized = serialize_image(frame);
        transport_->publish(output_topic_, serialized.data(), serialized.size(),
                            frame.timestamp);
        frame_count_++;
    });

    transport_->create_publisher(output_topic_, "Image");

    SRM_LOG_INFO(TAG, "[" + name_ + "] configured (camera opened, pub registered)");
    return true;
}

bool CameraNode::on_activate() {
    camera_->start_streaming();
    streaming_ = true;
    frame_count_ = 0;
    SRM_LOG_INFO(TAG, "[" + name_ + "] activated (streaming started)");
    return true;
}

void CameraNode::on_deactivate() {
    camera_->stop_streaming();
    streaming_ = false;
    SRM_LOG_INFO(TAG, "[" + name_ + "] deactivated (streaming stopped)");
}

void CameraNode::on_shutdown() {
    if (camera_) {
        camera_->close();
    }
    SRM_LOG_INFO(TAG, "[" + name_ + "] shutdown, captured " +
                 std::to_string(frame_count_) + " frames");
}

std::vector<uint8_t> CameraNode::serialize_image(const ImageMessage& img) {
    std::vector<uint8_t> buffer;
    size_t header_size = sizeof(Timestamp) + 3 * sizeof(uint32_t);
    buffer.resize(header_size + img.data.size());

    size_t offset = 0;
    std::memcpy(buffer.data() + offset, &img.timestamp, sizeof(Timestamp));
    offset += sizeof(Timestamp);

    std::memcpy(buffer.data() + offset, &img.width, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::memcpy(buffer.data() + offset, &img.height, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::memcpy(buffer.data() + offset, &img.channels, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (!img.data.empty()) {
        std::memcpy(buffer.data() + offset, img.data.data(), img.data.size());
    }

    return buffer;
}

bool CameraNode::read_frame_and_publish() {
    if (!streaming_ || !camera_) return false;
    return camera_->read_frame();
}

}
}
