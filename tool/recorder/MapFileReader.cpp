#include "MapFileReader.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <thread>

#if WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#if !defined(__APPLE__)
#include <sys/sysinfo.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

MapFileReader::MapFileReader(const fs::path &p) : size_(0), data_(nullptr) {
    assert(fs::is_regular_file(p));
    size_ = fs::file_size(p);
#if WIN32
    HANDLE hFile = CreateFile(p.string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(hFile != INVALID_HANDLE_VALUE);
    HANDLE hMap = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    assert(hMap != nullptr);
    data_ = static_cast<char*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, size_));
    assert(data_ != nullptr);
    CloseHandle(hMap);
    CloseHandle(hFile);
#else
    int fd = open(p.c_str(), O_RDONLY);
    assert(fd != -1);
    struct stat st_buf;
    int ret = fstat(fd, &st_buf);
    assert(ret != -1);
    assert(st_buf.st_size == size_);
    data_ = (char*)(mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0));
    assert(data_ != MAP_FAILED);
    ret = madvise(data_, size_, MADV_SEQUENTIAL);
    assert(ret != -1);
    close(fd);
#endif
}

MapFileReader::~MapFileReader() {
    if(data_ == nullptr) return;
#if WIN32
    UnmapViewOfFile(data_);
#else
    munmap(data_, size_);
#endif
}

void MapFileReader::processText(const std::function<void(const Record &)> &cb, const std::vector<void*> &user_data) {
    size_t num_thread = user_data.empty() ? ncpu() : std::min(ncpu(), user_data.size());
    std::vector<size_t> parts = splitText(num_thread);
    if (parts.size() <= 1) return;

    num_thread = parts.size() - 1;
    std::vector<Record> records(num_thread);

    for (size_t i = 0; i < num_thread; ++i) {
        records[i].user_data = i < user_data.size() ? user_data[i] : nullptr;
    }

    std::vector<std::thread> threads(num_thread);
    for (size_t i = 0; i < num_thread; ++i) {
        threads[i] = std::thread(processTextThread, data_ + parts[i], parts[i+1] - parts[i], cb, std::ref(records[i]));
    }

    for(auto &thread : threads) {
        thread.join();
    }
}

void MapFileReader::processTextThread(const char *data, size_t size, const std::function<void(const Record &)> &cb, Record &record) {
    for (size_t start = 0, end = 0; start < size; start = ++end) {
        while (end < size && data[end] != '\n') {
            ++end;
        }
        size_t last = end;
        if (last > start && data[last - 1] == '\r') {
            --last;
        }
        record.data = const_cast<char*>(data + start);
        record.size = last - start;
        cb(record);
    }
}

std::vector<size_t> MapFileReader::splitText(size_t max_parts) {
    std::vector<size_t> parts;
    if (size_ == 0 || data_ == nullptr || max_parts == 0) {
        return parts;
    }

    size_t part_size = size_ / max_parts;
    size_t start_inc = max_parts - size_ % max_parts;
    size_t current_offset = 0;

    parts.resize(max_parts + 1);

    for (size_t i = 0; i < max_parts; ++i) {
        parts[i] = current_offset;
        if (i == start_inc) {
            part_size++;
        }
        current_offset += part_size;
    }
    parts[max_parts] = size_;

    for (size_t i = 1; i < max_parts; ++i) {
        size_t prev = std::max(parts[i - 1], parts[i] - 1);
        while (prev < size_ && data_[prev] != '\n') {
            ++prev;
        }
        if ((parts[i] = prev + 1) >= size_) {
            parts[i] = size_;
            parts.resize(i + 1);
            break;
        }
    }
    return parts;
}

size_t MapFileReader::ncpu() {
#if WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return get_nprocs();
#endif
}
