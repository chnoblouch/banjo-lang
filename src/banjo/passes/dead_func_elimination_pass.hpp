#ifndef BANJO_PASSES_DEAD_FUNC_ELINIATION_PASS_H
#define BANJO_PASSES_DEAD_FUNC_ELINIATION_PASS_H

#include "banjo/passes/pass.hpp"

#include <string_view>
#include <unordered_set>

namespace banjo {

namespace passes {

class DeadFuncEliminationPass : public Pass {

private:
    ssa::Module *mod;
    std::unordered_set<ssa::Function *> used_funcs;
    std::unordered_set<std::string_view> used_extern_funcs;

public:
    DeadFuncEliminationPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void walk_call_graph(ssa::Function *func);
    void analyze_value(ssa::Value &value);
};

} // namespace passes

} // namespace banjo

#endif
