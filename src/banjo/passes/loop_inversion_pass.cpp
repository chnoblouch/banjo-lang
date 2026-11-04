#include "loop_inversion_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <utility>
#include <vector>

namespace banjo::passes {

LoopInversionPass::LoopInversionPass(target::Target *target) : Pass{"loop-inversion", target} {}

void LoopInversionPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void LoopInversionPass::run(ssa::Function &func) {
    bool changed = true;

    while (changed) {
        changed = false;

        ssa::ControlFlowGraph cfg = ssa::ControlFlowGraph::build(func);
        ssa::DominatorTree domtree = ssa::DominatorTree::build(cfg);
        ssa::LoopAnalyzer analyzer{cfg, domtree};
        std::vector<ssa::LoopAnalysis> loops = analyzer.analyze();

        for (const ssa::LoopAnalysis &loop : loops) {
            if (run(loop, cfg, func)) {
                changed = true;
                break;
            }
        }
    }
}

bool LoopInversionPass::run(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg, ssa::Function &func) {
    if (loop.exits.size() != 1 || loop.exits.begin()->from != loop.header) {
        return false;
    }

    canonicalize(loop, cfg);

    ssa::BasicBlockIter header = cfg.block(loop.header);
    ssa::BasicBlockIter tail = cfg.block(loop.tail);
    ssa::BasicBlockIter exit = cfg.block(loop.exits.begin()->to);

    // The header block must be the only predecessor of the the exit block.
    if (cfg.nodes[loop.exits.begin()->to].predecessors.size() != 1) {
        return false;
    }

    ssa::InstrIter cond_jump_iter = header->get_instrs().get_last_iter();
    ssa::InstrIter back_jump_iter = tail->get_instrs().get_last_iter();

    // Only invert loops where the tail branches unconditionally to the header.
    // Loops that branch conditionally to the header were probably already
    // inverted.
    if (back_jump_iter->get_opcode() != ssa::Opcode::JMP) {
        return false;
    }

    // Don't invert if there are other instructions in the header besides the
    // conditional branch.
    if (header->get_instrs().get_size() != 1) {
        return false;
    }

    if (is_logging()) {
        log() << func.name << ": inverting " << cfg.node_label(loop.header) << "\n";
    }

    // How this works: The header block becomes the guard block to check if the
    // loop is executed at least once. The block the old header branches to
    // becomes the new header block.

    ssa::BranchTarget &guard_enter_target = cond_jump_iter->get_operand(3).get_branch_target();
    ssa::BranchTarget &guard_exit_target = cond_jump_iter->get_operand(4).get_branch_target();

    for (unsigned i = 0; i < header->param_regs.size(); i++) {
        ssa::Type type = header->param_types[i];
        ssa::VirtualRegister arg_old_header = header->param_regs[i];
        ssa::VirtualRegister arg_guard = func.next_virtual_reg();
        ssa::VirtualRegister arg_header = func.next_virtual_reg();

        exit->param_regs.push_back(arg_old_header);
        exit->param_types.push_back(type);
        guard_enter_target.block->param_regs.push_back(arg_header);
        guard_enter_target.block->param_types.push_back(type);

        header->param_regs[i] = arg_guard;
        guard_enter_target.args.push_back(ssa::Operand::from_register(arg_guard, type));
        guard_exit_target.args.push_back(ssa::Operand::from_register(arg_guard, type));

        PassUtils::replace_in_block(*header, arg_old_header, ssa::Operand::from_register(arg_guard, type));

        for (ssa::ControlFlowGraph::NodeID body_node : loop.body) {
            ssa::BasicBlock &block = *cfg.block(body_node);
            PassUtils::replace_in_block(block, arg_old_header, ssa::Operand::from_register(arg_header, type));
        }
    }

    std::vector<ssa::Operand> back_args = back_jump_iter->get_operand(0).get_branch_target().args;
    *back_jump_iter = *cond_jump_iter;

    for (unsigned i = 0; i < back_args.size(); i++) {
        PassUtils::replace_in_instr(*back_jump_iter, header->param_regs[i], back_args[i]);
    }

    return true;
}

void LoopInversionPass::canonicalize(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg) {
    ssa::BasicBlockIter header_iter = cfg.block(loop.header);
    ssa::BasicBlockIter exit_iter = cfg.block(loop.exits.begin()->to);

    ssa::InstrIter cond_jump_iter = header_iter->get_instrs().get_last_iter();
    ssa::Operand &cmp_operand = cond_jump_iter->get_operand(1);
    ssa::BranchTarget &true_target = cond_jump_iter->get_operand(3).get_branch_target();
    ssa::BranchTarget &false_target = cond_jump_iter->get_operand(4).get_branch_target();

    // Make sure that the branch target if the condition is true is the entry
    // block of the loop.
    if (false_target.block != exit_iter) {
        cmp_operand.set_to_comparison(ssa::invert_comparison(cmp_operand.get_comparison()));
        std::swap(true_target, false_target);
    }
}

} // namespace banjo::passes
