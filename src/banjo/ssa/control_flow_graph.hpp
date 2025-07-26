#ifndef BANJO_SSA_CONTROL_FLOW_GRAPH_H
#define BANJO_SSA_CONTROL_FLOW_GRAPH_H

#include "banjo/ssa/function.hpp"

#include <optional>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace banjo {

namespace ssa {

class ControlFlowGraph {

public:
    struct Node {
        ssa::BasicBlockIter block;
        std::vector<unsigned> predecessors;
        std::vector<unsigned> successors;
    };

    struct Edge {
        unsigned from;
        unsigned to;

        friend bool operator==(const Edge &lhs, const Edge &rhs) { return lhs.from == rhs.from && lhs.to == rhs.to; }
        friend bool operator!=(const Edge &lhs, const Edge &rhs) { return !(lhs == rhs); }
    };

private:
    std::vector<Node> nodes;
    unsigned entry_index;
    std::unordered_map<ssa::BasicBlockIter, int> blocks2nodes;

public:
    ControlFlowGraph();
    ControlFlowGraph(ssa::Function *func);
    
    std::vector<Node> &get_nodes() { return nodes; }
    unsigned get_node_index(ssa::BasicBlockIter iter) { return blocks2nodes[iter]; }
    Node &get_node(unsigned index) { return nodes[index]; }
    Node &get_node(ssa::BasicBlockIter iter) { return nodes[get_node_index(iter)]; }
    bool contains(ssa::BasicBlockIter iter) { return blocks2nodes.contains(iter); }
    unsigned get_entry_index() const { return entry_index; }

    void dump(std::ostream &stream);

private:
    void create_nodes(ssa::BasicBlockIter block);
    void create_edge(ssa::BasicBlockIter from, ssa::BasicBlockIter to);
    void sort_in_post_order();

    void collect_nodes_in_post_order(
        unsigned index,
        std::unordered_map<unsigned, unsigned> &index_map,
        unsigned &cur_new_index
    );
};

// Based on: https://www.cs.rice.edu/~keith/EMBED/dom.pdf
class DominatorTree {

public:
    struct Node {
        unsigned index;
        unsigned parent_index;
        std::vector<unsigned> children_indices;
        std::vector<unsigned> dominance_frontiers;
    };

private:
    ControlFlowGraph &cfg;
    std::vector<Node> nodes;

public:
    DominatorTree(ControlFlowGraph &cfg);
    ControlFlowGraph &get_cfg() { return cfg; }
    Node &get_node(unsigned index) { return nodes[index]; }
    Node &get_node(ssa::BasicBlockIter iter) { return nodes[cfg.get_node_index(iter)]; }
    std::vector<ControlFlowGraph::Node> get_dominance_frontiers(ssa::BasicBlockIter iter);
    void dump(std::ostream &stream);

private:
    unsigned intersect(unsigned b1, unsigned b2, const std::vector<int> &doms);
    void compute_idoms();
    void compute_dominance_frontiers();
    void dump_tree(std::ostream &stream, Node &node, unsigned indentation);
    std::string get_debug_label(ssa::BasicBlockIter block);

}; // namespace DominatorTree

} // namespace ssa

} // namespace banjo

template <>
struct std::hash<banjo::ssa::ControlFlowGraph::Edge> {
    std::size_t operator()(const banjo::ssa::ControlFlowGraph::Edge &edge) const noexcept {
        return (std::size_t)(edge.from) << 32 | (std::size_t)edge.to;
    }
};

#endif
