#ifndef BANJO_MCODE_STACK_ADDRESS_H
#define BANJO_MCODE_STACK_ADDRESS_H

#include "banjo/mcode/stack_frame.hpp"

namespace banjo::mcode {

struct StackAddress {
    StackSlotID slot;
    unsigned offset;

    explicit StackAddress(StackSlotID slot) : slot{slot}, offset{0} {}
    StackAddress(StackSlotID slot, unsigned offset) : slot{slot}, offset{offset} {}

    friend bool operator==(const StackAddress &lhs, const StackAddress &rhs) = default;
    friend bool operator!=(const StackAddress &lhs, const StackAddress &rhs) = default;
};

} // namespace banjo::mcode

#endif
