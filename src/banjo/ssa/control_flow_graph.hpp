#ifndef BANJO_SSA_CONTROL_FLOW_GRAPH_H
#define BANJO_SSA_CONTROL_FLOW_GRAPH_H

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/utils/dominator_tree.hpp"
#include "banjo/utils/generic_cfg.hpp"

#include <unordered_map>
#include <vector>

namespace banjo::ssa {

class ControlFlowGraph final : public utils::GenericCFG {

private:
    std::unordered_map<ssa::BasicBlockIter, NodeID> blocks2nodes;
    std::vector<ssa::BasicBlockIter> nodes2blocks;

public:
    static ControlFlowGraph build(ssa::Function &func);

    NodeID node_id(ssa::BasicBlockIter iter) { return blocks2nodes.at(iter); }
    Node &node(ssa::BasicBlockIter iter) { return nodes[node_id(iter)]; }
    bool contains(ssa::BasicBlockIter iter) { return blocks2nodes.contains(iter); }
    BasicBlockIter block(NodeID node) { return nodes2blocks[node]; }

    std::string node_label(NodeID id) const override { return nodes2blocks[id]->get_debug_label(); }

private:
    void collect_nodes(ssa::BasicBlockIter block);
    void create_edge(ssa::BasicBlockIter from, ssa::BasicBlockIter to);
};

typedef utils::DominatorTree DominatorTree;

} // namespace banjo::ssa

#endif
