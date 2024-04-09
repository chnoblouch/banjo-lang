#include "call_graph.hpp"

namespace ir {

CallGraph::CallGraph(Module &mod) {
    for (Function *func : mod.get_functions()) {
        nodes.push_back({
            .preds = {},
            .succs = {},
        });

        funcs2nodes.insert({func, nodes.size() - 1});
    }

    for (Function *func : mod.get_functions()) {
        for (BasicBlock &block : *func) {
            for (Instruction &instr : block) {
                if (instr.get_opcode() == Opcode::CALL && instr.get_operand(0).is_func()) {
                    create_edge(func, instr.get_operand(0).get_func());
                }
            }
        }
    }
}

void CallGraph::create_edge(Function *from, Function *to) {
    unsigned from_index = funcs2nodes[from];
    unsigned to_index = funcs2nodes[to];

    nodes[from_index].succs.push_back(to_index);
    nodes[to_index].preds.push_back(from_index);
}

} // namespace ir
