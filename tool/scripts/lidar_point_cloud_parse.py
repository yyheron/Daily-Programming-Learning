# 激光雷达点云数据解析工具
# 功能：解析点云文件头部信息，提取时间戳并可视化
# 点云数据格式定义：
# 1. 点云数据头结构：
#    uint64_t timestamp_ms;  ///< 毫秒级时间戳
#    std::string lidar_type; ///< 雷达型号字符串
#    uint32_t point_num;   ///< 点云数据
#    uint32_t height;        ///< 点云高度
#    uint32_t width;         ///< 点云宽度
#    bool is_dense;          ///< 是否为密集点云
#    double timestamp;       ///< 高精度时间戳
#    uint32_t seq;           ///< 消息序列号
#    std::string frame_id;   ///< 坐标系ID
# 例如：545279 lidar 7321 0 0 0 545.279 5189 livox
# 其中lidar和livox都是固定的字符串
# 2. 点云数据体：紧跟头部后面的二进制点云数据
# 目前版本先跳过这些数据的解析
import matplotlib.pyplot as plt
import sys

def parse_lidar_log(log_file_path):
    """解析LIDAR日志文件，提取时间戳信息
    
    参数:
        log_file_path: 日志文件路径
    
    返回:
        timestamps: 时间戳列表
    """
    timestamps = []
    
    try:
        with open(log_file_path, 'r', encoding='utf-8') as file:
            while True:
                # 读取当前行（应该是包头）
                line = file.readline()
                if not line:  # 文件结束
                    break
                
                line = line.strip()
                if not line or line.startswith('#'):  # 跳过空行和注释行
                    continue
                
                # 分割行数据
                parts = line.split()
                if len(parts) < 3:
                    print(f"警告: 无效的包头格式: {line}")
                    continue
                
                # 提取时间戳和点云数量
                try:
                    timestamp = int(parts[0])  # 第一个字段是时间戳
                    point_num = int(parts[2])  # 第三个字段是点云数量
                    timestamps.append(timestamp)
                    
                    # 跳过point_num行数据
                    for _ in range(point_num):
                        # 读取但不处理数据行
                        file.readline()
                except ValueError as e:
                    print(f"警告: 解析数据失败: {e}，行内容: {line}")
                    continue
    
    except FileNotFoundError:
        print(f"错误: 找不到日志文件: {log_file_path}")
        sys.exit(1)
    except Exception as e:
        print(f"解析日志文件时出错: {str(e)}")
        sys.exit(1)
    
    return timestamps

def plot_timestamps(timestamps):
    if not timestamps:
        print("没有可绘制的时间戳数据")
        return
    indices = range(len(timestamps))
    plt.figure(figsize=(12, 6))
    plt.plot(indices, timestamps, linestyle='-', marker='o', markersize=3, color='b')
    plt.title('点云时间戳 vs 索引')
    plt.xlabel('索引 (Index)')
    plt.ylabel('时间戳 (毫秒)')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    log_file = "LIDAR_PointCloud_fprintf.log"  # 日志文件路径
    timestamps = parse_lidar_log(log_file)
    if timestamps:
        plot_timestamps(timestamps)