#ifndef PASSES_PEEPHOLE_OPTIMIZER_H
#define PASSES_PEEPHOLE_OPTIMIZER_H

#include "passes/pass.hpp"

namespace banjo {

namespace passes {

class PeepholeOptimizer : public Pass {

public:
    PeepholeOptimizer(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
    void run(ir::BasicBlock &block, ir::Function &func);

    void optimize_add(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func);
    void optimize_sub(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func);
    void optimize_mul(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func);
    void optimize_udiv(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func);
    void optimize_fmul(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func);

    void eliminate(ir::InstrIter &iter, ir::Value val, ir::BasicBlock &block, ir::Function &func);
    bool is_imm(ir::Operand &operand);
    bool is_zero(ir::Operand &operand);
    bool is_float_one(ir::Operand &operand);
};

} // namespace passes

} // namespace banjo

#endif
