#include "node_manager.h"
#include <chrono>

using namespace std::chrono_literals;


namespace mw {

NodeManager::NodeManager(const rclcpp::NodeOptions& options)
    : Node("node_manager", options) {
    this->declare_parameter("nodes_config", std::string(""));

    declare_nodes_from_config();

    monitor_timer_ = this->create_wall_timer(
        500ms, std::bind(&NodeManager::state_monitor_loop, this));

    RCLCPP_INFO(this->get_logger(), "NodeManager initialized with %zu nodes", nodes_.size());
}

void NodeManager::declare_nodes_from_config() {
    nodes_["camera_node"] = NodeInfo{
        "camera_node", {}, NodeState::UNKNOWN, "/camera_node/get_state"
    };
    nodes_["imu_node"] = NodeInfo{
        "imu_node", {}, NodeState::UNKNOWN, "/imu_node/get_state"
    };
    nodes_["chassis_node"] = NodeInfo{
        "chassis_node", {}, NodeState::UNKNOWN, "/chassis_node/get_state"
    };
    nodes_["perception_node"] = NodeInfo{
        "perception_node", {"camera_node", "imu_node"}, NodeState::UNKNOWN, "/perception_node/get_state"
    };
    nodes_["navigation_node"] = NodeInfo{
        "navigation_node", {"chassis_node", "imu_node"}, NodeState::UNKNOWN, "/navigation_node/get_state"
    };
    nodes_["fsm_node"] = NodeInfo{
        "fsm_node", {"perception_node", "navigation_node"}, NodeState::UNKNOWN, "/fsm_node/get_state"
    };

    for (auto& [name, info] : nodes_) {
        std::string srv_name = "/" + name + "/get_state";
        get_state_clients_[name] = this->create_client<lifecycle_msgs::srv::GetState>(srv_name);

        std::string change_srv_name = "/" + name + "/change_state";
        change_state_clients_[name] = this->create_client<lifecycle_msgs::srv::ChangeState>(change_srv_name);
    }
}

bool NodeManager::wait_for_node(const std::string& node_name, std::chrono::seconds timeout) {
    auto client = get_state_clients_[node_name];
    return client->wait_for_service(timeout);
}

uint8_t NodeManager::get_node_state_remote(const std::string& node_name) {
    auto client = get_state_clients_[node_name];
    if (!client->service_is_ready()) {
        return lifecycle_msgs::msg::State::PRIMARY_STATE_UNKNOWN;
    }

    auto request = std::make_shared<lifecycle_msgs::srv::GetState::Request>();
    auto result = client->async_send_request(request);
    
    if (rclcpp::spin_until_future_complete(this->shared_from_this(), result, 1s) ==
        rclcpp::FutureReturnCode::SUCCESS) {
        return result.get()->current_state.id;
    }
    return lifecycle_msgs::msg::State::PRIMARY_STATE_UNKNOWN;
}

bool NodeManager::change_node_state(const std::string& node_name, uint8_t transition) {
    auto client = change_state_clients_[node_name];
    if (!client->service_is_ready()) {
        return false;
    }

    auto request = std::make_shared<lifecycle_msgs::srv::ChangeState::Request>();
    request->transition.id = transition;
    auto result = client->async_send_request(request);

    return rclcpp::spin_until_future_complete(this->shared_from_this(), result, 5s) ==
        rclcpp::FutureReturnCode::SUCCESS;
}

bool NodeManager::all_dependencies_ready(const NodeInfo& node) {
    for (const auto& dep : node.dependencies) {
        auto it = nodes_.find(dep);
        if (it == nodes_.end()) return false;
        if (it->second.state != NodeState::ACTIVE) return false;
    }
    return true;
}

bool NodeManager::try_activate_node(NodeInfo& node) {
    if (node.state == NodeState::ACTIVE) return true;
    if (!all_dependencies_ready(node)) return false;

    uint8_t remote_state = get_node_state_remote(node.name);
    
    switch (remote_state) {
        case lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED:
            if (change_node_state(node.name, lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE)) {
                node.state = NodeState::INACTIVE;
                RCLCPP_INFO(this->get_logger(), "Configured node: %s", node.name.c_str());
            }
            break;
        case lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE:
            if (change_node_state(node.name, lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE)) {
                node.state = NodeState::ACTIVE;
                RCLCPP_INFO(this->get_logger(), "Activated node: %s", node.name.c_str());
            }
            break;
        case lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE:
            node.state = NodeState::ACTIVE;
            break;
        default:
            break;
    }
    
    return node.state == NodeState::ACTIVE;
}

void NodeManager::state_monitor_loop() {
    bool all_active = true;
    
    for (auto& [name, info] : nodes_) {
        if (info.state != NodeState::ACTIVE) {
            try_activate_node(info);
        }
        if (info.state != NodeState::ACTIVE) {
            all_active = false;
        }
    }

    if (all_active && !system_ready_) {
        system_ready_ = true;
        RCLCPP_INFO(this->get_logger(), "=====================================");
        RCLCPP_INFO(this->get_logger(), "ALL NODES ACTIVE - System is ready!");
        RCLCPP_INFO(this->get_logger(), "=====================================");
    }
}

bool NodeManager::start_all_nodes() {
    RCLCPP_INFO(this->get_logger(), "Starting all nodes...");
    return true;
}

bool NodeManager::stop_all_nodes() {
    RCLCPP_INFO(this->get_logger(), "Stopping all nodes...");
    return true;
}

NodeState NodeManager::get_node_state(const std::string& node_name) {
    auto it = nodes_.find(node_name);
    if (it != nodes_.end()) {
        return it->second.state;
    }
    return NodeState::UNKNOWN;
}

} // namespace mw

