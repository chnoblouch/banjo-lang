#ifndef BANJO_UTILS_WRITE_BUFFER_H
#define BANJO_UTILS_WRITE_BUFFER_H

#include <cstdint>
#include <utility>
#include <vector>

namespace banjo {

class WriteBuffer {

private:
    std::vector<std::uint8_t> data;
    std::uint32_t position = 0;

public:
    const std::vector<std::uint8_t> &get_data() const { return data; }
    std::vector<std::uint8_t> move_data() { return std::move(data); }
    std::size_t get_size() const { return data.size(); }

    void write_u8(std::uint8_t u8);
    void write_i8(std::int8_t i8);
    void write_u16(std::uint16_t u16);
    void write_i16(std::int32_t i16);
    void write_u32(std::uint32_t u32);
    void write_i32(std::int32_t i32);
    void write_u64(std::uint64_t u64);
    void write_i64(std::int64_t i64);
    void write_f32(float f32);
    void write_f64(double f64);
    void write_data(const void *data, std::size_t size);
    void write_data(const WriteBuffer &buffer);
    void write_zeroes(std::size_t size);
    void write_cstr(const char *cstr);
    void write_uleb128(std::uint64_t value);

    void read_data(void *data, std::size_t size);
    std::uint32_t read_u32();

    std::uint32_t tell() { return position; }
    void seek(std::uint32_t position) { this->position = position; }

private:
    void ensure_size(std::uint32_t size);
};

} // namespace banjo

#endif
