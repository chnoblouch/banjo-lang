#ifndef PASSES_DEAD_FUNC_ELINIATION_PASS_H
#define PASSES_DEAD_FUNC_ELINIATION_PASS_H

#include "banjo/passes/pass.hpp"

#include <string>
#include <unordered_set>

namespace banjo {

namespace passes {

class DeadFuncEliminationPass : public Pass {

private:
    ir::Module *mod;
    std::unordered_set<ir::Function *> used_funcs;

public:
    DeadFuncEliminationPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function &func);
    void walk_call_graph(ir::Function *func);
    void analyze_value(ir::Value &value);
};

} // namespace passes

} // namespace banjo

#endif
