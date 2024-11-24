#include "loop_analysis.hpp"

namespace banjo {

namespace ssa {

LoopAnalyzer::LoopAnalyzer(ControlFlowGraph &cfg, DominatorTree &domtree) : cfg(cfg), domtree(domtree) {}

std::vector<LoopAnalysis> LoopAnalyzer::analyze() {
    for (unsigned i = 0; i < cfg.get_nodes().size(); i++) {
        ControlFlowGraph::Node &node = cfg.get_node(i);

        for (unsigned succ : node.successors) {
            if (is_dominated_by(i, succ)) {
                analyze_back_edge(i, succ);
            }
        }
    }

    return loops;
}

bool LoopAnalyzer::is_dominated_by(unsigned a, unsigned b) {
    if (a == b) {
        return true;
    }

    unsigned idom = domtree.get_node(a).parent_index;

    while (domtree.get_node(idom).parent_index != idom) {
        if (idom == b) {
            return true;
        }

        idom = domtree.get_node(idom).parent_index;
    }

    return false;
}

void LoopAnalyzer::analyze_back_edge(unsigned from, unsigned to) {
    LoopAnalysis loop{
        .header = to,
        .body = {to},
        .tail = from,
        .entries = {},
        .exits = {},
    };

    collect_body_nodes(loop, loop.tail);

    for (unsigned pred_node : cfg.get_node(loop.header).predecessors) {
        if (!is_in_loop(loop, pred_node)) {
            loop.entries.insert(pred_node);
        }
    }

    for (unsigned body_node : loop.body) {
        collect_exit_nodes(loop, body_node);
    }

    loops.push_back(loop);
}

void LoopAnalyzer::collect_body_nodes(LoopAnalysis &loop, unsigned node) {
    if (is_in_loop(loop, node)) {
        return;
    }

    loop.body.insert(node);

    for (unsigned pred : cfg.get_node(node).predecessors) {
        collect_body_nodes(loop, pred);
    }
}

void LoopAnalyzer::collect_exit_nodes(LoopAnalysis &loop, unsigned node) {
    for (unsigned succ : cfg.get_node(node).successors) {
        if (!is_in_loop(loop, succ)) {
            loop.exits.insert({.from = node, .to = succ});
        }
    }
}

bool LoopAnalyzer::is_in_loop(LoopAnalysis &loop, unsigned node) {
    return loop.body.contains(node);
}

void LoopAnalyzer::dump(std::ostream &stream) {
    for (LoopAnalysis &loop : loops) {
        stream << "header: " << cfg.get_node(loop.header).block->get_debug_label() << '\n';

        for (unsigned node : loop.body) {
            if (node == loop.header || node == loop.tail) {
                continue;
            }

            stream << "  " << cfg.get_node(node).block->get_debug_label() << '\n';
        }

        stream << "tail: " << cfg.get_node(loop.tail).block->get_debug_label() << '\n';

        for (ControlFlowGraph::Edge exit : loop.exits) {
            stream << "exit: " << cfg.get_node(exit.from).block->get_debug_label() << " -> "
                   << cfg.get_node(exit.to).block->get_debug_label() << '\n';
        }

        stream << '\n';
    }
}

} // namespace ssa

} // namespace banjo
