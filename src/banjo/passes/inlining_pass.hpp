#ifndef PASSES_INLINING_PASS_H
#define PASSES_INLINING_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/call_graph.hpp"

#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace banjo {

namespace passes {

class InliningPass : public Pass {

private:
    ssa::CallGraph call_graph;

    struct CalleeInfo {
        bool multiple_blocks;
        bool multiple_returns;
    };

    struct Context {
        ssa::InstrIter call_instr;
        bool is_single_block;
        std::unordered_map<ssa::VirtualRegister, ssa::VirtualRegister> reg2reg;
        std::unordered_map<ssa::VirtualRegister, ssa::Value> reg2val;
        std::unordered_map<ssa::BasicBlockIter, ssa::BasicBlockIter> block_map;
        std::unordered_set<ssa::InstrIter> removed_instrs;
        ssa::BasicBlockIter end_block;
    };

private:
    std::unordered_set<ssa::Function *> funcs_visited;
    std::set<std::pair<ssa::Function *, ssa::Function *>> inlined_funcs;
    unsigned next_vreg = 10000;

public:
    InliningPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    void try_inline(ssa::Function *func, ssa::BasicBlockIter &block_iter, ssa::InstrIter &call_iter);
    CalleeInfo collect_info(ssa::Function *callee);

    void inline_func(ssa::Function &func, ssa::BasicBlockIter &block_iter, ssa::InstrIter &call_iter);
    void inline_instr(ssa::InstrIter instr_iter, ssa::BasicBlock &block, Context &ctx);
    ssa::Value get_inlined_value(ssa::Value &value, Context &ctx);

    int estimate_cost(ssa::Function &func);
    int estimate_gain(ssa::Function &func);
    bool is_inlining_beneficial(ssa::Function *caller, ssa::Function *callee);
    bool is_inlining_legal(ssa::Function *caller, ssa::Function *callee);
};

} // namespace passes

} // namespace banjo

#endif
