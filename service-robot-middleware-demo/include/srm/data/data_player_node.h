#pragma once

#include "srm/lifecycle/node_base.h"
#include "srm/dal/transport_interface.h"
#include "srm/data/data_player.h"
#include <string>
#include <memory>

namespace srm {
namespace data {

class DataPlayerNode : public lifecycle::NodeBase {
public:
    DataPlayerNode(const std::string& name,
                   std::shared_ptr<dal::IPubSubTransport> transport);
    ~DataPlayerNode() override;

    void set_data_file(const std::string& path) { data_file_ = path; }
    void set_speed(float speed) { speed_ = speed; }

    bool play_next();
    bool is_finished() const;

protected:
    bool on_configure() override;
    bool on_activate() override;
    void on_deactivate() override;
    void on_shutdown() override;

private:
    std::shared_ptr<dal::IPubSubTransport> transport_;
    std::shared_ptr<DataPlayer> player_;
    std::string data_file_;
    float speed_ = 1.0f;
    bool playing_ = false;
};

}
}
