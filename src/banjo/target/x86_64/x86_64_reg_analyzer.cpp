#include "x86_64_reg_analyzer.hpp"

#include "banjo/mcode/calling_convention.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace target {

X8664RegAnalyzer::X8664RegAnalyzer() {
    general_purpose_regs = {
        X8664Register::RAX,
        X8664Register::RCX,
        X8664Register::RDX,
        X8664Register::R8,
        X8664Register::R9,
        X8664Register::R10,
        X8664Register::R11,
        X8664Register::RBX,
        X8664Register::RSI,
        X8664Register::RDI,
        X8664Register::R12,
        X8664Register::R13,
        X8664Register::R14,
        X8664Register::R15,
    };

    sse_regs = {
        X8664Register::XMM0,
        X8664Register::XMM1,
        X8664Register::XMM2,
        X8664Register::XMM3,
        X8664Register::XMM4,
        X8664Register::XMM5,
        X8664Register::XMM6,
        X8664Register::XMM7,
        X8664Register::XMM8,
        X8664Register::XMM9,
        X8664Register::XMM10,
        X8664Register::XMM11,
        X8664Register::XMM12,
        X8664Register::XMM13,
        X8664Register::XMM14,
        X8664Register::XMM15,
    };
}

const std::vector<mcode::PhysicalReg> &X8664RegAnalyzer::get_candidates(codegen::RegClass reg_class) {
    switch (reg_class) {
        case X8664RegClass::GENERAL_PURPOSE: return general_purpose_regs;
        case X8664RegClass::SSE: return sse_regs;
        default: ASSERT_UNREACHABLE;
    }
}

std::vector<mcode::PhysicalReg> X8664RegAnalyzer::suggest_regs(
    codegen::RegAllocFunc &func,
    const codegen::Bundle &bundle
) {
    codegen::LiveRange first_range = bundle.segments[0].range;
    codegen::LiveRange last_range = bundle.segments[0].range;

    mcode::Instruction &first_def = *func.blocks[first_range.block].instrs[first_range.start.instr].iter;
    mcode::Instruction &last_use = *func.blocks[last_range.block].instrs[last_range.end.instr].iter;

    std::vector<mcode::PhysicalReg> suggested_regs;

    if (is_move_opcode(first_def.get_opcode()) && first_def.get_operand(1).is_physical_reg()) {
        suggested_regs.push_back(first_def.get_operand(1).get_physical_reg());
    }

    if (is_move_opcode(last_use.get_opcode()) && last_use.get_operand(0).is_physical_reg()) {
        suggested_regs.push_back(last_use.get_operand(0).get_physical_reg());
    }

    return suggested_regs;
}

bool X8664RegAnalyzer::is_reg_overridden(
    mcode::Instruction &instr,
    mcode::BasicBlock &basic_block,
    mcode::PhysicalReg reg
) {
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
        for (mcode::PhysicalReg physical_reg : block.get_func()->get_calling_conv()->get_volatile_regs()) {
            operands.push_back({mcode::Register::from_physical(physical_reg), mcode::RegUsage::KILL});
        }

        mcode::InstrIter prev = iter.get_prev();
        while (prev != block.begin().get_prev() && prev->get_opcode() != CALL) {
            if (prev->get_dest().is_physical_reg()) {
                operands.push_back({prev->get_dest().get_register(), mcode::RegUsage::USE});
            }

            prev = prev.get_prev();
        }

        // TODO: Move this into the calling convention.
        operands.push_back({mcode::Register::from_physical(X8664Register::RAX), mcode::RegUsage::DEF});
        operands.push_back({mcode::Register::from_physical(X8664Register::XMM0), mcode::RegUsage::DEF});
    }

    switch (instr.get_opcode()) {
        case MOV:
        case MOVSS:
        case MOVSD:
        case MOVAPS:
        case MOVUPS:
        case MOVD:
        case MOVQ: {
            mcode::Operand &dst = instr.get_operand(0);
            mcode::Operand &src = instr.get_operand(1);

            if (!(dst.is_register() && src.is_register() && src.get_register() == dst.get_register())) {
                collect_regs(dst, mcode::RegUsage::DEF, operands);
                collect_regs(src, mcode::RegUsage::USE, operands);
            }

            break;
        }

        case MOVSX:
        case MOVZX:
        case LEA:
        case CVTSS2SD:
        case CVTSD2SS:
        case CVTSI2SS:
        case CVTSI2SD:
        case CVTSS2SI:
        case CVTSD2SI:
            collect_regs(instr.get_operand(0), mcode::RegUsage::DEF, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            break;

        case PUSH:
        case CALL:
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
        case SQRTSD:
            collect_regs(instr.get_operand(0), mcode::RegUsage::USE_DEF, operands);
            collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            break;

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
                collect_regs(instr.get_operand(0), mcode::RegUsage::USE_DEF, operands);
                collect_regs(instr.get_operand(1), mcode::RegUsage::USE, operands);
            }

            break;
    }

    return operands;
}

