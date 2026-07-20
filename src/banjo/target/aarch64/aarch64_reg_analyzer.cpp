#include "aarch64_reg_analyzer.hpp"

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/mcode/calling_convention.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::target {

AArch64RegAnalyzer::AArch64RegAnalyzer(std::optional<mcode::PhysicalReg> fixed_reg) {
    for (mcode::PhysicalReg reg = AArch64Register::R0; reg <= AArch64Register::R28; reg++) {
        if (reg == SCRATCH_REGISTER) {
            continue;
        }

        if (fixed_reg && reg == fixed_reg) {
            continue;
        }

        general_purpose_regs.push_back(reg);
    }

    for (unsigned i = AArch64Register::V0; i <= AArch64Register::V30; i++) {
        fp_and_vector_regs.push_back(i);
    }
}

const std::vector<mcode::PhysicalReg> &AArch64RegAnalyzer::get_candidates(codegen::RegClass reg_class) {
    switch (reg_class) {
        case AArch64RegClass::GENERAL_PURPOSE: return general_purpose_regs;
        case AArch64RegClass::FP_AND_VECTOR: return fp_and_vector_regs;
        default: ASSERT_UNREACHABLE;
    }
}

void AArch64RegAnalyzer::suggest_regs(
    codegen::RegAllocFunc &func,
    const codegen::Bundle &bundle,
    std::vector<mcode::PhysicalReg> &suggested_regs
) {
    for (const codegen::Segment &segment : bundle.segments) {
        const codegen::LiveRange &range = segment.range;

        mcode::Instruction &first_def = *func.blocks[range.block].instrs[range.start.instr].iter;
        mcode::Instruction &last_use = *func.blocks[range.block].instrs[range.end.instr].iter;

        if (is_move_opcode(first_def.get_opcode()) && first_def.get_operand(1).is_physical_reg()) {
            suggested_regs.push_back(first_def.get_operand(1).get_physical_reg());
        }

        if (is_move_opcode(last_use.get_opcode()) && last_use.get_operand(0).is_physical_reg()) {
            suggested_regs.push_back(last_use.get_operand(0).get_physical_reg());
        }
    }
}

bool AArch64RegAnalyzer::is_reg_overridden(
    mcode::Instruction &instr,
    mcode::BasicBlock &basic_block,
    mcode::PhysicalReg reg
) {
    if (instr.get_opcode() == AArch64Opcode::BL || instr.get_opcode() == AArch64Opcode::BLR) {
        // TODO: dest calling conv instead of origin calling conv
        return basic_block.get_func()->get_calling_conv()->is_volatile(reg);
    }

    if (instr.get_opcode() >= AArch64Opcode::MOV && instr.get_opcode() <= AArch64Opcode::MOVK) {
        mcode::Operand &dest = instr.get_operand(0);
        if (dest.is_physical_reg() && dest.get_physical_reg() == reg) {
            return true;
        }
    }

    return false;
}

std::vector<mcode::RegOp> AArch64RegAnalyzer::get_operands(mcode::InstrIter iter, mcode::BasicBlock &block) {
    using namespace AArch64Opcode;

    mcode::Instruction &instr = *iter;
    std::vector<mcode::RegOp> operands;

    if (instr.get_opcode() == BL || instr.get_opcode() == BLR) {
        for (mcode::PhysicalReg physical_reg : block.get_func()->get_calling_conv()->get_volatile_regs()) {
            operands.push_back({mcode::Register::from_physical(physical_reg), mcode::RegUsage::KILL});
        }

        mcode::InstrIter prev = iter.get_prev();
        while (prev != block.begin().get_prev() && prev->get_opcode() != BL && prev->get_opcode() != BLR) {
            if (prev->get_dest().is_physical_reg()) {
                operands.push_back({prev->get_dest().get_register(), mcode::RegUsage::USE});
            }

            prev = prev.get_prev();
        }
    }

    switch (instr.get_opcode()) {
        case MOV:
        case FMOV: {
            mcode::Operand &dst = instr.get_operand(0);
            mcode::Operand &src = instr.get_operand(1);

            if (!(dst.is_register() && src.is_register() && src.get_register() == dst.get_register())) {
                collect_regs(dst, mcode::RegUsage::DEF, operands);
                collect_regs(src, mcode::RegUsage::USE, operands);
            }

            break;
        }

        case MOVZ:
        case MOVK:
        case LDR:
        case LDRB:
        case LDRH:
        case FCVT:
        case SCVTF:
        case UCVTF:
        case FCVTZS:
        case FCVTZU:
        case ADRP:
        case UXTB:
        case UXTH:
        case SXTB:
        case SXTH:
        case SXTW:
            collect_regs(instr.get_operand(0), mcode::RegUsage::DEF, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            break;

        case STR:
        case STRB:
        case STRH:
        case CMP:
        case FCMP:
            collect_regs(instr.get_operand(0), mcode::RegUsage::USE, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            break;

        case LDP:
            collect_regs(instr.get_operand(0), mcode::RegUsage::DEF, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::DEF, operands);
            collect_regs(instr.get_operand(2), mcode::RegUsage::USE, operands);
            break;

        case STP:
            collect_regs(instr.get_operand(0), mcode::RegUsage::USE, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            collect_regs(instr.get_operand(2), mcode::RegUsage::USE, operands);
            break;

        case ADD:
        case SUB:
        case MUL:
        case SDIV:
        case UDIV:
        case AND:
        case ORR:
        case EOR: // TODO: I guess we should handle the case where `src` and `dst` are the same register.
        case LSL:
        case LSR:
        case ASR:
        case CSEL:
        case FADD:
        case FSUB:
        case FMUL:
        case FDIV:
        case FCSEL:
            collect_regs(instr.get_operand(0), mcode::RegUsage::DEF, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            collect_regs(instr.get_operand(2), mcode::RegUsage::USE, operands);
            break;

        case MADD:
        case MSUB:
            collect_regs(instr.get_operand(0), mcode::RegUsage::DEF, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            collect_regs(instr.get_operand(2), mcode::RegUsage::USE, operands);
            collect_regs(instr.get_operand(3), mcode::RegUsage::USE, operands);
            break;

        case B:
        case BR:
        case B_EQ:
        case B_NE:
        case B_HS:
        case B_LO:
        case B_HI:
        case B_LS:
        case B_GE:
        case B_LT:
        case B_GT:
        case B_LE:
        case BLR: collect_regs(instr.get_operand(0), mcode::RegUsage::USE, operands); break;

        case BL:
        case RET: break;

        default: ASSERT_UNREACHABLE;
    }

    return operands;
}

void AArch64RegAnalyzer::assign_reg_classes(mcode::Instruction &instr, codegen::RegClassMap &reg_classes) {
    if (instr.get_operands().size() == 0 || !instr.get_operand(0).is_virtual_reg()) {
        return;
    }

    ssa::VirtualRegister reg = instr.get_operand(0).get_virtual_reg();

    if (is_float_instr(instr)) {
        reg_classes.insert({reg, AArch64RegClass::FP_AND_VECTOR});
    } else {
        reg_classes.insert({reg, AArch64RegClass::GENERAL_PURPOSE});
    }
}

bool AArch64RegAnalyzer::is_move_from(mcode::Instruction &instr, ssa::VirtualRegister src_reg) {
    mcode::Opcode opcode = instr.get_opcode();

    switch (opcode) {
        case AArch64Opcode::MOV:
        case AArch64Opcode::FMOV: break;
        default: return false;
    }

    mcode::Operand &src = instr.get_operand(1);
    return src.is_virtual_reg() && src.get_virtual_reg() == src_reg;
}

void AArch64RegAnalyzer::insert_load(SpilledRegUse use) {
    unsigned size = use.block.get_func()->get_stack_frame().get_stack_slot(use.stack_slot).get_size();
    unsigned reg_size = size == 8 ? 8 : 4;

    mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_physical(use.reg), reg_size);
    mcode::Operand addr = mcode::Operand::from_stack_slot(use.stack_slot, 8);

    mcode::Opcode opcode;

    switch (size) {
        case 1: opcode = AArch64Opcode::LDRB; break;
        case 2: opcode = AArch64Opcode::LDRH; break;
        case 4: opcode = AArch64Opcode::LDR; break;
        case 8: opcode = AArch64Opcode::LDR; break;
        default: ASSERT_UNREACHABLE;
    }

    use.block.insert_before(use.instr_iter, {opcode, {dst, addr}});
}

void AArch64RegAnalyzer::insert_store(SpilledRegUse use) {
    unsigned size = use.block.get_func()->get_stack_frame().get_stack_slot(use.stack_slot).get_size();
    unsigned reg_size = size == 8 ? 8 : 4;

    mcode::Operand src = mcode::Operand::from_register(mcode::Register::from_physical(use.reg), reg_size);
    mcode::Operand addr = mcode::Operand::from_stack_slot(use.stack_slot, 8);

    mcode::Opcode opcode;

    switch (size) {
        case 1: opcode = AArch64Opcode::STRB; break;
        case 2: opcode = AArch64Opcode::STRH; break;
        case 4: opcode = AArch64Opcode::STR; break;
        case 8: opcode = AArch64Opcode::STR; break;
        default: ASSERT_UNREACHABLE;
    }

    use.block.insert_after(use.instr_iter, {opcode, {src, addr}});
}

bool AArch64RegAnalyzer::is_instr_removable(mcode::Instruction &instr) {
    if (instr.get_opcode() != AArch64Opcode::MOV && instr.get_opcode() != AArch64Opcode::FMOV) {
        return false;
    }

    const mcode::Operand &dest = instr.get_operand(0);
    const mcode::Operand &src = instr.get_operand(1);
    return src.is_register() && dest.is_register() && src.get_register() == dest.get_register();
}

bool AArch64RegAnalyzer::is_move_opcode(mcode::Opcode opcode) {
    using namespace AArch64Opcode;
    return opcode == MOV || opcode == FMOV;
}

bool AArch64RegAnalyzer::is_float_instr(mcode::Instruction &instr) {
    using namespace AArch64Opcode;

    switch (instr.get_opcode()) {
        case FMOV:
        case FADD:
        case FSUB:
        case FMUL:
        case FDIV:
        case FCVT:
        case SCVTF:
        case UCVTF:
        case FCSEL:
        case FCMP: return true;
        default: return instr.is_flag(mcode::Instruction::FLAG_FLOAT);
    }
}

void AArch64RegAnalyzer::collect_regs(mcode::Operand &operand, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst) {
    if (operand.is_register()) {
        insert_reg(operand.get_register(), usage, dst);
    } else if (operand.is_aarch64_addr()) {
        collect_addr_regs(operand, dst);
    }
}

void AArch64RegAnalyzer::collect_addr_regs(mcode::Operand &operand, std::vector<mcode::RegOp> &dst) {
    const AArch64Address &addr = operand.get_aarch64_addr();
    insert_reg(addr.get_base(), mcode::RegUsage::USE, dst);

    if (addr.get_type() == AArch64Address::Type::BASE_OFFSET_REG) {
        insert_reg(addr.get_offset_reg().reg, mcode::RegUsage::USE, dst);
    }
}

void AArch64RegAnalyzer::insert_reg(mcode::Register reg, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst) {
    ASSERT(usage == mcode::RegUsage::USE || usage == mcode::RegUsage::DEF);

    for (mcode::RegOp &existing : dst) {
        if (existing.reg != reg) {
            continue;
        }

        if (existing.usage != usage) {
            existing.usage = mcode::RegUsage::USE_DEF;
        }

        return;
    }

    dst.push_back({.reg = reg, .usage = usage});
}

} // namespace banjo::target
