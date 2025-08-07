cmake_minimum_required(VERSION 3.1)
project(zero_copy_ipc_example CXX)

# 设置 C++ 标准并强制执行，这是解决问题的关键
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 推荐关闭编译器特定的扩展

# 查找 Boost 并指定所需组件
find_package(Boost REQUIRED COMPONENTS system thread chrono)

# 设置 include 目录，相对于此 CMakeLists.txt 的位置
include_directories(
    "${Boost_INCLUDE_DIRS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# 定义可执行文件，路径是相对于此 CMakeLists.txt
# add_executable(sample_publisher example/sample_publiser.cpp)
# add_executable(sample_subscriberA example/sample_subscriberA.cpp)
# add_executable(sample_subscriberB example/sample_subscriberB.cpp)
# add_executable(sample_delivery example/main.cpp)
add_executable(sample_callback example/callback.cpp)

# 链接库到目标
# 在 Linux 上，Boost.Interprocess 通常需要链接 rt 库
# 使用现代的 CMake 目标链接方式
# target_link_libraries(sample_publisher PRIVATE Boost::system Boost::thread Boost::chrono pthread rt)
# target_link_libraries(sample_subscriberA PRIVATE Boost::system Boost::thread Boost::chrono pthread rt)
# target_link_libraries(sample_subscriberB PRIVATE Boost::system Boost::thread Boost::chrono pthread rt)
# target_link_libraries(sample_delivery PRIVATE Boost::system Boost::thread Boost::chrono pthread rt)
target_link_libraries(sample_callback PRIVATE Boost::system Boost::thread Boost::chrono pthread rt)

# 将 run.sh 脚本复制到构建目录
# CMAKE_CURRENT_SOURCE_DIR 指向此 CMakeLists.txt 所在的目录
set(RUN_SCRIPT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/run.sh")
if(EXISTS ${RUN_SCRIPT_SOURCE})
    set(RUN_SCRIPT_DEST "${CMAKE_BINARY_DIR}/run.sh")
    file(COPY ${RUN_SCRIPT_SOURCE} DESTINATION ${CMAKE_BINARY_DIR})
    if(UNIX)
        execute_process(COMMAND chmod +x ${RUN_SCRIPT_DEST})
    endif()
endif()

