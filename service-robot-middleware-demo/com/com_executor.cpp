#include <rclcpp/rclcpp.hpp>
#include "camera/camera_get_data_node.hpp"
#include "camera/camera_process_data_node.hpp"
#include "imu/imu_get_data_node.hpp"
#include "imu/imu_process_data_node.hpp"
#include "chassis/chassis_get_data_node.hpp"
#include "chassis/chassis_process_data_node.hpp"
#include <thread>

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);

    rclcpp::NodeOptions options;
    options.node_base_name = "com_executor";

    // 1. 创建节点（构造函数只创建 Publisher/Subscriber）
    auto camera_get_data_node = rclcpp::make_shared<CameraGetDataNode>(options);
    auto camera_process_data_node = rclcpp::make_shared<CameraProcessDataNode>(options);
    // auto imu_get_data_node = rclcpp::make_shared<ImuGetDataNode>(options);
    // auto imu_process_data_node = rclcpp::make_shared<ImuProcessDataNode>(options);
    auto chassis_get_data_node = rclcpp::make_shared<ChassisGetDataNode>(options);
    auto chassis_process_data_node = rclcpp::make_shared<ChassisProcessDataNode>(options);

    // 2. 为每个节点创建独立的 executor 和线程
    auto exec_camera_get_data = rclcpp::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    auto exec_camera_process = rclcpp::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    // auto exec_imu_get_data = rclcpp::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    // auto exec_imu_process = rclcpp::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    auto exec_chassis_get_data = rclcpp::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    auto exec_chassis_process = rclcpp::make_shared<rclcpp::executors::SingleThreadedExecutor>();

    exec_camera_get_data->add_node(camera_get_data_node);
    exec_camera_process->add_node(camera_process_data_node);
    // exec_imu_get_data->add_node(imu_get_data_node);
    // exec_imu_process->add_node(imu_process_data_node);
    exec_chassis_get_data->add_node(chassis_get_data_node);
    exec_chassis_process->add_node(chassis_process_data_node);

    std::thread camera_get_data_thread([exec_camera_get_data]() { exec_camera_get_data->spin(); });
    std::thread camera_process_thread([exec_camera_process]() { exec_camera_process->spin(); });
    // std::thread imu_get_data_thread([exec_imu_get_data]() { exec_imu_get_data->spin(); });
    // std::thread imu_process_thread([exec_imu_process]() { exec_imu_process->spin(); });
    std::thread chassis_get_data_thread([exec_chassis_get_data]() { exec_chassis_get_data->spin(); });
    std::thread chassis_process_thread([exec_chassis_process]() { exec_chassis_process->spin(); });

    // 3. 启动 Driver Node 的 HAL 驱动（此时 GetData Node 已经在监听）
    camera_get_data_node->start();
    // imu_get_data_node->start();
    chassis_get_data_node->start();

    // 4. 等待进程结束
    camera_get_data_thread.join();
    camera_process_thread.join();
    // imu_get_data_thread.join();
    // imu_process_thread.join();
    chassis_get_data_thread.join();
    chassis_process_thread.join();

    rclcpp::shutdown();
    return 0;
}
