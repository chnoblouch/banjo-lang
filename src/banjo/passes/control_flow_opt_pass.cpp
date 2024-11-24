#include "control_flow_opt_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"

#include <unordered_set>

namespace banjo {

namespace passes {

ControlFlowOptPass::ControlFlowOptPass(target::Target *target) : Pass("control-flow-opt", target) {}

void ControlFlowOptPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        for (unsigned i = 0; i < 4; i++) {
            run(func);
        }
    }
}

void ControlFlowOptPass::run(ssa::Function *func) {
    optimize_blocks(*func);
}

void ControlFlowOptPass::optimize_blocks(ssa::Function &func) {
    ssa::ControlFlowGraph cfg(&func);

    // Merge blocks into their predecessors if there is only one of them.
    for (ssa::ControlFlowGraph::Node &node : cfg.get_nodes()) {
        if (node.predecessors.size() == 1) {
            unsigned pred_index = node.predecessors[0];
            ssa::BasicBlock &pred = *cfg.get_nodes()[pred_index].block;
            ssa::InstrIter branch_instr = pred.get_instrs().get_last_iter();

            // Only inline the block if the jump is unconditional.
            if (branch_instr->get_opcode() != ssa::Opcode::JMP) {
                continue;
            }

            const std::vector<ssa::Operand> &args = branch_instr->get_operand(0).get_branch_target().args;

            for (unsigned i = 0; i < node.block->get_param_regs().size(); i++) {
                ssa::VirtualRegister reg = node.block->get_param_regs()[i];
                ssa::Type type = node.block->get_param_types()[i];

                ssa::Operand value = args[i];
                value.set_type(type);

                PassUtils::replace_in_func(func, reg, value);
            }

            pred.remove(branch_instr);

            for (ssa::Instruction &instr : node.block->get_instrs()) {
                pred.append(instr);
            }
        }
    }

    cfg = ssa::ControlFlowGraph(&func);

    // Merge blocks with their predecessors if they only contain an unconditional branch instruction.
    for (ssa::ControlFlowGraph::Node &node : cfg.get_nodes()) {
        if (!node.block->get_param_regs().empty() || node.block->get_instrs().get_size() != 1 ||
            node.block->get_instrs().get_last().get_opcode() != ssa::Opcode::JMP) {
            continue;
        }

        ssa::BranchTarget target = node.block->get_instrs().get_last().get_operand(0).get_branch_target();

        for (unsigned pred_index : node.predecessors) {
            ssa::Instruction &branch_instr = cfg.get_nodes()[pred_index].block->get_instrs().get_last();
            for (ssa::Operand &operand : branch_instr.get_operands()) {
                if (operand.is_branch_target() && operand.get_branch_target().block == node.block) {
                    operand.get_branch_target() = target;
                }
            }
        }
    }

    cfg = ssa::ControlFlowGraph(&func);

    // Remove all blocks that are not in the control flow graph and hence unreachable.
    for (ssa::BasicBlockIter iter = func.begin(); iter != func.end(); ++iter) {
        if (!cfg.contains(iter)) {
            iter = iter.get_prev();
            func.get_basic_blocks().remove(iter.get_next());
        }
    }

    Precomputing::precompute_instrs(func);
}

} // namespace passes

} // namespace banjo
