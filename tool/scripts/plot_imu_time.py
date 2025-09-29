import matplotlib.pyplot as plt
import numpy as np

def parse_log_file(filename):
    """解析日志文件，提取时间戳和索引"""
    timestamps = []
    indices = []
    
    with open(filename, 'r') as file:
        for index, line in enumerate(file):
            # 跳过空行
            if not line.strip():
                continue
                
            parts = line.split()
            # 确保行有足够的数据
            if len(parts) < 1:
                continue
                
            # 提取第一列的时间戳（毫秒）并转换为秒
            timestamp_ms = float(parts[0])
            timestamp_s = timestamp_ms / 1000.0
            
            timestamps.append(timestamp_s)
            indices.append(index)
    
    return indices, timestamps

def plot_trajectory(indices, timestamps):
    """绘制索引-时间戳轨迹图"""
    plt.figure(figsize=(10, 6))
    plt.plot(indices, timestamps, 'b-', linewidth=1)
    plt.xlabel('索引')
    plt.ylabel('时间 (秒)')
    plt.title('索引-时间戳轨迹图')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.show()

def main():
    # 输入文件名
    filename = input("请输入日志文件名: ")
    
    try:
        # 解析日志文件
        indices, timestamps = parse_log_file(filename)

        base_time_delta = timestamps[0]
        print("data start time is: ", base_time_delta)
        for t in timestamps:
            if t - base_time_delta > 0.1:
                print("breakpoint time is: ",t)
                break
            else:
                base_time_delta = t
        
        # 打印一些基本信息
        print(f"读取了 {len(indices)} 条数据")
        print(f"时间范围: {timestamps[0]:.3f}s 到 {timestamps[-1]:.3f}s")
        print(f"总时长: {timestamps[-1] - timestamps[0]:.3f}s")
        
        # 绘制轨迹图
        plot_trajectory(indices, timestamps)
        
    except FileNotFoundError:
        print(f"错误: 找不到文件 '{filename}'")
    except Exception as e:
        print(f"处理文件时出错: {e}")

if __name__ == "__main__":
    main()