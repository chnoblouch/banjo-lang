#include "aarch64_ir_lowerer.hpp"

#include "target/aarch64/aapcs_calling_conv.hpp"
#include "target/aarch64/aarch64_immediate.hpp"
#include "target/aarch64/aarch64_opcode.hpp"
#include "target/aarch64/aarch64_register.hpp"
#include "utils/macros.hpp"

namespace target {

// clang-format off

AArch64IRLowerer::AArch64IRLowerer(Target* target) :
IRLowerer(target) {}

mcode::Operand AArch64IRLowerer::lower_value(const ir::Operand& operand) {
    int size = get_size(operand.get_type());

    if(operand.is_immediate()) return move_const_into_register(operand, operand.get_type());
    else if(operand.is_register()) return lower_reg_val(operand.get_register(), size);
    else if(operand.is_symbol()) return move_symbol_into_register(operand.get_symbol_name());
    else if(operand.is_string()) return lower_str_val(operand.get_string(), size);

    return {};
}

mcode::Value AArch64IRLowerer::lower_global_value(ir::Value &value) {
    int size = get_size(value.get_type());

    if (value.is_int_immediate()) return mcode::Value::from_immediate(value.get_int_immediate().to_string(), size);
    else if (value.is_fp_immediate()) return mcode::Value::from_immediate(std::to_string(value.get_fp_immediate()), size);
    else return lower_value(value);
}

void AArch64IRLowerer::lower_fp_operation(mcode::Opcode opcode, ir::Instruction& instr) {
    mcode::Register reg = lower_reg(*instr.get_dest());
    int size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(opcode, {
        mcode::Operand::from_register(reg, size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

mcode::Operand AArch64IRLowerer::lower_reg_val(ir::VirtualRegister virtual_reg, int size) {
    mcode::Register reg = lower_reg(virtual_reg);

    if(reg.is_stack_slot()) {
        mcode::Register temp_reg = mcode::Register::from_virtual(get_func().next_virtual_reg());
        emit(mcode::Instruction(AArch64Opcode::ADD, {
            mcode::Operand::from_register(temp_reg, size),
            mcode::Operand::from_register(mcode::Register::from_physical(AArch64Register::SP), size),
            mcode::Operand::from_stack_slot_offset({reg.get_stack_slot(), 0})
        }));
        return mcode::Operand::from_register(temp_reg, size);
    }
        
    return mcode::Operand::from_register(reg, size);
}

mcode::Operand AArch64IRLowerer::lower_str_val(const std::string& str, int size) {
    return mcode::Operand::from_data(std::vector<char>(str.begin(), str.end()), size);
}

mcode::Operand AArch64IRLowerer::lower_address(const ir::Operand& operand) {
    if (operand.is_register()) {
        mcode::Register reg = lower_reg(operand.get_register());
        if(reg.is_stack_slot()) {
            return mcode::Operand::from_register(reg, 8);
        } else {
            return mcode::Operand::from_aarch64_addr(AArch64Address::new_base(reg));
        }
    } else if (operand.is_symbol()) {
        mcode::Value temp_val = move_symbol_into_register(operand.get_symbol_name());
        AArch64Address addr = AArch64Address::new_base(temp_val.get_register());
        return mcode::Operand::from_aarch64_addr(addr);
    }

    return {};
}

mcode::CallingConvention* AArch64IRLowerer::get_calling_convention(ir::CallingConv calling_conv) {
    switch(calling_conv) {
        case ir::CallingConv::AARCH64_AAPCS: return (mcode::CallingConvention*) &AAPCSCallingConv::INSTANCE;
        default: return nullptr;
    }
}

void AArch64IRLowerer::lower_load(ir::Instruction& instr) {
    const ir::Type& type = instr.get_operand(0).get_type();
    int size = get_size(type);
    unsigned flag = type.is_floating_point() ? mcode::Instruction::FLAG_FLOAT : 0;

    mcode::Opcode opcode;
    if(size == 1) opcode = AArch64Opcode::LDRB;
    else if(size == 2) opcode = AArch64Opcode::LDRH;
    else opcode = AArch64Opcode::LDR;

    emit(mcode::Instruction(opcode, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), get_size(type)),
        lower_address(instr.get_operand(1))
    }, flag));
}

void AArch64IRLowerer::lower_store(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());

    mcode::Opcode opcode;
    if(size == 1) opcode = AArch64Opcode::STRB;
    else if(size == 2) opcode = AArch64Opcode::STRH;
    else opcode = AArch64Opcode::STR;

    emit({opcode, {lower_value(instr.get_operand(0)), lower_address(instr.get_operand(1))}});
}

void AArch64IRLowerer::lower_loadarg(ir::Instruction& instr) {
    ir::Type type = instr.get_operand(0).get_type();
    unsigned param = instr.get_operand(1).get_int_immediate().to_u64();
    ir::VirtualRegister dest = instr.get_dest().value();

    int size = get_size(type);
    bool is_fp = type.is_floating_point();
    
    mcode::CallingConvention* calling_conv = get_machine_func()->get_calling_conv();
    std::vector<mcode::ArgStorage> arg_storage = calling_conv->get_arg_storage(get_func().get_params());
    mcode::ArgStorage cur_arg_storage = arg_storage[param];

    mcode::Register reg = mcode::Register::from_virtual(-1);
    if(cur_arg_storage.in_reg) {
        reg = mcode::Register::from_physical(cur_arg_storage.reg);
    } else {
        int slot_index = get_machine_func()->get_parameters()[param].get_stack_slot_index();
        reg = mcode::Register::from_stack_slot(slot_index);
    }

    mcode::Instruction machine_instr(is_fp ? AArch64Opcode::FMOV : AArch64Opcode::MOV, {
        mcode::Operand::from_register(mcode::Register::from_virtual(dest), size),
        mcode::Operand::from_register(reg, size)
    });
    machine_instr.set_flag(mcode::Instruction::FLAG_ARG_STORE);
    emit(machine_instr);
}

void AArch64IRLowerer::lower_add(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::ADD, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_sub(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::SUB, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_mul(ir::Instruction& instr) {
    const ir::Value &lhs = instr.get_operand(0);
    const ir::Value &rhs = instr.get_operand(1);

    int size = get_size(lhs.get_type());

    mcode::Operand multiplier;
    if(rhs.is_immediate()) {
        multiplier = move_const_into_register(rhs, rhs.get_type());
    } else {
        multiplier = lower_value(rhs);
    }

    emit(mcode::Instruction(AArch64Opcode::MUL, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(lhs),
        multiplier
    }));
}

void AArch64IRLowerer::lower_sdiv(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::SDIV, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_srem(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());
    mcode::Operand dividend = lower_value(instr.get_operand(0));
    mcode::Operand divisor = lower_value(instr.get_operand(1));
    mcode::Operand tmp1 = mcode::Operand::from_register(lower_reg(get_func().next_virtual_reg()), size);
    mcode::Operand tmp2 = mcode::Operand::from_register(lower_reg(get_func().next_virtual_reg()), size);
    mcode::Operand remainder = mcode::Operand::from_register(lower_reg(*instr.get_dest()), size);

    emit(mcode::Instruction(AArch64Opcode::SDIV, {tmp1, dividend, divisor}));
    emit(mcode::Instruction(AArch64Opcode::MUL, {tmp2, tmp1, divisor}));
    emit(mcode::Instruction(AArch64Opcode::SUB, {remainder, dividend, tmp2}));
}

void AArch64IRLowerer::lower_udiv(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::UDIV, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_urem(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());
    mcode::Operand dividend = lower_value(instr.get_operand(0));
    mcode::Operand divisor = lower_value(instr.get_operand(1));
    mcode::Operand tmp1 = mcode::Operand::from_register(lower_reg(get_func().next_virtual_reg()), size);
    mcode::Operand tmp2 = mcode::Operand::from_register(lower_reg(get_func().next_virtual_reg()), size);
    mcode::Operand remainder = mcode::Operand::from_register(lower_reg(*instr.get_dest()), size);

    emit(mcode::Instruction(AArch64Opcode::UDIV, {tmp1, dividend, divisor}));
    emit(mcode::Instruction(AArch64Opcode::MUL, {tmp2, tmp1, divisor}));
    emit(mcode::Instruction(AArch64Opcode::SUB, {remainder, dividend, tmp2}));
}

void AArch64IRLowerer::lower_fadd(ir::Instruction& instr) {
    lower_fp_operation(AArch64Opcode::FADD, instr);
}

void AArch64IRLowerer::lower_fsub(ir::Instruction& instr) {
    lower_fp_operation(AArch64Opcode::FSUB, instr);
}

void AArch64IRLowerer::lower_fmul(ir::Instruction& instr) {
    lower_fp_operation(AArch64Opcode::FMUL, instr);
}

void AArch64IRLowerer::lower_fdiv(ir::Instruction& instr) {
    lower_fp_operation(AArch64Opcode::FDIV, instr);
}

void AArch64IRLowerer::lower_and(ir::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::AND, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_or(ir::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::ORR, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_xor(ir::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::EOR, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_shl(ir::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::LSL, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_shr(ir::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(AArch64Opcode::ASR, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(1))
    }));
}

void AArch64IRLowerer::lower_jmp(ir::Instruction& instr) {
    move_branch_args(instr.get_operand(0).get_branch_target());

    emit(mcode::Instruction(AArch64Opcode::B, {
        mcode::Operand::from_label(instr.get_operand(0).get_branch_target().block->get_label())
    }));
}

void AArch64IRLowerer::lower_cjmp(ir::Instruction& instr) {
    move_branch_args(instr.get_operand(3).get_branch_target());
    move_branch_args(instr.get_operand(4).get_branch_target());

    emit(mcode::Instruction(AArch64Opcode::CMP, {
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(2))
    })); 

    AArch64Condition condition = lower_condition(instr.get_operand(1).get_comparison());
    mcode::Opcode branch_opcode = AArch64Opcode::B_EQ + (unsigned)condition;

    emit(mcode::Instruction(branch_opcode, {
        mcode::Operand::from_label(instr.get_operand(3).get_branch_target().block->get_label())
    }));

    emit(mcode::Instruction(AArch64Opcode::B, {
        mcode::Operand::from_label(instr.get_operand(4).get_branch_target().block->get_label())
    }));
}

void AArch64IRLowerer::lower_fcjmp(ir::Instruction &instr) {
    move_branch_args(instr.get_operand(3).get_branch_target());
    move_branch_args(instr.get_operand(4).get_branch_target());

    emit(mcode::Instruction(AArch64Opcode::FCMP, {
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(2))
    })); 

    AArch64Condition condition = lower_condition(instr.get_operand(1).get_comparison());
    mcode::Opcode branch_opcode = AArch64Opcode::B_EQ + (unsigned)condition;

    emit(mcode::Instruction(branch_opcode, {
        mcode::Operand::from_label(instr.get_operand(3).get_branch_target().block->get_label())
    }));

    emit(mcode::Instruction(AArch64Opcode::B, {
        mcode::Operand::from_label(instr.get_operand(4).get_branch_target().block->get_label())
    }));
}

void AArch64IRLowerer::lower_select(ir::Instruction &instr) {
    int size = get_size(instr.get_operand(0).get_type());
    bool is_fp = instr.get_operand(0).get_type().is_floating_point();
    
    emit(mcode::Instruction(is_fp ? AArch64Opcode::FCMP : AArch64Opcode::CMP, {
        lower_value(instr.get_operand(0)),
        lower_value(instr.get_operand(2))
    })); 

    AArch64Condition condition = lower_condition(instr.get_operand(1).get_comparison());

    emit(mcode::Instruction(is_fp ? AArch64Opcode::FCSEL : AArch64Opcode::CSEL, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(3)),
        lower_value(instr.get_operand(4)),
        mcode::Operand::from_aarch64_condition(condition)
    }));
}

