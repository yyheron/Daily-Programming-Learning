#include "srm/common/logging.h"
#include "srm/common/sleep_util.h"
#include "srm/dal/inproc_transport.h"
#include "srm/lifecycle/launcher.h"
#include "srm/data/data_player_node.h"
#include "srm/perception/gdc_process.h"
#include "srm/decision/fake_vla.h"
#include <memory>
#include <iostream>
static const char* TAG = "main_sim";

int main(int argc, char* argv[]) {
    std::string data_file = "sample_data.mcap";
    if (argc > 1) {
        data_file = argv[1];
    }

    SRM_LOG_INFO(TAG, "========================================");
    SRM_LOG_INFO(TAG, "  Service Robot Middleware - SIM MODE");
    SRM_LOG_INFO(TAG, "  (Data-driven simulation on x86)");
    SRM_LOG_INFO(TAG, "  Data file: " + data_file);
    SRM_LOG_INFO(TAG, "========================================");

    auto transport = std::make_shared<srm::dal::InprocTransport>();
    transport->init();

    srm::lifecycle::Launcher launcher(transport);

    launcher.register_factory("data::DataPlayerNode",
        [data_file](const std::string& name,
                    std::shared_ptr<srm::dal::IPubSubTransport> t) {
            auto node = std::make_shared<srm::data::DataPlayerNode>(name, t);
            node->set_data_file(data_file);
            node->set_speed(1.0f);
            return node;
        });

    launcher.register_factory("perception::GdcProcessNode",
        [](const std::string& name,
           std::shared_ptr<srm::dal::IPubSubTransport> t) {
            return std::make_shared<srm::perception::GdcProcessNode>(name, t);
        });

    launcher.register_factory("decision::FakeVlaNode",
        [](const std::string& name,
           std::shared_ptr<srm::dal::IPubSubTransport> t) {
            return std::make_shared<srm::decision::FakeVlaNode>(name, t);
        });

    launcher.add_node("data_player", "data::DataPlayerNode", {});
    launcher.add_node("gdc_process", "perception::GdcProcessNode", {"data_player"});
    launcher.add_node("fake_vla", "decision::FakeVlaNode", {"gdc_process"});

    if (!launcher.start_all()) {
        SRM_LOG_ERROR(TAG, "Failed to start all nodes!");
        return 1;
    }

    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "  NOTE: gdc_process and fake_vla use");
    SRM_LOG_INFO(TAG, "  the EXACT SAME code as deploy mode!");
    SRM_LOG_INFO(TAG, "  They have no idea they're running in sim.");
    SRM_LOG_INFO(TAG, "");

    auto player = launcher.get_node<srm::data::DataPlayerNode>("data_player");

    SRM_LOG_INFO(TAG, "=== Starting playback ===");

    int msg_count = 0;
    while (player && !player->is_finished()) {
        if (player->play_next()) {
            msg_count++;
            if (msg_count % 50 == 0) {
                SRM_LOG_INFO(TAG, "Played " + std::to_string(msg_count) + " messages...");
            }
        } else {
            srm::common::sleep_ms(1);
        }
    }

    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "Playback complete. Total messages: " + std::to_string(msg_count));

    launcher.stop_all();

    SRM_LOG_INFO(TAG, "========================================");
    SRM_LOG_INFO(TAG, "  Sim mode finished");
    SRM_LOG_INFO(TAG, "  Same code, different data source ✓");
    SRM_LOG_INFO(TAG, "========================================");

    return 0;
}
