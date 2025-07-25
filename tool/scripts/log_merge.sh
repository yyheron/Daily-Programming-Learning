#!/bin/bash

# 设置包含压缩文件的目录
SOURCE_DIR=$(pwd)

# 遍历目录中的每个文件
for FILE in "$SOURCE_DIR"/*; do
    # 检查文件是否为压缩文件（这里以.tar.gz为例）
    if [[ "$FILE" == *.tar ]]; then
        # 获取文件名（不带路径和扩展名）
        FILENAME=$(basename "$FILE" .tar)
        
        # 创建解压目标目录
        TARGET_DIR="$SOURCE_DIR/$FILENAME"
           
        # 创建目录
        mkdir -p "$TARGET_DIR"
        
        # 解压文件到目标目录
        tar -xf "$FILE" -C "$TARGET_DIR"

        # 临时文件，存储timestamp的排序
        INVALID_LIST=$(mktemp)
        VALID_LIST=$(mktemp)
        TOP_INVALID_LIST=$(mktemp)
        TOP_VALID_LIST=$(mktemp)
        TOPH_INVALID_LIST=$(mktemp)
        TOPH_VALID_LIST=$(mktemp)
        
     	for FILE_2 in "$TARGET_DIR"/* "$TARGET_DIR"/mnt/data/rockrobo/devtest/*/*; do
     		if [[ "$FILE_2" == *.xz ]]; then
                if [[ "$FILE_2" == *topH*.xz ]]; then
                    # 处理 topH*.xz 文件
                    datetime=$(xzcat "$FILE_2" | grep -m1 -oP '==========\K\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}(?===========)')
                    echo "$datetime"
                    if [[ -n "$datetime" ]]; then
                        # 将日期时间转换为 Unix 时间戳（用于排序）
                        timestamp=$(date -d "$datetime" +%s 2>/dev/null)
                        if [[ -n "$timestamp" ]]; then
                            echo "$timestamp:$FILE_2" >> "$TOPH_VALID_LIST"
                        else
                            # 时间格式无效，放到最前面
                            echo "0:$FILE_2" >> "$TOPH_VALID_LIST"
                        fi
                    else
                        # 未找到时间戳，放到最前面
                        echo "0:$FILE_2" >> "$TOPH_VALID_LIST"
                    fi
                    continue
                elif [[ "$FILE_2" == *top*.xz ]]; then
                    # 处理 top*.xz 文件
                    datetime=$(xzcat "$FILE_2" | grep -m1 -oP '==========\K\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}(?===========)')
                    echo "$datetime"
                    if [[ -n "$datetime" ]]; then
                        # 将日期时间转换为 Unix 时间戳（用于排序）
                        timestamp=$(date -d "$datetime" +%s 2>/dev/null)
                        if [[ -n "$timestamp" ]]; then
                            echo "$timestamp:$FILE_2" >> "$TOP_VALID_LIST"
                        else
                            # 时间格式无效，放到最前面
                            echo "0:$FILE_2" >> "$TOP_VALID_LIST"
                        fi
                    else
                        # 未找到时间戳，放到最前面
                        echo "0:$FILE_2" >> "$TOP_VALID_LIST"
                    fi
                    continue
                fi
                if [[ "$FILE_2" == *bin*.xz ]]; then #按照文件名序号排序
                    echo "0:$FILE_2" >> "$VALID_LIST"
                    continue
                fi
                # Step 1: 提取首行中的各个字段 timestamp是第3个字段
                first_line=$(xzcat "$FILE_2" | head -n1)
                date_field=$(echo "$first_line" | awk '{print $1}')
                time_field=$(echo "$first_line" | awk '{print $2}')
                timestamp=$(echo "$first_line" | awk '{print $3}')

                # 判断日期和时间字段格式是否合法
                if [[ "$date_field" =~ ^[0-9]{4}/[0-9]{2}/[0-9]{2}$ ]] && \
                   [[ "$time_field" =~ ^[0-9]{2}:[0-9]{2}:[0-9]{2}$ ]]; then
                    # 格式合法：提取序号，记录到 VALID_LIST
                    echo "$timestamp:$FILE_2" >> "$VALID_LIST"
                else
                    # 格式非法：记录到 INVALID_LIST
                    echo "there is no timestamp logged for $FILE_2"
                    echo "$FILE_2" >> "$INVALID_LIST"
                fi

     			# cat $FILE_2 >> "$SOURCE_DIR/$FILENAME.log.xz"
     		 	#echo "Extracted '$FILE_2'"
     		fi
     	done

        # 先合并没有时间戳的，再合并有时间戳的
        while read -r invalid_file; do
            cat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.log.xz"
        done < "$INVALID_LIST"

        sort -n -t: -k1 "$VALID_LIST" | cut -d: -f2- | while read -r valid_file; do
            echo "$valid_file"
            cat "$valid_file" >> "$SOURCE_DIR/$FILENAME.log.xz"
        done

        # 合并 top*.xz 无效时间戳文件
        while read -r invalid_file; do
            cat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.top.log.xz"
        done < "$TOP_INVALID_LIST"

        # 合并 top*.xz 有效时间戳文件
        sort -n -t: -k1 "$TOP_VALID_LIST" | cut -d: -f2- | while read -r valid_file; do
            cat "$valid_file" >> "$SOURCE_DIR/$FILENAME.top.log.xz"
        done

        # 合并 topH*.xz 无效时间戳文件
        while read -r invalid_file; do
            cat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.xz"
        done < "$TOPH_INVALID_LIST"

        # 合并 topH*.xz 有效时间戳文件
        sort -n -t: -k1 "$TOPH_VALID_LIST" | cut -d: -f2- | while read -r valid_file; do
            cat "$valid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.xz"
        done

     	xz -d $FILENAME.log.xz
        xz -d $FILENAME.top.log.xz
        xz -d $FILENAME.topH.log.xz
     	
     	rm -rf $TARGET_DIR
     	rm -rf $TARGET_DIR.log.xz
        rm -f "$INVALID_LIST" 
        rm -f "$VALID_LIST"
        rm -f "$TOP_INVALID_LIST"
        rm -f "$TOP_VALID_LIST"
        rm -f "$TOPH_INVALID_LIST"
        rm -f "$TOPH_VALID_LIST"
     	
        echo "Extracted '$FILE' to '$TARGET_DIR'"
    fi

done
