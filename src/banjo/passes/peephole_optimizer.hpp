#ifndef BANJO_PASSES_PEEPHOLE_OPTIMIZER_H
#define BANJO_PASSES_PEEPHOLE_OPTIMIZER_H

#include "banjo/passes/analysis/stack_layout.hpp"
#include "banjo/passes/pass.hpp"

#include <vector>

namespace banjo::passes {

class PeepholeOptimizer : public Pass {

private:
    ssa::Module *mod;
    ssa::Function *func;

    StackLayout stack_layout;
    std::vector<std::optional<ssa::Value>> stack_values;

public:
    PeepholeOptimizer(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void run(ssa::BasicBlock &block, ssa::Function &func);

    void process_load(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_store(ssa::InstrIter &iter);
    void process_add(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_sub(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_mul(ssa::InstrIter &iter, ssa::BasicBlock &block);
    void process_udiv(ssa::InstrIter &iter, ssa::BasicBlock &block);
    void process_fmul(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_call(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_offsetptr(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_memberptr(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);

    void process_memcpy(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_memmove(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);
    void process_strlen(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func);

    void eliminate(ssa::InstrIter &iter, ssa::Value val, ssa::BasicBlock &block, ssa::Function &func);

    void discard_stack_values();
    bool types_compatible(ssa::Type a, ssa::Type b);

    bool is_imm(ssa::Operand &operand);
    bool is_zero(ssa::Operand &operand);
    bool is_float_one(ssa::Operand &operand);
};

} // namespace banjo::passes

#endif
