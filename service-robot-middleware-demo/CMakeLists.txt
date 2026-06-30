cmake_minimum_required(VERSION 3.8)
project(service_robot_middleware_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TARGET_SIM "Compile for simulation target (X86, single process)" OFF)

if(TARGET_SIM)
    message(STATUS "Compiling for SIMULATION target (X86, single process)")
    add_compile_definitions(TARGET_SIM)
else()
    message(STATUS "Compiling for NORMAL target (ARM, multi-process)")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rcutils REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(rosbag2_storage REQUIRED)
find_package(rosbag2_compression REQUIRED)
find_package(rosbag2_transport REQUIRED)
find_package(yaml_cpp_vendor REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(lifecycle_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/ObjectInfo.msg"
  "msg/PerceptionResult.msg"
  "msg/HalImage.msg"
  "msg/HalImu.msg"
  "msg/HalChassisState.msg"
  DEPENDENCIES std_msgs sensor_msgs geometry_msgs
)

include_directories(
  include
  .
)

set(dependencies
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  lifecycle_msgs
  rcutils
  sensor_msgs
  std_msgs
  geometry_msgs
  nav_msgs
  rosbag2_cpp
  rosbag2_storage
  rosbag2_compression
  rosbag2_transport
  yaml_cpp_vendor
)

if(NOT TARGET_SIM)
    # Normal mode: compile HAL library and all nodes
    add_library(dummy_hal SHARED
        dummy_hal/dummy_camera.cpp
        dummy_hal/chassis_interface.c
    )
    ament_target_dependencies(dummy_hal ${dependencies})
    target_include_directories(dummy_hal PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )

    # COM process: camera + chassis in one process
    add_executable(com_node
        com/com_node.cpp
        com/camera/camera_hal_if.cpp
        com/camera/camera_process.cpp
        com/chassis/chassis_hal_if.cpp
        com/chassis/chassis_process.cpp
    )
    ament_target_dependencies(com_node ${dependencies})
    target_link_libraries(com_node dummy_hal)
    rosidl_target_interfaces(com_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

    # perception_node: independent process
    add_executable(perception_node per/perception_node.cpp)
    ament_target_dependencies(perception_node ${dependencies})
    rosidl_target_interfaces(perception_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

    # navigation_node: independent process
    add_executable(navigation_node nav/navigation_node.cpp)
    ament_target_dependencies(navigation_node ${dependencies})
    rosidl_target_interfaces(navigation_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

    # DCAP: data recorder (only in normal mode)
    add_executable(data_recorder_node middleware/dcap/data_recorder_node.cpp)
    ament_target_dependencies(data_recorder_node ${dependencies})
    rosidl_target_interfaces(data_recorder_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

    # node_manager: commented out for now
    # add_executable(node_manager middleware/node_manager/node_manager.cpp)
    # ament_target_dependencies(node_manager ${dependencies})
    # rosidl_target_interfaces(node_manager ${PROJECT_NAME} "rosidl_typesupport_cpp")

    install(TARGETS
        com_node
        perception_node
        navigation_node
        data_recorder_node
        # node_manager  # commented out
        DESTINATION lib/${PROJECT_NAME}
    )
else()
    # Simulation mode: single process with all nodes
    add_executable(sim_node
        sim/sim_node.cpp
    )
    ament_target_dependencies(sim_node ${dependencies})
    rosidl_target_interfaces(sim_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

    # DREP for data playback
    add_executable(data_player_node middleware/drep/data_player_node.cpp)
    ament_target_dependencies(data_player_node ${dependencies})
    rosidl_target_interfaces(data_player_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

    install(TARGETS
        sim_node
        data_player_node
        DESTINATION lib/${PROJECT_NAME}
    )
endif()

install(DIRECTORY
    launch
    config
    DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY include/
    DESTINATION include
)

ament_export_include_directories(include)
ament_export_dependencies(rosidl_default_runtime)
ament_package()
