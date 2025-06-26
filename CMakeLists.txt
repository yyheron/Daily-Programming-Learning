cmake_minimum_required(VERSION 3.10)
project(zero_copy_ipc_example)

set(CMAKE_CXX_STANDARD 17)

# Boost 依赖
find_package(Boost REQUIRED COMPONENTS system thread)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

# 示例源文件
set(EXAMPLE_SRC_DIR ${CMAKE_SOURCE_DIR}/example)
add_executable(sample_publisher ${EXAMPLE_SRC_DIR}/sample_publiser.cpp)
add_executable(sample_subscriber ${EXAMPLE_SRC_DIR}/sample_subscriber.cpp)

target_link_libraries(sample_publisher PRIVATE ${Boost_LIBRARIES} pthread)
target_link_libraries(sample_subscriber PRIVATE ${Boost_LIBRARIES} pthread)

#安装run.sh到build/example文件夹下
set(RUN_SCRIPT_SOURCE "${CMAKE_SOURCE_DIR}/run.sh")
set(RUN_SCRIPT_DEST "${CMAKE_BINARY_DIR}/run.sh")
file(COPY ${RUN_SCRIPT_SOURCE} DESTINATION ${CMAKE_BINARY_DIR})
execute_process(COMMAND chmod +x ${RUN_SCRIPT_DEST})

