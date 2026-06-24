#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace srm {
namespace lifecycle {

enum class NodeState {
    UNCONFIGURED = 0,
    INACTIVE = 1,
    ACTIVE = 2,
    ERROR = 3,
    FINALIZED = 4
};

inline const char* state_to_string(NodeState s) {
    switch (s) {
        case NodeState::UNCONFIGURED: return "UNCONFIGURED";
        case NodeState::INACTIVE:     return "INACTIVE";
        case NodeState::ACTIVE:       return "ACTIVE";
        case NodeState::ERROR:        return "ERROR";
        case NodeState::FINALIZED:  return "FINALIZED";
    }
    return "UNKNOWN";
}

class NodeBase {
public:
    explicit NodeBase(std::string name) : name_(std::move(name)) {}
    virtual ~NodeBase() = default;

    const std::string& get_name() const { return name_; }
    NodeState get_state() const { return state_; }

    const std::vector<std::string>& get_dependencies() const { return dependencies_; }

    void add_dependency(const std::string& dep) {
        dependencies_.push_back(dep);
    }

    bool configure() {
        if (state_ != NodeState::UNCONFIGURED) return false;
        if (!on_configure()) {
            state_ = NodeState::ERROR;
            return false;
        }
        state_ = NodeState::INACTIVE;
        return true;
    }

    bool activate() {
        if (state_ != NodeState::INACTIVE) return false;
        if (!on_activate()) {
            state_ = NodeState::ERROR;
            return false;
        }
        state_ = NodeState::ACTIVE;
        return true;
    }

    bool deactivate() {
        if (state_ != NodeState::ACTIVE) return false;
        on_deactivate();
        state_ = NodeState::INACTIVE;
        return true;
    }

    void shutdown() {
        on_shutdown();
        state_ = NodeState::FINALIZED;
    }

protected:
    virtual bool on_configure() = 0;
    virtual bool on_activate() = 0;
    virtual void on_deactivate() {}
    virtual void on_shutdown() {}

    std::string name_;
    NodeState state_ = NodeState::UNCONFIGURED;
    std::vector<std::string> dependencies_;
};

using NodePtr = std::shared_ptr<NodeBase>;

}
}
