#include "stack_slot_merge_pass.hpp"

#include <unordered_map>
#include <unordered_set>

namespace banjo {

namespace passes {

StackSlotMergePass::StackSlotMergePass(target::Target *target) : Pass("stack-slot-merge", target) {}

void StackSlotMergePass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func);
    }
}

void StackSlotMergePass::run(ssa::Function *func) {
    std::unordered_map<ssa::VirtualRegister, StackSlotInfo> stack_slots;
    std::unordered_set<ssa::VirtualRegister> used_stack_slots;

    for (ssa::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ssa::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
            if (iter->get_opcode() == ssa::Opcode::ALLOCA) {
                stack_slots.insert({*iter->get_dest(), {basic_block, iter}});
            } else {
                for (ssa::Operand &operand : iter->get_operands()) {
                    if (operand.is_register() && stack_slots.contains(operand.get_register())) {
                        used_stack_slots.insert(operand.get_register());
                    }
                }
            }
        }
    }

    for (ssa::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ssa::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
            if (iter->get_opcode() == ssa::Opcode::ALLOCA && !used_stack_slots.contains(*iter->get_dest())) {
                ssa::InstrIter new_iter = iter.get_prev();
                basic_block.remove(iter);
                iter = new_iter;
            }
        }
    }
}

} // namespace passes

} // namespace banjo
