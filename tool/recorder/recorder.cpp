#ifdef WIN32
#include "queue.hpp"
#include <windows.h>
#else
#include <boost/lockfree/spsc_queue.hpp>
#endif
#include <cassert>
#include <cmath>
#include <filesystem>
#include <thread>
#include <cstring>

#include "Logger.h"
#include "BinToText.h"
#include "BinDef.h"
#include "MapFileReader.h"
#include "getfile.h"

namespace fs = std::filesystem;

FILE *fout;

std::atomic_bool running;

#define OUTPUT_QUEUE_SIZE 20
#define OUTPUT_BUF_NUM OUTPUT_QUEUE_SIZE + 2
#define OUTPUT_BUF_SIZE (24 * 1024 * 1024)
char output_buf[OUTPUT_BUF_NUM][OUTPUT_BUF_SIZE];
int output_buf_len[OUTPUT_BUF_NUM];

#ifdef WIN32
ThreadSafeSPSCQueue<int> output_queue(OUTPUT_QUEUE_SIZE);
#else
boost::lockfree::spsc_queue<int, boost::lockfree::capacity<OUTPUT_QUEUE_SIZE>> output_queue;
#endif

#ifdef WIN32
void WriteOutputFile() {
    int buf_id;
    while(running || !output_queue.empty()) {
        if(output_queue.pop(buf_id)) {
            // LOGLINE("Popped output_buf_id: %d", buf_id);
            fwrite(output_buf[buf_id], output_buf_len[buf_id], 1, fout);
            fflush(fout);
        }
    }
}
#else
void WriteOutputFile() {
    bool found_data;
    int buf_id;
    int cnt = 0, tot = 0;
    while((found_data = output_queue.pop(buf_id)) || running) {
        ++cnt;
        if(!found_data) {
            fwrite(output_buf[buf_id], output_buf_len[buf_id], 1, fout);
            ++tot;
        }
    }
}
#endif

#define SPC (*s++) = ' ';

typedef std::pair<const char *, size_t> LogName;

