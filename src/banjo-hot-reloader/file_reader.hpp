#ifndef BANJO_HOT_RELOADER_FILE_READER_H
#define BANJO_HOT_RELOADER_FILE_READER_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace banjo {

namespace hot_reloader {

class FileReader {

private:
    std::ifstream stream;

public:
    FileReader(const std::string &path) : stream(path) {}

    std::uint64_t tell() { return stream.tellg(); }
    void seek(std::uint64_t offset) { stream.seekg(offset, std::ios::beg); }
    void skip(std::uint64_t size) { stream.seekg(size, std::ios::cur); }

    bool read(void *buffer, std::uint64_t size) {
        stream.read(reinterpret_cast<char *>(buffer), size);
        return stream.good();
    }

    template <typename T>
    T read() {
        T value;
        stream.read(reinterpret_cast<char *>(&value), sizeof(T));
        return value;
    }

    std::uint8_t read_u8() { return read<std::uint8_t>(); }
    std::uint16_t read_u16() { return read<std::uint16_t>(); }
    std::uint32_t read_u32() { return read<std::uint32_t>(); }
    std::uint64_t read_u64() { return read<std::uint64_t>(); }

    void close() { stream.close(); }
};

} // namespace hot_reloader

} // namespace banjo

#endif
