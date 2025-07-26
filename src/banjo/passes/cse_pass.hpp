#ifndef BANJO_PASSES_CSE_PASS_H
#define BANJO_PASSES_CSE_PASS_H

#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class CSEPass : public Pass {

public:
    CSEPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    void run(ssa::BasicBlock &block, ssa::Function *func);
};

} // namespace passes

} // namespace banjo

#endif
