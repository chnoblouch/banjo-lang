#ifndef PASSES_PASS_UTILS_H
#define PASSES_PASS_UTILS_H

#include "ir/control_flow_graph.hpp"
#include "ir/function.hpp"
#include "ir/operand.hpp"
#include "ir/virtual_register.hpp"

#include <functional>

namespace passes {

namespace PassUtils {

void replace_in_func(ir::Function &func, ir::VirtualRegister old_register, ir::VirtualRegister new_register);
void replace_in_block(ir::BasicBlock &block, ir::VirtualRegister old_register, ir::VirtualRegister new_register);
void replace_in_func(ir::Function &func, ir::VirtualRegister reg, ir::Value value);
void replace_in_block(ir::BasicBlock &block, ir::VirtualRegister reg, ir::Value value);
void replace_in_range(ir::InstrIter start, ir::InstrIter end, ir::VirtualRegister reg, ir::Value value);
void replace_in_instr(ir::Instruction &instr, ir::VirtualRegister reg, ir::Value value);
bool is_arith_opcode(ir::Opcode opcode);
bool is_branch_opcode(ir::Opcode opcode);
void iter_values(std::vector<ir::Operand> &operands, std::function<void(ir::Value &value)> func);
void iter_regs(std::vector<ir::Operand> &operands, std::function<void(ir::VirtualRegister reg)> func);
void iter_imms(std::vector<ir::Operand> &operands, std::function<void(ir::Value &val)> func);
void replace_block(ir::Function *func, ir::ControlFlowGraph &cfg, unsigned node, unsigned replacement);

} // namespace PassUtils

} // namespace passes

#endif
