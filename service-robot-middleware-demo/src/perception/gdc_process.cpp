#include "srm/perception/gdc_process.h"
#include "srm/common/logging.h"
#include <cstring>
#include <cmath>

namespace srm {
namespace perception {

static const char* TAG = "GdcProcess";

GdcProcessNode::GdcProcessNode(const std::string& name,
                               std::shared_ptr<dal::IPubSubTransport> transport)
    : NodeBase(name), transport_(std::move(transport)) {
}

GdcProcessNode::~GdcProcessNode() = default;

bool GdcProcessNode::on_configure() {
    transport_->create_subscriber(
        input_topic_, "Image",
        [this](const uint8_t* data, size_t size, uint64_t timestamp) {
            if (get_state() == lifecycle::NodeState::ACTIVE) {
                this->on_camera_image(data, size, timestamp);
            }
        }
    );

    transport_->create_publisher(output_topic_, "Detection2DArray");

    SRM_LOG_INFO(TAG, "[" + name_ + "] configured (pub/sub registered)");
    return true;
}

bool GdcProcessNode::on_activate() {
    frame_count_ = 0;
    total_detections_ = 0;
    SRM_LOG_INFO(TAG, "[" + name_ + "] activated");
    return true;
}

void GdcProcessNode::on_deactivate() {
    SRM_LOG_INFO(TAG, "[" + name_ + "] deactivated");
}

void GdcProcessNode::on_shutdown() {
    SRM_LOG_INFO(TAG, "[" + name_ + "] shutdown, processed " +
                 std::to_string(frame_count_) + " frames, " +
                 std::to_string(total_detections_) + " detections");
}

ImageMessage GdcProcessNode::deserialize_image(const uint8_t* data, size_t size) {
    ImageMessage img;
    size_t offset = 0;

    std::memcpy(&img.timestamp, data + offset, sizeof(Timestamp));
    offset += sizeof(Timestamp);

    std::memcpy(&img.width, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::memcpy(&img.height, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::memcpy(&img.channels, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    size_t data_size = size - offset;
    img.data.assign(data + offset, data + offset + data_size);

    return img;
}

std::vector<uint8_t> GdcProcessNode::serialize_detection(const DetectionMessage& msg) {
    std::vector<uint8_t> buffer;
    buffer.resize(sizeof(Timestamp) + sizeof(uint32_t) + msg.detections.size() * sizeof(Detection));

    size_t offset = 0;
    std::memcpy(buffer.data() + offset, &msg.timestamp, sizeof(Timestamp));
    offset += sizeof(Timestamp);

    uint32_t count = static_cast<uint32_t>(msg.detections.size());
    std::memcpy(buffer.data() + offset, &count, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (!msg.detections.empty()) {
        std::memcpy(buffer.data() + offset, msg.detections.data(),
                    msg.detections.size() * sizeof(Detection));
    }

    return buffer;
}

std::vector<Detection> GdcProcessNode::detect_objects(const ImageMessage& image) {
    std::vector<Detection> detections;

    if (image.data.empty()) return detections;

    int grid_rows = 3;
    int grid_cols = 3;
    float cell_w = static_cast<float>(image.width) / grid_cols;
    float cell_h = static_cast<float>(image.height) / grid_rows;

    for (int r = 0; r < grid_rows; ++r) {
        for (int c = 0; c < grid_cols; ++c) {
            size_t center_x = static_cast<size_t>((c + 0.5f) * cell_w);
            size_t center_y = static_cast<size_t>((r + 0.5f) * cell_h);
            size_t idx = (center_y * image.width + center_x) * image.channels;

            if (idx + 2 < image.data.size()) {
                uint8_t r_val = image.data[idx];
                uint8_t g_val = image.data[idx + 1];
                uint8_t b_val = image.data[idx + 2];

                float brightness = (r_val + g_val + b_val) / 3.0f / 255.0f;

                if (brightness > 0.4f) {
                    Detection det;
                    det.x = c * cell_w + cell_w * 0.25f;
                    det.y = r * cell_h + cell_h * 0.25f;
                    det.w = cell_w * 0.5f;
                    det.h = cell_h * 0.5f;
                    det.class_id = (r * grid_cols + c) % 5;
                    det.confidence = 0.5f + brightness * 0.5f;
                    detections.push_back(det);
                }
            }
        }
    }

    return detections;
}

void GdcProcessNode::on_camera_image(const uint8_t* data, size_t size, uint64_t timestamp) {
    ImageMessage img = deserialize_image(data, size);

    auto detections = detect_objects(img);

    DetectionMessage det_msg;
    det_msg.timestamp = img.timestamp;
    det_msg.detections = std::move(detections);

    auto serialized = serialize_detection(det_msg);
    transport_->publish(output_topic_, serialized.data(), serialized.size(), img.timestamp);

    frame_count_++;
    total_detections_ += det_msg.detections.size();

    if (frame_count_ % 30 == 0) {
        SRM_LOG_INFO(TAG, "[" + name_ + "] frame " + std::to_string(frame_count_) +
                     ", " + std::to_string(det_msg.detections.size()) + " detections");
    }
}

}
}
