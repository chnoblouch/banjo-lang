#ifndef PASSES_INLINING_PASS_H
#define PASSES_INLINING_PASS_H

#include "ir/call_graph.hpp"
#include "passes/pass.hpp"

#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace banjo {

namespace passes {

class InliningPass : public Pass {

private:
    ir::CallGraph call_graph;

    struct CalleeInfo {
        bool multiple_blocks;
        bool multiple_returns;
    };

    struct Context {
        ir::InstrIter call_instr;
        bool single_block;
        std::unordered_map<ir::VirtualRegister, ir::VirtualRegister> reg2reg;
        std::unordered_map<ir::VirtualRegister, ir::Value> reg2val;
        std::unordered_map<ir::BasicBlockIter, ir::BasicBlockIter> block_map;
        std::unordered_set<ir::InstrIter> removed_instrs;
        ir::BasicBlockIter end_block;
    };

private:
    std::unordered_set<ir::Function *> funcs_visited;
    std::set<std::pair<ir::Function *, ir::Function *>> inlined_funcs;
    unsigned next_vreg = 10000;

public:
    InliningPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
    void try_inline(ir::Function *func, ir::BasicBlockIter &block_iter, ir::InstrIter &call_iter);
    CalleeInfo collect_info(ir::Function *callee);
    void inline_instr(ir::InstrIter instr_iter, ir::BasicBlock &block, Context &ctx);
    ir::Value get_inlined_value(ir::Value &value, Context &ctx);
    int estimate_cost(ir::Function &func);
    int estimate_gain(ir::Function &func);
    bool is_inlining_beneficial(ir::Function *caller, ir::Function *callee);
    bool is_inlining_legal(ir::Function *caller, ir::Function *callee);
};

} // namespace passes

} // namespace banjo

#endif
