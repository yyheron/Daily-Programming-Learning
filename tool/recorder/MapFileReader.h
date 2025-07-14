#ifndef CPP_MAP_FILE_READER_H
#define CPP_MAP_FILE_READER_H
#include <cstddef>
#include <filesystem>
#include <functional>
#include <vector>

class MapFileReader {
    public:
    
    struct Record {
        void *data;
        size_t size;
        void *user_data;
    };

    explicit MapFileReader(const std::filesystem::path &p);
    ~MapFileReader();

    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] const char *data() const { return data_; }

    void processText(const std::function<void(const Record &)> &cb, const std::vector<void*> &user_data = std::vector<void*>());

    std::vector<size_t> splitText(size_t max_parts);

    static size_t ncpu();

    private:
    static void processTextThread(const char *data, size_t size, const std::function<void(const Record &)> &cb, Record &record);


    size_t size_;
    char *data_;
};

#endif // CPP_MAP_FILE_READER_H