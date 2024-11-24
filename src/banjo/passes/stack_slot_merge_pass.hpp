#ifndef PASSES_STACK_SLOT_MERGE_PASS_H
#define PASSES_STACK_SLOT_MERGE_PASS_H

#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class StackSlotMergePass : public Pass {

private:
    struct StackSlotInfo {
        ssa::BasicBlock &alloca_basic_block;
        ssa::InstrIter alloca_iter;
    };

public:
    StackSlotMergePass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
};

} // namespace passes

} // namespace banjo

#endif
