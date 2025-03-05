#ifndef BANJO_SSA_UTILS_H
#define BANJO_SSA_UTILS_H

#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/instruction.hpp"

namespace banjo {
namespace ssa {

ssa::FunctionType get_call_func_type(ssa::Instruction &call_instr);

} // namespace ssa
} // namespace banjo

#endif
