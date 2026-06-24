#pragma once

#include "srm/lifecycle/node_base.h"
#include "srm/dal/transport_interface.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace srm {
namespace lifecycle {

using NodeFactory = std::function<NodePtr(const std::string& name,
    std::shared_ptr<dal::IPubSubTransport>)>;

class Launcher {
public:
    explicit Launcher(std::shared_ptr<dal::IPubSubTransport> transport);
    ~Launcher();

    void register_factory(const std::string& type, NodeFactory factory);

    void add_node(const std::string& name, const std::string& type,
                  const std::vector<std::string>& depends_on = {});

    bool start_all();

    void stop_all();

    bool is_all_active() const;

    size_t node_count() const { return nodes_.size(); }

    std::vector<std::string> get_node_names() const;

    NodeState get_node_state(const std::string& name) const;

    template <typename T>
    std::shared_ptr<T> get_node(const std::string& name) {
        auto it = nodes_.find(name);
        if (it == nodes_.end()) return nullptr;
        return std::dynamic_pointer_cast<T>(it->second);
    }

private:
    bool topological_sort(std::vector<NodePtr>& out_order);

    std::shared_ptr<dal::IPubSubTransport> transport_;
    std::unordered_map<std::string, NodeFactory> factories_;
    std::unordered_map<std::string, NodePtr> nodes_;
    std::vector<std::string> node_order_;
    bool started_ = false;
};

}
}
