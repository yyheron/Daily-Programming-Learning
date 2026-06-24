#pragma once

#include "srm/lifecycle/node_base.h"
#include "srm/dal/transport_interface.h"
#include "srm/hal/dummy_camera.h"
#include <string>
#include <memory>

namespace srm {
namespace hal {

class CameraNode : public lifecycle::NodeBase {
public:
    CameraNode(const std::string& name,
               std::shared_ptr<dal::IPubSubTransport> transport);
    ~CameraNode() override;

    bool read_frame_and_publish();
    bool is_streaming() const { return streaming_; }
    size_t frame_count() const { return frame_count_; }

protected:
    bool on_configure() override;
    bool on_activate() override;
    void on_deactivate() override;
    void on_shutdown() override;

private:
    std::vector<uint8_t> serialize_image(const ImageMessage& img);

    std::shared_ptr<dal::IPubSubTransport> transport_;
    std::shared_ptr<DummyCamera> camera_;
    std::string output_topic_ = "/camera/front";
    bool streaming_ = false;
    size_t frame_count_ = 0;
};

}
}
