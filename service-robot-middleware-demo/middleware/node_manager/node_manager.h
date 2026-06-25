#pragma once

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <lifecycle_msgs/srv/get_state.hpp>
#include <lifecycle_msgs/srv/change_state.hpp>

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>


namespace mw {

enum class NodeState {
    UNCONFIGURED = 0,
    INACTIVE = 1,
    ACTIVE = 2,
    FINALIZED = 3,
    ERROR = 4,
    UNKNOWN = 99
};

struct NodeInfo {
    std::string name;
    std::vector<std::string> dependencies;
    NodeState state = NodeState::UNKNOWN;
    std::string lifecycle_node_name;
};

class NodeManager : public rclcpp::Node {
public:
    explicit NodeManager(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~NodeManager() override = default;

    bool start_all_nodes();
    bool stop_all_nodes();
    NodeState get_node_state(const std::string& node_name);

private:
    using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

    void declare_nodes_from_config();
    bool wait_for_node(const std::string& node_name, std::chrono::seconds timeout);
    bool change_node_state(const std::string& node_name, uint8_t transition);
    uint8_t get_node_state_remote(const std::string& node_name);

    void state_monitor_loop();
    bool all_dependencies_ready(const NodeInfo& node);
    bool try_activate_node(NodeInfo& node);

    std::map<std::string, NodeInfo> nodes_;
    rclcpp::TimerBase::SharedPtr monitor_timer_;

    std::map<std::string, 
        rclcpp::Client<lifecycle_msgs::srv::GetState>::SharedPtr> get_state_clients_;
    std::map<std::string,
        rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr> change_state_clients_;

    bool system_ready_ = false;
};

} // namespace mw

