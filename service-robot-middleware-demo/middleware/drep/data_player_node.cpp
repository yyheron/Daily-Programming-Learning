#include <rclcpp/rclcpp.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <rosbag2_cpp/converter_options.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <rosgraph_msgs/msg/clock.hpp>
#include <unordered_map>
#include <chrono>

namespace mw {

class DataPlayerNode : public rclcpp::Node {
public:
    explicit DataPlayerNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
        : Node("data_player_node", options) {
        this->declare_parameter("input_bag_path", "");
        this->declare_parameter("playback_speed", 1.0);
        this->declare_parameter("loop", false);
        this->declare_parameter("start_offset_ns", 0LL);
        this->declare_parameter("publish_clock", true);
        this->declare_parameter("wait_for_subscribers", true);

        std::string bag_path = this->get_parameter("input_bag_path").as_string();
        playback_speed_ = this->get_parameter("playback_speed").as_double();
        loop_ = this->get_parameter("loop").as_bool();
        start_offset_ns_ = this->get_parameter("start_offset_ns").as_int();
        bool publish_clock = this->get_parameter("publish_clock").as_bool();
        bool wait_for_subs = this->get_parameter("wait_for_subscribers").as_bool();

        if (bag_path.empty()) {
            RCLCPP_ERROR(this->get_logger(), "No input bag file specified!");
            return;
        }

        open_bag(bag_path);

        if (publish_clock) {
            clock_pub_ = this->create_publisher<rosgraph_msgs::msg::Clock>(
                "/clock", rclcpp::QoS(1));
        }

        // Wait for subscribers before starting playback
        if (wait_for_subs) {
            RCLCPP_INFO(this->get_logger(), "Waiting for subscribers...");
            rclcpp::sleep_for(std::chrono::seconds(2));
        }

        // Create publishers for each topic in bag
        for (const auto& topic : reader_->get_all_topics_and_types()) {
            auto pub = this->create_generic_publisher(
                topic.name, topic.type, rclcpp::SensorDataQoS());
            publishers_[topic.name] = std::move(pub);
            RCLCPP_INFO(this->get_logger(),
                "Publisher created: %s [%s]", topic.name.c_str(), topic.type.c_str());
        }

        // Playback timer
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(1),
            [this]() { playback_step(); });

        RCLCPP_INFO(this->get_logger(),
            "DataPlayerNode started: bag=%s, speed=%.1fx, messages=%zu",
            bag_path.c_str(), playback_speed_, total_messages_);
    }

private:
    void open_bag(const std::string& path) {
        rosbag2_storage::StorageOptions storage_options;
        storage_options.uri = path;
        storage_options.storage_id = "mcap";

        rosbag2_cpp::ConverterOptions converter_options;
        converter_options.input_serialization_format = "cdr";
        converter_options.output_serialization_format = "cdr";

        reader_ = std::make_unique<rosbag2_cpp::Reader>();
        reader_->open(storage_options, converter_options);

        // Count total messages
        total_messages_ = 0;
        while (reader_->has_next()) {
            reader_->read_next();
            total_messages_++;
        }

        // Reset reader
        reader_->close();
        reader_->open(storage_options, converter_options);

        // Skip to start offset
        if (start_offset_ns_ > 0) {
            auto target_time = reader_->get_metadata().starting_time +
                std::chrono::nanoseconds(start_offset_ns_);
            while (reader_->has_next()) {
                auto msg = reader_->read_next();
                if (msg->time_stamp >= target_time) {
                    // Put back by reopening... (simplified)
                    break;
                }
            }
        }
    }

    void playback_step() {
        if (!reader_->has_next()) {
            if (loop_) {
                RCLCPP_INFO(this->get_logger(), "Looping playback...");
                // Reopen bag for looping
                auto path = reader_->get_metadata().bag_size > 0 ? ""
                    : reader_->get_metadata().topics_with_message_count[0].topic_metadata.name;
                (void)path;
                // Simplified: just stop for now
                timer_->cancel();
                return;
            }
            timer_->cancel();
            RCLCPP_INFO(this->get_logger(), "Playback finished");
            return;
        }

        auto msg = reader_->read_next();

        auto now = std::chrono::steady_clock::now();
        if (base_time_.time_since_epoch().count() == 0) {
            base_time_ = now;
            bag_start_time_ = msg->time_stamp;
        }

        // Time synchronization
        auto bag_elapsed = std::chrono::duration<double>(
            msg->time_stamp - bag_start_time_).count();
        auto real_elapsed = std::chrono::duration<double>(
            now - base_time_).count() * playback_speed_;

        if (bag_elapsed > real_elapsed) {
            // Wait until it's time to publish
            return;
        }

        // Publish message
        auto it = publishers_.find(msg->topic_name);
        if (it != publishers_.end()) {
            it->second->publish(*msg->serialized_data);
        }

        // Publish /clock
        if (clock_pub_) {
            rosgraph_msgs::msg::Clock clock_msg;
            clock_msg.clock = rclcpp::Time(msg->time_stamp.time_since_epoch().count());
            clock_pub_->publish(clock_msg);
        }

        current_msg_++;
        if (current_msg_ % 100 == 0) {
            RCLCPP_DEBUG(this->get_logger(),
                "Playback progress: %zu / %zu", current_msg_, total_messages_);
        }
    }

    std::unique_ptr<rosbag2_cpp::Reader> reader_;
    std::unordered_map<std::string, rclcpp::GenericPublisher::SharedPtr> publishers_;
    rclcpp::Publisher<rosgraph_msgs::msg::Clock>::SharedPtr clock_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    double playback_speed_ = 1.0;
    bool loop_ = false;
    int64_t start_offset_ns_ = 0;

    std::chrono::steady_clock::time_point base_time_;
    std::chrono::time_point<std::chrono::high_resolution_clock> bag_start_time_;
    size_t total_messages_ = 0;
    size_t current_msg_ = 0;
};

} // namespace mw

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<mw::DataPlayerNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
