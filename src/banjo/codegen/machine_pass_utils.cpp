#include "machine_pass_utils.hpp"

#include "banjo/mcode/calling_convention.hpp"

#include <algorithm>

namespace banjo {

namespace codegen {

void MachinePassUtils::replace_virtual_reg(mcode::BasicBlock &basic_block, long old_register, long new_register) {
    mcode::Register m_reg = mcode::Register::from_virtual(new_register);

    for (mcode::Instruction &instr : basic_block) {
        for (mcode::Operand &operand : instr.get_operands()) {
            if (operand.is_virtual_reg() && operand.get_virtual_reg() == old_register) {
                operand = mcode::Operand::from_register(m_reg, operand.get_size());
            }

            if (operand.is_addr()) {
                mcode::IndirectAddress &addr = operand.get_addr();

                if (addr.is_base_reg() && addr.get_base_reg() == mcode::Register::from_virtual(old_register)) {
                    addr.set_base(m_reg);
                }

                if (addr.has_reg_offset() && addr.get_reg_offset() == mcode::Register::from_virtual(old_register)) {
                    addr.set_reg_offset(m_reg);
                }
            }
        }
    }
}

void MachinePassUtils::replace_reg(mcode::Instruction &instr, mcode::Register old_reg, mcode::Register new_reg) {
    for (mcode::Operand &operand : instr.get_operands()) {
        if (operand.is_register() && operand.get_register() == old_reg) {
            operand = mcode::Operand::from_register(new_reg, operand.get_size());
        }

        if (operand.is_addr()) {
            mcode::IndirectAddress &addr = operand.get_addr();

            if (addr.is_base_reg() && addr.get_base_reg() == old_reg) {
                addr.set_base(new_reg);
            }

            if (addr.has_reg_offset() && addr.get_reg_offset() == old_reg) {
                addr.set_reg_offset(new_reg);
            }
        }
    }
}

void MachinePassUtils::replace(mcode::BasicBlock &basic_block, mcode::Operand old_operand, mcode::Operand new_operand) {
    for (mcode::Instruction &instr : basic_block) {
        for (mcode::Operand &operand : instr.get_operands()) {
            if (operand == old_operand) {
                operand = new_operand;
            }
        }
    }
}

std::vector<long> MachinePassUtils::get_modified_volatile_regs(mcode::Function *func) {
    std::vector<long> result;

    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (mcode::Instruction &instr : basic_block) {
            if (instr.has_dest() && instr.get_dest().is_physical_reg() &&
                !basic_block.get_func()->get_calling_conv()->is_volatile(instr.get_dest().get_physical_reg()) &&
                std::find(result.begin(), result.end(), instr.get_dest().get_physical_reg()) == result.end()) {
                result.push_back(instr.get_dest().get_physical_reg());
            }
        }
    }

    return result;
}

} // namespace codegen

} // namespace banjo
