#include "legalizer.hpp"
#include "banjo/ssa/operand.hpp"

namespace banjo::passes {

Legalizer::Legalizer(target::Target *target) : Pass("legalizer", target) {}

void Legalizer::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void Legalizer::run(ssa::Function &func) {
    for (ssa::BasicBlockIter iter = func.begin(); iter != func.end(); ++iter) {
        run(func, iter);
    }
}

void Legalizer::run(ssa::Function &func, ssa::BasicBlockIter block) {
    // Insert 'trampolines' for conditional branch instructions with the same target but different
    // arguments because the backends can't generate code for them.

    ssa::InstrIter instr = block->get_exit_iter();

    if (instr->get_opcode() != ssa::Opcode::CJMP && instr->get_opcode() != ssa::Opcode::FCJMP) {
        return;
    }

    ssa::BranchTarget &true_target = instr->get_operand(3).get_branch_target();
    ssa::BranchTarget &false_target = instr->get_operand(4).get_branch_target();

    if (true_target.block != false_target.block) {
        return;
    }

    ssa::BasicBlockIter new_true_target = func.insert_after(block);
    ssa::BasicBlockIter new_false_target = func.insert_after(new_true_target);

    new_true_target->append({ssa::Opcode::JMP, {ssa::Operand::from_branch_target(true_target)}});
    new_false_target->append({ssa::Opcode::JMP, {ssa::Operand::from_branch_target(false_target)}});

    true_target = ssa::BranchTarget{.block = new_true_target};
    false_target = ssa::BranchTarget{.block = new_false_target};
}

} // namespace banjo::passes
