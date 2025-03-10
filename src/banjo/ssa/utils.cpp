#include "utils.hpp"

namespace banjo {
namespace ssa {

ssa::FunctionType get_call_func_type(ssa::Instruction &call_instr) {
    ssa::FunctionType type{
        .params = std::vector<ssa::Type>{call_instr.get_operands().size() - 1},
        .return_type = call_instr.get_operand(0).get_type(),
        .calling_conv = {},
        .variadic = false,
        .first_variadic_index = 0,
    };

    for (unsigned i = 1; i < call_instr.get_operands().size(); i++) {
        type.params[i - 1] = call_instr.get_operand(i).get_type();
    }

    if (call_instr.get_attr() == ssa::Instruction::Attribute::VARIADIC) {
        type.variadic = true;
        type.first_variadic_index = call_instr.get_attr_data();
    }

    return type;
}

} // namespace ssa
} // namespace banjo
