#ifndef PASSES_LICM_PASS_H
#define PASSES_LICM_PASS_H

#include "passes/loop_analysis.hpp"
#include "passes/pass.hpp"

namespace passes {

class LICMPass : public Pass {

public:
    LICMPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
    void run(const ir::LoopAnalysis &loop, ir::ControlFlowGraph &cfg, ir::Function *func);
    bool is_volatile_load(ir::InstrIter iter, ir::Function *func);
};

} // namespace passes

#endif
