#ifndef MCODE_PARAMETER_H
#define MCODE_PARAMETER_H

#include "ir/type.hpp"
#include "mcode/register.hpp"

namespace banjo {

namespace mcode {

class Parameter {

public:
    Register src_reg;
    long stack_slot_index;

public:
    Parameter(Register src_reg, long stack_slot_index) : src_reg(src_reg), stack_slot_index(stack_slot_index) {}

    Register get_src_reg() { return src_reg; }
    long get_stack_slot_index() { return stack_slot_index; }
    ir::Type get_type() { return ir::Primitive::I32; }
};

} // namespace mcode

} // namespace banjo

#endif
