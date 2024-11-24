#ifndef PASSES_BRANCH_ELIMINATION_H
#define PASSES_BRANCH_ELIMINATION_H

#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class BranchElimination : public Pass {

public:
    BranchElimination(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
};

} // namespace passes

} // namespace banjo

#endif
