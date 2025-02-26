#include "loop_inversion_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"


#include <iostream>
#include <utility>

namespace banjo {

namespace passes {

LoopInversionPass::LoopInversionPass(target::Target *target) : Pass("loop-inversion", target) {
    enable_logging(std::cout);
}

void LoopInversionPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func);
    }
}

void LoopInversionPass::run(ssa::Function *func) {
    bool changed = true;

    while (changed) {
        changed = false;

        ssa::ControlFlowGraph cfg(func);
        ssa::DominatorTree domtree(cfg);
        ssa::LoopAnalyzer analyzer(cfg, domtree);
        std::vector<ssa::LoopAnalysis> loops = analyzer.analyze();

        if (is_logging()) {
            analyzer.dump(get_logging_stream());
        }

        for (const ssa::LoopAnalysis &loop : loops) {
            if (run(loop, cfg, func)) {
                changed = true;
                break;
            }
        }
    }
}

bool LoopInversionPass::run(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg, ssa::Function *func) {
    if (loop.exits.size() != 1 || loop.exits.begin()->from != loop.header) {
        return false;
    }

    canonicalize(loop, cfg);

    ssa::BasicBlockIter header_iter = cfg.get_node(loop.header).block;
    ssa::BasicBlock &tail = *cfg.get_node(loop.tail).block;

    ssa::InstrIter cond_jump_iter = header_iter->get_instrs().get_last_iter();
    ssa::InstrIter back_jump_iter = tail.get_instrs().get_last_iter();

    // Only invert loops where the tail branches unconditionally to the header.
    // Loops that branch conditionally to the header were probably already inverted.
    if (back_jump_iter->get_opcode() != ssa::Opcode::JMP) {
        return false;
    }

    if (is_logging()) {
        const std::string &label = cfg.get_node(loop.header).block->get_debug_label();
        get_logging_stream() << func->name << ": inverting " << label << "\n";
    }

    ssa::BranchTarget true_target = cond_jump_iter->get_operand(3).get_branch_target();
    ssa::BranchTarget false_target = cond_jump_iter->get_operand(4).get_branch_target();
    ssa::BranchTarget back_target = back_jump_iter->get_operand(0).get_branch_target();

    // Copy the parameters of the header block to the entry block.
    true_target.block->get_param_regs() = header_iter->get_param_regs();
    true_target.block->get_param_types() = header_iter->get_param_types();

    ssa::BranchTarget new_true_target{
        .block = true_target.block,
        .args = back_target.args,
    };

    // Update the branch instruction of the tail to jump to the entry instead of the header.
    ssa::Instruction tail_cond_jump = *cond_jump_iter;
    tail_cond_jump.get_operand(3) = ssa::Operand::from_branch_target(new_true_target);
    tail_cond_jump.get_operand(4) = ssa::Operand::from_branch_target(false_target);
    tail.get_instrs().replace(back_jump_iter, tail_cond_jump);

    // Copy all instructions of the header into the tail block.
    for (ssa::InstrIter iter = header_iter->begin(); iter != cond_jump_iter; ++iter) {
        tail.insert_before(tail.get_instrs().get_last_iter(), *iter);
    }

    // Replace the parameters of the header block because these registers are now the parameters of the entry block.
    for (unsigned i = 0; i < header_iter->get_param_regs().size(); i++) {
        ssa::VirtualRegister old_reg = header_iter->get_param_regs()[i];
        ssa::VirtualRegister new_reg = func->next_virtual_reg();

        header_iter->get_param_regs()[i] = new_reg;
        PassUtils::replace_in_block(*header_iter, old_reg, new_reg);

        // Pass the arguments of the header block when jumping to the entry block.
        ssa::Value arg = ssa::Value::from_register(new_reg, header_iter->get_param_types()[i]);
        cond_jump_iter->get_operand(3).get_branch_target().args.push_back(arg);
    }

    // Replace the registers in the header block because these registers are now defined in the tail block.
    for (ssa::Instruction &instr : header_iter->get_instrs()) {
        if (instr.get_dest()) {
            PassUtils::replace_in_block(*header_iter, *instr.get_dest(), func->next_virtual_reg());
        }
    }

    return true;
}

void LoopInversionPass::canonicalize(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg) {
    ssa::BasicBlockIter header_iter = cfg.get_node(loop.header).block;
    ssa::BasicBlockIter exit_iter = cfg.get_node(loop.exits.begin()->to).block;

    ssa::InstrIter cond_jump_iter = header_iter->get_instrs().get_last_iter();
    ssa::Operand &cmp_operand = cond_jump_iter->get_operand(1);
    ssa::BranchTarget &true_target = cond_jump_iter->get_operand(3).get_branch_target();
    ssa::BranchTarget &false_target = cond_jump_iter->get_operand(4).get_branch_target();

    // Make sure that the branch target if the condition is true is the entry block of the loop.
    if (false_target.block != exit_iter) {
        cmp_operand.set_to_comparison(ssa::invert_comparison(cmp_operand.get_comparison()));
        std::swap(true_target, false_target);
    }
}

} // namespace passes

} // namespace banjo
