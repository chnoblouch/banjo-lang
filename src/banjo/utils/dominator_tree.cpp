
#include "dominator_tree.hpp"

#include "banjo/utils/generic_cfg.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::utils {

constexpr utils::GenericCFG::NodeID INVALID_NODE = 0xFFFFFFFF;

DominatorTree DominatorTree::build(GenericCFG &cfg) {
    // The algorithm for building the dominator tree is based on:
    // https://c9x.me/compile/bib/quickdom.pdf.

    DominatorTree domtree;
    domtree.nodes.resize(cfg.nodes.size());

    domtree.compute_idoms(cfg);
    domtree.compute_dominance_frontiers(cfg);

    return domtree;
}

void DominatorTree::compute_idoms(GenericCFG &cfg) {
    std::vector<NodeID> node_ids = cfg.reverse_post_order();
    std::vector<unsigned> node_indices(node_ids.size());

    for (unsigned i = 0; i < node_ids.size(); i++) {
        node_indices[node_ids[i]] = i;
    }

    std::vector<NodeID> doms;
    doms.assign(cfg.nodes.size(), INVALID_NODE);

    bool changed = true;

    while (changed) {
        changed = false;

        for (NodeID id : node_ids) {
            if (id == cfg.entry) {
                doms[id] = id;
                continue;
            }

            NodeID new_idom = INVALID_NODE;

            for (NodeID pred : cfg.nodes[id].predecessors) {
                if (doms[pred] == INVALID_NODE) {
                    continue;
                }

                if (new_idom == INVALID_NODE) {
                    new_idom = pred;
                } else {
                    new_idom = intersect(new_idom, pred, doms, node_indices);
                }
            }

            if (doms[id] != new_idom) {
                doms[id] = new_idom;
                changed = true;
            }
        }
    }

    for (NodeID id = 0; id < doms.size(); id++) {
        NodeID idom = doms[id];
        nodes[id].parent = idom;

        if (id != cfg.entry) {
            nodes[idom].children.push_back(id);
        }
    }
}

DominatorTree::NodeID DominatorTree::intersect(
    NodeID b1,
    NodeID b2,
    std::vector<NodeID> &doms,
    std::vector<NodeID> &node_indices
) {
    while (b1 != b2) {
        if (node_indices[b1] > node_indices[b2]) {
            ASSERT(doms[b1] != INVALID_NODE);
            b1 = doms[b1];
        } else {
            ASSERT(doms[b2] != INVALID_NODE);
            b2 = doms[b2];
        }
    }

    return b1;
}

void DominatorTree::compute_dominance_frontiers(GenericCFG &cfg) {
    for (NodeID id = 0; id < cfg.nodes.size(); id++) {
        GenericCFG::Node &node = cfg.nodes[id];
        NodeID idom = nodes[id].parent;

        // Skip if this node is not a join point.
        if (node.predecessors.size() < 2) {
            continue;
        }

        for (NodeID pred : node.predecessors) {
            NodeID runner = pred;

            // Move up the dominance tree and add the node to the dominance frontier sets of the predecessors.
            while (runner != idom) {
                nodes[runner].dominance_frontiers.push_back(id);
                runner = nodes[runner].parent;
            }
        }
    }
}

bool DominatorTree::dominates(NodeID a, NodeID b) {
    if (a == b) {
        return true;
    }

    for (NodeID child : nodes[a].children) {
        if (dominates(child, b)) {
            return true;
        }
    }

    return false;
}

void DominatorTree::dump(GenericCFG &cfg, std::ostream &stream) {
    dump_tree(cfg, stream, cfg.entry, 0);
}

void DominatorTree::dump_tree(GenericCFG &cfg, std::ostream &stream, NodeID node_id, unsigned indent) {
    stream << std::string(2 * indent, ' ') << " - ";
    stream << cfg.node_label(node_id);

    Node &node = nodes[node_id];

    if (!node.dominance_frontiers.empty()) {
        stream << " [ ";

        for (NodeID id = 0; id < node.dominance_frontiers.size(); id++) {
            stream << cfg.node_label(node.dominance_frontiers[id]);

            if (id != node.dominance_frontiers.size() - 1) {
                stream << ", ";
            }
        }

        stream << " ]";
    }

    stream << '\n';

    for (NodeID child : node.children) {
        dump_tree(cfg, stream, child, indent + 1);
    }
}

} // namespace banjo::utils
