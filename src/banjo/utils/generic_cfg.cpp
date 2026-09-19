#include "generic_cfg.hpp"

#include "banjo/utils/bit_set.hpp"

#include <algorithm>

namespace banjo::utils {

static void walk_post_order(
    const GenericCFG &cfg,
    unsigned index,
    std::vector<GenericCFG::NodeID> &order,
    BitSet &visited
) {
    visited.set(index);

    for (unsigned successor : cfg.nodes[index].successors) {
        if (visited.get(successor)) {
            continue;
        }

        walk_post_order(cfg, successor, order, visited);
    }

    order.push_back(index);
}

std::vector<GenericCFG::NodeID> GenericCFG::reverse_post_order() const {
    std::vector<NodeID> order;
    BitSet visited;

    order.reserve(nodes.size());
    walk_post_order(*this, entry, order, visited);
    std::reverse(order.begin(), order.end());

    return order;
}

void GenericCFG::dump(std::ostream &stream) {
    for (unsigned i = 0; i < nodes.size(); i++) {
        Node &node = nodes[i];

        stream << i << " ";
        stream << node_label(i) << ":\n";

        for (NodeID predecessor : node.predecessors) {
            stream << "  <- " << node_label(predecessor) << "\n";
        }

        for (NodeID successor : node.successors) {
            stream << "  -> " << node_label(successor) << "\n";
        }

        stream << "\n";
    }
}

} // namespace banjo::utils
