#include "srm/decision/fake_vla.h"
#include "srm/common/logging.h"
#include <cstring>
#include <string>

namespace srm {
namespace decision {

static const char* TAG = "FakeVLA";

FakeVlaNode::FakeVlaNode(const std::string& name,
                         std::shared_ptr<dal::IPubSubTransport> transport)
    : NodeBase(name), transport_(std::move(transport)) {
}

FakeVlaNode::~FakeVlaNode() = default;

bool FakeVlaNode::on_configure() {
    transport_->create_subscriber(
        input_topic_, "Detection2DArray",
        [this](const uint8_t* data, size_t size, uint64_t timestamp) {
            if (get_state() == lifecycle::NodeState::ACTIVE) {
                this->on_detection(data, size, timestamp);
            }
        }
    );

    transport_->create_publisher(output_topic_, "Action");

    SRM_LOG_INFO(TAG, "[" + name_ + "] configured (pub/sub registered)");
    return true;
}

bool FakeVlaNode::on_activate() {
    decision_count_ = 0;
    SRM_LOG_INFO(TAG, "[" + name_ + "] activated, instruction: " + instruction_);
    return true;
}

void FakeVlaNode::on_deactivate() {
    SRM_LOG_INFO(TAG, "[" + name_ + "] deactivated");
}

void FakeVlaNode::on_shutdown() {
    SRM_LOG_INFO(TAG, "[" + name_ + "] shutdown, made " +
                 std::to_string(decision_count_) + " decisions");
}

DetectionMessage FakeVlaNode::deserialize_detection(const uint8_t* data, size_t size) {
    DetectionMessage msg;
    size_t offset = 0;

    std::memcpy(&msg.timestamp, data + offset, sizeof(Timestamp));
    offset += sizeof(Timestamp);

    uint32_t count = 0;
    std::memcpy(&count, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (count > 0 && offset + count * sizeof(Detection) <= size) {
        msg.detections.resize(count);
        std::memcpy(msg.detections.data(), data + offset, count * sizeof(Detection));
    }

    return msg;
}

std::vector<uint8_t> FakeVlaNode::serialize_action(const ActionMessage& msg) {
    std::vector<uint8_t> buffer;
    size_t header_size = sizeof(Timestamp) + sizeof(uint32_t) + 64;
    buffer.resize(header_size + msg.params.size() * sizeof(float));

    size_t offset = 0;
    std::memcpy(buffer.data() + offset, &msg.timestamp, sizeof(Timestamp));
    offset += sizeof(Timestamp);

    std::memcpy(buffer.data() + offset, msg.action_type.c_str(),
                msg.action_type.size() + 1);
    offset += 64;

    uint32_t param_count = static_cast<uint32_t>(msg.params.size());
    std::memcpy(buffer.data() + offset, &param_count, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (!msg.params.empty()) {
        std::memcpy(buffer.data() + offset, msg.params.data(),
                    msg.params.size() * sizeof(float));
    }

    return buffer;
}

ActionMessage FakeVlaNode::make_decision(const DetectionMessage& detection) {
    ActionMessage action;
    action.timestamp = detection.timestamp;

    if (detection.detections.empty()) {
        action.action_type = "stop";
        action.params = {};
        return action;
    }

    const Detection& target = detection.detections[0];

    float center_x = target.x + target.w / 2.0f;
    float img_width = 640.0f;

    if (center_x < img_width * 0.33f) {
        action.action_type = "turn_left";
    } else if (center_x > img_width * 0.67f) {
        action.action_type = "turn_right";
    } else {
        action.action_type = "move_forward";
    }

    action.params = {0.1f, static_cast<float>(target.class_id)};

    return action;
}

void FakeVlaNode::on_detection(const uint8_t* data, size_t size, uint64_t timestamp) {
    DetectionMessage det_msg = deserialize_detection(data, size);

    ActionMessage action = make_decision(det_msg);

    auto serialized = serialize_action(action);
    transport_->publish(output_topic_, serialized.data(), serialized.size(), timestamp);

    decision_count_++;

    if (decision_count_ % 30 == 0) {
        SRM_LOG_INFO(TAG, "[" + name_ + "] decision " + std::to_string(decision_count_) +
                     ": " + action.action_type);
    }
}

}
}