void AArch64IRLowerer::lower_call(ir::Instruction& instr) {
    get_machine_func()->get_calling_conv()->lower_call(*this, instr);
}

void AArch64IRLowerer::lower_ret(ir::Instruction& instr) {
    if(!instr.get_operands().empty()) {
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

void AArch64IRLowerer::lower_uextend(ir::Instruction& instr) {
    // FIXME

    emit(mcode::Instruction(AArch64Opcode::MOV, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), 8),
        lower_value(instr.get_operand(0))
    }));
}

void AArch64IRLowerer::lower_sextend(ir::Instruction& instr) {
    emit(mcode::Instruction(AArch64Opcode::SXTW, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), 8),
        lower_value(instr.get_operand(0))
    }));
}

void AArch64IRLowerer::lower_truncate(ir::Instruction& instr) {
    mcode::Register dst_reg = lower_reg(*instr.get_dest());
    mcode::Operand src = lower_value(instr.get_operand(0));
    
    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    src.set_size(dst_size);

    emit(mcode::Instruction(AArch64Opcode::MOV, {
        mcode::Operand::from_register(dst_reg, dst_size),
        src
    }));
}

void AArch64IRLowerer::lower_fpromote(ir::Instruction& instr) {
    emit(mcode::Instruction(AArch64Opcode::FCVT, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), 8),
        lower_value(instr.get_operand(0))
    }));
}

