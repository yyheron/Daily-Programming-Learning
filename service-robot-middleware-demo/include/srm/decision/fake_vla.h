#pragma once

#include "srm/lifecycle/node_base.h"
#include "srm/dal/transport_interface.h"
#include "srm/common/types.h"
#include <string>
#include <memory>

namespace srm {
namespace decision {

class FakeVlaNode : public lifecycle::NodeBase {
public:
    FakeVlaNode(const std::string& name,
                std::shared_ptr<dal::IPubSubTransport> transport);
    ~FakeVlaNode() override;

    void set_instruction(const std::string& instruction) { instruction_ = instruction; }

    size_t decision_count() const { return decision_count_; }

protected:
    bool on_configure() override;
    bool on_activate() override;
    void on_deactivate() override;
    void on_shutdown() override;

private:
    void on_detection(const uint8_t* data, size_t size, uint64_t timestamp);

    DetectionMessage deserialize_detection(const uint8_t* data, size_t size);
    std::vector<uint8_t> serialize_action(const ActionMessage& msg);

    ActionMessage make_decision(const DetectionMessage& detection);

    std::shared_ptr<dal::IPubSubTransport> transport_;
    std::string input_topic_ = "/perception/detections";
    std::string output_topic_ = "/decision/action";
    std::string instruction_ = "go to the cup";

    size_t decision_count_ = 0;
};

}
}
