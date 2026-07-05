#include "dead_code_elimination_pass.hpp"

#include "banjo/passes/analysis/stack_layout.hpp"
#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/dead_code_elimination.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

namespace banjo::passes {

DeadCodeEliminationPass::DeadCodeEliminationPass(target::Target *target)
  : Pass{"dead-code-elimination", target},
    stack_layout{*target} {}

void DeadCodeEliminationPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void DeadCodeEliminationPass::run(ssa::Function &func) {
    stack_layout.build(func);
    used_stack_slots = {};

    for (ssa::BasicBlock &block : func) {
        for (ssa::Instruction &instr : block) {
            analyze_stack_access(instr);
        }
    }

    for (ssa::BasicBlock &block : func) {
        for (ssa::InstrIter instr = block.begin(); instr != block.end(); ++instr) {
            bool eliminate = false;

            if (instr->get_opcode() == ssa::Opcode::STORE) {
                eliminate = can_eliminate_store(*instr);
            }

            if (eliminate) {
                ssa::InstrIter prev = instr.get_prev();
                block.remove(instr);
                instr = prev;
            }
        }
    }

    ssa::DeadCodeElimination{}.run(func);
}

void DeadCodeEliminationPass::analyze_stack_access(ssa::Instruction &instr) {
    ssa::Opcode opcode = instr.get_opcode();

    if (opcode == ssa::Opcode::STORE) {
        ssa::Operand &src = instr.get_operand(0);
        if (!src.is_register()) {
            return;
        }

        if (auto stack_slot = stack_layout.find_base_slot(src.get_register())) {
            used_stack_slots.set(*stack_slot);
        }
    } else {
        if (opcode == ssa::Opcode::MEMBERPTR || opcode == ssa::Opcode::OFFSETPTR) {
            return;
        }

        PassUtils::iter_regs(instr.get_operands(), [&](ssa::VirtualRegister reg) {
            if (auto stack_slot = stack_layout.find_base_slot(reg)) {
                used_stack_slots.set(*stack_slot);
            }
        });
    }
}

bool DeadCodeEliminationPass::can_eliminate_store(ssa::Instruction &instr) {
    ssa::Operand &addr = instr.get_operand(1);
    if (!addr.is_register()) {
        return false;
    }

    StackLayout::Member *stack_member = stack_layout.find_member(addr.get_register());
    if (!stack_member) {
        return false;
    }

    return !used_stack_slots.get(stack_member->slot);
}

} // namespace banjo::passes
