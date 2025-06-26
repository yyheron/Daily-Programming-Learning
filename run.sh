#!/bin/bash
set -e

# --- 配置 ---
# Topic name 必须与 C++ 代码中的一致
TOPIC_NAME="cameraToRobot"
SHM_NAME="${TOPIC_NAME}_shm"
LOCK_NAME="${SHM_NAME}_pub_lock"

# --- 清理阶段 (扮演 Manager 的清理角色) ---
echo "Cleaning up previous resources for topic: ${TOPIC_NAME}..."
# This is a good practice for development to ensure a clean state
rm -f "/dev/shm/${SHM_NAME}"
rm -f "/dev/shm/sem.${LOCK_NAME}"

echo "Cleanup complete."


# --- 启动阶段 ---
echo "Starting subscriber in the background..."
# The subscriber will now wait patiently for the publisher to appear.
# Assuming executables are in ./build/
./build/example/sample_subscriber &
SUB_PID=$!

# Give a moment for the subscriber process to launch, though it will be in a wait loop.
sleep 0.5

echo "Starting publisher..."
./build/example/sample_publisher

# --- 结束阶段 ---
echo "Publisher finished. Shutting down subscriber..."
kill $SUB_PID
wait $SUB_PID 2>/dev/null

echo "All processes finished." 