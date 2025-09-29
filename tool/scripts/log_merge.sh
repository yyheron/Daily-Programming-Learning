#!/bin/bash

# 设置包含压缩文件的目录
SOURCE_DIR=$(pwd)
if ! command -v zstdcat &> /dev/null; then
    echo "zstdcat is not available. Install zstd automatically." >&2
    if [[ -f "zstd-1.5.7.tar.gz" ]]; then
        tar -zxvf zstd-1.5.7.tar.gz
        cd zstd-1.5.7
        make -j$(nproc) PREFIX=$HOME/.local
        make install PREFIX=$HOME/.local
        export PATH=$HOME/.local/bin:$PATH
        cd ..
        echo "zstdcat installed successfully."  >&2
        rm -rf zstd-1.5.7
        if ! command -v zstdcat &> /dev/null; then
            echo "zstdcat install failed! Please install zstd manually." >&2
            exit 1
        fi
    else
        echo "zstd-1.5.7.tar.gz not found! Please copy it from share folder. Or download it or install zstd manually." >&2
        exit 1
    fi
fi

# 1. 合并所有 tar_name.*.tar 到 tar_name.tar
for FILE in "$SOURCE_DIR"/*.tar; do
    [[ -f "$FILE" ]] || continue
    base_name=$(basename "$FILE")
    # 跳过标准 tar_name.tar，只处理带后缀的
    dot_count=$(grep -o "\." <<< "$base_name" | wc -l)
    if (( dot_count >= 2 )); then
        prefix="${base_name%%.*}"
        target_tar="$SOURCE_DIR/$prefix.tar"
        if [[ "$FILE" != "$target_tar" ]]; then
            tmpdir=$(mktemp -d)
            tar -xf "$FILE" -C "$tmpdir"
            if [[ -f "$target_tar" ]]; then
                tar -xf "$target_tar" -C "$tmpdir"
            fi
            tar -cf "$target_tar.new" -C "$tmpdir" .
            mv "$target_tar.new" "$target_tar"
            rm -rf "$tmpdir"
            rm "$FILE"
            echo "Merged $base_name into $prefix.tar"
        fi
    fi
done

# 2. 合并所有 tar_name.*.{xz,zst,gz,lz4} 到 tar_name.tar
for comp_file in "$SOURCE_DIR"/*.{xz,zst,gz,lz4}; do
    [[ -f "$comp_file" ]] || continue
    comp_filename=$(basename "$comp_file")
    prefix="${comp_filename%%.*}"
    tar_file="$SOURCE_DIR/$prefix.tar"
    tmpdir=$(mktemp -d)
    # 如果已有 tar 包，先解包到临时目录
    if [[ -f "$tar_file" ]]; then
        tar -xf "$tar_file" -C "$tmpdir"
    fi
    # 把压缩文件复制到临时目录
    cp "$comp_file" "$tmpdir/"
    # 重新打包
    tar -cf "$tar_file.new" -C "$tmpdir" .
    mv "$tar_file.new" "$tar_file"
    rm -rf "$tmpdir"
    rm "$comp_file"
    echo "Merged $comp_filename into $tar_file"
done


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
     		if [[ "$FILE_2" == *.xz || "$FILE_2" == *.zst ]]; then
                if [[ "$FILE_2" == *topH*.xz || "$FILE_2" == *topH*.zst || "$FILE_2" == *topH*.gz || "$FILE_2" == *topH*.lz4 ]]; then
                    # 处理 topH*.xz 和 topH*.zst 文件
                    if [[ "$FILE_2" == *.xz ]]; then
                        content_cmd="xzcat"
                    elif [[ "$FILE_2" == *.zst ]]; then
                        content_cmd="zstdcat"
                    elif [[ "$FILE_2" == *.gzip ]]; then
                        content_cmd="zcat"
                    elif [[ "$FILE_2" == *.lz4 ]]; then
                        content_cmd="lz4cat"
                    fi
                    # 处理 topH*.xz 文件
                    datetime=$($content_cmd "$FILE_2" | grep -m1 -oP '==========\K\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}(?===========)')
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
                elif [[ "$FILE_2" == *top*.xz || "$FILE_2" == *top*.zst || "$FILE_2" == *top*.gz || "$FILE_2" == *top*.lz4 ]]; then
                    # 处理 top*.xz 和 top*.zst 文件
                    if [[ "$FILE_2" == *.xz ]]; then
                        content_cmd="xzcat"
                    elif [[ "$FILE_2" == *.zst ]]; then
                        content_cmd="zstdcat"
                    elif [[ "$FILE_2" == *.gzip ]]; then
                        content_cmd="zcat"
                    elif [[ "$FILE_2" == *.lz4 ]]; then
                        content_cmd="lz4cat"
                    fi
                    # 处理 top*.xz 文件
                    datetime=$($content_cmd "$FILE_2" | grep -m1 -oP '==========\K\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}(?===========)')
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
                if [[ "$FILE_2" == *bin*.xz || "$FILE_2" == *bin*.zst || "$FILE_2" == *bin*.gz || "$FILE_2" == *bin*.lz4 ]]; then #按照文件名序号排序
                    echo "0:$FILE_2" >> "$VALID_LIST"
                    continue
                fi
                # Step 1: 提取首行中的各个字段 timestamp是第3个字段
                if [[ "$FILE_2" == *.xz ]]; then
                    content_cmd="xzcat"
                elif [[ "$FILE_2" == *.zst ]]; then
                    content_cmd="zstdcat"
                elif [[ "$FILE_2" == *.gzip ]]; then
                    content_cmd="zcat"
                elif [[ "$FILE_2" == *.lz4 ]]; then
                    content_cmd="lz4cat"
                fi
                first_line=$($content_cmd "$FILE_2" | head -n1)
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
            if [[ "$invalid_file" == *.xz ]]; then
                xzcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.log.xz"
            elif [[ "$invalid_file" == *.zst ]]; then
                zstdcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.log.zst"
            elif [[ "$invalid_file" == *.gzip ]]; then
                zcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.log.gzip"
            elif [[ "$invalid_file" == *.lz4 ]]; then
                lz4cat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.log.lz4"
            fi
        done < "$INVALID_LIST"

        sort -n -t: -k1 "$VALID_LIST" | cut -d: -f2- | while read -r valid_file; do
            echo "$valid_file"
            if [[ "$valid_file" == *.xz ]]; then
                xzcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.log.xz"
            elif [[ "$valid_file" == *.zst ]]; then
                zstdcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.log.zst"
            elif [[ "$valid_file" == *.gzip ]]; then
                zcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.log.gzip"
            elif [[ "$valid_file" == *.lz4 ]]; then
                lz4cat "$valid_file" >> "$SOURCE_DIR/$FILENAME.log.lz4"
            fi
        done

        # 合并 top*.xz 无效时间戳文件
        while read -r invalid_file; do
            if [[ "$invalid_file" == *.xz ]]; then
                xzcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.top.log.xz"
            elif [[ "$invalid_file" == *.zst ]]; then
                zstdcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.top.log.zst"
            elif [[ "$invalid_file" == *.gzip ]]; then
                zcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.top.log.gzip"
            elif [[ "$invalid_file" == *.lz4 ]]; then
                lz4cat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.top.log.lz4"
            fi
        done < "$TOP_INVALID_LIST"

        # 合并 top*.xz 有效时间戳文件
        sort -n -t: -k1 "$TOP_VALID_LIST" | cut -d: -f2- | while read -r valid_file; do
            if [[ "$valid_file" == *.xz ]]; then
                xzcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.top.log.xz"
            elif [[ "$valid_file" == *.zst ]]; then
                zstdcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.top.log.zst"
            elif [[ "$valid_file" == *.gzip ]]; then
                zcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.top.log.gzip"
            elif [[ "$valid_file" == *.lz4 ]]; then
                lz4cat "$valid_file" >> "$SOURCE_DIR/$FILENAME.top.log.lz4"
            fi
        done

        # 合并 topH*.xz 无效时间戳文件
        while read -r invalid_file; do
            if [[ "$invalid_file" == *.xz ]]; then
                xzcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.xz"
            elif [[ "$invalid_file" == *.zst ]]; then
                zstdcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.zst"
            elif [[ "$invalid_file" == *.gzip ]]; then
                zcat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.gzip"
            elif [[ "$invalid_file" == *.lz4 ]]; then
                lz4cat "$invalid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.lz4"
            fi
        done < "$TOPH_INVALID_LIST"

        # 合并 topH*.xz 有效时间戳文件
        sort -n -t: -k1 "$TOPH_VALID_LIST" | cut -d: -f2- | while read -r valid_file; do
            if [[ "$valid_file" == *.xz ]]; then
                xzcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.xz"
            elif [[ "$valid_file" == *.zst ]]; then
                zstdcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.zst"
            elif [[ "$valid_file" == *.gzip ]]; then
                zcat "$valid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.gzip"
            elif [[ "$valid_file" == *.lz4 ]]; then
                lz4cat "$valid_file" >> "$SOURCE_DIR/$FILENAME.topH.log.lz4"
            fi
        done

        if [[ -f "$FILENAME.log.xz" ]]; then
     	    mv $FILENAME.log.xz $FILENAME.log
        fi
        if [[ -f "$FILENAME.top.log.xz" ]]; then
     	    mv $FILENAME.top.log.xz $FILENAME.top.log
        fi
        if [[ -f " $FILENAME.topH.log.xz" ]]; then
     	    mv $FILENAME.topH.log.xz $FILENAME.topH.log
        fi
        if [[ -f "$FILENAME.log.gzip" ]]; then
     	    mv $FILENAME.log.gzip $FILENAME.log
        fi
        if [[ -f "$FILENAME.top.log.gzip" ]]; then
     	    mv $FILENAME.top.log.gzip $FILENAME.top.log
        fi
        if [[ -f "$FILENAME.topH.log.gzip" ]]; then
     	    mv $FILENAME.topH.log.gzip $FILENAME.topH.log
        fi
        if [[ -f "$FILENAME.log.lz4" ]]; then
     	    mv $FILENAME.log.lz4 $FILENAME.log
        fi
        if [[ -f "$FILENAME.top.log.lz4" ]]; then
     	    mv $FILENAME.top.log.lz4 $FILENAME.top.log
        fi
        if [[ -f "$FILENAME.topH.log.lz4" ]]; then
     	    mv $FILENAME.topH.log.lz4 $FILENAME.topH.log
        fi
        if [[ -f "$FILENAME.log.zst" ]]; then
     	    mv $FILENAME.log.zst $FILENAME.log
        fi
        if [[ -f "$FILENAME.top.log.zst" ]]; then
     	    mv $FILENAME.top.log.zst $FILENAME.top.log
        fi
        if [[ -f "$FILENAME.topH.log.zst" ]]; then
     	    mv $FILENAME.topH.log.zst $FILENAME.topH.log
        fi

     	rm -rf $TARGET_DIR
     	rm -rf $TARGET_DIR.log.xz
        rm -rf $TARGET_DIR.log.zst
        rm -rf $TARGET_DIR.log.gzip
        rm -rf $TARGET_DIR.log.lz4
        
        rm -f "$INVALID_LIST" 
        rm -f "$VALID_LIST"
        rm -f "$TOP_INVALID_LIST"
        rm -f "$TOP_VALID_LIST"
        rm -f "$TOPH_INVALID_LIST"
        rm -f "$TOPH_VALID_LIST"
     	
        echo "Extracted '$FILE' to '$TARGET_DIR'"
    fi
done
