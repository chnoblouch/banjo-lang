#include "loop_analysis.hpp"

#include "banjo/ssa/control_flow_graph.hpp"

namespace banjo::ssa {

LoopAnalyzer::LoopAnalyzer(ControlFlowGraph &cfg, DominatorTree &domtree) : cfg{cfg}, domtree{domtree} {}

std::vector<LoopAnalysis> LoopAnalyzer::analyze() {
    for (ControlFlowGraph::NodeID node = 0; node < cfg.nodes.size(); node++) {
        for (ControlFlowGraph::NodeID succ : cfg.nodes[node].successors) {
            if (domtree.dominates(succ, node)) {
                analyze_back_edge(node, succ);
            }
        }
    }

    return loops;
}

void LoopAnalyzer::analyze_back_edge(ControlFlowGraph::NodeID from, ControlFlowGraph::NodeID to) {
    LoopAnalysis loop{
        .header = to,
        .body = {to},
        .tail = from,
        .entries = {},
        .exits = {},
    };

    collect_body_nodes(loop, loop.tail);

    for (ControlFlowGraph::NodeID pred_node : cfg.nodes[loop.header].predecessors) {
        if (!is_in_loop(loop, pred_node)) {
            loop.entries.insert(pred_node);
        }
    }

    for (ControlFlowGraph::NodeID body_node : loop.body) {
        collect_exit_nodes(loop, body_node);
    }

    loops.push_back(loop);
}

void LoopAnalyzer::collect_body_nodes(LoopAnalysis &loop, ControlFlowGraph::NodeID node) {
    if (is_in_loop(loop, node)) {
        return;
    }

    loop.body.insert(node);

    for (ControlFlowGraph::NodeID pred : cfg.nodes[node].predecessors) {
        collect_body_nodes(loop, pred);
    }
}

void LoopAnalyzer::collect_exit_nodes(LoopAnalysis &loop, ControlFlowGraph::NodeID node) {
    for (ControlFlowGraph::NodeID succ : cfg.nodes[node].successors) {
        if (!is_in_loop(loop, succ)) {
            loop.exits.insert({.from = node, .to = succ});
        }
    }
}

bool LoopAnalyzer::is_in_loop(LoopAnalysis &loop, ControlFlowGraph::NodeID node) {
    return loop.body.contains(node);
}

void LoopAnalyzer::dump(std::ostream &stream) {
    for (LoopAnalysis &loop : loops) {
        stream << "header: " << cfg.node_label(loop.header) << '\n';

        for (unsigned node : loop.body) {
            if (node == loop.header || node == loop.tail) {
                continue;
            }

            stream << "  " << cfg.node_label(node) << '\n';
        }

        stream << "tail: " << cfg.node_label(loop.tail) << '\n';

        for (ControlFlowGraph::Edge exit : loop.exits) {
            stream << "exit: " << cfg.node_label(exit.from) << " -> " << cfg.node_label(exit.to) << '\n';
        }

        stream << '\n';
    }
}

} // namespace banjo::ssa
