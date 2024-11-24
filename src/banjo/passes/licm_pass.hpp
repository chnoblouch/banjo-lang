#ifndef PASSES_LICM_PASS_H
#define PASSES_LICM_PASS_H

#include "banjo/passes/loop_analysis.hpp"
#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class LICMPass : public Pass {

public:
    LICMPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    void run(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg, ssa::Function *func);
    bool is_volatile_load(ssa::InstrIter iter, ssa::Function *func);
};

} // namespace passes

} // namespace banjo

#endif
