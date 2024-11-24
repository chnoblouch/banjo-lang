#ifndef PASSES_PEEPHOLE_OPTIMIZER_H
#define PASSES_PEEPHOLE_OPTIMIZER_H

#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class PeepholeOptimizer : public Pass {

public:
    PeepholeOptimizer(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func, ssa::Module &mod);
    void run(ssa::BasicBlock &block, ssa::Function &func, ssa::Module &mod);

    void optimize_add(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void optimize_sub(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void optimize_mul(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void optimize_udiv(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void optimize_fmul(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void optimize_call(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func, ssa::Module &mod);

    void eliminate(ssa::InstrIter &iter, ssa::Value val, ssa::BasicBlock &block, ssa::Function &func);
    bool is_imm(ssa::Operand &operand);
    bool is_zero(ssa::Operand &operand);
    bool is_float_one(ssa::Operand &operand);
};

} // namespace passes

} // namespace banjo

#endif
