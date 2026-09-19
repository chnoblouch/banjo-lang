#ifndef BANJO_UTILS_DOMINATOR_TREE_H
#define BANJO_UTILS_DOMINATOR_TREE_H

#include "banjo/utils/generic_cfg.hpp"

#include <ostream>
#include <vector>

namespace banjo::utils {

class DominatorTree {

public:
    typedef utils::GenericCFG::NodeID NodeID;

    struct Node {
        NodeID parent;
        std::vector<NodeID> children;
        std::vector<NodeID> dominance_frontiers;
    };

public:
    std::vector<Node> nodes;

    static DominatorTree build(GenericCFG &cfg);

    bool dominates(NodeID a, NodeID b);
    void dump(GenericCFG &cfg, std::ostream &stream);

private:
    void compute_idoms(GenericCFG &cfg);
    void compute_dominance_frontiers(GenericCFG &cfg);
    NodeID intersect(NodeID b1, NodeID b2, std::vector<NodeID> &doms, std::vector<NodeID> &node_indices);
    void dump_tree(GenericCFG &cfg, std::ostream &stream, NodeID node_id, unsigned indent);
};

} // namespace banjo::utils

#endif