void AArch64IRLowerer::lower_fdemote(ir::Instruction& instr) {
    emit(mcode::Instruction(AArch64Opcode::FCVT, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), 4),
        lower_value(instr.get_operand(0))
    }));
}

void AArch64IRLowerer::lower_stof(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(1).get_type());

    emit(mcode::Instruction(AArch64Opcode::SCVTF, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0))
    }));
}

void AArch64IRLowerer::lower_ftou(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(1).get_type());

    emit(mcode::Instruction(AArch64Opcode::FCVTZU, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0))
    })); 
}

void AArch64IRLowerer::lower_ftos(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(1).get_type());

    emit(mcode::Instruction(AArch64Opcode::FCVTZS, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
        lower_value(instr.get_operand(0))
    })); 
}

void AArch64IRLowerer::lower_offsetptr(ir::Instruction& instr) {
    ir::Operand &operand = instr.get_operand(1);
    const ir::Type &base_type = instr.get_operand(2).get_type();
    
    Address addr;
    addr.base = lower_value(instr.get_operand(0));

    if(operand.is_immediate()) {
        unsigned int_offset = operand.get_int_immediate().to_u64();
        addr.imm_offset = int_offset * get_size(base_type);
    } else if(operand.is_register()) {
        addr.reg_offset = lower_reg(operand.get_register());
        addr.reg_scale = get_size(base_type);
    }

    calculate_address(lower_reg(*instr.get_dest()), addr);
}

