#ifndef BANJO_PASSES_LOOP_INVERSION_PASS_H
#define BANJO_PASSES_LOOP_INVERSION_PASS_H

#include "banjo/passes/loop_analysis.hpp"
#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class LoopInversionPass : public Pass {

public:
    LoopInversionPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    bool run(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg, ssa::Function *func);

    void canonicalize(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg);
};

} // namespace passes

} // namespace banjo

#endif
