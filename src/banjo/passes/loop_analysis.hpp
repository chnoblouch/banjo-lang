#ifndef BANJO_LOOP_ANALYSIS_H
#define BANJO_LOOP_ANALYSIS_H

#include "banjo/ssa/control_flow_graph.hpp"

#include <ostream>
#include <unordered_set>

namespace banjo {

namespace ssa {

struct LoopAnalysis {
    unsigned header;
    std::unordered_set<unsigned> body;
    unsigned tail;
    std::unordered_set<unsigned> entries;
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
    bool is_dominated_by(unsigned a, unsigned b);
    void analyze_back_edge(unsigned from, unsigned to);
    void collect_body_nodes(LoopAnalysis &loop, unsigned node);
    void collect_exit_nodes(LoopAnalysis &loop, unsigned node);
    bool is_in_loop(LoopAnalysis &loop, unsigned node);
};

} // namespace ssa

} // namespace banjo

#endif
