#include "srm/common/logging.h"
#include "srm/common/sleep_util.h"
#include "srm/dal/inproc_transport.h"
#include "srm/lifecycle/launcher.h"
#include "srm/hal/camera_node.h"
#include "srm/perception/gdc_process.h"
#include "srm/decision/fake_vla.h"
#include "srm/data/data_recorder.h"
#include <memory>

static const char* TAG = "main_deploy";

int main() {
    SRM_LOG_INFO(TAG, "========================================");
    SRM_LOG_INFO(TAG, "  Service Robot Middleware - DEPLOY MODE");
    SRM_LOG_INFO(TAG, "  (Simulated ARM deployment)");
    SRM_LOG_INFO(TAG, "========================================");

    auto transport = std::make_shared<srm::dal::InprocTransport>();
    transport->init();

    auto recorder = std::make_shared<srm::data::DataRecorder>();
    recorder->open("sample_data.mcap");
    recorder->add_topic("/camera/front", "Image");
    recorder->add_topic("/perception/detections", "Detection2DArray");
    recorder->add_topic("/decision/action", "Action");

    transport->set_pub_listener(
        [recorder](const std::string& topic,
                   const uint8_t* data, size_t size, uint64_t ts) {
            recorder->record_message(topic, data, size, ts);
        });

    srm::lifecycle::Launcher launcher(transport);

    launcher.register_factory("hal::CameraNode",
        [](const std::string& name,
           std::shared_ptr<srm::dal::IPubSubTransport> t) {
            return std::make_shared<srm::hal::CameraNode>(name, t);
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

    launcher.add_node("hal_camera", "hal::CameraNode", {});
    launcher.add_node("gdc_process", "perception::GdcProcessNode", {"hal_camera"});
    launcher.add_node("fake_vla", "decision::FakeVlaNode", {"gdc_process"});

    if (!launcher.start_all()) {
        SRM_LOG_ERROR(TAG, "Failed to start all nodes!");
        return 1;
    }

    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "=== Running camera stream for ~5 seconds ===");

    auto camera = launcher.get_node<srm::hal::CameraNode>("hal_camera");

    int total_frames = 150;
    for (int i = 0; i < total_frames; ++i) {
        if (camera) {
            camera->read_frame_and_publish();
        }

        srm::common::sleep_ms(33);
    }

    SRM_LOG_INFO(TAG, "");
    SRM_LOG_INFO(TAG, "=== Shutting down ===");

    launcher.stop_all();
    recorder->close();

    SRM_LOG_INFO(TAG, "========================================");
    SRM_LOG_INFO(TAG, "  Deploy mode finished");
    SRM_LOG_INFO(TAG, "  Recorded file: sample_data.mcap");
    SRM_LOG_INFO(TAG, "========================================");

    return 0;
}
