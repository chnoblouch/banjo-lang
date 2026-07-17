#include "aarch64_ssa_lowerer.hpp"

#include "banjo/mcode/operand.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_address.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/mcode/stack_slot.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/aarch64/aapcs_calling_conv.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_immediate.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/bit_operations.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include <variant>

namespace banjo::target {

// FIXME: Relocations for loads and stores are not generated correctly!

AArch64SSALowerer::AArch64SSALowerer(Target *target) : SSALowerer(target) {}

mcode::Operand AArch64SSALowerer::lower_value(const ssa::Operand &operand) {
    unsigned size = get_size(operand.get_type());

    if (operand.is_immediate()) return move_const_into_register(operand, operand.get_type());
    else if (operand.is_register()) return lower_reg_val(operand.get_register(), size);
    else if (operand.is_symbol())
        return mcode::Operand::from_register(move_symbol_into_register(operand.get_symbol_name()), 8);
    else ASSERT_UNREACHABLE;
}

void AArch64SSALowerer::lower_fp_operation(mcode::Opcode opcode, ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(opcode, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_cond_branch(mcode::Opcode cmp_opcode, ssa::Instruction &instr) {
    ssa::Comparison comparison = instr.get_operand(1).get_comparison();
    ssa::BranchTarget &target_true = instr.get_operand(3).get_branch_target();
    ssa::BranchTarget &target_false = instr.get_operand(4).get_branch_target();

    mcode::Operand m_cmp_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_cmp_rhs = lower_value(instr.get_operand(2));

    mcode::Operand m_target_true = mcode::Operand::from_label(target_true.block->get_label());
    mcode::Operand m_target_false = mcode::Operand::from_label(target_false.block->get_label());

    if (target_true.block == get_basic_block_iter().get_next()) {
        AArch64Condition condition = lower_condition(ssa::invert_comparison(comparison));
        mcode::Opcode branch_opcode = AArch64Opcode::B_EQ + static_cast<unsigned>(condition);

        move_branch_args(target_false);
        emit({cmp_opcode, {m_cmp_lhs, m_cmp_rhs}});
        emit({branch_opcode, {m_target_false}});
        move_branch_args(target_true);
    } else {
        AArch64Condition condition = lower_condition(comparison);
        mcode::Opcode branch_opcode = AArch64Opcode::B_EQ + static_cast<unsigned>(condition);

        move_branch_args(target_true);
        emit({cmp_opcode, {m_cmp_lhs, m_cmp_rhs}});
        emit({branch_opcode, {m_target_true}});
        move_branch_args(target_false);

        if (target_false.block != get_basic_block_iter().get_next()) {
            emit({AArch64Opcode::B, {m_target_false}});
        }
    }
}

mcode::Operand AArch64SSALowerer::lower_reg_val(ssa::VirtualRegister virtual_reg, unsigned size) {
    std::variant<mcode::Register, mcode::StackSlotID> m_vreg = map_vreg(virtual_reg);

    if (std::holds_alternative<mcode::Register>(m_vreg)) {
        return mcode::Operand::from_register(std::get<mcode::Register>(m_vreg), size);
    } else if (std::holds_alternative<mcode::StackSlotID>(m_vreg)) {
        mcode::StackSlotID stack_slot = std::get<mcode::StackSlotID>(m_vreg);

        mcode::Operand m_tmp = mcode::Operand::from_register(create_tmp_reg(), size);
        mcode::Operand m_sp = mcode::Operand::from_register(mcode::Register::from_physical(AArch64Register::SP), 8);
        mcode::Operand m_offset = mcode::Operand::from_stack_offset({stack_slot, 0});
        emit(mcode::Instruction(AArch64Opcode::ADD, {m_tmp, m_sp, m_offset}));

        return m_tmp;
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Operand AArch64SSALowerer::lower_address(ssa::Operand &operand, AddrUsage usage) {
    AddrComponents addr = collect_addr(operand);
    return lower_address(addr, usage);
}

mcode::Operand AArch64SSALowerer::lower_address(AddrComponents addr, AddrUsage usage) {
    std::variant<mcode::Register, mcode::StackSlotID> base;

    if (addr.base.is_register()) {
        base = map_vreg(addr.base.get_register());
    } else if (addr.base.is_symbol()) {
        base = move_symbol_into_register(addr.base.get_symbol_name());
    } else {
        ASSERT_UNREACHABLE;
    }

    bool is_stack_slot = std::holds_alternative<mcode::StackSlotID>(base);

    if (is_stack_slot) {
        mcode::StackSlotID stack_slot = std::get<mcode::StackSlotID>(base);
        mcode::Register sp = mcode::Register::from_physical(AArch64Register::SP);
        mcode::StackAddress stack_addr{stack_slot, addr.const_offset.to_unsigned()};

        if (usage == AddrUsage::MEMORY_ACCESS) {
            if (addr.reg_offset) {
                // TODO: Support shifted register offsets (e.g. ldr x0, [x0, x1, lsl #2])
                
                mcode::Operand m_dst = create_temp_value(8);
                mcode::Operand m_sp = mcode::Operand::from_register(sp, 8);

                emit({AArch64Opcode::ADD, {m_dst, m_sp, mcode::Operand::from_stack_offset(stack_addr)}});
                m_dst = emit_add_scaled(m_dst, addr.reg_offset->reg, addr.reg_offset->scale);

                AArch64Address m_addr = AArch64Address::new_base(m_dst.get_register());
                return mcode::Operand::from_aarch64_addr(m_addr, 8);
            } else {
                AArch64Address m_addr = AArch64Address::new_base_offset(sp, stack_addr);
                return mcode::Operand::from_aarch64_addr(m_addr);
            }
        } else if (usage == AddrUsage::ADDR_VALUE) {
            mcode::Operand m_dst = create_temp_value(8);
            mcode::Operand m_sp = mcode::Operand::from_register(sp, 8);
            emit({AArch64Opcode::ADD, {m_dst, m_sp, mcode::Operand::from_stack_offset(stack_addr)}});

            if (addr.reg_offset) {
                m_dst = emit_add_scaled(m_dst, addr.reg_offset->reg, addr.reg_offset->scale);
            }

            return m_dst;
        } else {
            ASSERT_UNREACHABLE;
        }
    } else {
        mcode::Register base_reg = std::get<mcode::Register>(base);

        if (usage == AddrUsage::MEMORY_ACCESS) {
            if (addr.reg_offset) {
                // TODO: Support shifted register offsets (e.g. ldr x0, [x0, x1, lsl #2])

                mcode::Operand m_dst = mcode::Operand::from_register(base_reg, 8);

                if (addr.const_offset != 0) {
                    m_dst = emit_add_imm(m_dst, addr.const_offset);
                }

                m_dst = emit_add_scaled(m_dst, addr.reg_offset->reg, addr.reg_offset->scale);

                AArch64Address m_addr = AArch64Address::new_base(m_dst.get_register());
                return mcode::Operand::from_aarch64_addr(m_addr, 8);
            } else {
                if (addr.const_offset == 0) {
                    AArch64Address m_addr = AArch64Address::new_base(base_reg);
                    return mcode::Operand::from_aarch64_addr(m_addr, 8);
                } else {
                    unsigned offset = addr.const_offset.to_unsigned();
                    AArch64Address m_addr = AArch64Address::new_base_offset(base_reg, offset);
                    return mcode::Operand::from_aarch64_addr(m_addr, 8);
                }
            }
        } else if (usage == AddrUsage::ADDR_VALUE) {
            mcode::Operand m_dst = mcode::Operand::from_register(base_reg, 8);

            if (addr.const_offset != 0) {
                m_dst = emit_add_imm(m_dst, addr.const_offset);
            }

            if (addr.reg_offset) {
                m_dst = emit_add_scaled(m_dst, addr.reg_offset->reg, addr.reg_offset->scale);
            }

            return m_dst;
        } else {
            ASSERT_UNREACHABLE;
        }
    }
}

mcode::CallingConvention *AArch64SSALowerer::get_calling_convention(ssa::CallingConv calling_conv) {
    ASSERT(calling_conv == ssa::CallingConv::AARCH64_AAPCS);

    if (target->get_descr().is_darwin()) {
        return &AAPCSCallingConv::INSTANCE_APPLE;
    } else {
        return &AAPCSCallingConv::INSTANCE_STANDARD;
    }
}

void AArch64SSALowerer::init_func(ssa::Function &func) {
    block_arg_tmps.clear();

    for (ssa::BasicBlock &block : func) {
        for (ssa::VirtualRegister arg_reg : block.get_param_regs()) {
            block_arg_tmps.emplace(arg_reg, func.next_virtual_reg());
        }
    }
}

void AArch64SSALowerer::emit_block_prologue(ssa::BasicBlock &block) {
    for (unsigned i = 0; i < block.get_param_regs().size(); i++) {
        ssa::VirtualRegister arg_reg = block.get_param_regs()[i];
        ssa::Type type = block.get_param_types()[i];

        ssa::VirtualRegister tmp_reg = block_arg_tmps.at(arg_reg);
        mcode::Opcode opcode = type.is_floating_point() ? AArch64Opcode::FMOV : AArch64Opcode::MOV;
        unsigned reg_size = get_size(type) == 8 ? 8 : 4;

        mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_virtual(arg_reg), reg_size);
        mcode::Operand src = mcode::Operand::from_register(mcode::Register::from_virtual(tmp_reg), reg_size);
        emit({opcode, {dst, src}});
    }
}

void AArch64SSALowerer::lower_load(ssa::Instruction &instr) {
    const ssa::Type &type = instr.get_operand(0).get_type();
    unsigned size = get_size(type);
    unsigned flag = type.is_floating_point() ? mcode::Instruction::FLAG_FLOAT : 0;

    if (size == 0) {
        // HACK: When the size is zero, we zero the register to make sure the register allocator has
        // a def for that register so it can assign a register class. Maybe we should instead just
        // remove all uses of that register?

        mcode::Opcode opcode = type.is_floating_point() ? AArch64Opcode::FMOV : AArch64Opcode::MOV;
        mcode::Operand m_dst = map_vreg_dst(instr, size);
        mcode::Operand m_zero = mcode::Operand::from_int_immediate(0, 4);
        emit(mcode::Instruction(opcode, {m_dst, m_zero}));
        return;
    }

    mcode::Operand m_dst = map_vreg_dst(instr, size).with_size(size > 4 ? 8 : 4);
    AddrComponents addr = collect_addr(instr.get_operand(1));

    if (size == 1) {
        emit({AArch64Opcode::LDRB, {m_dst, lower_address(addr, AddrUsage::MEMORY_ACCESS)}, flag});
    } else if (size == 2) {
        emit({AArch64Opcode::LDRH, {m_dst, lower_address(addr, AddrUsage::MEMORY_ACCESS)}, flag});
    } else if (size == 3) {
        mcode::Operand m_reg0 = create_temp_value(4);
        mcode::Operand m_reg1 = create_temp_value(4);

        emit({AArch64Opcode::LDRH, {m_reg0, lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::LDRB, {m_reg1, lower_address(addr.offset(2), AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::ORR, {m_dst, m_reg0, m_reg1, mcode::Operand::from_aarch64_left_shift(16)}});
    } else if (size == 4) {
        emit({AArch64Opcode::LDR, {m_dst, lower_address(addr, AddrUsage::MEMORY_ACCESS)}, flag});
    } else if (size == 5) {
        mcode::Operand m_reg0 = create_temp_value(8);
        mcode::Operand m_reg1 = create_temp_value(8);

        emit({AArch64Opcode::LDR, {m_reg0.with_size(4), lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::LDRB, {m_reg1.with_size(4), lower_address(addr.offset(4), AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::ORR, {m_dst, m_reg0, m_reg1, mcode::Operand::from_aarch64_left_shift(32)}});
    } else if (size == 6) {
        mcode::Operand m_reg0 = create_temp_value(8);
        mcode::Operand m_reg1 = create_temp_value(8);

        emit({AArch64Opcode::LDR, {m_reg0.with_size(4), lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::LDRH, {m_reg1.with_size(4), lower_address(addr.offset(4), AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::ORR, {m_dst, m_reg0, m_reg1, mcode::Operand::from_aarch64_left_shift(32)}});
    } else if (size == 7) {
        mcode::Operand m_reg0 = create_temp_value(8);
        mcode::Operand m_reg1 = create_temp_value(8);
        mcode::Operand m_reg2 = create_temp_value(8);
        mcode::Operand m_reg3 = create_temp_value(8);

        emit({AArch64Opcode::LDR, {m_reg0.with_size(4), lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::LDRH, {m_reg1.with_size(4), lower_address(addr.offset(4), AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::LDRB, {m_reg2.with_size(4), lower_address(addr.offset(6), AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::ORR, {m_reg3, m_reg0, m_reg1, mcode::Operand::from_aarch64_left_shift(32)}});
        emit({AArch64Opcode::ORR, {m_dst, m_reg3, m_reg2, mcode::Operand::from_aarch64_left_shift(48)}});
    } else if (size == 8) {
        emit({AArch64Opcode::LDR, {m_dst, lower_address(addr, AddrUsage::MEMORY_ACCESS)}, flag});
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64SSALowerer::lower_store(ssa::Instruction &instr) {
    const ssa::Type &type = instr.get_operand(0).get_type();
    unsigned size = get_size(type);

    if (size == 0) {
        return;
    }

    mcode::Operand m_src = lower_value(instr.get_operand(0));
    AddrComponents addr = collect_addr(instr.get_operand(1));

    if (size == 1) {
        emit({AArch64Opcode::STRB, {m_src, lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
    } else if (size == 2) {
        emit({AArch64Opcode::STRH, {m_src, lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
    } else if (size == 3) {
        mcode::Operand m_reg = create_temp_value(4);
        mcode::Operand m_shift_reg = create_temp_value(4); // TODO: Support immediates in the encoder.

        emit({AArch64Opcode::MOV, {m_shift_reg, mcode::Operand::from_int_immediate(16)}});
        emit({AArch64Opcode::LSR, {m_reg, m_src.with_size(4), m_shift_reg}});
        emit({AArch64Opcode::STRH, {m_src, lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::STRB, {m_reg.with_size(4), lower_address(addr.offset(2), AddrUsage::MEMORY_ACCESS)}});
    } else if (size == 4) {
        emit({AArch64Opcode::STR, {m_src, lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
    } else if (size == 5) {
        mcode::Operand m_reg = create_temp_value(8);
        mcode::Operand m_shift_reg = create_temp_value(8); // TODO: Support immediates in the encoder.

        emit({AArch64Opcode::MOV, {m_shift_reg, mcode::Operand::from_int_immediate(32)}});
        emit({AArch64Opcode::LSR, {m_reg, m_src.with_size(8), m_shift_reg}});
        emit({AArch64Opcode::STR, {m_src.with_size(4), lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::STRB, {m_reg.with_size(4), lower_address(addr.offset(4), AddrUsage::MEMORY_ACCESS)}});
    } else if (size == 6) {
        mcode::Operand m_reg = create_temp_value(8);
        mcode::Operand m_shift_reg = create_temp_value(8); // TODO: Support immediates in the encoder.

        emit({AArch64Opcode::MOV, {m_shift_reg, mcode::Operand::from_int_immediate(32)}});
        emit({AArch64Opcode::LSR, {m_reg, m_src.with_size(8), m_shift_reg}});
        emit({AArch64Opcode::STR, {m_src.with_size(4), lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::STRH, {m_reg.with_size(4), lower_address(addr.offset(4), AddrUsage::MEMORY_ACCESS)}});
    } else if (size == 7) {
        mcode::Operand m_reg0 = create_temp_value(8);
        mcode::Operand m_reg1 = create_temp_value(8);
        mcode::Operand m_shift_reg = create_temp_value(8); // TODO: Support immediates in the encoder.

        emit({AArch64Opcode::MOV, {m_shift_reg, mcode::Operand::from_int_immediate(32)}});
        emit({AArch64Opcode::LSR, {m_reg0, m_src.with_size(8), m_shift_reg}});
        emit({AArch64Opcode::MOV, {m_shift_reg, mcode::Operand::from_int_immediate(48)}});
        emit({AArch64Opcode::LSR, {m_reg1, m_src.with_size(8), m_shift_reg}});
        emit({AArch64Opcode::STR, {m_src.with_size(4), lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::STRH, {m_reg0.with_size(4), lower_address(addr.offset(4), AddrUsage::MEMORY_ACCESS)}});
        emit({AArch64Opcode::STRB, {m_reg1.with_size(4), lower_address(addr.offset(6), AddrUsage::MEMORY_ACCESS)}});
    } else if (size == 8) {
        emit({AArch64Opcode::STR, {m_src, lower_address(addr, AddrUsage::MEMORY_ACCESS)}});
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64SSALowerer::lower_loadarg(ssa::Instruction &instr) {
    ssa::Type type = instr.get_operand(0).get_type();
    unsigned param_index = instr.get_operand(1).get_int_immediate().to_u64();
    unsigned size = get_size(type);

    mcode::CallingConvention *calling_conv = get_machine_func()->get_calling_conv();
    std::vector<mcode::ArgStorage> arg_storage = calling_conv->get_arg_storage(get_func().type);
    mcode::ArgStorage cur_arg_storage = arg_storage[param_index];

    mcode::Operand m_dst = map_vreg_dst(instr, size);

    if (cur_arg_storage.in_reg) {
        mcode::Opcode opcode = type.is_floating_point() ? AArch64Opcode::FMOV : AArch64Opcode::MOV;
        mcode::Operand m_src = mcode::Operand::from_register(mcode::Register::from_physical(cur_arg_storage.reg), size);
        emit({opcode, {m_dst, m_src}, mcode::Instruction::FLAG_ARG_STORE});
    } else {
        mcode::Opcode opcode;

        switch (size) {
            case 1: opcode = AArch64Opcode::LDRB; break;
            case 2: opcode = AArch64Opcode::LDRH; break;
            case 4:
            case 8: opcode = AArch64Opcode::LDR; break;
            default: ASSERT_UNREACHABLE;
        }

        mcode::Parameter &param = get_machine_func()->get_parameters()[param_index];
        mcode::StackSlotID slot_index = std::get<mcode::StackSlotID>(param.storage);
        mcode::Operand m_src = mcode::Operand::from_stack_slot(slot_index, size);
        emit({opcode, {m_dst, m_src}});
    }
}

void AArch64SSALowerer::lower_add(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::ADD, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_sub(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::SUB, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_mul(ssa::Instruction &instr) {
    const ssa::Value &lhs = instr.get_operand(0);
    const ssa::Value &rhs = instr.get_operand(1);

    mcode::Operand m_lhs = lower_value(lhs);
    mcode::Operand m_rhs;

    if (rhs.is_immediate()) {
        m_rhs = move_const_into_register(rhs, rhs.get_type());
    } else {
        m_rhs = lower_value(rhs);
    }

    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::MUL, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_sdiv(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::SDIV, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_srem(ssa::Instruction &instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());
    mcode::Operand m_dividend = lower_value(instr.get_operand(0));
    mcode::Operand m_divisor = lower_value(instr.get_operand(1));
    mcode::Operand m_tmp = mcode::Operand::from_register(create_tmp_reg(), size);
    mcode::Operand m_dst = map_vreg_dst(instr, size);

    emit(mcode::Instruction(AArch64Opcode::SDIV, {m_tmp, m_dividend, m_divisor}));
    emit(mcode::Instruction(AArch64Opcode::MUL, {m_tmp, m_tmp, m_divisor}));
    emit(mcode::Instruction(AArch64Opcode::SUB, {m_dst, m_dividend, m_tmp}));
}

void AArch64SSALowerer::lower_udiv(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::UDIV, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_urem(ssa::Instruction &instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());
    mcode::Operand m_dividend = lower_value(instr.get_operand(0));
    mcode::Operand m_divisor = lower_value(instr.get_operand(1));
    mcode::Operand m_tmp = mcode::Operand::from_register(create_tmp_reg(), size);
    mcode::Operand m_dst = map_vreg_dst(instr, size);

    emit(mcode::Instruction(AArch64Opcode::UDIV, {m_tmp, m_dividend, m_divisor}));
    emit(mcode::Instruction(AArch64Opcode::MUL, {m_tmp, m_tmp, m_divisor}));
    emit(mcode::Instruction(AArch64Opcode::SUB, {m_dst, m_dividend, m_tmp}));
}

void AArch64SSALowerer::lower_fadd(ssa::Instruction &instr) {
    lower_fp_operation(AArch64Opcode::FADD, instr);
}

void AArch64SSALowerer::lower_fsub(ssa::Instruction &instr) {
    lower_fp_operation(AArch64Opcode::FSUB, instr);
}

void AArch64SSALowerer::lower_fmul(ssa::Instruction &instr) {
    lower_fp_operation(AArch64Opcode::FMUL, instr);
}

void AArch64SSALowerer::lower_fdiv(ssa::Instruction &instr) {
    lower_fp_operation(AArch64Opcode::FDIV, instr);
}

void AArch64SSALowerer::lower_and(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::AND, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_or(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::ORR, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_xor(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::EOR, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_shl(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::LSL, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_shr(ssa::Instruction &instr) {
    mcode::Operand m_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_rhs = lower_value(instr.get_operand(1));
    mcode::Operand m_dst = map_vreg_dst(instr, m_lhs.get_size());
    emit(mcode::Instruction(AArch64Opcode::ASR, {m_dst, m_lhs, m_rhs}));
}

void AArch64SSALowerer::lower_jmp(ssa::Instruction &instr) {
    move_branch_args(instr.get_operand(0).get_branch_target());

    mcode::Operand m_target = mcode::Operand::from_label(instr.get_operand(0).get_branch_target().block->get_label());
    emit(mcode::Instruction(AArch64Opcode::B, {m_target}));
}

void AArch64SSALowerer::lower_cjmp(ssa::Instruction &instr) {
    lower_cond_branch(AArch64Opcode::CMP, instr);
}

void AArch64SSALowerer::lower_fcjmp(ssa::Instruction &instr) {
    lower_cond_branch(AArch64Opcode::FCMP, instr);
}

void AArch64SSALowerer::lower_select(ssa::Instruction &instr) {
    ssa::Type type_in = instr.get_operand(0).get_type();
    ssa::Type type_out = instr.get_operand(3).get_type();

    mcode::Opcode cmp_opcode = type_in.is_floating_point() ? AArch64Opcode::FCMP : AArch64Opcode::CMP;
    mcode::Operand m_cmp_lhs = lower_value(instr.get_operand(0));
    mcode::Operand m_cmp_rhs = lower_value(instr.get_operand(2));
    AArch64Condition condition = lower_condition(instr.get_operand(1).get_comparison());
    mcode::Operand m_cmp = mcode::Operand::from_aarch64_condition(condition);

    mcode::Opcode sel_opcode = type_out.is_floating_point() ? AArch64Opcode::FCSEL : AArch64Opcode::CSEL;
    mcode::Operand m_val_true = lower_value(instr.get_operand(3));
    mcode::Operand m_val_false = lower_value(instr.get_operand(4));
    mcode::Operand m_dst = map_vreg_dst(instr, m_val_true.get_size());

    emit(mcode::Instruction(cmp_opcode, {m_cmp_lhs, m_cmp_rhs}));
    emit(mcode::Instruction(sel_opcode, {m_dst, m_val_true, m_val_false, m_cmp}));
}

void AArch64SSALowerer::lower_call(ssa::Instruction &instr) {
    get_machine_func()->get_calling_conv()->lower_call(*this, instr);
}

void AArch64SSALowerer::lower_ret(ssa::Instruction &instr) {
    if (!instr.get_operands().empty()) {
        bool is_fp = instr.get_operand(0).get_type().is_floating_point();
        mcode::Opcode opcode = is_fp ? AArch64Opcode::FMOV : AArch64Opcode::MOV;

        mcode::PhysicalReg reg = is_fp ? AArch64Register::V0 : AArch64Register::R0;
        mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_physical(reg));
        mcode::Operand src = lower_value(instr.get_operand(0));
        dst.set_size(src.get_size());

        emit(mcode::Instruction(opcode, {dst, src}));
    }

    emit(mcode::Instruction(AArch64Opcode::RET));
}

void AArch64SSALowerer::lower_uextend(ssa::Instruction &instr) {
    mcode::Opcode opcode;
    unsigned src_size = get_size(instr.get_operand(0).get_type());

    switch (src_size) {
        case 1: opcode = AArch64Opcode::UXTB; break;
        case 2: opcode = AArch64Opcode::UXTH; break;
        case 4: opcode = AArch64Opcode::MOV; break;
        default: ASSERT_UNREACHABLE;
    }

    mcode::Operand m_src = lower_value(instr.get_operand(0));
    mcode::Operand m_dst = map_vreg_dst(instr, 4);
    emit(mcode::Instruction(opcode, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_sextend(ssa::Instruction &instr) {
    mcode::Opcode opcode;
    unsigned src_size = get_size(instr.get_operand(0).get_type());

    switch (src_size) {
        case 1: opcode = AArch64Opcode::SXTB; break;
        case 2: opcode = AArch64Opcode::SXTH; break;
        case 4: opcode = AArch64Opcode::SXTW; break;
        default: ASSERT_UNREACHABLE;
    }

    mcode::Operand m_src = lower_value(instr.get_operand(0));
    mcode::Operand m_dst = map_vreg_dst(instr, 8);
    emit(mcode::Instruction(opcode, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_truncate(ssa::Instruction &instr) {
    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    mcode::Operand m_src = lower_value(instr.get_operand(0)).with_size(dst_size);
    mcode::Operand m_dst = map_vreg_dst(instr, dst_size);
    emit(mcode::Instruction(AArch64Opcode::MOV, {m_dst, m_src}));

    // TODO: Use the immediate directly in the `AND` instruction.
    // This requires support for bitmask immediates in the encoder.

    if (dst_size == 1) {
        mcode::Operand m_mask_reg = mcode::Operand::from_register(create_tmp_reg(), 4);
        emit({AArch64Opcode::MOV, {m_mask_reg, mcode::Operand::from_int_immediate(0xFF)}});
        emit({AArch64Opcode::AND, {m_dst, m_dst, m_mask_reg}});
    } else if (dst_size == 2) {
        mcode::Operand m_mask_reg = mcode::Operand::from_register(create_tmp_reg(), 4);
        emit({AArch64Opcode::MOV, {m_mask_reg, mcode::Operand::from_int_immediate(0xFFFF)}});
        emit({AArch64Opcode::AND, {m_dst, m_dst, m_mask_reg}});
    }
}

void AArch64SSALowerer::lower_fpromote(ssa::Instruction &instr) {
    mcode::Operand m_dst = map_vreg_dst(instr, 8);
    mcode::Operand m_src = lower_value(instr.get_operand(0));
    emit(mcode::Instruction(AArch64Opcode::FCVT, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_fdemote(ssa::Instruction &instr) {
    mcode::Operand m_dst = map_vreg_dst(instr, 4);
    mcode::Operand m_src = lower_value(instr.get_operand(0));
    emit(mcode::Instruction(AArch64Opcode::FCVT, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_utof(ssa::Instruction &instr) {
    // FIXME: Conversion from 8-bit and 16-bit values.

    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    mcode::Operand m_src = lower_value(instr.get_operand(0));
    mcode::Operand m_dst = map_vreg_dst(instr, dst_size);
    emit(mcode::Instruction(AArch64Opcode::UCVTF, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_stof(ssa::Instruction &instr) {
    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    mcode::Operand m_src = lower_value(instr.get_operand(0));
    mcode::Operand m_dst = map_vreg_dst(instr, dst_size);
    emit(mcode::Instruction(AArch64Opcode::SCVTF, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_ftou(ssa::Instruction &instr) {
    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    mcode::Operand m_src = lower_value(instr.get_operand(0));
    mcode::Operand m_dst = map_vreg_dst(instr, dst_size);
    emit(mcode::Instruction(AArch64Opcode::FCVTZU, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_ftos(ssa::Instruction &instr) {
    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    mcode::Operand m_src = lower_value(instr.get_operand(0));
    mcode::Operand m_dst = map_vreg_dst(instr, dst_size);
    emit(mcode::Instruction(AArch64Opcode::FCVTZS, {m_dst, m_src}));
}

void AArch64SSALowerer::lower_offsetptr(ssa::Instruction &instr) {
    AddrComponents addr = collect_addr(instr.get_operand(0));

    ssa::Operand &offset = instr.get_operand(1);
    ssa::Type base_type = instr.get_operand(2).get_type();

    if (offset.is_immediate()) {
        addr.const_offset += offset.get_int_immediate() * get_size(base_type);
    } else if (offset.is_register()) {
        addr.reg_offset = RegOffset{
            .reg = map_vreg_as_reg(offset.get_register()),
            .scale = get_size(base_type),
        };
    } else {
        ASSERT_UNREACHABLE;
    }

    mcode::Operand m_dst = map_vreg_as_operand(*instr.get_dest(), 8);
    emit({AArch64Opcode::MOV, {m_dst, lower_address(addr, AddrUsage::ADDR_VALUE)}});
}

void AArch64SSALowerer::lower_memberptr(ssa::Instruction &instr) {
    AddrComponents addr = collect_addr(instr.get_operand(1));

    ssa::Structure &struct_ = *instr.get_operand(0).get_type().get_struct();
    unsigned member_index = instr.get_operand(2).get_int_immediate().to_u64();
    addr.const_offset += get_member_offset(&struct_, member_index);

    mcode::Operand m_dst = map_vreg_as_operand(*instr.get_dest(), 8);
    emit({AArch64Opcode::MOV, {m_dst, lower_address(addr, AddrUsage::ADDR_VALUE)}});
}

mcode::Value AArch64SSALowerer::move_const_into_register(const ssa::Value &value, ssa::Type type) {
    unsigned size = get_size(type);

    if (value.is_int_immediate()) return move_int_into_register(value.get_int_immediate(), size);
    else if (value.is_fp_immediate()) return move_float_into_register(value.get_fp_immediate(), size);
    else ASSERT_UNREACHABLE;
}

mcode::Value AArch64SSALowerer::move_int_into_register(LargeInt value, unsigned size) {
    std::uint64_t bits = value.to_bits();
    mcode::Value result = create_temp_value(size);

    // Zero can be moved directly without decomposing because MOVN sets the upper bits to zero.
    if (bits == 0) {
        emit(mcode::Instruction(AArch64Opcode::MOV, {result, mcode::Operand::from_int_immediate(0)}));
        return result;
    }

    if (size == 1 || size == 2) {
        emit(mcode::Instruction(AArch64Opcode::MOV, {result, mcode::Operand::from_int_immediate(bits)}));
    } else if (size == 4) {
        std::uint16_t elements[2];
        AArch64Immediate::decompose_u32_u16(bits, elements);
        move_elements_into_register(result, elements, 2);
    } else if (size == 8) {
        std::uint16_t elements[4];
        AArch64Immediate::decompose_u64_u16(bits, elements);
        move_elements_into_register(result, elements, 4);
    } else {
        ASSERT_UNREACHABLE;
    }

    return result;
}

mcode::Value AArch64SSALowerer::move_float_into_register(double value, unsigned size) {
    mcode::Value result = create_temp_value(size);

    // Emit FMOV if the value can be represented using 8 bits.
    if (AArch64Immediate::is_float_encodable(value)) {
        emit(mcode::Instruction(AArch64Opcode::FMOV, {result, mcode::Operand::from_fp_immediate(value)}));
        return result;
    }

    if (size == 4) {
        // 32-bit floats can be stored by moving the bits into a Wn register using MOV and MOVK and
        // moving from Wn to Sn afterwards.

        // TODO: FMOV is not required when this instruction is used for storing floats in memory. We
        // could just return the general-purpose register with the bits directly.

        float value_f32 = (float)value;
        std::uint32_t bits = BitOperations::get_bits_32(value_f32);
        std::uint16_t elements[2];
        AArch64Immediate::decompose_u32_u16(bits, elements);

        mcode::Value bits_value = create_temp_value(size);
        move_elements_into_register(bits_value, elements, 2);
        emit(mcode::Instruction(AArch64Opcode::FMOV, {result, bits_value}));
    } else {
        mcode::Global global{
            .name = "double." + std::to_string(next_const_index++),
            .size = 8,
            .alignment = 8,
            .value = value,
        };

        get_machine_module().add(global);

        mcode::Operand symbol_addr = mcode::Operand::from_register(move_symbol_into_register(global.name), 8);
        AArch64Address addr = AArch64Address::new_base(symbol_addr.get_register());
        mcode::Value m_addr = mcode::Value::from_aarch64_addr(addr);
        emit(mcode::Instruction(AArch64Opcode::LDR, {result, m_addr}, mcode::Instruction::FLAG_FLOAT));
    }

    return result;
}

// clang-format off

void AArch64SSALowerer::move_elements_into_register(mcode::Value value, std::uint16_t* elements, unsigned count) {
    // Count the non-zero elements.
    unsigned num_non_zero_elements = 0;
    unsigned non_zero_element_index = 0;
    for(int i = 0; i < count; i++) {
        if(elements[i] != 0) {
            num_non_zero_elements++;
            non_zero_element_index = i;
        }
    }

    // Emit a single MOVZ if all but one element are zero.
    if(num_non_zero_elements == 1) {
        unsigned element = elements[non_zero_element_index];
        emit(mcode::Instruction(AArch64Opcode::MOVZ, {
            value,
            mcode::Operand::from_int_immediate(element),
            mcode::Operand::from_aarch64_left_shift(16 * non_zero_element_index)
        }));
        return;
    }
    
    // Move the lower 16 bits and set all other bits to zero.
    emit(mcode::Instruction(AArch64Opcode::MOVZ, {
        value,
        mcode::Operand::from_int_immediate(elements[0])
    }));

    for(unsigned i = 0; i < count; i++) {
        unsigned element = elements[i];
        
        // Skip zero elements because the bits were already set to zero by the first MOVZ.
        if(element == 0) {
            continue;
        }

        // Set the bits by shifting the value by 16 * index to the left and moving them.
        emit(mcode::Instruction(AArch64Opcode::MOVK, {
            value,
            mcode::Operand::from_int_immediate(element),
            mcode::Operand::from_aarch64_left_shift(16 * i)
        }));
    }
}

// clang-format on

mcode::Register AArch64SSALowerer::move_symbol_into_register(const std::string &symbol) {
    mcode::Register dst_reg = create_tmp_reg();
    mcode::Operand m_dst = mcode::Operand::from_register(dst_reg, 8);

    mcode::Relocation adrp_reloc;
    mcode::Relocation add_reloc;
    mcode::Relocation adrp_got_reloc;
    mcode::Relocation ldr_got_reloc;

    if (target->get_descr().is_darwin()) {
        adrp_reloc = mcode::Relocation::PAGE21;
        add_reloc = mcode::Relocation::PAGEOFF12;
        adrp_got_reloc = mcode::Relocation::GOT_LOAD_PAGE21;
        ldr_got_reloc = mcode::Relocation::GOT_LOAD_PAGEOFF12;
    } else {
        adrp_reloc = mcode::Relocation::ADR_PREL_PG_HI21;
        add_reloc = mcode::Relocation::ADD_ABS_LO12_NC;
        adrp_got_reloc = mcode::Relocation::ADR_GOT_PAGE;
        ldr_got_reloc = mcode::Relocation::LD64_GOT_LO12_NC;
    }

    if (get_machine_module().get_external_symbols().contains(symbol)) {
        mcode::Operand m_adrp_addr = mcode::Operand::from_symbol({symbol, adrp_got_reloc});

        mcode::Symbol offset_symbol = mcode::Symbol(symbol, ldr_got_reloc);
        AArch64Address ldr_addr = AArch64Address::new_base_offset(m_dst.get_register(), offset_symbol);
        mcode::Operand m_ldr_addr = mcode::Operand::from_aarch64_addr(ldr_addr, 8);

        emit({AArch64Opcode::ADRP, {m_dst, m_adrp_addr}});
        emit({AArch64Opcode::LDR, {m_dst, m_ldr_addr}});
    } else {
        mcode::Operand m_adrp_addr = mcode::Operand::from_symbol({symbol, adrp_reloc});
        mcode::Operand m_add_addr = mcode::Operand::from_symbol({symbol, add_reloc});

        emit({AArch64Opcode::ADRP, {m_dst, m_adrp_addr}});
        emit({AArch64Opcode::ADD, {m_dst, m_dst, m_add_addr}});
    }

    return dst_reg;
}

void AArch64SSALowerer::build_address(const mcode::Operand &m_dst, AddrComponents addr) {
    LargeInt offset = addr.const_offset;

    if (addr.const_offset == 0 && !addr.reg_offset) {
        emit({AArch64Opcode::MOV, {m_dst, lower_value(addr.base)}});
        return;
    }

    mcode::Operand m_tmp_dst;

    if (addr.const_offset != 0) {
        m_tmp_dst = addr.reg_offset ? create_temp_value(8) : m_dst;

        std::optional<mcode::StackSlotID> stack_slot;

        if (addr.base.is_register()) {
            stack_slot = find_stack_slot(addr.base.get_register());
        }

        if (stack_slot) {
            mcode::StackAddress stack_addr{*stack_slot, offset.to_unsigned()};

            mcode::Register m_sp_reg = mcode::Register::from_physical(AArch64Register::SP);
            mcode::Operand m_sp = mcode::Operand::from_register(m_sp_reg, 8);
            mcode::Operand m_offset = mcode::Operand::from_stack_offset(stack_addr, 0);

            // Note: Offsets that end up outside the range [0; 4096) during stack frame building will get fixed in the
            // stack offset fixup pass.
            emit({AArch64Opcode::ADD, {m_tmp_dst, m_sp, m_offset}});
        } else {
            mcode::Operand m_base = lower_value(addr.base);

            if (offset >= 0 && offset < 4096) {
                mcode::Operand m_offset = mcode::Operand::from_int_immediate(offset, 8);
                emit({AArch64Opcode::ADD, {m_tmp_dst, m_base, m_offset}});
            } else {
                mcode::Value m_offset = move_int_into_register(offset, 8);
                emit({AArch64Opcode::ADD, {m_tmp_dst, m_base, m_offset}});
            }
        }
    } else {
        m_tmp_dst = lower_value(addr.base);
    }

    if (!addr.reg_offset) {
        return;
    }

    mcode::Operand m_offset = mcode::Operand::from_register(addr.reg_offset->reg, 8);
    unsigned shift = 0;

    switch (addr.reg_offset->scale) {
        case 1: shift = 0; break;
        case 2: shift = 1; break;
        case 4: shift = 2; break;
        case 8: shift = 3; break;
        default:
            mcode::Operand scale_val = create_temp_value(8);
            mcode::Operand imm_val = mcode::Operand::from_int_immediate(addr.reg_offset->scale, 8);
            mcode::Operand m_tmp = create_temp_value(8);

            emit(mcode::Instruction(AArch64Opcode::MOV, {scale_val, imm_val}));
            emit(mcode::Instruction(AArch64Opcode::MUL, {m_tmp, m_offset, scale_val}));
            m_offset = m_tmp;

            break;
    }

    mcode::Operand m_shift = mcode::Operand::from_aarch64_left_shift(shift);
    emit({AArch64Opcode::ADD, {m_dst, m_tmp_dst, m_offset, m_shift}});
}

mcode::Value AArch64SSALowerer::create_temp_value(int size) {
    ssa::VirtualRegister reg = get_func().next_virtual_reg();
    return mcode::Operand::from_register(mcode::Register::from_virtual(reg), size);
}

AArch64Condition AArch64SSALowerer::lower_condition(ssa::Comparison comparison) {
    switch (comparison) {
        case ssa::Comparison::EQ: return AArch64Condition::EQ;
        case ssa::Comparison::NE: return AArch64Condition::NE;
        case ssa::Comparison::UGT: return AArch64Condition::HI;
        case ssa::Comparison::UGE: return AArch64Condition::HS;
        case ssa::Comparison::ULT: return AArch64Condition::LO;
        case ssa::Comparison::ULE: return AArch64Condition::LS;
        case ssa::Comparison::SGT: return AArch64Condition::GT;
        case ssa::Comparison::SGE: return AArch64Condition::GE;
        case ssa::Comparison::SLT: return AArch64Condition::LT;
        case ssa::Comparison::SLE: return AArch64Condition::LE;
        case ssa::Comparison::FEQ: return AArch64Condition::EQ;
        case ssa::Comparison::FNE: return AArch64Condition::NE;
        case ssa::Comparison::FGT: return AArch64Condition::GT;
        case ssa::Comparison::FGE: return AArch64Condition::GE;
        case ssa::Comparison::FLT: return AArch64Condition::LT;
        case ssa::Comparison::FLE: return AArch64Condition::LE;
    }
}

void AArch64SSALowerer::move_branch_args(ssa::BranchTarget &target) {
    for (unsigned i = 0; i < target.args.size(); i++) {
        ssa::Value &arg = target.args[i];
        ssa::VirtualRegister arg_reg = target.block->get_param_regs()[i];
        ssa::VirtualRegister tmp_reg = block_arg_tmps.at(arg_reg);

        mcode::Opcode opcode = arg.get_type().is_floating_point() ? AArch64Opcode::FMOV : AArch64Opcode::MOV;
        unsigned reg_size = get_size(arg.get_type()) == 8 ? 8 : 4;

        mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_virtual(tmp_reg), reg_size);
        mcode::Operand src = lower_value(arg);
        emit({opcode, {dst, src}, mcode::Instruction::FLAG_CALL_ARG});
    }
}

mcode::Operand AArch64SSALowerer::lower_as_move_into_reg(mcode::Register reg, const ssa::Value &value) {
    unsigned size = get_size(value.get_type());
    bool is_fp = value.get_type().is_floating_point();

    mcode::Opcode opcode = is_fp ? AArch64Opcode::FMOV : AArch64Opcode::MOV;
    mcode::Operand m_dst = mcode::Operand::from_register(reg, size);
    mcode::Operand m_src = lower_value(value);

    emit({opcode, {m_dst, m_src}});
    return m_dst;
}

mcode::Operand AArch64SSALowerer::emit_add_scaled(mcode::Operand m_base, mcode::Register reg, unsigned scale) {
    mcode::Operand m_dst = create_temp_value(8);
    mcode::Operand m_offset = mcode::Operand::from_register(reg, 8);
    unsigned shift = 0;

    if (scale == 1) {
        shift = 0;
    } else if (scale == 2) {
        shift = 1;
    } else if (scale == 4) {
        shift = 2;
    } else if (scale == 8) {
        shift = 3;
    } else {
        mcode::Operand m_scale = create_temp_value(8);
        mcode::Operand m_offset_scaled = create_temp_value(8);

        emit({AArch64Opcode::MOV, {m_scale, mcode::Operand::from_int_immediate(scale, 8)}});
        emit({AArch64Opcode::MUL, {m_offset_scaled, m_offset, m_scale}});
        emit({AArch64Opcode::ADD, {std::move(m_dst), std::move(m_base), m_offset_scaled}});

        return m_dst;
    }

    mcode::Operand m_shift = mcode::Operand::from_aarch64_left_shift(shift);
    emit({AArch64Opcode::ADD, {std::move(m_dst), std::move(m_base), m_offset, m_shift}});
    return m_dst;
}

mcode::Operand AArch64SSALowerer::emit_add_imm(mcode::Operand m_lhs, LargeInt immediate) {
    mcode::Operand m_dst = create_temp_value(8);

    if (immediate >= 0 && immediate < 4096) {
        mcode::Operand m_addend = mcode::Operand::from_int_immediate(immediate, 8);
        emit({AArch64Opcode::ADD, {std::move(m_dst), std::move(m_lhs), m_addend}});
    } else {
        mcode::Operand m_addend = move_int_into_register(immediate, 8);
        emit({AArch64Opcode::ADD, {std::move(m_dst), std::move(m_lhs), m_addend}});
    }

    return m_dst;
}

} // namespace banjo::target
