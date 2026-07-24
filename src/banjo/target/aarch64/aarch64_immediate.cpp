#include "aarch64_immediate.hpp"

#include "banjo/utils/bit_operations.hpp"

#include <cmath>

namespace banjo::target {

static std::unordered_set<std::uint64_t> encodable_pos_floats;

std::array<std::uint16_t, 4> AArch64Immediate::decompose_u64_u16(std::uint64_t value) {
    return {
        static_cast<std::uint16_t>((value & 0x000000000000FFFF) >> 0),
        static_cast<std::uint16_t>((value & 0x00000000FFFF0000) >> 16),
        static_cast<std::uint16_t>((value & 0x0000FFFF00000000) >> 32),
        static_cast<std::uint16_t>((value & 0xFFFF000000000000) >> 48),
    };
}

std::array<std::uint16_t, 2> AArch64Immediate::decompose_u32_u16(std::uint32_t value) {
    return {
        static_cast<std::uint16_t>((value & 0x0000FFFF) >> 0),
        static_cast<std::uint16_t>((value & 0xFFFF0000) >> 16),
    };
}

bool AArch64Immediate::is_float_encodable(double value) {
    double positive = std::fabs(value);
    std::uint64_t bits = BitOperations::get_bits_64(positive);
    return get_encodable_pos_floats().count(bits);
}

const std::unordered_set<std::uint64_t> &AArch64Immediate::get_encodable_pos_floats() {
    if (!encodable_pos_floats.empty()) {
        return encodable_pos_floats;
    }

    for (std::uint32_t encoding = 0; encoding < 128; encoding++) {
        std::uint64_t value = 0;
        value |= (encoding & 0x40) ? 0x3FC0000000000000 : 0x4000000000000000;
        value |= (encoding & 0x3Full) << 48;
        encodable_pos_floats.insert(value);
    }

    return encodable_pos_floats;
}

} // namespace banjo::target
