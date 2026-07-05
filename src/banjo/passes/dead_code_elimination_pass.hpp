#ifndef BANJO_PASSES_DEAD_CODE_ELINIATION_PASS_H
#define BANJO_PASSES_DEAD_CODE_ELINIATION_PASS_H

#include "banjo/passes/analysis/stack_layout.hpp"
#include "banjo/passes/pass.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/utils/bit_set.hpp"

namespace banjo::passes {

class DeadCodeEliminationPass : public Pass {

private:
    StackLayout stack_layout;
    BitSet used_stack_slots;

public:
    DeadCodeEliminationPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void analyze_stack_access(ssa::Instruction &instr);
    bool can_eliminate_store(ssa::Instruction &instr);
};

} // namespace banjo::passes

#endif