LogName binNames[400];
size_t binDataSize[400];
void InitBinLogTypes() {
#define NAME(name, type)                                                 \
    binNames[BinType_##type] = LogName(" " #name " ", sizeof(#name) + 1); \
    binDataSize[BinType_##type] = sizeof(Bin##type##Data);
    NAME(lidar, LidarPointCloud)
    NAME(imu, LidarImu)
#undef NAME
}

void ProduceBinData(const char *buf, const char* buf_end) {
    InitBinLogTypes();
    int output_buf_id = 0;

    const auto buf_start = buf;

    const BinHeader *prevHeader = nullptr;

    while(buf < buf_end) {
        const auto &header = *(BinHeader *)(buf);
        // LOGLINE("Produced bin data: type=%d, timestamp=%llu, length=%u",
        //         header.type, header.timestamp, header.length);

        fflush(stdout);
        buf += sizeof(BinHeader);
        char *s = output_buf[output_buf_id];

        auto type = header.type;
        WriteUint32(header.timestamp, s);
        memcpy(s, binNames[type].first, binNames[type].second);
        s += binNames[type].second;

        switch(type) {
            case BinType_LidarPointCloud: {
                const auto &data = *(BinLidarPointCloudData *)buf;
                std::string frame_id(data.frame_id);

                // LOGLINE("point_num=%u, height=%u, width=%u, is_dense=%d, lidar_timestamp=%.3f, seq=%u, frame_id=%s",
                //         data.point_num, data.height, data.width, data.is_dense,
                //         data.lidar_timestamp, data.seq, frame_id.c_str());

                WriteUint32(data.point_num, s);
                SPC
                WriteUint32(data.height, s);
                SPC
                WriteUint32(data.width, s);
                SPC
                WriteUint8(data.is_dense, s);
                SPC
                WriteDecimal(data.lidar_timestamp, 3, s);
                SPC
                WriteUint32(data.seq, s);
                SPC
                WriteString(frame_id.c_str(), s);
                SPC
                (*s++) = '\n';

                uint32_t headerSize = sizeof(data.ap_timestamp) + sizeof(data.point_num)
                                    + sizeof(data.height) + sizeof(data.width) 
                                    + sizeof(data.is_dense) + sizeof(data.lidar_timestamp) 
                                    + sizeof(data.seq) + frame_id.size() + 1;
                const BinLidarPoint *f= (BinLidarPoint *)(buf + headerSize);
                for( int i = 0; i < data.point_num; ++i) {
                    // LOGLINE("Point %d: x=%.6f, y=%.6f, z=%.6f, intensity=%u, ring=%u, timestamp_offset=%.6f",
                    //         i, f[i].x, f[i].y, f[i].z,
                    //         f[i].intensity, f[i].ring, f[i].timestamp_offset);
                    WriteDecimal(f[i].x, 6, s);
                    SPC
                    WriteDecimal(f[i].y, 6, s);
                    SPC
                    WriteDecimal(f[i].z, 6, s);
                    SPC
                    WriteUint8(f[i].intensity, s);
                    SPC
                    WriteUint16(f[i].ring, s);
                    SPC
                    WriteDecimal(f[i].timestamp_offset, 6, s);
                    SPC
                    WriteUint8(f[i].tag, s);
                    SPC
                    (*s++) = '\n';
                }
                binDataSize[type] = header.length;
                break;
            }

            case BinType_LidarImu: {
                const auto &data = *(BinLidarImuData *)buf;

                const float *f = (const float *)buf;
                for(int i = 0; i < 10; ++i) {
                    WriteDecimal(f[i], 6, s);
                    SPC
                }

                binDataSize[type] = header.length;
                break;
            }
            default:
                LOGLINE("Unknown bin type %d", type);
                buf += header.length;
                break;
        }

        (*s++) = '\n';
        buf += binDataSize[type];
        output_buf_len[output_buf_id] = s - output_buf[output_buf_id];
        assert(output_buf_len[output_buf_id] < OUTPUT_BUF_SIZE);
        while (!output_queue.push(output_buf_id));

        if(++output_buf_id >= OUTPUT_BUF_NUM) {
            output_buf_id = 0;
        }
        prevHeader = &header;
    }
    LOGLINE("current Offset %zu, end Offset %zu",
                buf - buf_start, buf_end - buf_start);

    assert(buf == buf_end);
}

bool isValidPath(const fs::path &path, int id){
    std::string name = path.filename().string();

    std::string prefix = LidarPointPrefix;
    if(id == 11) {
        prefix = LidarPointPrefix;
    } else if(id == 12) {
        prefix = LidarImuPrefix;
    }

    LOGLINE("Checking file: %s with prefix: %s ", name.c_str(), prefix.c_str());

    return name.starts_with(prefix);
}

bool findFile(const fs::path &dir, fs::path &file, int id) {
    for(const auto &entry : fs::directory_iterator(".")) {
        if(!entry.is_regular_file() || !isValidPath(entry.path(), id)) continue;
        LOGLINE("Found file %s", entry.path().string().c_str());
        file = entry.path();
        return true;
    }
    LOGLINE("No file found in %s", dir.string().c_str());
    return false;
}

int main(int argc, char *argv[]) {
    fs::path dir;
    fs::path input;
    fs::path output;
    std::string name;

    for(int id = 11; id <= 12; ++id) {
        if(findFile(dir, input, id)) {
            MapFileReader reader(input);
            if(reader.data() == nullptr || reader.size() == 0) {
                LOGLINE("Failed to read file %s", input.string().c_str());
                return 1;
            }

            if(id == 11) {
                name = LidarPointName;
            } else if(id == 12) {
                name = LidarImuName;
            }

            output = fs::path(name);

            fout = fopen(output.string().c_str(), "wb");
            assert(fout != nullptr);

            running = true;
            std::thread outputThread(WriteOutputFile);

            ProduceBinData(reader.data(), reader.data() + reader.size());
            running = false;
            outputThread.join();

            fclose(fout);
        }
    }
    LOGLINE("All files processed successfully.");
    return 0;
}