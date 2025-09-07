#include "aarch64_stack_offset_fixup_pass.hpp"

#include "banjo/mcode/instruction.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/utils/utils.hpp"

namespace banjo {
namespace target {

void AArch64StackOffsetFixupPass::run(mcode::Module &module_) {
    for (mcode::Function *func : module_.get_functions()) {
        for (mcode::BasicBlock &block : *func) {
            run(block);
        }
    }
}

void AArch64StackOffsetFixupPass::run(mcode::BasicBlock &block) {
    mcode::StackFrame &frame = block.get_func()->get_stack_frame();

    for (mcode::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        mcode::Instruction &instr = *iter;
        mcode::Opcode opcode = instr.get_opcode();

        if (!Utils::is_one_of(opcode, {AArch64Opcode::ADD, AArch64Opcode::SUB})) {
            continue;
        }

        mcode::Operand &operand = instr.get_operand(2);
        if (!operand.is_stack_slot_offset() || instr.get_operands().size() == 4) {
            continue;
        }

        mcode::Operand::StackSlotOffset offset = operand.get_stack_slot_offset();
        mcode::StackSlot &stack_slot = frame.get_stack_slot(offset.slot_index);
        unsigned total_offset = stack_slot.get_offset() + offset.addend;

        if (total_offset < 4096) {
            continue;
        }

        ASSERT(total_offset < 4096 * 4096);

        operand = mcode::Operand::from_int_immediate(total_offset >> 12, operand.get_size());
        instr.get_operands().push_back(mcode::Operand::from_aarch64_left_shift(12));

        mcode::Operand m_imm_remainder = mcode::Operand::from_int_immediate(total_offset & 0xFFF);
        block.insert_after(
            iter,
            mcode::Instruction(opcode, {instr.get_operand(0), instr.get_operand(0), m_imm_remainder})
        );
    }
}

} // namespace target
} // namespace banjo
