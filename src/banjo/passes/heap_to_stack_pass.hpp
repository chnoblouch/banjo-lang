#ifndef PASSES_HEAP_TO_STACK_PASS_H
#define PASSES_HEAP_TO_STACK_PASS_H

#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class HeapToStackPass : public Pass {

public:
    HeapToStackPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function &func);
    void run(ir::BasicBlock &block, ir::Function &func);
};

} // namespace passes

} // namespace banjo

#endif
