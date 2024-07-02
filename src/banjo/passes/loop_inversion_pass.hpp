#ifndef PASSES_LOOP_INVERSION_PASS_H
#define PASSES_LOOP_INVERSION_PASS_H

#include "passes/loop_analysis.hpp"
#include "passes/pass.hpp"

namespace banjo {

namespace passes {

class LoopInversionPass : public Pass {

public:
    LoopInversionPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
    bool run(const ir::LoopAnalysis &loop, ir::ControlFlowGraph &cfg, ir::Function *func);

    void canonicalize(const ir::LoopAnalysis &loop, ir::ControlFlowGraph &cfg);
};

} // namespace passes

} // namespace banjo

#endif
