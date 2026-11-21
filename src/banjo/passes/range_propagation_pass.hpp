#ifndef BANJO_PASSES_RANGE_PROPAGATION_PASS_H
#define BANJO_PASSES_RANGE_PROPAGATION_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <optional>

namespace banjo::passes {

class RangePropagationPass : public Pass {

private:
    ssa::ControlFlowGraph cfg;
    ssa::DominatorTree dom_tree;

public:
    RangePropagationPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void run(ssa::ControlFlowGraph::NodeID node_id);

    void analyze_branch(ssa::ControlFlowGraph::NodeID node, ssa::Instruction &instr);

    std::optional<ssa::Comparison> compute_relationship(
        ssa::ControlFlowGraph::NodeID node,
        ssa::VirtualRegister lhs,
        ssa::VirtualRegister rhs
    );

    std::optional<bool> try_compute_comparison(ssa::Comparison relationship, ssa::Comparison comparison);
};

} // namespace banjo::passes

#endif
