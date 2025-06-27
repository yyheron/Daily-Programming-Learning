#pragma once
#include <string>

namespace zero_copy_ipc {

enum class Topic {
    CameraToRobot,
    LidarToRobot,
    RadarToRobot,
    // ... 其他 topic
};

inline const std::string& topic_to_string(Topic t) {
    static const std::string cameraToRobot = "cameraToRobot";
    static const std::string lidarToRobot = "lidarToRobot";
    static const std::string radarToRobot = "radarToRobot";
    // ... 其他映射
    switch (t) {
        case Topic::CameraToRobot: return cameraToRobot;
        case Topic::LidarToRobot:  return lidarToRobot;
        case Topic::RadarToRobot:  return radarToRobot;
        // ...
        default: 
            // 错误处理
            throw std::invalid_argument("Unknown Topic type");
    }
}

} // namespace zero_copy_ipc
