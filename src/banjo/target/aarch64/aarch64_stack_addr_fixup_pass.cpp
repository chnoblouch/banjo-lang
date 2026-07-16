#include "aarch64_stack_addr_fixup_pass.hpp"

#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_address.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_encoding_info.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_reg_analyzer.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"

namespace banjo::target {

void AArch64StackAddrFixupPass::run(mcode::Module &module_) {
    for (mcode::Function *func : module_.get_functions()) {
        for (mcode::BasicBlock &block : *func) {
            run(block);
        }
    }
}

void AArch64StackAddrFixupPass::run(mcode::BasicBlock &block) {
    for (mcode::InstrIter instr = block.begin(); instr != block.end(); ++instr) {
        switch (instr->get_opcode()) {
            case AArch64Opcode::ADD:
            case AArch64Opcode::SUB: process_add_sub(block, instr); break;
            case AArch64Opcode::LDR:
            case AArch64Opcode::STR:
            case AArch64Opcode::LDP:
            case AArch64Opcode::STP: process_ldr_str(block, instr, instr->get_operand(0).get_size()); break;
            case AArch64Opcode::LDRB:
            case AArch64Opcode::STRB: process_ldr_str(block, instr, 1); break;
            case AArch64Opcode::LDRH:
            case AArch64Opcode::STRH: process_ldr_str(block, instr, 2); break;
        }
    }
}

void AArch64StackAddrFixupPass::process_add_sub(mcode::BasicBlock &block, mcode::InstrIter instr) {
    mcode::StackFrame &frame = block.get_func()->get_stack_frame();

    mcode::Operand &operand = instr->get_operand(2);
    if (!operand.is_stack_offset() || instr->get_operands().size() == 4) {
        return;
    }

    mcode::StackAddress stack_addr = operand.get_stack_offset();
    mcode::StackSlot &stack_slot = frame.get_stack_slot(stack_addr.slot);
    unsigned total_offset = stack_slot.get_offset() + stack_addr.offset;

    if (total_offset < 4096) {
        return;
    }

    ASSERT(total_offset < 4096 * 4096);

    operand = mcode::Operand::from_int_immediate(total_offset >> 12, operand.get_size());
    instr->get_operands().push_back(mcode::Operand::from_aarch64_left_shift(12));

    mcode::Operand m_imm_remainder = mcode::Operand::from_int_immediate(total_offset & 0xFFF);
    block.insert_after(instr, {instr->get_opcode(), {instr->get_operand(0), instr->get_operand(0), m_imm_remainder}});
}

void AArch64StackAddrFixupPass::process_ldr_str(mcode::BasicBlock &block, mcode::InstrIter instr, unsigned size) {
    mcode::StackFrame &frame = block.get_func()->get_stack_frame();
    mcode::Operand &addr = instr->get_operand(1);

    if (!addr.is_stack_slot()) {
        return;
    }

    mcode::StackSlot &stack_slot = frame.get_stack_slot(addr.get_stack_slot());
    unsigned offset = stack_slot.get_offset();

    if (AArch64EncodingInfo::is_addr_offset_encodable(offset, size)) {
        return;
    }

    ASSERT(offset < 4096);

    mcode::Register tmp_reg = mcode::Register::from_physical(AArch64RegAnalyzer::SCRATCH_REGISTER);

    mcode::Operand m_tmp = mcode::Operand::from_register(tmp_reg, 8);
    mcode::Operand m_sp = mcode::Operand::from_register(mcode::Register::from_physical(AArch64Register::SP), 8);
    mcode::Operand m_offset = mcode::Operand::from_int_immediate(offset);
    block.insert_before(instr, {AArch64Opcode::ADD, {m_tmp, m_sp, m_offset}});

    addr = mcode::Operand::from_aarch64_addr(AArch64Address::new_base(tmp_reg));
}

} // namespace banjo::target
