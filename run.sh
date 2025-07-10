#!/bin/bash
set -e

# 解析命令行参数
if [ "$1" = "--inter-process" ] && [ -n "$2" ]; then
    TEST_TYPE="inter-process"
    MESSAGE_TYPE="$2"
    if [[ ! "$MESSAGE_TYPE" =~ ^(1K|1M|10M|20M)$ ]]; then
        echo "Invalid message type. Available message types: 1K, 1M, 10M, 20M"
        exit 1
    fi
elif [ "$1" = "--intra-process" ]; then
    TEST_TYPE="intra-process"
else
    echo "Usage: $0 [--inter-process <message_type>]|--intra-process"
    echo "Available message types: 1K, 1M, 10M, 20M"
    exit 1
fi

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

if [ "$TEST_TYPE" = "inter-process" ]; then
    # --- 启动阶段（进程间测试） ---

    echo "Starting subscriberA in the background with message type: ${MESSAGE_TYPE}..."
    ./sample_subscriberA "$MESSAGE_TYPE" &
    SUB_PID_A=$!

    echo "Starting subscriberB in the background with message type: ${MESSAGE_TYPE}..."
    ./sample_subscriberB "$MESSAGE_TYPE" &
    SUB_PID_B=$!
    
    sleep 2 # publisher后启动，先让subscriber等一会。确保所有消息能收到
    echo "Starting publisher in the background with message type: ${MESSAGE_TYPE}..."
    ./sample_publisher "$MESSAGE_TYPE" &
    PUB_PID=$!


    # 等 publisher 跑完
    wait $PUB_PID
    sleep 1

    # --- 结束阶段（进程间测试） ---
    echo "Publisher finished. Shutting down subscriberA..."
    kill -SIGTERM $SUB_PID_A
    sleep 3
    if kill -0 $SUB_PID_A 2>/dev/null; then
        echo "SubscriberA did not exit after SIGTERM, sending SIGKILL."
        kill -SIGKILL $SUB_PID_A
        wait $SUB_PID_A 2>/dev/null
    else
        wait $SUB_PID_A 2>/dev/null
    fi

    echo "SubscriberA finished. Shutting down subscriberB..."
    kill -SIGTERM $SUB_PID_B
    sleep 3
    if kill -0 $SUB_PID_B 2>/dev/null; then
        echo "SubscriberB did not exit after SIGTERM, sending SIGKILL."
        kill -SIGKILL $SUB_PID_B
        wait $SUB_PID_B 2>/dev/null
    else
        wait $SUB_PID_B 2>/dev/null
    fi

    echo "All processes finished."
elif [ "$TEST_TYPE" = "intra-process" ]; then
    # --- 进程内速度测试 ---
    echo "Starting intra-process speed test (take)..."
    ./sample_delivery take
    echo "Intra-process speed test (take) finished."
    echo "Starting intra-process speed test (batch, batch_size=8)..."
    ./sample_delivery batch 8
    echo "Intra-process speed test (batch) finished."
else 
    echo "Unknown test type. Please use [--inter-process <message_type>]|--intra-process."
fi