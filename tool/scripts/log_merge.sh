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
        
        INVALID_LIST=$(mktemp)
        VALID_LIST=$(mktemp)

     	for FILE_2 in "$TARGET_DIR"/* "$TARGET_DIR"/mnt/data/rockrobo/devtest/*/*; do
     		if [[ "$FILE_2" == *.xz ]]; then
                if [[ "$FILE_2" == *top*.xz ]]; then
                    datatime=$(xzcat "$FILE_2" | grep -m1 -oP '==========\K\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}(?===========)')
                    echo "$datatime"
                    if [[ "$datatime" ]]; then
                        timestamp=$(date -d "$datatime" 2>/dev/null)
                        if [[ -n "$timestamp" ]]; then
                            echo "$timestamp:$FILE_2" >> "$VALID_LIST"
                        else
                            echo "0:$FILE_2" >> "$VALID_LIST"
                        fi
                    else
                        echo "0:$FILE_2" >> "$VALID_LIST"
                    fi
                    continue
                fi

                first_line=$(xzcat "$FILE_2" | head -n 1)
                data_filed=$(echo "$first_line" | awk '{print $1}')
                time_filed=$(echo "$first_line" | awk '{print $2}')
                timestamp=$(echo "$time_filed" | awk '{print $3}')
            
                if [[ "$data_filed" =~ ^[0-9]{4}/[0-9]{2}/[0-9]{2}$ ]] && \
                   [[ "$time_filed" =~ ^[0-9]{2}:[0-9]{2}:[0-9]{2}$ ]]; then
                    echo "$timestamp:$FILE_2" >> "$VALID_LIST"
                else
                    echo "0:$FILE_2" >> "$VALID_LIST"
                fi
            fi
     	done

        while read -r invalid_file; do
            cat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.log.XZ"
        done < "$INVALID_LIST"
        
        sort -n -t: -k1 "$VALID_LIST" | cut -d: -f2- | while read -r valid_file; do
            cat "$valid_file" >> "$SOURCE_DIR/$FILENAME.log.XZ"
        done
     	
     	xz -d $FILENAME.log.xz
     	
     	rm -rf $TARGET_DIR
     	rm -rf $TARGET_DIR.log.xz
     	rm -f $VALID_LIST
        rm -f $INVALID_LIST

        echo "Extracted '$FILE' to '$TARGET_DIR'"
    fi

done
