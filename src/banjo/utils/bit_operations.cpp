#include "bit_operations.hpp"

std::uint32_t BitOperations::transmute_s32_to_u32(std::int32_t value) {
    union {
        std::int32_t s32;
        std::uint32_t u32;
    } value_union = {value};
    return value_union.u32;
}

std::uint16_t BitOperations::get_upper_u32(std::uint32_t value) {
    return value >> 16;
}

std::uint16_t BitOperations::get_lower_u32(std::uint32_t value) {
    return value & 0xFFFF;
}

bool BitOperations::is_power_of_two(std::int64_t value) {
    return (value & (value - 1)) == 0;
}

unsigned BitOperations::get_first_bit_set(std::int64_t value) {
    int shift = -1;
    while (value != 0) {
        value = value >> 1;
        shift += 1;
    }

    return shift;
}
