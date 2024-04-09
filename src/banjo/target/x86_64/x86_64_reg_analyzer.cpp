#include "x86_64_reg_analyzer.hpp"

#include "target/x86_64/x86_64_opcode.hpp"

namespace target {

X8664RegAnalyzer::X8664RegAnalyzer() {
    using namespace X8664Register;
    general_purpose_regs = {RAX, RCX, RDX, R8, R9, R10, R11, RBX, RSI, RDI, R12, R13, /*R14, R15*/};
    float_regs =
        {XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, /*XMM14, XMM15*/};
}

std::vector<int> X8664RegAnalyzer::get_all_registers() {
    std::vector<int> regs;
    for (int i = 0; i < NUM_REGS; i++) {
        regs.push_back(i);
    }
    return regs;
}

const std::vector<int> &X8664RegAnalyzer::get_candidates(mcode::Instruction &instr) {
    return is_float_operand(instr.get_opcode(), mcode::RegUsage::DEF) ? float_regs : general_purpose_regs;
}

std::vector<mcode::PhysicalReg> X8664RegAnalyzer::suggest_regs(
    codegen::RegAllocFunc &func,
    const codegen::LiveRangeGroup &group
) {
    codegen::LiveRange first_range = group.ranges[0];
    codegen::LiveRange last_range = group.ranges[0];

    mcode::Instruction &first_def = *func.blocks[first_range.block].instrs[first_range.start].iter;
    mcode::Instruction &last_use = *func.blocks[last_range.block].instrs[last_range.end].iter;

    std::vector<mcode::PhysicalReg> suggested_regs;

    if (is_move_opcode(first_def.get_opcode()) && first_def.get_operand(1).is_physical_reg()) {
        suggested_regs.push_back(first_def.get_operand(1).get_physical_reg());
    }

    if (is_move_opcode(last_use.get_opcode()) && last_use.get_operand(0).is_physical_reg()) {
        suggested_regs.push_back(last_use.get_operand(0).get_physical_reg());
    }

    return suggested_regs;
}

bool X8664RegAnalyzer::is_reg_overridden(mcode::Instruction &instr, mcode::BasicBlock &basic_block, int reg) {
    if (instr.get_opcode() == X8664Opcode::CALL) {
        // TODO: dest calling conv instead of origin calling conv
        return basic_block.get_func()->get_calling_conv()->is_volatile(reg);
    }

    if (instr.get_opcode() == X8664Opcode::IDIV) {
        return reg == X8664Register::RAX || reg == X8664Register::RDX;
    }

    if (instr.get_operand(0).is_physical_reg()) {
        return instr.get_operand(0).get_physical_reg() == reg;
    } else if (instr.get_operand(1).is_physical_reg()) {
        return instr.get_operand(1).get_physical_reg() == reg;
    } else {
        return false;
    }
}

std::vector<mcode::RegOp> X8664RegAnalyzer::get_operands(mcode::InstrIter iter, mcode::BasicBlock &block) {
    using namespace X8664Opcode;

    mcode::Instruction &instr = *iter;
    std::vector<mcode::RegOp> operands;

    if (instr.get_opcode() == CALL) {
        std::vector<mcode::PhysicalReg> physical_regs = block.get_func()->get_calling_conv()->get_volatile_regs();
        for (mcode::PhysicalReg physical_reg : physical_regs) {
            operands.push_back({mcode::Register::from_physical(physical_reg), mcode::RegUsage::KILL});
        }

        mcode::InstrIter prev = iter.get_prev();
        while (prev != block.begin().get_prev() && prev->get_opcode() != CALL) {
            if (prev->get_dest().is_physical_reg()) {
                operands.push_back({prev->get_dest().get_register(), mcode::RegUsage::USE});
            }

            prev = prev.get_prev();
        }
    }

    switch (instr.get_opcode()) {
        case MOV:
        case MOVSX:
        case MOVZX:
        case MOVSS:
        case MOVSD:
        case MOVAPS:
        case MOVUPS:
        case MOVD:
        case MOVQ:
        case LEA:
        case CVTSS2SD:
        case CVTSD2SS:
        case CVTSI2SS:
        case CVTSI2SD:
        case CVTSS2SI:
        case CVTSD2SI: add_du_ops(instr, operands); break;

        case PUSH:
        case CALL:
        case RET:
            if (instr.get_operand(0).is_register()) {
                operands.push_back({instr.get_operand(0).get_register(), mcode::RegUsage::USE});
            } else if (instr.get_operand(0).is_addr()) {
                collect_addr_regs(instr.get_operand(0), operands);
            }

            break;

        case POP: operands.push_back({instr.get_operand(0).get_register(), mcode::RegUsage::DEF}); break;

        case ADD:
        case SUB:
        case IMUL:
        case AND:
        case OR:
        case SHL:
        case SHR:
        case CMOVE:
        case CMOVNE:
        case CMOVA:
        case CMOVAE:
        case CMOVB:
        case CMOVBE:
        case CMOVG:
        case CMOVGE:
        case CMOVL:
        case CMOVLE:
        case ADDSS:
        case ADDSD:
        case SUBSS:
        case SUBSD:
        case MULSS:
        case MULSD:
        case DIVSS:
        case DIVSD:
        case MINSS:
        case MINSD:
        case MAXSS:
        case MAXSD:
        case SQRTSS:
        case SQRTSD: add_udu_ops(instr, operands); break;

        case CDQ:
        case CQO:
            operands.push_back({mcode::Register::from_physical(X8664Register::RAX), mcode::RegUsage::USE});
            operands.push_back({mcode::Register::from_physical(X8664Register::RDX), mcode::RegUsage::DEF});
            break;

        case IDIV:
            operands.push_back({instr.get_operand(0).get_register(), mcode::RegUsage::USE});
            operands.push_back({mcode::Register::from_physical(X8664Register::RAX), mcode::RegUsage::USE_DEF});
            operands.push_back({mcode::Register::from_physical(X8664Register::RDX), mcode::RegUsage::USE_DEF});
            break;

        case CMP:
        case UCOMISS:
        case UCOMISD:
            collect_regs(instr.get_operand(0), mcode::RegUsage::USE, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            break;

        case XOR:
        case XORPS:
        case XORPD:
            if (instr.get_operand(0) == instr.get_operand(1)) {
                // If dst and src are the same register, this instruction sets the register to zero
                // and is handled as if it doesn't read its inputs.
                operands.push_back({instr.get_operand(0).get_register(), mcode::RegUsage::DEF});
            } else {
                add_udu_ops(instr, operands);
            }

            break;
    }

    return operands;
}

mcode::PhysicalReg X8664RegAnalyzer::insert_spill_reload(SpilledRegUse use) {
    int dst_size = use.instr_iter->get_operand(0).get_size();
    int src_size = use.instr_iter->get_operand(1).get_size();
    bool is_float = is_float_operand(use.instr_iter->get_opcode(), use.usage);
    int move_opcode = is_float ? (src_size == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD) : target::X8664Opcode::MOV;

    mcode::PhysicalReg tmp_reg;
    if (is_float) tmp_reg = target::X8664Register::XMM15 - use.spill_tmp_regs;
    else tmp_reg = target::X8664Register::R15 - use.spill_tmp_regs;

    mcode::Register stack_slot_reg = mcode::Register::from_stack_slot(use.stack_slot);
    mcode::Operand src = mcode::Operand::from_register(stack_slot_reg, src_size);
    mcode::Operand tmp_val = mcode::Operand::from_register(mcode::Register::from_physical(tmp_reg), src_size);
    mcode::Operand dst = mcode::Operand::from_register(stack_slot_reg, dst_size);

    if (use.usage == mcode::RegUsage::USE) {
        use.block.insert_before(use.instr_iter, mcode::Instruction(move_opcode, {tmp_val, src}));
    } else if (use.usage == mcode::RegUsage::DEF) {
        use.block.insert_after(use.instr_iter, mcode::Instruction(move_opcode, {dst, tmp_val}));
    } else if (use.usage == mcode::RegUsage::USE_DEF) {
        use.block.insert_before(use.instr_iter, mcode::Instruction(move_opcode, {tmp_val, src}));
        use.block.insert_after(use.instr_iter, mcode::Instruction(move_opcode, {dst, tmp_val}));
    }

    return tmp_reg;
}

bool X8664RegAnalyzer::is_instr_removable(mcode::Instruction &instr) {
    if (!is_move_opcode(instr.get_opcode())) {
        return false;
    }

    mcode::Operand &dst = instr.get_dest();
    mcode::Operand &src = instr.get_operands()[1];
    return dst.is_register() && src.is_register() && dst.get_register() == src.get_register();
}

bool X8664RegAnalyzer::is_move_opcode(mcode::Opcode opcode) {
    using namespace X8664Opcode;
    return opcode == MOV || (opcode >= MOVSS && opcode <= MOVUPS);
}

bool X8664RegAnalyzer::is_float_operand(mcode::Opcode opcode, mcode::RegUsage usage) {
    using namespace X8664Opcode;

    if ((opcode >= MOVSS && opcode <= MOVUPS) || (opcode >= ADDSS && opcode <= UCOMISD) || opcode == CVTSS2SD ||
        opcode == CVTSD2SS) {
        return true;
    }

    if (usage == mcode::RegUsage::DEF || usage == mcode::RegUsage::USE_DEF) {
        return opcode == CVTSI2SS || opcode == CVTSI2SD;
    } else if (usage == mcode::RegUsage::USE) {
        return opcode == CVTSS2SI || opcode == CVTSD2SI;
    }

    return false;
}

void X8664RegAnalyzer::add_du_ops(mcode::Instruction &instr, std::vector<mcode::RegOp> &dst) {
    collect_regs(instr.get_operand(0), mcode::RegUsage::DEF, dst);
    collect_regs(instr.get_operand(1), mcode::RegUsage::USE, dst);
}

void X8664RegAnalyzer::add_udu_ops(mcode::Instruction &instr, std::vector<mcode::RegOp> &dst) {
    collect_regs(instr.get_operand(0), mcode::RegUsage::USE_DEF, dst);
    collect_regs(instr.get_operand(1), mcode::RegUsage::USE, dst);
}

void X8664RegAnalyzer::collect_regs(mcode::Operand &operand, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst) {
    if (operand.is_register()) {
        dst.push_back({.reg = operand.get_register(), .usage = usage});
    } else if (operand.is_addr()) {
        collect_addr_regs(operand, dst);
    }
}

void X8664RegAnalyzer::collect_addr_regs(mcode::Operand &operand, std::vector<mcode::RegOp> &dst) {
    mcode::IndirectAddress &addr = operand.get_addr();

    if (!addr.get_base().is_stack_slot()) {
        dst.push_back({addr.get_base(), mcode::RegUsage::USE});
    }

    if (addr.has_reg_offset() && !addr.get_reg_offset().is_stack_slot()) {
        dst.push_back({addr.get_reg_offset(), mcode::RegUsage::USE});
    }
}

} // namespace target
