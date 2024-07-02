#ifndef PASSES_CSE_PASS_H
#define PASSES_CSE_PASS_H

#include "passes/pass.hpp"

namespace banjo {

namespace passes {

class CSEPass : public Pass {

public:
    CSEPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
    void run(ir::BasicBlock &block, ir::Function *func);
};

} // namespace passes

} // namespace banjo

#endif
