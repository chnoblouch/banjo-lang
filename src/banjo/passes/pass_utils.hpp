#ifndef BANJO_PASSES_PASS_UTILS_H
#define BANJO_PASSES_PASS_UTILS_H

#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace passes {

namespace PassUtils {

typedef std::unordered_map<ssa::VirtualRegister, std::vector<ssa::InstrIter>> UseMap;

void replace_in_func(ssa::Function &func, ssa::VirtualRegister old_register, ssa::VirtualRegister new_register);
void replace_in_block(ssa::BasicBlock &block, ssa::VirtualRegister old_register, ssa::VirtualRegister new_register);
void replace_in_func(ssa::Function &func, ssa::VirtualRegister reg, ssa::Value value);
void replace_in_block(ssa::BasicBlock &block, ssa::VirtualRegister reg, ssa::Value value);
void replace_in_range(ssa::InstrIter start, ssa::InstrIter end, ssa::VirtualRegister reg, ssa::Value value);
void replace_in_instr(ssa::Instruction &instr, ssa::VirtualRegister reg, ssa::Value value);
bool is_arith_opcode(ssa::Opcode opcode);
bool is_branch_opcode(ssa::Opcode opcode);
void iter_values(std::vector<ssa::Operand> &operands, std::function<void(ssa::Value &value)> func);
void iter_regs(std::vector<ssa::Operand> &operands, std::function<void(ssa::VirtualRegister reg)> func);
void iter_imms(std::vector<ssa::Operand> &operands, std::function<void(ssa::Value &val)> func);
void replace_block(ssa::Function *func, ssa::ControlFlowGraph &cfg, unsigned node, unsigned replacement);
UseMap collect_uses(ssa::Function &func);

} // namespace PassUtils

} // namespace passes

} // namespace banjo

#endif
