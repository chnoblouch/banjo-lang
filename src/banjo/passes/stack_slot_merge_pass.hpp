#ifndef PASSES_STACK_SLOT_MERGE_PASS_H
#define PASSES_STACK_SLOT_MERGE_PASS_H

#include "passes/pass.hpp"

namespace banjo {

namespace passes {

class StackSlotMergePass : public Pass {

private:
    struct StackSlotInfo {
        ir::BasicBlock &alloca_basic_block;
        ir::InstrIter alloca_iter;
    };

public:
    StackSlotMergePass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
};

} // namespace passes

} // namespace banjo

#endif
