#ifndef PASSES_PASS_UTILS_H
#define PASSES_PASS_UTILS_H

#include "banjo/ir/control_flow_graph.hpp"
#include "banjo/ir/function.hpp"
#include "banjo/ir/instruction.hpp"
#include "banjo/ir/operand.hpp"
#include "banjo/ir/virtual_register.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace passes {

namespace PassUtils {

typedef std::unordered_map<ir::VirtualRegister, std::vector<ir::InstrIter>> UseMap;

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
UseMap collect_uses(ir::Function &func);

} // namespace PassUtils

} // namespace passes

} // namespace banjo

#endif
