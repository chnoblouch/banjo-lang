#ifndef BANJO_SSA_CALL_GRAPH_H
#define BANJO_SSA_CALL_GRAPH_H

#include "banjo/ssa/module.hpp"
#include <unordered_map>

namespace banjo {

namespace ssa {

class CallGraph {

public:
    struct Node {
        std::vector<unsigned> preds;
        std::vector<unsigned> succs;
    };

private:
    std::vector<Node> nodes;
    std::unordered_map<Function *, unsigned> funcs2nodes;

public:
    CallGraph() {}
    CallGraph(Module &mod);

    std::vector<Node> &get_nodes() { return nodes; }
    unsigned get_node_index(Function *func) { return funcs2nodes[func]; }
    Node &get_node(unsigned index) { return nodes[index]; }
    Node &get_node(Function *func) { return nodes[get_node_index(func)]; }

private:
    void create_edge(Function *from, Function *to);
};

} // namespace ssa

} // namespace banjo

#endif
