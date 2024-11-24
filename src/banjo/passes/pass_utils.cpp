#include "pass_utils.hpp"

#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

namespace banjo {

namespace passes {

using namespace ssa;

void PassUtils::replace_in_func(
    ssa::Function &func,
    ssa::VirtualRegister old_register,
    ssa::VirtualRegister new_register
) {
    for (ssa::BasicBlock &block : func) {
        replace_in_block(block, old_register, new_register);
    }
}

void PassUtils::replace_in_block(ssa::BasicBlock &block, VirtualRegister old_register, VirtualRegister new_register) {
    for (ssa::Instruction &instr : block.get_instrs()) {
        if (instr.get_dest() && *instr.get_dest() == old_register) {
            instr.set_dest(new_register);
        }

        for (Operand &operand : instr.get_operands()) {
            if (operand.is_register() && operand.get_register() == old_register) {
                operand.set_to_register(new_register);
            }

            if (operand.is_branch_target()) {
                for (ssa::Value &arg : operand.get_branch_target().args) {
                    if (arg.is_register(old_register)) {
                        arg.set_to_register(new_register);
                    }
                }
            }
        }
    }
}

void PassUtils::replace_in_func(ssa::Function &func, ssa::VirtualRegister reg, ssa::Value value) {
    for (ssa::BasicBlock &block : func) {
        replace_in_block(block, reg, value);
    }
}

void PassUtils::replace_in_block(ssa::BasicBlock &block, VirtualRegister reg, Value value) {
    replace_in_range(block.begin(), block.end(), reg, std::move(value));
}

void PassUtils::replace_in_range(ssa::InstrIter start, ssa::InstrIter end, ssa::VirtualRegister reg, ssa::Value value) {
    for (ssa::InstrIter iter = start; iter != end; ++iter) {
        replace_in_instr(*iter, reg, value);
    }
}

void PassUtils::replace_in_instr(ssa::Instruction &instr, ssa::VirtualRegister reg, ssa::Value value) {
    for (Operand &operand : instr.get_operands()) {
        if (operand.is_register(reg)) {
            operand = value.with_type(operand.get_type());
        } else if (operand.is_branch_target()) {
            for (ssa::Value &arg : operand.get_branch_target().args) {
                if (arg.is_register(reg)) {
                    arg = value.with_type(arg.get_type());
                }
            }
        }
    }
}

bool PassUtils::is_arith_opcode(ssa::Opcode opcode) {
    return opcode == Opcode::ADD || opcode == Opcode::SUB || opcode == Opcode::MUL || opcode == Opcode::SDIV;
}

bool PassUtils::is_branch_opcode(ssa::Opcode opcode) {
    return opcode == Opcode::JMP || opcode == Opcode::CJMP || opcode == Opcode::FCJMP;
}

void PassUtils::iter_values(std::vector<ssa::Operand> &operands, std::function<void(ssa::Value &value)> func) {
    for (ssa::Operand &operand : operands) {
        func(operand);

        if (operand.is_branch_target()) {
            iter_values(operand.get_branch_target().args, func);
        }
    }
}

void PassUtils::iter_regs(std::vector<ssa::Operand> &operands, std::function<void(ssa::VirtualRegister reg)> func) {
    for (ssa::Operand &operand : operands) {
        if (operand.is_register()) {
            func(operand.get_register());
        } else if (operand.is_branch_target()) {
            iter_regs(operand.get_branch_target().args, func);
        }
    }
}

void PassUtils::iter_imms(std::vector<ssa::Operand> &operands, std::function<void(ssa::Value &val)> func) {
    for (ssa::Operand &operand : operands) {
        if (operand.is_int_immediate() || operand.is_fp_immediate()) {
            func(operand);
        } else if (operand.is_branch_target()) {
            iter_imms(operand.get_branch_target().args, func);
        }
    }
}

void PassUtils::replace_block(ssa::Function *func, ssa::ControlFlowGraph &cfg, unsigned node, unsigned replacement) {
    ssa::BasicBlockIter node_iter = cfg.get_node(node).block;
    ssa::BasicBlockIter replacement_iter = cfg.get_node(replacement).block;

    for (unsigned pred : cfg.get_node(node).predecessors) {
        ssa::BasicBlock &pred_block = *cfg.get_node(pred).block;
        ssa::Instruction &branch_instr = pred_block.get_instrs().get_last();

        for (ssa::Operand &operand : branch_instr.get_operands()) {
            if (operand.is_branch_target() && operand.get_branch_target().block == node_iter) {
                operand.get_branch_target().block = replacement_iter;
            }
        }
    }
}

PassUtils::UseMap PassUtils::collect_uses(ssa::Function &func) {
    UseMap uses;

    for (ssa::BasicBlock &block : func) {
        for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
            iter_regs(iter->get_operands(), [&uses, iter](ssa::VirtualRegister reg) { uses[reg].push_back(iter); });
        }
    }

    return uses;
}

} // namespace passes

} // namespace banjo
