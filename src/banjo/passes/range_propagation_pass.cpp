#include "range_propagation_pass.hpp"

#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include <optional>

namespace banjo::passes {

RangePropagationPass::RangePropagationPass(target::Target *target) : Pass{"range-propagation", target} {}

void RangePropagationPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void RangePropagationPass::run(ssa::Function &func) {
    cfg = ssa::ControlFlowGraph::build(func);
    dom_tree = ssa::DominatorTree::build(cfg);

    run(cfg.entry);
}

void RangePropagationPass::run(ssa::ControlFlowGraph::NodeID node) {
    ssa::Instruction &instr = *cfg.block(node)->get_exit_iter();

    if (instr.is_cond_branch()) {
        analyze_branch(node, instr);
    }

    for (ssa::ControlFlowGraph::NodeID child : dom_tree.nodes[node].children) {
        run(child);
    }
}

void RangePropagationPass::analyze_branch(ssa::ControlFlowGraph::NodeID node, ssa::Instruction &instr) {
    ssa::Operand &lhs = instr.get_operand(0);
    ssa::Comparison comparison = instr.get_operand(1).get_comparison();
    ssa::Operand &rhs = instr.get_operand(2);
    ssa::BranchTarget &true_target = instr.get_operand(3).get_branch_target();
    ssa::BranchTarget &false_target = instr.get_operand(4).get_branch_target();

    if (!lhs.is_register() || !rhs.is_register()) {
        return;
    }

    ssa::VirtualRegister lhs_reg = lhs.get_register();
    ssa::VirtualRegister rhs_reg = rhs.get_register();
    std::optional<ssa::Comparison> relationship = compute_relationship(node, lhs_reg, rhs_reg);

    if (!relationship) {
        return;
    }

    std::optional<bool> compute_result = try_compute_comparison(*relationship, comparison);

    if (compute_result) {
        ssa::BranchTarget target = *compute_result ? true_target : false_target;
        instr = {ssa::Opcode::JMP, {ssa::Operand::from_branch_target(target)}};
    }
}

std::optional<ssa::Comparison> RangePropagationPass::compute_relationship(
    ssa::ControlFlowGraph::NodeID node,
    ssa::VirtualRegister lhs,
    ssa::VirtualRegister rhs
) {
    while (true) {
        ssa::ControlFlowGraph::NodeID current = node;
        ssa::ControlFlowGraph::NodeID parent = dom_tree.nodes[node].parent;
        node = parent;

        if (parent == current) {
            break;
        }

        ssa::Instruction &instr = *cfg.block(parent)->get_exit_iter();

        if (!instr.is_cond_branch()) {
            continue;
        }

        ssa::Operand &branch_lhs = instr.get_operand(0);
        ssa::Comparison comparison = instr.get_operand(1).get_comparison();
        ssa::Operand &branch_rhs = instr.get_operand(2);
        ssa::BranchTarget &true_target = instr.get_operand(3).get_branch_target();
        ssa::BranchTarget &false_target = instr.get_operand(4).get_branch_target();

        if (!branch_lhs.is_register() || !branch_rhs.is_register()) {
            continue;
        }

        ssa::Comparison relationship;

        if (branch_lhs.get_register() == lhs && branch_rhs.get_register() == rhs) {
            relationship = comparison;
        } else if (branch_lhs.get_register() == rhs && branch_rhs.get_register() == lhs) {
            relationship = ssa::comparison_swapped(comparison);
        } else {
            continue;
        }

        if (cfg.node_id(true_target.block) == current) {
            return relationship;
        } else if (cfg.node_id(false_target.block) == current) {
            return ssa::invert_comparison(relationship);
        }
    }

    return {};
}

std::optional<bool> RangePropagationPass::try_compute_comparison(
    ssa::Comparison relationship,
    ssa::Comparison comparison
) {
    switch (relationship) {
        case ssa::Comparison::EQ:
            switch (comparison) {
                case ssa::Comparison::EQ: return true;
                case ssa::Comparison::NE: return false;
                case ssa::Comparison::UGT: return false;
                case ssa::Comparison::UGE: return true;
                case ssa::Comparison::ULT: return false;
                case ssa::Comparison::ULE: return true;
                case ssa::Comparison::SGT: return false;
                case ssa::Comparison::SGE: return true;
                case ssa::Comparison::SLT: return false;
                case ssa::Comparison::SLE: return true;
                default: return {};
            }

        case ssa::Comparison::NE:
            switch (comparison) {
                case ssa::Comparison::EQ: return false;
                case ssa::Comparison::NE: return true;
                default: return {};
            }

        case ssa::Comparison::UGT:
            switch (comparison) {
                case ssa::Comparison::EQ: return false;
                case ssa::Comparison::NE: return true;
                case ssa::Comparison::UGT: return true;
                case ssa::Comparison::UGE: return true;
                case ssa::Comparison::ULT: return false;
                case ssa::Comparison::ULE: return false;
                default: return {};
            }

        case ssa::Comparison::UGE:
            switch (comparison) {
                case ssa::Comparison::UGE: return true;
                case ssa::Comparison::ULT: return false;
                default: return {};
            }

        case ssa::Comparison::ULT:
            switch (comparison) {
                case ssa::Comparison::EQ: return false;
                case ssa::Comparison::NE: return true;
                case ssa::Comparison::UGT: return false;
                case ssa::Comparison::UGE: return false;
                case ssa::Comparison::ULT: return true;
                case ssa::Comparison::ULE: return true;
                default: return {};
            }

        case ssa::Comparison::ULE:
            switch (comparison) {
                case ssa::Comparison::UGT: return false;
                case ssa::Comparison::ULE: return true;
                default: return {};
            }

        default: return {};
    }

    if (relationship == comparison) {
        return true;
    } else {
        return {};
    }
}

} // namespace banjo::passes
