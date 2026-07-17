#ifndef BANJO_TARGET_X86_64_ADDRESS_H
#define BANJO_TARGET_X86_64_ADDRESS_H

#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_frame.hpp"

#include <variant>

namespace banjo::target {

class X8664Address {

private:
    std::variant<mcode::Register, mcode::StackSlotID> base;
    std::variant<std::monostate, mcode::Register, int> offset;
    int scale;

public:
    X8664Address(std::variant<mcode::Register, mcode::StackSlotID> base) : base(base) {}

    X8664Address(std::variant<mcode::Register, mcode::StackSlotID> base, mcode::Register offset, int scale)
      : base(base),
        offset(offset),
        scale(scale) {}

    X8664Address(std::variant<mcode::Register, mcode::StackSlotID> base, int offset, int scale)
      : base(base),
        offset(offset),
        scale(scale) {}

    bool is_base_reg() const { return base.index() == 0; }
    bool is_base_stack_slot() const { return base.index() == 1; }

    mcode::Register get_base_reg() const { return std::get<0>(base); }
    mcode::StackSlotID get_base_stack_slot() const { return std::get<1>(base); }

    mcode::Register get_reg_offset() const { return std::get<1>(offset); }
    int get_int_offset() const { return std::get<2>(offset); }
    bool has_offset() const { return offset.index() != 0; }
    bool has_reg_offset() const { return offset.index() == 1; }
    bool has_int_offset() const { return offset.index() == 2; }
    int get_scale() const { return scale; }

    void set_base(mcode::Register base) { this->base = base; }
    void set_no_offset() { this->offset = std::monostate(); }
    void set_reg_offset(mcode::Register offset) { this->offset = offset; }
    void set_int_offset(int offset) { this->offset = offset; }
    void set_scale(int scale) { this->scale = scale; }

    friend bool operator==(const X8664Address &left, const X8664Address &right) {
        return left.base == right.base && left.offset == right.offset;
    }

    friend bool operator!=(const X8664Address &left, const X8664Address &right) { return !(left == right); }
};

} // namespace banjo::target

#endif
