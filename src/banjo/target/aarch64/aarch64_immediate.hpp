#ifndef BANJO_TARGET_AARCH64_IMMEDIATE_H
#define BANJO_TARGET_AARCH64_IMMEDIATE_H

#include <cstdint>
#include <string>
#include <unordered_set>

namespace banjo {

namespace target {

namespace AArch64Immediate {

void decompose_u64_u16(std::uint64_t value, std::uint16_t *elements);
void decompose_u32_u16(std::uint32_t value, std::uint16_t *elements);

bool is_float_encodable(double value);
const std::unordered_set<std::uint64_t> &get_encodable_pos_floats();

} // namespace AArch64Immediate

} // namespace target

} // namespace banjo

#endif
