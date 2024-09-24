#include "write_buffer.hpp"

#include <cstring>

namespace banjo {

void WriteBuffer::write_u8(std::uint8_t u8) {
    ensure_size(position + 1);
    data[position] = u8;
    position += 1;
}

void WriteBuffer::write_i8(std::int8_t i8) {
    ensure_size(position + 1);
    data[position] = reinterpret_cast<std::uint8_t &>(i8);
    position += 1;
}

void WriteBuffer::write_u16(std::uint16_t u16) {
    ensure_size(position + 2);

    data[position + 0] = (std::uint8_t)(u16 >> 0);
    data[position + 1] = (std::uint8_t)(u16 >> 8);

    position += 2;
}

void WriteBuffer::write_i16(std::int32_t i16) {
    ensure_size(position + 2);

    std::uint16_t u16 = reinterpret_cast<std::uint16_t &>(i16);
    data[position + 0] = (std::uint8_t)(u16 >> 0);
    data[position + 1] = (std::uint8_t)(u16 >> 8);

    position += 2;
}

void WriteBuffer::write_u32(std::uint32_t u32) {
    ensure_size(position + 4);

    data[position + 0] = (std::uint8_t)(u32 >> 0);
    data[position + 1] = (std::uint8_t)(u32 >> 8);
    data[position + 2] = (std::uint8_t)(u32 >> 16);
    data[position + 3] = (std::uint8_t)(u32 >> 24);

    position += 4;
}

void WriteBuffer::write_i32(std::int32_t i32) {
    write_u32(reinterpret_cast<std::uint32_t &>(i32));
}

void WriteBuffer::write_u64(std::uint64_t u64) {
    ensure_size(position + 8);

    data[position + 0] = (std::uint8_t)(u64 >> 0);
    data[position + 1] = (std::uint8_t)(u64 >> 8);
    data[position + 2] = (std::uint8_t)(u64 >> 16);
    data[position + 3] = (std::uint8_t)(u64 >> 24);
    data[position + 4] = (std::uint8_t)(u64 >> 32);
    data[position + 5] = (std::uint8_t)(u64 >> 40);
    data[position + 6] = (std::uint8_t)(u64 >> 48);
    data[position + 7] = (std::uint8_t)(u64 >> 56);

    position += 8;
}

void WriteBuffer::write_i64(std::int64_t i64) {
    write_u64(reinterpret_cast<std::uint64_t &>(i64));
}

void WriteBuffer::write_f32(float f32) {
    ensure_size(position + 4);
    // TODO: this assumes that float is implemented as a standard 32-bit float
    std::memcpy(&data[position], &f32, 4);
    position += 4;
}

void WriteBuffer::write_f64(double f64) {
    ensure_size(position + 8);
    // TODO: this assumes that double is implemented as a standard 64-bit float
    std::memcpy(&data[position], &f64, 8);
    position += 8;
}

void WriteBuffer::write_data(const void *data, std::size_t size) {
    ensure_size(position + size);
    std::memcpy(&this->data[position], data, size);
    position += size;
}

void WriteBuffer::write_data(const WriteBuffer &buffer) {
    if (buffer.get_data().empty()) {
        return;
    }

    if (data.empty()) {
        data = buffer.get_data();
        position += buffer.get_size();
        return;
    }

    write_data((void *)buffer.get_data().data(), buffer.get_size());
}

void WriteBuffer::write_zeroes(std::size_t size) {
    ensure_size(data.size() + size);
    std::memset(&this->data[position], 0, size);
    position += size;
}

void WriteBuffer::write_cstr(const char *cstr) {
    for (const char *c = cstr; *c != '\0'; c++) {
        write_u8(*c);
    }
}

void WriteBuffer::ensure_size(std::uint32_t size) {
    if (data.size() < size) {
        data.resize(size);
    }
}

} // namespace banjo
