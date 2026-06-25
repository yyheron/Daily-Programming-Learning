#include <rclcpp/rclcpp.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <rosbag2_cpp/converter_options.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "service_robot_middleware_demo/msg/perception_result.hpp"

namespace mw {

class DataRecorderNode : public rclcpp::Node {
public:
    explicit DataRecorderNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
        : Node("data_recorder_node", options) {
        this->declare_parameter("output_bag_path", "/tmp/srm_recording");
        this->declare_parameter("topics", std::vector<std::string>{
            "/camera/front/image_raw",
            "/imu/front/imu_data",
            "/perception/result"
        });
        this->declare_parameter("compression_mode", "file");   // "file" or "message"
        this->declare_parameter("compression_format", "zstd");
        this->declare_parameter("max_bag_size", 10 * 1024 * 1024 * 1024LL);  // 10GB
        this->declare_parameter("max_cache_size", 2LL * 1024 * 1024 * 1024LL);  // 2GB
        this->declare_parameter("storage_id", "mcap");
        this->declare_parameter("recording_mode", "continuous");  // continuous / triggered

        std::string bag_path = this->get_parameter("output_bag_path").as_string();
        auto topics = this->get_parameter("topics").as_string_array();
        std::string compression_mode = this->get_parameter("compression_mode").as_string();
        std::string compression_format = this->get_parameter("compression_format").as_string();
        int64_t max_bag_size = this->get_parameter("max_bag_size").as_int();
        int64_t max_cache_size = this->get_parameter("max_cache_size").as_int();
        std::string storage_id = this->get_parameter("storage_id").as_string();
        recording_mode_ = this->get_parameter("recording_mode").as_string();

        rosbag2_storage::StorageOptions storage_options;
        storage_options.uri = bag_path;
        storage_options.storage_id = storage_id;
        storage_options.max_bagfile_size = static_cast<uint64_t>(max_bag_size);
        storage_options.max_cache_size = static_cast<uint64_t>(max_cache_size);

        rosbag2_cpp::ConverterOptions converter_options;
        converter_options.input_serialization_format = "cdr";
        converter_options.output_serialization_format = "cdr";

        writer_ = std::make_unique<rosbag2_cpp::Writer>();
        writer_->open(storage_options, converter_options);

        // Subscribe to topics dynamically
        for (const auto& topic : topics) {
            subscribe_topic(topic);
        }

        // Triggered recording: service to start/stop
        if (recording_mode_ == "triggered") {
            trigger_service_ = this->create_service<std_srvs::srv::Trigger>(
                "/recorder/trigger",
                [this](const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
                       std::shared_ptr<std_srvs::srv::Trigger::Response> resp) {
                    (void)req;
                    recording_active_ = !recording_active_;
                    resp->success = true;
                    resp->message = recording_active_ ? "Recording started" : "Recording paused";
                    RCLCPP_INFO(this->get_logger(), "%s", resp->message.c_str());
                });
            recording_active_ = false;
        }

        RCLCPP_INFO(this->get_logger(),
            "DataRecorderNode started: mode=%s, bag=%s, storage=%s, compression=%s/%s",
            recording_mode_.c_str(), bag_path.c_str(), storage_id.c_str(),
            compression_mode.c_str(), compression_format.c_str());
    }

    ~DataRecorderNode() {
        if (writer_) {
            writer_->close();
        }
    }

private:
    void subscribe_topic(const std::string& topic) {
        // Generic subscription using type erasure
        auto sub = this->create_generic_subscription(
            topic, "*", rclcpp::SensorDataQoS(),
            [this, topic](std::shared_ptr<rclcpp::SerializedMessage> msg) {
                if (recording_mode_ == "triggered" && !recording_active_) {
                    return;
                }

                try {
                    writer_->write(msg, topic, this->now());
                    message_count_++;
                } catch (const std::exception& e) {
                    RCLCPP_ERROR(this->get_logger(),
                        "Failed to write message: %s", e.what());
                }
            });

        subscriptions_.push_back(std::move(sub));
        RCLCPP_INFO(this->get_logger(), "Subscribed to: %s", topic.c_str());
    }

    std::unique_ptr<rosbag2_cpp::Writer> writer_;
    std::vector<rclcpp::GenericSubscription::SharedPtr> subscriptions_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr trigger_service_;

    std::string recording_mode_ = "continuous";
    std::atomic<bool> recording_active_{true};
    std::atomic<size_t> message_count_{0};
};

} // namespace mw

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<mw::DataRecorderNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
