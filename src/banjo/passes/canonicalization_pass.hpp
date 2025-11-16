#ifndef BANJO_PASSES_CANONICALIZATION_PASS_H
#define BANJO_PASSES_CANONICALIZATION_PASS_H

#include "banjo/passes/pass.hpp"

namespace banjo::passes {

class CanonicalizationPass : public Pass {

public:
    CanonicalizationPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void run(ssa::Function &func, ssa::BasicBlock &block);
};

} // namespace banjo::passes

#endif
