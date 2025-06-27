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
echo "Starting publisher in the background..."
./sample_publisher &
PUB_PID=$!


echo "Starting subscriberA in the background..."
# The subscriber will now wait patiently for the publisher to appear.
# Assuming executables are in ./build/
./sample_subscriberA &
SUB_PID_A=$!

echo "Starting subscriberB in the background..."
# The subscriber will now wait patiently for the publisher to appear.
# Assuming executables are in ./build/
./sample_subscriberB &
SUB_PID_B=$!

# 等 publisher 跑完
wait $PUB_PID
sleep 1

# --- 结束阶段 ---
echo "Publisher finished. Shutting down subscriberA..."
kill -SIGTERM $SUB_PID_A
sleep 1
wait $SUB_PID_A 2>/dev/null

echo "SubscriberA finished. Shutting down subscriberB..."
kill -SIGTERM $SUB_PID_B
sleep 1
wait $SUB_PID_B 2>/dev/null

echo "All processes finished." 