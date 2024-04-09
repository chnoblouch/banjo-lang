#ifndef BIT_OPERATIONS_H
#define BIT_OPERATIONS_H

#include <cassert>
#include <cstdint>
#include <cstring>

typedef std::uint64_t Bits;

namespace BitOperations {

std::uint32_t transmute_s32_to_u32(std::int32_t value);
std::uint16_t get_upper_u32(std::uint32_t value);
std::uint16_t get_lower_u32(std::uint32_t value);

bool is_power_of_two(std::int64_t value);
unsigned get_first_bit_set(std::int64_t value);

template <typename T>
std::uint32_t get_bits_32(T value) {
    static_assert(sizeof(T) == 4, "type size is not 32 bits");

    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(std::uint32_t));
    return bits;
}

template <typename T>
std::uint64_t get_bits_64(T value) {
    static_assert(sizeof(T) == 8, "type size is not 64 bits");

    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof(std::uint64_t));
    return bits;
}

} // namespace BitOperations

#endif
