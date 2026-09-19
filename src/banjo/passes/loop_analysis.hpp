#ifndef BANJO_LOOP_ANALYSIS_H
#define BANJO_LOOP_ANALYSIS_H

#include "banjo/ssa/control_flow_graph.hpp"

#include <ostream>
#include <unordered_set>

namespace banjo::ssa {

struct LoopAnalysis {
    ControlFlowGraph::NodeID header;
    std::unordered_set<ControlFlowGraph::NodeID> body;
    ControlFlowGraph::NodeID tail;
    std::unordered_set<ControlFlowGraph::NodeID> entries;
    std::unordered_set<ControlFlowGraph::Edge> exits;
};

class LoopAnalyzer {

private:
    ControlFlowGraph &cfg;
    DominatorTree &domtree;
    std::vector<LoopAnalysis> loops;

public:
    LoopAnalyzer(ControlFlowGraph &cfg, DominatorTree &domtree);
    std::vector<LoopAnalysis> analyze();
    void dump(std::ostream &stream);

private:
    void analyze_back_edge(ControlFlowGraph::NodeID from, ControlFlowGraph::NodeID to);
    void collect_body_nodes(LoopAnalysis &loop, ControlFlowGraph::NodeID node);
    void collect_exit_nodes(LoopAnalysis &loop, ControlFlowGraph::NodeID node);
    bool is_in_loop(LoopAnalysis &loop, ControlFlowGraph::NodeID node);
};

} // namespace banjo::ssa

#endif
