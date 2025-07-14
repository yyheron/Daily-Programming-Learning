#!/bin/bash
echo start
# 读当前路径下的LIDAR_binId11.* 或LIDAR_binId12.* 文件，先逐个压缩为.xz，再并合并为一个tar文件
# 合并当前路径下的LIDAR_binId11.*的文件名为LIDAR_binId11.tar
# 合并当前路径下的LIDAR_binId12.*的文件名为LIDAR_binId12.tar
# 检查当前路径下是否有LIDAR_binId11.*文件
if ls LIDAR_binId11.* 1> /dev/null 2>&1; then
    # 如果有，逐个压缩为.xz
    for file in LIDAR_binId11.*; do
        xz -z "$file"
    done
    echo "LIDAR_binId11.* compressed successfully."
    # 如果有，合并为LIDAR_binId11.tar
    tar -czvf LIDAR_binId11.tar --transform='s,^./,,' --exclude=LIDAR_binId11.tar LIDAR_binId11.*.xz
    echo "LIDAR_binId11.tar created successfully."
else
    # 如果没有，输出提示信息
    echo "No LIDAR_binId11.* files found in the current directory."
fi
# 检查当前路径下是否有LIDAR_binId12.*文件
if ls LIDAR_binId12.* 1> /dev/null 2>&1; then
    # 如果有，逐个压缩为.xz
    for file in LIDAR_binId12.*; do
        xz -z "$file"
    done
    echo "LIDAR_binId12.* compressed successfully."
    # 如果有，合并为LIDAR_binId12.tar
    tar -czvf LIDAR_binId12.tar --transform='s,^./,,' --exclude=LIDAR_binId12.tar LIDAR_binId12.*.xz
    echo "LIDAR_binId12.tar created successfully."
else
    # 如果没有，输出提示信息
    echo "No LIDAR_binId12.* files found in the current directory."
fi
#删除中间.xz文件
rm LIDAR_binId11.*.xz
rm LIDAR_binId12.*.xz



# # 检查当前路径下是否有LIDAR_binId11.*文件
# if ls LIDAR_binId11.* 1> /dev/null 2>&1; then
#     # 如果有，逐个压缩为.tar.gz
#     for file in LIDAR_binId11.*; do
#         tar -czvf "${file}.tar.gz" "$file"
#     done
#     echo "LIDAR_binId11.* compressed successfully."
#     # 如果有，合并为LIDAR_binId11.tar
#     tar -czvf LIDAR_binId11.tar --transform='s,^./,,' --exclude=LIDAR_binId11.tar LIDAR_binId11.*.tar.gz
#     echo "LIDAR_binId11.tar created successfully."
# else
#     # 如果没有，输出提示信息
#     echo "No LIDAR_binId11.* files found in the current directory."
# fi
# # 检查当前路径下是否有LIDAR_binId12.*文件
# if ls LIDAR_binId12.* 1> /dev/null 2>&1; then
#     # 如果有，逐个压缩为.tar.gz
#     for file in LIDAR_binId12.*; do
#         tar -czvf "${file}.tar.gz" "$file"
#     done
#     echo "LIDAR_binId12.* compressed successfully."
#     # 如果有，合并为LIDAR_binId12.tar
#     tar -czvf LIDAR_binId12.tar --transform='s,^./,,' --exclude=LIDAR_binId12.tar LIDAR_binId12.*.tar.gz
#     echo "LIDAR_binId12.tar created successfully."
# else
#     # 如果没有，输出提示信息
#     echo "No LIDAR_binId12.* files found in the current directory."
# fi

# #删除中间.tar.gz文件
# # rm LIDAR_binId11.*.tar.gz
# # rm LIDAR_binId12.*.tar.gz