void AArch64IRLowerer::lower_memberptr(ir::Instruction& instr) {
    const ir::Type &type = instr.get_operand(0).get_type();
    unsigned int_offset = instr.get_operand(2).get_int_immediate().to_u64();

    Address addr;
    addr.base = lower_value(instr.get_operand(1));

    if (type.is_struct()) {
        ir::Structure* struct_ = type.get_struct();
        addr.imm_offset = get_member_offset(struct_, int_offset);
    } else if (type.is_tuple()) {
        const std::vector<ir::Type>& tuple_types = type.get_tuple_types();
        addr.imm_offset = get_member_offset(tuple_types, int_offset);
    }

    calculate_address(lower_reg(*instr.get_dest()), addr);
}

mcode::Value AArch64IRLowerer::move_const_into_register(const ir::Value &value, ir::Type type) {
    int size = get_size(type);
    
    if (value.is_int_immediate()) return move_int_into_register(value.get_int_immediate(), size);
    else if (value.is_fp_immediate()) return move_float_into_register(value.get_fp_immediate(), size);
    else ASSERT_UNREACHABLE;
}

mcode::Value AArch64IRLowerer::move_int_into_register(LargeInt value, int size) {
    std::uint64_t bits = value.to_bits();
    mcode::Value result = create_temp_value(size);

    // Zero can be moved directly without decomposing because MOVN sets the upper bits to zero.
    if(bits == 0) {
        emit(mcode::Instruction(AArch64Opcode::MOV, {result, mcode::Operand::from_immediate("0")}));
        return result;
    }

    if(size == 1 || size == 2) {
        std::string imm = std::to_string(bits);
        emit(mcode::Instruction(AArch64Opcode::MOV, {result, mcode::Operand::from_immediate(imm)}));
    } else if(size == 4) {
        std::uint16_t elements[2];
        AArch64Immediate::decompose_u32_u16(bits, elements);
        move_elements_into_register(result, elements, 2);
    } else if(size == 8) {
        std::uint16_t elements[4];
        AArch64Immediate::decompose_u64_u16(bits, elements);
        move_elements_into_register(result, elements, 4);
    }

    return result;
}

