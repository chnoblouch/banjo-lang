#ifndef PASSES_ADDR_TABLE_PASS_H
#define PASSES_ADDR_TABLE_PASS_H

#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class AddrTablePass : public Pass {

public:
    AddrTablePass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void replace_uses(ssa::Module &mod, ssa::Function *func);
    void replace_uses(ssa::Module &mod, ssa::Function *func, ssa::BasicBlock &basic_block);
};

} // namespace passes

} // namespace banjo

#endif
