#include "aarch64_immediate.hpp"

#include "banjo/utils/bit_operations.hpp"

#include <cmath>

namespace banjo {

namespace target {

static std::unordered_set<std::uint64_t> encodable_pos_floats;

void AArch64Immediate::decompose_u64_u16(std::uint64_t value, std::uint16_t *elements) {
    elements[0] = (value & 0x000000000000FFFF) >> 0;
    elements[1] = (value & 0x00000000FFFF0000) >> 16;
    elements[2] = (value & 0x0000FFFF00000000) >> 32;
    elements[3] = (value & 0xFFFF000000000000) >> 48;
}

void AArch64Immediate::decompose_u32_u16(std::uint32_t value, std::uint16_t *elements) {
    elements[0] = (value & 0x0000FFFF) >> 0;
    elements[1] = (value & 0xFFFF0000) >> 16;
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

} // namespace target

} // namespace banjo
