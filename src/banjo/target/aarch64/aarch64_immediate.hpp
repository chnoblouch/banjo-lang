#ifndef BANJO_TARGET_AARCH64_IMMEDIATE_H
#define BANJO_TARGET_AARCH64_IMMEDIATE_H

#include <array>
#include <cstdint>
#include <unordered_set>

namespace banjo::target {

namespace AArch64Immediate {

std::array<std::uint16_t, 4> decompose_u64_u16(std::uint64_t value);
std::array<std::uint16_t, 2> decompose_u32_u16(std::uint32_t value);

bool is_float_encodable(double value);
const std::unordered_set<std::uint64_t> &get_encodable_pos_floats();

} // namespace AArch64Immediate

} // namespace banjo::target

#endif