mcode::Value AArch64IRLowerer::move_float_into_register(double value, int size) {
    std::string immediate = std::to_string(value);
    mcode::Value result = create_temp_value(size);

    // Emit FMOV if the value can be represented using 8 bits.
    if(AArch64Immediate::is_float_encodable(value)) {
        emit(mcode::Instruction(AArch64Opcode::FMOV, {result, mcode::Operand::from_immediate(immediate)}));
        return result;
    }

    if(size == 4) {
        // 32-bit floats can be stored by moving the bits into a Wn register using
        // MOV and MOVK and moving from Wn to Sn afterwards.

        // TODO: FMOV is not required when this instruction is used for storing floats in memory.
        // We could just return the general-purpose register with the bits directly.

        float value_f32 = (float)value;
        std::uint32_t bits = BitOperations::get_bits_32(value_f32);
        std::uint16_t elements[2];
        AArch64Immediate::decompose_u32_u16(bits, elements);

        mcode::Value bits_value = create_temp_value(size);
        move_elements_into_register(bits_value, elements, 2);
        emit(mcode::Instruction(AArch64Opcode::FMOV, {result, bits_value}));
    } else {
        float value_f64 = (double)value;
        mcode::Value value = mcode::Value::from_fp(value_f64, size);
        std::string symbol_name = "double." + std::to_string(next_const_index++);
        get_machine_module().add(mcode::Global(symbol_name, value));

        mcode::Value symbol_addr = move_symbol_into_register(symbol_name);
        AArch64Address addr = AArch64Address::new_base(symbol_addr.get_register());
        emit(mcode::Instruction(AArch64Opcode::LDR, {result, mcode::Value::from_aarch64_addr(addr)}));
    }

    return result;
}

void AArch64IRLowerer::move_elements_into_register(mcode::Value value, std::uint16_t* elements, int count) {
    // Count the non-zero elements.
    int num_non_zero_elements = 0;
    int non_zero_element_index = 0;
    for(int i = 0; i < count; i++) {
        if(elements[i] != 0) {
            num_non_zero_elements++;
            non_zero_element_index = i;
        }
    }

    // Emit a single MOVZ if all but one element are zero.
    if(num_non_zero_elements == 1) {
        int element = elements[non_zero_element_index];
        emit(mcode::Instruction(AArch64Opcode::MOVZ, {
            value,
            mcode::Operand::from_immediate(std::to_string(element)),
            mcode::Operand::from_aarch64_left_shift(16 * non_zero_element_index)
        }));
        return;
    }
    
    // Move the lower 16 bits and set all other bits to zero.
    emit(mcode::Instruction(AArch64Opcode::MOVZ, {
        value,
        mcode::Operand::from_immediate(std::to_string(elements[0]))
    }));

    for(int i = 0; i < count; i++) {
        int element = elements[i];
        
        // Skip zero elements because the bits were already set to zero by the first MOVZ.
        if(element == 0) {
            continue;
        }

        // Set the bits by shifting the value by 16 * index to the left and moving them.
        emit(mcode::Instruction(AArch64Opcode::MOVK, {
            value,
            mcode::Operand::from_immediate(std::to_string(element)),
            mcode::Operand::from_aarch64_left_shift(16 * i)
        }));
    }
}

mcode::Value AArch64IRLowerer::move_symbol_into_register(std::string symbol) {
    mcode::Value result = create_temp_value(8);

    bool is_darwin = target->get_descr().is_darwin();
        
    mcode::Operand symbol_page = mcode::Operand::from_symbol({
        symbol,
        is_darwin ? mcode::Directive::PAGE : mcode::Directive::NONE,
    });

    mcode::Operand symbol_pageoff = mcode::Operand::from_symbol({
        symbol,
        is_darwin ? mcode::Relocation::NONE : mcode::Relocation::LO12,
        is_darwin ? mcode::Directive::PAGEOFF : mcode::Directive::NONE,
    });
    
    emit(mcode::Instruction(AArch64Opcode::ADRP, {result, symbol_page}));
    emit(mcode::Instruction(AArch64Opcode::ADD, {result, result, symbol_pageoff}));

    return result;
}

