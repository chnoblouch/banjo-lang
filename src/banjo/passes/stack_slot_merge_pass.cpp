#include "stack_slot_merge_pass.hpp"

#include <unordered_map>
#include <unordered_set>

namespace banjo {

namespace passes {

StackSlotMergePass::StackSlotMergePass(target::Target *target) : Pass("stack-slot-merge", target) {}

void StackSlotMergePass::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(func);
    }
}

void StackSlotMergePass::run(ir::Function *func) {
    std::unordered_map<ir::VirtualRegister, StackSlotInfo> stack_slots;
    std::unordered_set<ir::VirtualRegister> used_stack_slots;

    for (ir::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ir::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
            if (iter->get_opcode() == ir::Opcode::ALLOCA) {
                stack_slots.insert({*iter->get_dest(), {basic_block, iter}});
            } else {
                for (ir::Operand &operand : iter->get_operands()) {
                    if (operand.is_register() && stack_slots.contains(operand.get_register())) {
                        used_stack_slots.insert(operand.get_register());
                    }
                }
            }
        }
    }

    for (ir::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ir::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
            if (iter->get_opcode() == ir::Opcode::ALLOCA && !used_stack_slots.contains(*iter->get_dest())) {
                ir::InstrIter new_iter = iter.get_prev();
                basic_block.remove(iter);
                iter = new_iter;
            }
        }
    }
}

} // namespace passes

} // namespace banjo
