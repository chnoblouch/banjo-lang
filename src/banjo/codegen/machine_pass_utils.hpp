#ifndef BANJO_CODEGEN_MACHINE_PASS_UTILS_H
#define BANJO_CODEGEN_MACHINE_PASS_UTILS_H

#include "banjo/mcode/module.hpp"

#include <vector>

namespace banjo {

namespace codegen {

namespace MachinePassUtils {

void replace_virtual_reg(mcode::BasicBlock &basic_block, long old_register, long new_register);
void replace_reg(mcode::Instruction &instr, mcode::Register old_reg, mcode::Register new_reg);
void replace(mcode::BasicBlock &basic_block, mcode::Operand old_operand, mcode::Operand new_operand);
std::vector<long> get_modified_volatile_regs(mcode::Function *func);

} // namespace MachinePassUtils

} // namespace codegen

} // namespace banjo

#endif