void AArch64IRLowerer::calculate_address(mcode::Register dst, Address addr) {
    mcode::Operand dst_operand = mcode::Operand::from_register(dst, 8);

    if(addr.reg_offset.get_virtual_reg() != -1) {
        auto offset = mcode::Operand::from_register(addr.reg_offset, 8);

        int shift_amount = 0;
        if (addr.reg_scale == 1) shift_amount = 0;
        else if (addr.reg_scale == 2) shift_amount = 1;
        else if (addr.reg_scale == 4) shift_amount = 2;
        else if (addr.reg_scale == 8) shift_amount = 3;
        else {
            mcode::Register scale_reg = lower_reg(get_func().next_virtual_reg());
            mcode::Operand scale_val = mcode::Operand::from_register(scale_reg, 8);
            mcode::Operand imm_val = mcode::Operand::from_immediate(std::to_string(addr.reg_scale), 8);

            emit(mcode::Instruction(AArch64Opcode::MOV, {scale_val, imm_val}));
            emit(mcode::Instruction(AArch64Opcode::MUL, {offset, offset, scale_val}));
        }

        auto shift = mcode::Operand::from_aarch64_left_shift(shift_amount);
        emit(mcode::Instruction(AArch64Opcode::ADD, {dst_operand, addr.base, offset, shift}));
    } else if(addr.imm_offset >= 0 && addr.imm_offset < 4096) {
        auto offset = mcode::Operand::from_immediate(std::to_string(addr.imm_offset), 8);
        emit(mcode::Instruction(AArch64Opcode::ADD, {dst_operand, addr.base, offset}));
    } else {
        mcode::Value offset = move_int_into_register(LargeInt{addr.imm_offset}, 8);
        emit(mcode::Instruction(AArch64Opcode::ADD, {dst_operand, addr.base, offset}));
    }
}

mcode::Value AArch64IRLowerer::create_temp_value(int size) {
    ir::VirtualRegister reg = get_func().next_virtual_reg();
    return mcode::Operand::from_register(mcode::Register::from_virtual(reg), size);
}

AArch64Condition AArch64IRLowerer::lower_condition(ir::Comparison comparison) {
    switch(comparison) {
        case ir::Comparison::EQ: return AArch64Condition::EQ;
        case ir::Comparison::NE: return AArch64Condition::NE;
        case ir::Comparison::UGT: return AArch64Condition::HI;
        case ir::Comparison::UGE: return AArch64Condition::HS;
        case ir::Comparison::ULT: return AArch64Condition::LO;
        case ir::Comparison::ULE: return AArch64Condition::LS;
        case ir::Comparison::SGT: return AArch64Condition::GT;
        case ir::Comparison::SGE: return AArch64Condition::GE;
        case ir::Comparison::SLT: return AArch64Condition::LT;
        case ir::Comparison::SLE: return AArch64Condition::LE;
        case ir::Comparison::FEQ: return AArch64Condition::EQ;
        case ir::Comparison::FNE: return AArch64Condition::NE;
        case ir::Comparison::FGT: return AArch64Condition::GT;
        case ir::Comparison::FGE: return AArch64Condition::GE;
        case ir::Comparison::FLT: return AArch64Condition::LT;
        case ir::Comparison::FLE: return AArch64Condition::LE;
    }
}

void AArch64IRLowerer::move_branch_args(ir::BranchTarget &target) {
    for (unsigned i = 0; i < target.args.size(); i++) {
        ir::Value &arg = target.args[i];
        ir::VirtualRegister param_reg = target.block->get_param_regs()[i];

        bool is_fp = arg.get_type().is_floating_point();
        mcode::Opcode move_opcode = is_fp ? AArch64Opcode::FMOV : AArch64Opcode::MOV;

        unsigned size = get_size(arg.get_type());
        mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_virtual(param_reg), size);

        emit(mcode::Instruction(move_opcode, {dst, lower_value(arg)}, mcode::Instruction::FLAG_CALL_ARG));
    }
}

// clang-format on

} // namespace target
