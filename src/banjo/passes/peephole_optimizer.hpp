#ifndef BANJO_PASSES_PEEPHOLE_OPTIMIZER_H
#define BANJO_PASSES_PEEPHOLE_OPTIMIZER_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <optional>
#include <unordered_map>

namespace banjo {

namespace passes {

class PeepholeOptimizer : public Pass {

private:
    struct StackSlotMemberState {
        unsigned offset;
        ssa::Type type;
        std::optional<ssa::Value> value;
    };

    struct StackSlotState {
        std::vector<StackSlotMemberState> members;
    };

private:
    ssa::Function *func;
    std::unordered_map<ssa::VirtualRegister, StackSlotState> stack_slots;

public:
    PeepholeOptimizer(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    void run(ssa::BasicBlock &block, ssa::Function &func);

    void process_alloca(ssa::InstrIter &iter);
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

    void eliminate(ssa::InstrIter &iter, ssa::Value val, ssa::BasicBlock &block, ssa::Function &func);

    void collect_members(StackSlotState &slot_state, ssa::Type type, unsigned base_offset);
    StackSlotMemberState *find_stack_slot_member(ssa::VirtualRegister reg);
    void discard_stack_slot_values();

    bool is_imm(ssa::Operand &operand);
    bool is_zero(ssa::Operand &operand);
    bool is_float_one(ssa::Operand &operand);
};

} // namespace passes

} // namespace banjo

#endif
