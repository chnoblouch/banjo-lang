#ifndef BANJO_SSA_UTILS_H
#define BANJO_SSA_UTILS_H

#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/instruction.hpp"

namespace banjo::ssa {

Type get_result_type(Instruction &instr, Type addr_type);
FunctionType get_call_func_type(Instruction &call_instr);

} // namespace banjo::ssa

#endif
