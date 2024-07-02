#include "loop_inversion_pass.hpp"

#include "ir/comparison.hpp"
#include "ir/operand.hpp"
#include "ir/virtual_register.hpp"
#include "passes/pass_utils.hpp"

#include <iostream>
#include <utility>

namespace banjo {

namespace passes {

LoopInversionPass::LoopInversionPass(target::Target *target) : Pass("loop-inversion", target) {
    enable_logging(std::cout);
}

void LoopInversionPass::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(func);
    }
}

void LoopInversionPass::run(ir::Function *func) {
    bool changed = true;

    while (changed) {
        changed = false;

        ir::ControlFlowGraph cfg(func);
        ir::DominatorTree domtree(cfg);
        ir::LoopAnalyzer analyzer(cfg, domtree);
        std::vector<ir::LoopAnalysis> loops = analyzer.analyze();

        if (is_logging()) {
            analyzer.dump(get_logging_stream());
        }

        for (const ir::LoopAnalysis &loop : loops) {
            if (run(loop, cfg, func)) {
                changed = true;
                break;
            }
        }
    }
}

bool LoopInversionPass::run(const ir::LoopAnalysis &loop, ir::ControlFlowGraph &cfg, ir::Function *func) {
    if (loop.exits.size() != 1 || loop.exits.begin()->from != loop.header) {
        return false;
    }

    canonicalize(loop, cfg);

    ir::BasicBlockIter header_iter = cfg.get_node(loop.header).block;
    ir::BasicBlock &tail = *cfg.get_node(loop.tail).block;

    ir::InstrIter cond_jump_iter = header_iter->get_instrs().get_last_iter();
    ir::InstrIter back_jump_iter = tail.get_instrs().get_last_iter();

    // Only invert loops where the tail branches unconditionally to the header.
    // Loops that branch conditionally to the header were probably already inverted.
    if (back_jump_iter->get_opcode() != ir::Opcode::JMP) {
        return false;
    }

    if (is_logging()) {
        const std::string &label = cfg.get_node(loop.header).block->get_debug_label();
        get_logging_stream() << func->get_name() << ": inverting " << label << "\n";
    }

    ir::BranchTarget true_target = cond_jump_iter->get_operand(3).get_branch_target();
    ir::BranchTarget false_target = cond_jump_iter->get_operand(4).get_branch_target();
    ir::BranchTarget back_target = back_jump_iter->get_operand(0).get_branch_target();

    // Copy the parameters of the header block to the entry block.
    true_target.block->get_param_regs() = header_iter->get_param_regs();
    true_target.block->get_param_types() = header_iter->get_param_types();

    ir::BranchTarget new_true_target{
        .block = true_target.block,
        .args = back_target.args,
    };

    // Update the branch instruction of the tail to jump to the entry instead of the header.
    ir::Instruction tail_cond_jump = *cond_jump_iter;
    tail_cond_jump.get_operand(3) = ir::Operand::from_branch_target(new_true_target);
    tail_cond_jump.get_operand(4) = ir::Operand::from_branch_target(false_target);
    tail.get_instrs().replace(back_jump_iter, tail_cond_jump);

    // Copy all instructions of the header into the tail block.
    for (ir::InstrIter iter = header_iter->begin(); iter != cond_jump_iter; ++iter) {
        tail.insert_before(tail.get_instrs().get_last_iter(), *iter);
    }

    // Replace the parameters of the header block because these registers are now the parameters of the entry block.
    for (unsigned i = 0; i < header_iter->get_param_regs().size(); i++) {
        ir::VirtualRegister old_reg = header_iter->get_param_regs()[i];
        ir::VirtualRegister new_reg = func->next_virtual_reg();

        header_iter->get_param_regs()[i] = new_reg;
        PassUtils::replace_in_block(*header_iter, old_reg, new_reg);

        // Pass the arguments of the header block when jumping to the entry block.
        ir::Value arg = ir::Value::from_register(new_reg, header_iter->get_param_types()[i]);
        cond_jump_iter->get_operand(3).get_branch_target().args.push_back(arg);
    }

    // Replace the registers in the header block because these registers are now defined in the tail block.
    for (ir::Instruction &instr : header_iter->get_instrs()) {
        if (instr.get_dest()) {
            PassUtils::replace_in_block(*header_iter, *instr.get_dest(), func->next_virtual_reg());
        }
    }

    return true;
}

void LoopInversionPass::canonicalize(const ir::LoopAnalysis &loop, ir::ControlFlowGraph &cfg) {
    ir::BasicBlockIter header_iter = cfg.get_node(loop.header).block;
    ir::BasicBlockIter exit_iter = cfg.get_node(loop.exits.begin()->to).block;

    ir::InstrIter cond_jump_iter = header_iter->get_instrs().get_last_iter();
    ir::Operand &cmp_operand = cond_jump_iter->get_operand(1);
    ir::BranchTarget &true_target = cond_jump_iter->get_operand(3).get_branch_target();
    ir::BranchTarget &false_target = cond_jump_iter->get_operand(4).get_branch_target();

    // Make sure that the branch target if the condition is true is the entry block of the loop.
    if (false_target.block != exit_iter) {
        cmp_operand.set_to_comparison(ir::invert_comparison(cmp_operand.get_comparison()));
        std::swap(true_target, false_target);
    }
}

} // namespace passes

} // namespace banjo
