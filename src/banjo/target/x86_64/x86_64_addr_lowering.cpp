#include "x86_64_addr_lowering.hpp"

#include "banjo/config/config.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_ssa_lowerer.hpp"

namespace banjo {

namespace target {

X8664AddrLowering::X8664AddrLowering(X8664SSALowerer &lowerer) : lowerer(lowerer) {}

mcode::Operand X8664AddrLowering::lower_address(const ssa::Operand &operand) {
    if (operand.is_register()) {
        return lower_reg_addr(operand.get_register());
    } else if (operand.is_symbol()) {
        return lower_symbol_addr(operand);
    } else {
        return {};
    }
}

mcode::Operand X8664AddrLowering::lower_reg_addr(ssa::VirtualRegister vreg) {
    mcode::Register reg = lowerer.lower_reg(vreg);

    if (reg.is_virtual_reg()) {
        return lower_vreg_addr(reg);
    } else {
        return mcode::Operand::from_addr(mcode::IndirectAddress(reg), 8);
    }
}

mcode::Operand X8664AddrLowering::lower_vreg_addr(mcode::Register reg) {
    ssa::InstrIter producer = lowerer.get_producer_globally(reg.get_virtual_reg());

    if (producer->get_opcode() == ssa::Opcode::MEMBERPTR) {
        lowerer.discard_use(reg.get_virtual_reg());
        return mcode::Operand::from_addr(calc_memberptr_addr(*producer), 8);
    } else if (producer->get_opcode() == ssa::Opcode::OFFSETPTR) {
        lowerer.discard_use(reg.get_virtual_reg());
        return mcode::Operand::from_addr(calc_offsetptr_addr(*producer), 8);
    }

    return mcode::Operand::from_addr(mcode::IndirectAddress(reg), 8);
}

mcode::Operand X8664AddrLowering::lower_symbol_addr(const ssa::Operand &operand) {
    mcode::Relocation reloc = mcode::Relocation::NONE;
    if (lang::Config::instance().pic && lowerer.get_target()->get_descr().is_unix()) {
        if (operand.is_extern_func()) reloc = mcode::Relocation::PLT;
        else if (operand.is_extern_global()) reloc = mcode::Relocation::GOT;
    }

    mcode::Symbol symbol(operand.get_symbol_name(), reloc);
    unsigned size = lowerer.get_size(operand.get_type());
    return lowerer.deref_symbol_addr(symbol, size);
}

mcode::IndirectAddress X8664AddrLowering::calc_offsetptr_addr(ssa::Instruction &instr) {
    mcode::Register base = lowerer.lower_reg(instr.get_operand(0).get_register());
    ssa::Operand &operand = instr.get_operand(1);
    const ssa::Type &base_type = instr.get_operand(2).get_type();

    mcode::IndirectAddress addr(base, 0, 1);
    if (operand.is_int_immediate()) {
        unsigned int_offset = operand.get_int_immediate().to_u64();
        addr.set_int_offset(int_offset * lowerer.get_size(base_type));
    } else if (operand.is_register()) {
        addr.set_reg_offset(lowerer.lower_reg(operand.get_register()));
        addr.set_scale(lowerer.get_size(base_type));
    }

    if (addr.has_reg_offset()) {
        unsigned scale = addr.get_scale();

        if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
            mcode::Register offset_reg = lowerer.lower_reg(lowerer.get_func().next_virtual_reg());
            mcode::Register tmp_reg = lowerer.lower_reg(lowerer.get_func().next_virtual_reg());

            if (addr.get_base().is_virtual_reg()) {
                lowerer.emit(mcode::Instruction(
                    X8664Opcode::MOV,
                    {mcode::Operand::from_register(tmp_reg, 8), mcode::Operand::from_register(addr.get_base(), 8)}
                ));
            } else if (addr.get_base().is_stack_slot()) {
                lowerer.emit(mcode::Instruction(
                    X8664Opcode::LEA,
                    {mcode::Operand::from_register(tmp_reg, 8), mcode::Operand::from_register(addr.get_base(), 8)}
                ));
            }

            lowerer.emit(mcode::Instruction(
                X8664Opcode::MOV,
                {mcode::Operand::from_register(offset_reg, 8), mcode::Operand::from_register(addr.get_reg_offset(), 8)}
            ));

            lowerer.emit(mcode::Instruction(
                X8664Opcode::IMUL,
                {mcode::Operand::from_register(offset_reg, 8), mcode::Operand::from_int_immediate(addr.get_scale())}
            ));

            lowerer.emit(mcode::Instruction(
                X8664Opcode::ADD,
                {mcode::Operand::from_register(tmp_reg, 8), mcode::Operand::from_register(offset_reg, 8)}
            ));

            return mcode::IndirectAddress(tmp_reg);
        }
    }

    return addr;
}

mcode::IndirectAddress X8664AddrLowering::calc_memberptr_addr(ssa::Instruction &instr) {
    const ssa::Type &type = instr.get_operand(0).get_type();
    const ssa::Operand &base_operand = instr.get_operand(1);
    unsigned int_offset = instr.get_operand(2).get_int_immediate().to_u64();

    unsigned byte_offset = lowerer.get_member_offset(type.get_struct(), int_offset);
    mcode::Register base = mcode::Register::from_physical(0);

    // Try to merge the instruction with previous pointer operations.
    if (base_operand.is_register()) {
        ssa::InstrIter base_producer_iter = lowerer.get_producer_globally(base_operand.get_register());

        if (base_producer_iter != lowerer.get_block().end() &&
            base_producer_iter->get_opcode() == ssa::Opcode::MEMBERPTR) {
            if (lowerer.get_num_uses(*instr.get_dest()) == 0) {
                lowerer.discard_use(base_operand.get_register());
            }

            mcode::IndirectAddress addr = calc_memberptr_addr(*base_producer_iter);
            addr.set_int_offset(addr.get_int_offset() + byte_offset);
            return addr;
        }

        base = lowerer.lower_reg(base_operand.get_register());
    } else if (base_operand.is_global()) {
        base = lowerer.lower_reg(lowerer.get_func().next_virtual_reg());

        lowerer.emit(mcode::Instruction(
            X8664Opcode::LEA,
            {
                mcode::Operand::from_register(base, 8),
                mcode::Operand::from_symbol_deref(base_operand.get_global_name(), 8),
            }
        ));
    }

    return mcode::IndirectAddress(base, byte_offset, 1);
}

} // namespace target

} // namespace banjo
