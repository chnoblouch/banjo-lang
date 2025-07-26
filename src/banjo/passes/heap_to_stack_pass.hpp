#ifndef BANJO_PASSES_HEAP_TO_STACK_PASS_H
#define BANJO_PASSES_HEAP_TO_STACK_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace banjo {

namespace passes {

class HeapToStackPass : public Pass {

private:
    struct InstrInContext {
        ssa::BasicBlockIter block;
        ssa::InstrIter instr;
    };

    struct Allocation {
        InstrInContext alloc_instr;
        std::vector<InstrInContext> free_instrs;
    };

    std::unordered_map<ssa::VirtualRegister, Allocation> allocs;
    ssa::ControlFlowGraph cfg;

public:
    HeapToStackPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void collect_heap_instrs(ssa::BasicBlockIter block);

    bool is_always_freed(
        Allocation &alloc,
        ssa::BasicBlockIter block,
        std::unordered_set<ssa::BasicBlockIter> &visited_blocks
    );

    void promote(ssa::Function &func, Allocation &alloc);
};

} // namespace passes

} // namespace banjo

#endif
