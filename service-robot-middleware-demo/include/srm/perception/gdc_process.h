#pragma once

#include "srm/lifecycle/node_base.h"
#include "srm/dal/transport_interface.h"
#include "srm/common/types.h"
#include <string>
#include <memory>

namespace srm {
namespace perception {

class GdcProcessNode : public lifecycle::NodeBase {
public:
    GdcProcessNode(const std::string& name,
                   std::shared_ptr<dal::IPubSubTransport> transport);
    ~GdcProcessNode() override;

    size_t frame_count() const { return frame_count_; }
    size_t total_detections() const { return total_detections_; }

protected:
    bool on_configure() override;
    bool on_activate() override;
    void on_deactivate() override;
    void on_shutdown() override;

private:
    void on_camera_image(const uint8_t* data, size_t size, uint64_t timestamp);

    ImageMessage deserialize_image(const uint8_t* data, size_t size);
    std::vector<uint8_t> serialize_detection(const DetectionMessage& msg);

    std::vector<Detection> detect_objects(const ImageMessage& image);

    std::shared_ptr<dal::IPubSubTransport> transport_;
    std::string input_topic_ = "/camera/front";
    std::string output_topic_ = "/perception/detections";

    size_t frame_count_ = 0;
    size_t total_detections_ = 0;
};

}
}