void X8664RegAnalyzer::assign_reg_classes(mcode::Instruction &instr, codegen::RegClassMap &reg_classes) {
    using namespace X8664Opcode;

    if (instr.get_operands().size() == 0 || !instr.get_operand(0).is_virtual_reg()) {
        return;
    }

    mcode::Opcode opcode = instr.get_opcode();
    ssa::VirtualRegister reg = instr.get_operand(0).get_virtual_reg();

    if ((opcode >= MOVSS && opcode <= MOVUPS) || (opcode >= ADDSS && opcode <= UCOMISD) || opcode == CVTSS2SD ||
        opcode == CVTSD2SS || opcode == CVTSI2SS || opcode == CVTSI2SD) {
        reg_classes.insert({reg, X8664RegClass::SSE});
    } else {
        reg_classes.insert({reg, X8664RegClass::GENERAL_PURPOSE});
    }
}

bool X8664RegAnalyzer::is_move_from(mcode::Instruction &instr, ssa::VirtualRegister src_reg) {
    using namespace X8664Opcode;

    mcode::Opcode opcode = instr.get_opcode();
    if (opcode != MOV && opcode != MOVSS && opcode != MOVSD && opcode != MOVAPS && opcode != MOVUPS) {
        return false;
    }

    const mcode::Operand &src = instr.get_operand(1);
    return src.is_virtual_reg() && src.get_virtual_reg() == src_reg;
}

void X8664RegAnalyzer::insert_load(SpilledRegUse use) {
    unsigned size = use.block.get_func()->get_stack_frame().get_stack_slot(use.stack_slot).get_size();
    mcode::Operand src = mcode::Operand::from_stack_slot(use.stack_slot, size);
    mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_physical(use.reg), size);

    if (is_memory_operand_allowed(*use.instr_iter)) {
        use.instr_iter->get_operand(1) = src;
        return;
    }

    mcode::Opcode move_opcode = get_move_opcode(use.reg_class, size);
    use.block.insert_before(use.instr_iter, mcode::Instruction(move_opcode, {dst, src}));
}

void X8664RegAnalyzer::insert_store(SpilledRegUse use) {
    unsigned size = use.block.get_func()->get_stack_frame().get_stack_slot(use.stack_slot).get_size();
    mcode::Operand src = mcode::Operand::from_register(mcode::Register::from_physical(use.reg), size);
    mcode::Operand dst = mcode::Operand::from_stack_slot(use.stack_slot, size);

    if (is_memory_operand_allowed(*use.instr_iter)) {
        use.instr_iter->get_operand(0) = dst;
        return;
    }

    mcode::Opcode move_opcode = get_move_opcode(use.reg_class, size);
    use.block.insert_after(use.instr_iter, mcode::Instruction(move_opcode, {dst, src}));
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

bool X8664RegAnalyzer::is_memory_operand_allowed(mcode::Instruction &instr) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    if (!dst.is_register() || !src.is_register()) {
        return false;
    }

    switch (instr.get_opcode()) {
        case X8664Opcode::MOV:
        case X8664Opcode::MOVSS:
        case X8664Opcode::ADD: return true;
        default: return false;
    }
}

mcode::Opcode X8664RegAnalyzer::get_move_opcode(codegen::RegClass reg_class, unsigned size) {
    if (reg_class == X8664RegClass::GENERAL_PURPOSE) {
        return target::X8664Opcode::MOV;
    } else if (reg_class == X8664RegClass::SSE) {
        return size == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;
    } else {
        ASSERT_UNREACHABLE;
    }
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

    if (addr.is_base_reg()) {
        dst.push_back({addr.get_base_reg(), mcode::RegUsage::USE});
    }

    if (addr.has_reg_offset()) {
        dst.push_back({addr.get_reg_offset(), mcode::RegUsage::USE});
    }
}

} // namespace target

} // namespace banjo
