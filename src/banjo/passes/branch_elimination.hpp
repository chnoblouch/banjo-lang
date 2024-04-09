#ifndef PASSES_BRANCH_ELIMINATION_H
#define PASSES_BRANCH_ELIMINATION_H

#include "passes/pass.hpp"

namespace passes {

class BranchElimination : public Pass {

public:
    BranchElimination(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
};

} // namespace passes

#endif
