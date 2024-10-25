#include "pass_utils.hpp"

#include "banjo/ir/instruction.hpp"
#include "banjo/ir/virtual_register.hpp"

namespace banjo {

namespace passes {

using namespace ir;

void PassUtils::replace_in_func(
    ir::Function &func,
    ir::VirtualRegister old_register,
    ir::VirtualRegister new_register
) {
    for (ir::BasicBlock &block : func) {
        replace_in_block(block, old_register, new_register);
    }
}

void PassUtils::replace_in_block(ir::BasicBlock &block, VirtualRegister old_register, VirtualRegister new_register) {
    for (ir::Instruction &instr : block.get_instrs()) {
        if (instr.get_dest() && *instr.get_dest() == old_register) {
            instr.set_dest(new_register);
        }

        for (Operand &operand : instr.get_operands()) {
            if (operand.is_register() && operand.get_register() == old_register) {
                operand.set_to_register(new_register);
            }

            if (operand.is_branch_target()) {
                for (ir::Value &arg : operand.get_branch_target().args) {
                    if (arg.is_register(old_register)) {
                        arg.set_to_register(new_register);
                    }
                }
            }
        }
    }
}

void PassUtils::replace_in_func(ir::Function &func, ir::VirtualRegister reg, ir::Value value) {
    for (ir::BasicBlock &block : func) {
        replace_in_block(block, reg, value);
    }
}

void PassUtils::replace_in_block(ir::BasicBlock &block, VirtualRegister reg, Value value) {
    replace_in_range(block.begin(), block.end(), reg, std::move(value));
}

void PassUtils::replace_in_range(ir::InstrIter start, ir::InstrIter end, ir::VirtualRegister reg, ir::Value value) {
    for (ir::InstrIter iter = start; iter != end; ++iter) {
        replace_in_instr(*iter, reg, value);
    }
}

void PassUtils::replace_in_instr(ir::Instruction &instr, ir::VirtualRegister reg, ir::Value value) {
    for (Operand &operand : instr.get_operands()) {
        if (operand.is_register(reg)) {
            operand = value.with_type(operand.get_type());
        } else if (operand.is_branch_target()) {
            for (ir::Value &arg : operand.get_branch_target().args) {
                if (arg.is_register(reg)) {
                    arg = value.with_type(arg.get_type());
                }
            }
        }
    }
}

bool PassUtils::is_arith_opcode(ir::Opcode opcode) {
    return opcode == Opcode::ADD || opcode == Opcode::SUB || opcode == Opcode::MUL || opcode == Opcode::SDIV;
}

bool PassUtils::is_branch_opcode(ir::Opcode opcode) {
    return opcode == Opcode::JMP || opcode == Opcode::CJMP || opcode == Opcode::FCJMP;
}

void PassUtils::iter_values(std::vector<ir::Operand> &operands, std::function<void(ir::Value &value)> func) {
    for (ir::Operand &operand : operands) {
        func(operand);

        if (operand.is_branch_target()) {
            iter_values(operand.get_branch_target().args, func);
        }
    }
}

void PassUtils::iter_regs(std::vector<ir::Operand> &operands, std::function<void(ir::VirtualRegister reg)> func) {
    for (ir::Operand &operand : operands) {
        if (operand.is_register()) {
            func(operand.get_register());
        } else if (operand.is_branch_target()) {
            iter_regs(operand.get_branch_target().args, func);
        }
    }
}

void PassUtils::iter_imms(std::vector<ir::Operand> &operands, std::function<void(ir::Value &val)> func) {
    for (ir::Operand &operand : operands) {
        if (operand.is_int_immediate() || operand.is_fp_immediate()) {
            func(operand);
        } else if (operand.is_branch_target()) {
            iter_imms(operand.get_branch_target().args, func);
        }
    }
}

void PassUtils::replace_block(ir::Function *func, ir::ControlFlowGraph &cfg, unsigned node, unsigned replacement) {
    ir::BasicBlockIter node_iter = cfg.get_node(node).block;
    ir::BasicBlockIter replacement_iter = cfg.get_node(replacement).block;

    for (unsigned pred : cfg.get_node(node).predecessors) {
        ir::BasicBlock &pred_block = *cfg.get_node(pred).block;
        ir::Instruction &branch_instr = pred_block.get_instrs().get_last();

        for (ir::Operand &operand : branch_instr.get_operands()) {
            if (operand.is_branch_target() && operand.get_branch_target().block == node_iter) {
                operand.get_branch_target().block = replacement_iter;
            }
        }
    }
}

PassUtils::UseMap PassUtils::collect_uses(ir::Function &func) {
    UseMap uses;

    for (ir::BasicBlock &block : func) {
        for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
            iter_regs(iter->get_operands(), [&uses, iter](ir::VirtualRegister reg) { uses[reg].push_back(iter); });
        }
    }

    return uses;
}

} // namespace passes

} // namespace banjo
