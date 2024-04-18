#include "aarch64_reg_analyzer.hpp"

#include "target/aarch64/aarch64_opcode.hpp"
#include "utils/macros.hpp"

namespace target {

AArch64RegAnalyzer::AArch64RegAnalyzer() {
    using namespace AArch64Register;

    for (int i = R0; i <= R_LAST; i++) {
        general_purpose_regs.push_back(i);
    }

    for (int i = V0; i <= V_LAST; i++) {
        fp_and_vector_regs.push_back(i);
    }
}

std::vector<mcode::PhysicalReg> AArch64RegAnalyzer::get_all_registers() {
    std::vector<mcode::PhysicalReg> regs;
    for (mcode::PhysicalReg i = 0; i < NUM_REGS; i++) {
        regs.push_back(i);
    }
    return regs;
}

const std::vector<mcode::PhysicalReg> &AArch64RegAnalyzer::get_candidates(codegen::RegClass reg_class) {
    switch (reg_class) {
        case AArch64RegClass::GENERAL_PURPOSE: return general_purpose_regs;
        case AArch64RegClass::FP_AND_VECTOR: return fp_and_vector_regs;
        default: ASSERT_UNREACHABLE;
    }
}

std::vector<mcode::PhysicalReg> AArch64RegAnalyzer::suggest_regs(
    codegen::RegAllocFunc &func,
    const codegen::Bundle &bundle
) {
    // codegen::LiveRange first_range = group.ranges[0];
    // codegen::LiveRange last_range = group.ranges[0];

    // mcode::Instruction &first_def = *func.blocks[first_range.block].instrs[first_range.start].iter;
    // mcode::Instruction &last_use = *func.blocks[last_range.block].instrs[last_range.end].iter;

    std::vector<mcode::PhysicalReg> suggested_regs;

    // if (is_move_opcode(first_def.get_opcode()) && first_def.get_operand(1).is_physical_reg()) {
    //     suggested_regs.push_back(first_def.get_operand(1).get_physical_reg());
    // }

    // if (is_move_opcode(last_use.get_opcode()) && last_use.get_operand(0).is_physical_reg()) {
    //     suggested_regs.push_back(last_use.get_operand(0).get_physical_reg());
    // }

    return suggested_regs;
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
        std::vector<mcode::PhysicalReg> physical_regs = block.get_func()->get_calling_conv()->get_volatile_regs();
        for (mcode::PhysicalReg physical_reg : physical_regs) {
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
        case MOVZ:
        case MOVK:
        case LDR:
        case LDRB:
        case LDRH:
        case FMOV:
        case FCVT:
        case SCVTF:
        case FCVTZS:
        case FCVTZU:
        case ADRP:
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
        case EOR:
        case LSL:
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

        case RET: break;
    }

    return operands;
}

void AArch64RegAnalyzer::assign_reg_classes(mcode::Instruction &instr, codegen::RegClassMap &reg_classes) {
    if (instr.get_operands().get_size() == 0 || !instr.get_operand(0).is_virtual_reg()) {
        return;
    }

    ir::VirtualRegister reg = instr.get_operand(0).get_virtual_reg();

    if (is_float_instr(instr)) {
        reg_classes.insert({reg, AArch64RegClass::FP_AND_VECTOR});
    } else {
        reg_classes.insert({reg, AArch64RegClass::GENERAL_PURPOSE});
    }
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
        case FCSEL:
        case FCMP: return true;
        default: return instr.is_flag(mcode::Instruction::FLAG_FLOAT);
    }
}

void AArch64RegAnalyzer::collect_regs(mcode::Operand &operand, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst) {
    if (operand.is_register()) {
        dst.push_back({.reg = operand.get_register(), .usage = usage});
    } else if (operand.is_aarch64_addr()) {
        collect_addr_regs(operand, dst);
    }
}

void AArch64RegAnalyzer::collect_addr_regs(mcode::Operand &operand, std::vector<mcode::RegOp> &dst) {
    const AArch64Address &addr = operand.get_aarch64_addr();
    dst.push_back({addr.get_base(), mcode::RegUsage::USE});

    if (addr.get_type() == AArch64Address::Type::BASE_OFFSET_REG) {
        dst.push_back({addr.get_offset_reg(), mcode::RegUsage::KILL});
    }
}

} // namespace target
