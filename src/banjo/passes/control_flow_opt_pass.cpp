#include "control_flow_opt_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

#define DEBUG_LOG is_logging() && log()

constexpr static unsigned NUM_ITERATIONS = 4;

namespace banjo::passes {

ControlFlowOptPass::ControlFlowOptPass(target::Target *target) : Pass("control-flow-opt", target) {}

void ControlFlowOptPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void ControlFlowOptPass::run(ssa::Function &func) {
    DEBUG_LOG << func.name << ": \n";

    this->func = &func;
    arg_blocks.clear();
    blocks_with_escaping_args.clear();

    for (ssa::BasicBlockIter block = func.begin(); block != func.end(); ++block) {
        for (ssa::VirtualRegister arg : block->get_param_regs()) {
            arg_blocks.emplace(arg, block);
        }
    }

    for (ssa::BasicBlockIter block = func.begin(); block != func.end(); ++block) {
        for (ssa::InstrIter instr = block->begin(); instr != block->end(); ++instr) {
            PassUtils::iter_regs(instr->get_operands(), [&](ssa::VirtualRegister reg) {
                auto iter = arg_blocks.find(reg);

                if (iter != arg_blocks.end() && iter->second != block) {
                    blocks_with_escaping_args.emplace(iter->second);
                }
            });
        }
    }

    if (!blocks_with_escaping_args.empty()) {
        DEBUG_LOG << "  blocks with escaping arguments:\n";

        for (auto iter = blocks_with_escaping_args.begin(); iter != blocks_with_escaping_args.end(); ++iter) {
            DEBUG_LOG << "    - " << (*iter)->get_debug_label() << "\n";
        }

        DEBUG_LOG << "\n";
    }

    for (unsigned i = 0; i < NUM_ITERATIONS; i++) {
        run_iteration(func);
    }

    DEBUG_LOG << "\n";
}

void ControlFlowOptPass::run_iteration(ssa::Function &func) {
    ssa::ControlFlowGraph cfg{&func};
    ssa::DominatorTree dom_tree{cfg};

    this->dom_tree = &dom_tree;

    for (ssa::BasicBlockIter block = func.begin(); block != func.end(); ++block) {
        ssa::Instruction &branch_instr = *block->get_exit_iter();

        if (branch_instr.get_opcode() == ssa::Opcode::JMP) {
            try_inline_into_jmp(block, branch_instr);
        } else if (branch_instr.is_cond_branch()) {
            ssa::BranchTarget &target_true = branch_instr.get_operand(3).get_branch_target();
            ssa::BranchTarget &target_false = branch_instr.get_operand(4).get_branch_target();

            try_inline_into_cjmp(block, target_true);
            try_inline_into_cjmp(block, target_false);
        }
    }

    cfg = ssa::ControlFlowGraph{&func};

    // Merge blocks into their predecessors if there is only one of them.
    for (ssa::ControlFlowGraph::Node &node : cfg.get_nodes()) {
        if (node.predecessors.size() != 1) {
            continue;
        }

        unsigned pred_index = node.predecessors[0];
        ssa::BasicBlock &pred = *cfg.get_nodes()[pred_index].block;
        ssa::InstrIter branch_instr = pred.get_exit_iter();

        // Only inline the block if the jump is unconditional.
        if (branch_instr->get_opcode() != ssa::Opcode::JMP) {
            continue;
        }

        const std::vector<ssa::Operand> &args = branch_instr->get_operand(0).get_branch_target().args;

        for (unsigned i = 0; i < node.block->get_param_regs().size(); i++) {
            ssa::VirtualRegister reg = node.block->get_param_regs()[i];
            ssa::Type type = node.block->get_param_types()[i];
            PassUtils::replace_in_func(func, reg, args[i].with_type(type));
        }

        pred.remove(branch_instr);

        for (ssa::Instruction &instr : node.block->get_instrs()) {
            pred.append(instr);
        }
    }

    cfg = ssa::ControlFlowGraph{&func};

    // Remove all blocks that are not in the control flow graph and hence unreachable.
    for (ssa::BasicBlockIter iter = func.begin(); iter != func.end(); ++iter) {
        if (!cfg.contains(iter)) {
            iter = iter.get_prev();
            func.get_basic_blocks().remove(iter.get_next());
        }
    }

    Precomputing::precompute_instrs(func);
}

void ControlFlowOptPass::try_inline_into_jmp(ssa::BasicBlockIter origin_block, ssa::Instruction &branch_instr) {
    ssa::BranchTarget &target = branch_instr.get_operand(0).get_branch_target();
    if (target.block->get_instrs().get_size() != 1) {
        return;
    }

    if (blocks_with_escaping_args.contains(target.block)) {
        DEBUG_LOG << "  cannot inline branch to " << target.block->get_debug_label();
        DEBUG_LOG << " into " << origin_block->get_debug_label();
        DEBUG_LOG << " (target block arguments escape)\n";
        return;
    }

    ssa::InstrIter target_branch_instr = target.block->get_exit_iter();

    if (target_branch_instr->get_opcode() == ssa::Opcode::JMP) {
        ssa::BranchTarget new_target = target_branch_instr->get_operand(0).get_branch_target();

        for (ssa::Operand &operand : new_target.args) {
            if (!try_replace_regs_with_block_args(operand, target, origin_block)) {
                DEBUG_LOG << "  cannot inline branch to " << target.block->get_debug_label();
                DEBUG_LOG << " into " << origin_block->get_debug_label();
                DEBUG_LOG << " (not all registers have known values)\n";
                return;
            }
        }

        DEBUG_LOG << "  inlining branch to " << target.block->get_debug_label();
        DEBUG_LOG << " into " << origin_block->get_debug_label();
        DEBUG_LOG << " (substituting unconditional branch)\n";

        target = new_target;
    } else if (target_branch_instr->is_cond_branch()) {
        ssa::Instruction new_branch_instr = *target_branch_instr;

        for (ssa::Operand &operand : new_branch_instr.get_operands()) {
            if (!try_replace_regs_with_block_args(operand, target, origin_block)) {
                DEBUG_LOG << "  cannot inline branch to " << target.block->get_debug_label();
                DEBUG_LOG << " into " << origin_block->get_debug_label();
                DEBUG_LOG << " (not all registers have known values)\n";
                return;
            }
        }

        DEBUG_LOG << "  inlining branch to " << target.block->get_debug_label();
        DEBUG_LOG << " into " << origin_block->get_debug_label();
        DEBUG_LOG << " (replacing unconditional branch with target conditional branch)\n";

        branch_instr = new_branch_instr;
    }
}

void ControlFlowOptPass::try_inline_into_cjmp(ssa::BasicBlockIter origin_block, ssa::BranchTarget &target) {
    if (target.block->get_instrs().get_size() != 1) {
        return;
    }

    if (blocks_with_escaping_args.contains(target.block)) {
        DEBUG_LOG << "  cannot inline branch to " << target.block->get_debug_label();
        DEBUG_LOG << " into " << origin_block->get_debug_label();
        DEBUG_LOG << " (target block arguments escape)\n";
        return;
    }

    ssa::Instruction &branch_instr = *target.block->get_exit_iter();

    if (branch_instr.get_opcode() == ssa::Opcode::JMP) {
        // If the target block contains an uncoditional branch, try to inline it into the
        // conditional branch.

        ssa::BranchTarget new_target = branch_instr.get_operand(0).get_branch_target();

        for (ssa::Operand &operand : new_target.args) {
            if (!try_replace_regs_with_block_args(operand, target, origin_block)) {
                DEBUG_LOG << "  cannot inline branch to " << target.block->get_debug_label();
                DEBUG_LOG << " into " << origin_block->get_debug_label();
                DEBUG_LOG << " (not all registers have known values)\n";
                return;
            }
        }

        DEBUG_LOG << "  inlining branch to " << target.block->get_debug_label();
        DEBUG_LOG << " into " << origin_block->get_debug_label();
        DEBUG_LOG << " (unconditional branch target into conditional branch arm)\n";

        target = new_target;
    } else if (branch_instr.is_cond_branch()) {
        // If the target block only contains a conditional branch instruction, try to precompute
        // which branch will be taken and inline it.

        ssa::Operand lhs = branch_instr.get_operand(0);
        ssa::Comparison comparison = branch_instr.get_operand(1).get_comparison();
        ssa::Operand rhs = branch_instr.get_operand(2);

        try_replace_regs_with_block_args(lhs, target, origin_block);
        try_replace_regs_with_block_args(rhs, target, origin_block);

        std::optional<bool> condition_value = Precomputing::try_precompute_cmp(lhs, rhs, comparison);
        if (!condition_value) {
            return;
        }

        ssa::BranchTarget new_target;

        if (*condition_value) {
            new_target = branch_instr.get_operand(3).get_branch_target();
        } else {
            new_target = branch_instr.get_operand(4).get_branch_target();
        }

        for (ssa::Value &arg : new_target.args) {
            if (!try_replace_regs_with_block_args(arg, target, origin_block)) {
                DEBUG_LOG << "  cannot inline branch to " << target.block->get_debug_label();
                DEBUG_LOG << " into " << origin_block->get_debug_label();
                DEBUG_LOG << " (not all registers in target have known values)\n";
                return;
            }
        }

        DEBUG_LOG << "  inlining branch to " << target.block->get_debug_label();
        DEBUG_LOG << " into " << origin_block->get_debug_label();
        DEBUG_LOG << " (condition is always " << ((*condition_value) ? "true" : "false") << ")\n";

        target = new_target;
    }
}

bool ControlFlowOptPass::try_replace_regs_with_block_args(
    ssa::Value &value,
    ssa::BranchTarget &target,
    ssa::BasicBlockIter block
) {
    if (value.is_register()) {
        ssa::VirtualRegister reg = value.get_register();

        for (unsigned i = 0; i < target.block->get_param_regs().size(); i++) {
            if (target.block->get_param_regs()[i] == reg) {
                value = target.args[i];
                return true;
            }
        }

        return is_reg_always_defined(reg, block);
    } else if (value.is_branch_target()) {
        for (ssa::Value &arg : value.get_branch_target().args) {
            if (!try_replace_regs_with_block_args(arg, target, block)) {
                return false;
            }
        }
    }

    return true;
}

bool ControlFlowOptPass::is_reg_always_defined(ssa::VirtualRegister reg, ssa::BasicBlockIter block) {
    ssa::BasicBlockIter def_block = PassUtils::find_def_block(*func, reg);

    if (!def_block) {
        return false;
    }

    return dom_tree->dominates(def_block, block);
}

} // namespace banjo::passes
