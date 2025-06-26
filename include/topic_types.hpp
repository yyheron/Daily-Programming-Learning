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
    static const std::string camera = "cameraToRobot";
    static const std::string lidar = "lidarToRobot";
    static const std::string radar = "radarToRobot";
    // ... 其他映射
    switch (t) {
        case Topic::Camera: return camera;
        case Topic::Lidar:  return lidar;
        case Topic::Radar:  return radar;
        // ...
        default: // 错误处理
    }
}

} // namespace zero_copy_ipc
