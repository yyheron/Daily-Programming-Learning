#include "Logger.h"
#include <cassert>  
#include <cmath>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

const std::string LidarPointPrefix = "LIDAR_binId11.log";
const std::string LidarPointName = "LIDAR_PointCLoud_fprintf.log";

const std::string LidarImuPrefix = "LIDAR_binId12.log";
const std::string LidarImuName = "LIDAR_Imu_fprintf.log";

