#ifndef BANJO_UTILS_GENERIC_CFG_H
#define BANJO_UTILS_GENERIC_CFG_H

#include <ostream>
#include <string>
#include <vector>

namespace banjo::utils {

class GenericCFG {

public:
    typedef unsigned NodeID;

    struct Node {
        std::vector<NodeID> predecessors;
        std::vector<NodeID> successors;
    };

    struct Edge {
        NodeID from;
        NodeID to;

        friend bool operator==(const Edge &lhs, const Edge &rhs) = default;
        friend bool operator!=(const Edge &lhs, const Edge &rhs) = default;
    };

    std::vector<Node> nodes;
    NodeID entry;

    std::vector<NodeID> reverse_post_order() const;
    void dump(std::ostream &stream);

    virtual std::string node_label(NodeID id) const = 0;
};

} // namespace banjo::utils

template <>
struct std::hash<banjo::utils::GenericCFG::Edge> {
    std::size_t operator()(const banjo::utils::GenericCFG::Edge &edge) const noexcept {
        return static_cast<std::size_t>(edge.from) << 32 | static_cast<std::size_t>(edge.to);
    }
};

#endif
