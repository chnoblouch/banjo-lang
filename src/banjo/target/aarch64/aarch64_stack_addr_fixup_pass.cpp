#include "aarch64_stack_addr_fixup_pass.hpp"

#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_address.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_encoding_info.hpp"
#include "banjo/target/aarch64/aarch64_immediate.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_reg_analyzer.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include <array>
#include <cstdint>

namespace banjo::target {

void AArch64StackAddrFixupPass::run(mcode::Module &mod) {
    for (mcode::Function *func : mod.get_functions()) {
        for (mcode::BasicBlock &block : *func) {
            run(block);
        }
    }
}

void AArch64StackAddrFixupPass::run(mcode::BasicBlock &block) {
    this->block = &block;

    for (mcode::InstrIter instr = block.begin(); instr != block.end(); ++instr) {
        switch (instr->get_opcode()) {
            case AArch64Opcode::ADD:
            case AArch64Opcode::SUB: process_add_sub(instr); break;
            case AArch64Opcode::LDR:
            case AArch64Opcode::STR:
            case AArch64Opcode::LDP:
            case AArch64Opcode::STP: process_ldr_str(instr, instr->get_operand(0).get_size()); break;
            case AArch64Opcode::LDRB:
            case AArch64Opcode::STRB: process_ldr_str(instr, 1); break;
            case AArch64Opcode::LDRH:
            case AArch64Opcode::STRH: process_ldr_str(instr, 2); break;
        }
    }
}

void AArch64StackAddrFixupPass::process_add_sub(mcode::InstrIter instr) {
    mcode::StackFrame &frame = block->get_func()->get_stack_frame();

    mcode::Operand &m_offset = instr->get_operand(2);
    if (!m_offset.is_stack_offset() || instr->get_operands().size() == 4) {
        return;
    }

    mcode::StackAddress stack_addr = m_offset.get_stack_offset();
    mcode::StackSlot &stack_slot = frame.get_stack_slot(stack_addr.slot);
    unsigned total_offset = stack_slot.get_offset() + stack_addr.offset;

    if (total_offset >= 4096) {
        m_offset = emit_imm_mov(instr, total_offset);
    }
}

void AArch64StackAddrFixupPass::process_ldr_str(mcode::InstrIter instr, unsigned size) {
    mcode::StackFrame &frame = block->get_func()->get_stack_frame();
    mcode::Operand &m_addr = instr->get_operand(1);

    if (m_addr.is_stack_slot()) {
        mcode::StackSlot &stack_slot = frame.get_stack_slot(m_addr.get_stack_slot());
        unsigned offset = stack_slot.get_offset();

        if (!AArch64EncodingInfo::is_addr_offset_encodable(offset, size)) {
            m_addr = emit_compute_stack_addr(instr, offset);
        }
    } else if (m_addr.is_aarch64_addr()) {
        const AArch64Address &addr = m_addr.get_aarch64_addr();

        if (addr.get_type() != AArch64Address::Type::BASE_OFFSET_STACK_ADDR) {
            return;
        }

        unsigned offset = frame.offset_of(addr.get_offset_stack_addr());

        if (!AArch64EncodingInfo::is_addr_offset_encodable(offset, size)) {
            m_addr = emit_compute_stack_addr(instr, offset);
        }
    }
}

mcode::Operand AArch64StackAddrFixupPass::emit_compute_stack_addr(mcode::InstrIter instr, unsigned offset) {
    mcode::Register tmp_reg = mcode::Register::from_physical(AArch64RegAnalyzer::SCRATCH_REGISTER);

    mcode::Operand m_tmp = mcode::Operand::from_register(tmp_reg, 8);
    mcode::Operand m_sp = mcode::Operand::from_register(mcode::Register::from_physical(AArch64Register::SP), 8);
    mcode::Operand m_offset;

    if (offset < 4096) {
        m_offset = mcode::Operand::from_int_immediate(offset, 8);
    } else {
        m_offset = emit_imm_mov(instr, offset);
    }

    block->insert_before(instr, {AArch64Opcode::ADD, {m_tmp, m_sp, m_offset}});
    return mcode::Operand::from_aarch64_addr(AArch64Address::new_base(tmp_reg));
}

mcode::Operand AArch64StackAddrFixupPass::emit_imm_mov(mcode::InstrIter instr, unsigned immediate) {
    mcode::Register tmp_reg = mcode::Register::from_physical(AArch64RegAnalyzer::SCRATCH_REGISTER);
    mcode::Operand m_tmp = mcode::Operand::from_register(tmp_reg, 8);

    if (immediate <= 0xFFFF) {
        mcode::Operand m_imm = mcode::Operand::from_int_immediate(immediate, 8);
        block->insert_before(instr, {AArch64Opcode::MOVZ, {m_tmp, m_imm}});
    } else {
        // TODO: Optimize edge cases.

        std::array<std::uint16_t, 2> elements = AArch64Immediate::decompose_u32_u16(immediate);
        mcode::Operand m_lower = mcode::Operand::from_int_immediate(elements[0], 8);
        mcode::Operand m_upper = mcode::Operand::from_int_immediate(elements[1], 8);
        mcode::Operand m_shift = mcode::Operand::from_aarch64_left_shift(16, 8);

        block->insert_before(instr, {AArch64Opcode::MOVZ, {m_tmp, m_lower}});
        block->insert_before(instr, {AArch64Opcode::MOVK, {m_tmp, m_upper, m_shift}});
    }

    return m_tmp;
}

} // namespace banjo::target
