#include "srm/lifecycle/launcher.h"
#include "srm/common/logging.h"
#include <algorithm>
#include <queue>

namespace srm {
namespace lifecycle {

static const char* TAG = "Launcher";

Launcher::Launcher(std::shared_ptr<dal::IPubSubTransport> transport)
    : transport_(std::move(transport)) {
}

Launcher::~Launcher() {
    stop_all();
}

void Launcher::register_factory(const std::string& type, NodeFactory factory) {
    factories_[type] = std::move(factory);
    SRM_LOG_INFO(TAG, "Registered factory: " + type);
}

void Launcher::add_node(const std::string& name, const std::string& type,
                        const std::vector<std::string>& depends_on) {
    auto it = factories_.find(type);
    if (it == factories_.end()) {
        SRM_LOG_ERROR(TAG, "Unknown node type: " + type);
        return;
    }

    auto node = it->second(name, transport_);
    for (const auto& dep : depends_on) {
        node->add_dependency(dep);
    }

    nodes_[name] = node;
    SRM_LOG_INFO(TAG, "Added node: " + name + " (type: " + type + ")");
    if (!depends_on.empty()) {
        std::string dep_str;
        for (size_t i = 0; i < depends_on.size(); ++i) {
            if (i > 0) dep_str += ", ";
            dep_str += depends_on[i];
        }
        SRM_LOG_INFO(TAG, "  dependencies: " + dep_str);
    }
}

bool Launcher::topological_sort(std::vector<NodePtr>& out_order) {
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, std::vector<std::string>> adj;

    for (const auto& kv : nodes_) {
        in_degree[kv.first] = 0;
    }

    for (const auto& kv : nodes_) {
        for (const auto& dep : kv.second->get_dependencies()) {
            if (nodes_.find(dep) == nodes_.end()) {
                SRM_LOG_ERROR(TAG, "Node '" + kv.first + "' depends on unknown node: " + dep);
                return false;
            }
            adj[dep].push_back(kv.first);
            in_degree[kv.first]++;
        }
    }

    std::queue<std::string> q;
    for (const auto& kv : in_degree) {
        if (kv.second == 0) {
            q.push(kv.first);
        }
    }

    out_order.clear();
    while (!q.empty()) {
        std::string name = q.front();
        q.pop();
        out_order.push_back(nodes_[name]);

        for (const auto& next : adj[name]) {
            in_degree[next]--;
            if (in_degree[next] == 0) {
                q.push(next);
            }
        }
    }

    if (out_order.size() != nodes_.size()) {
        SRM_LOG_ERROR(TAG, "Cycle detected in node dependencies!");
        return false;
    }

    return true;
}

bool Launcher::start_all() {
    if (started_) {
        SRM_LOG_WARN(TAG, "Already started");
        return false;
    }

    SRM_LOG_INFO(TAG, "========================================");
    SRM_LOG_INFO(TAG, "  Starting all nodes");
    SRM_LOG_INFO(TAG, "========================================");

    std::vector<NodePtr> order;
    if (!topological_sort(order)) {
        return false;
    }

    node_order_.clear();
    for (const auto& node : order) {
        node_order_.push_back(node->get_name());
    }

    SRM_LOG_INFO(TAG, "Startup order:");
    for (size_t i = 0; i < order.size(); ++i) {
        SRM_LOG_INFO(TAG, "  " + std::to_string(i + 1) + ". " +
                 order[i]->get_name() +
                 " (deps: " + std::to_string(order[i]->get_dependencies().size()) + ")");
    }

    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "=== Phase 1: Configure all nodes (register pub/sub) ===");
    for (const auto& node : order) {
        SRM_LOG_INFO(TAG, "Configuring '" + node->get_name() + "'...");
        if (!node->configure()) {
            SRM_LOG_ERROR(TAG, "Failed to configure node: " + node->get_name());
            return false;
        }
        SRM_LOG_INFO(TAG, "  '" + node->get_name() + "' -> INACTIVE");
    }

    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "=== Phase 2: Discovery wait ===");
    SRM_LOG_INFO(TAG, "All pub/sub registered. Waiting for discovery...");
    SRM_LOG_INFO(TAG, "(In real system: wait for DDS/SHM discovery completion)");

    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "=== Phase 3: Activate all nodes (Ready Barrier) ===");
    SRM_LOG_INFO(TAG, "Broadcasting ACTIVATE signal...");
    for (const auto& node : order) {
        SRM_LOG_INFO(TAG, "Activating '" + node->get_name() + "'...");
        if (!node->activate()) {
            SRM_LOG_ERROR(TAG, "Failed to activate node: " + node->get_name());
            return false;
        }
        SRM_LOG_INFO(TAG, "  '" + node->get_name() + "' -> ACTIVE");
    }

    started_ = true;
    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "========================================");
    SRM_LOG_INFO(TAG, "  All nodes ACTIVE - system ready");
    SRM_LOG_INFO(TAG, "========================================");
    return true;
}

void Launcher::stop_all() {
    if (!started_) {
        for (const auto& kv : nodes_) {
            kv.second->shutdown();
        }
        return;
    }

    SRM_LOG_INFO(TAG, "========================================");
    SRM_LOG_INFO(TAG, "  Stopping all nodes");
    SRM_LOG_INFO(TAG, "========================================");

    for (auto it = node_order_.rbegin(); it != node_order_.rend(); ++it) {
        auto node = nodes_[*it];
        SRM_LOG_INFO(TAG, "Shutting down '" + node->get_name() + "'...");
        node->shutdown();
        SRM_LOG_INFO(TAG, "  '" + node->get_name() + "' -> FINALIZED");
    }

    started_ = false;
    SRM_LOG_INFO(TAG, "All nodes stopped");
}

bool Launcher::is_all_active() const {
    for (const auto& kv : nodes_) {
        if (kv.second->get_state() != NodeState::ACTIVE) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> Launcher::get_node_names() const {
    std::vector<std::string> names;
    for (const auto& name : node_order_) {
        names.push_back(name);
    }
    return names;
}

NodeState Launcher::get_node_state(const std::string& name) const {
    auto it = nodes_.find(name);
    if (it == nodes_.end()) {
        return NodeState::UNCONFIGURED;
    }
    return it->second->get_state();
}

}
}
