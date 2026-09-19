#include "control_flow_graph.hpp"

#include "banjo/ssa/operand.hpp"
#include "banjo/utils/generic_cfg.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::ssa {

ControlFlowGraph ControlFlowGraph::build(ssa::Function &func) {
    ControlFlowGraph cfg;
    cfg.entry = 0;

    cfg.nodes.reserve(func.get_basic_blocks().get_size());
    cfg.collect_nodes(func.get_entry_block_iter());

    return cfg;
}

void ControlFlowGraph::collect_nodes(ssa::BasicBlockIter block) {
    ASSERT_MESSAGE(block->get_size() != 0, "block is empty");

    if (blocks2nodes.contains(block)) {
        return;
    }

    nodes.push_back(Node{});
    nodes2blocks.push_back(block);
    blocks2nodes.insert({block, nodes.size() - 1});

    ssa::Instruction &exit_instr = block->get_instrs().get_last();

    switch (exit_instr.get_opcode()) {
        case ssa::Opcode::JMP: {
            ssa::BranchTarget &target = exit_instr.get_operand(0).get_branch_target();
            create_edge(block, target.block);
            break;
        }

        case ssa::Opcode::CJMP:
        case ssa::Opcode::FCJMP: {
            ssa::BranchTarget &target_true = exit_instr.get_operand(3).get_branch_target();
            ssa::BranchTarget &target_false = exit_instr.get_operand(4).get_branch_target();

            create_edge(block, target_true.block);
            create_edge(block, target_false.block);

            break;
        }

        case ssa::Opcode::RET: break;
        default: ASSERT_UNREACHABLE;
    }
}

void ControlFlowGraph::create_edge(ssa::BasicBlockIter from, ssa::BasicBlockIter to) {
    collect_nodes(to);

    NodeID from_id = blocks2nodes[from];
    NodeID to_id = blocks2nodes[to];

    nodes[from_id].successors.push_back(to_id);
    nodes[to_id].predecessors.push_back(from_id);
}

} // namespace banjo::ssa
