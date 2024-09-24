#include "x86_64_ir_lowerer.hpp"

#include "banjo/config/config.hpp"
#include "banjo/ir/comparison.hpp"
#include "banjo/ir/instruction.hpp"
#include "banjo/ir/operand.hpp"
#include "banjo/mcode/calling_convention.hpp"
#include "banjo/mcode/global.hpp"
#include "banjo/target/x86_64/ms_abi_calling_conv.hpp"
#include "banjo/target/x86_64/sys_v_calling_conv.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/utils/bit_operations.hpp"
#include "banjo/utils/macros.hpp"

#include <algorithm>
#include <string>

#include <iostream>

// clang-format off

namespace banjo {

namespace target {

X8664IRLowerer::X8664IRLowerer(Target *target) : IRLowerer(target), addr_lowering(*this), const_lowering(*this) {}

void X8664IRLowerer::init_module(ir::Module &mod) {
    mcode::Global::Bytes const_neg_zero{
        0, 0, 0, 1u << 7,
        0, 0, 0, 1u << 7,
        0, 0, 0, 1u << 7,
        0, 0, 0, 1u << 7,
    };

    get_machine_module().add(mcode::Global{
        .name = "const.neg_zero",
        .size = 4,
        .value = const_neg_zero,
    });
}

mcode::Value X8664IRLowerer::lower_global_value(ir::Value &value) {
    if (value.get_type().is_primitive(ir::Primitive::F32)) {
        return mcode::Value::from_immediate("__float32__(" + std::to_string(value.get_fp_immediate()) + ")", 4);
    } else if (value.get_type().is_primitive(ir::Primitive::F64)) {
        return mcode::Value::from_immediate("__float64__(" + std::to_string(value.get_fp_immediate()) + ")", 8);
    }

    return lower_value(value);
}

mcode::Operand X8664IRLowerer::lower_value(const ir::Operand& operand) {
    unsigned size = get_size(operand.get_type());

    if (operand.is_int_immediate()) {
        if (size != 8 || operand.get_int_immediate().to_bits() < (1ull << 32)) {
            return mcode::Operand::from_immediate(operand.get_int_immediate().to_string(), size);
        } else {
            mcode::Register reg = mcode::Register::from_virtual(get_func().next_virtual_reg());
            emit(mcode::Instruction(X8664Opcode::MOV, {
                mcode::Operand::from_register(reg, size),
                mcode::Operand::from_immediate(operand.get_int_immediate().to_string(), size)
            }));
            return mcode::Operand::from_register(reg, size);
        }
    } else if (operand.is_fp_immediate()) {
        unsigned size = get_size(operand.get_type());
        bool is_64_bit = size == 8;
        ir::VirtualRegister temp_reg = get_func().next_virtual_reg();

        if (operand.get_fp_immediate() == 0.0) {
            mcode::Operand result = mcode::Operand::from_register(mcode::Register::from_virtual(temp_reg));
            emit(mcode::Instruction(is_64_bit ? X8664Opcode::XORPD : X8664Opcode::XORPS, {result, result}));
            return result;
        }

        emit(mcode::Instruction(is_64_bit ? X8664Opcode::MOVSD : X8664Opcode::MOVSS, {
            mcode::Operand::from_register(mcode::Register::from_virtual(temp_reg)),
            get_float_addr(operand.get_fp_immediate(), operand.get_type())
        }));

        return mcode::Operand::from_register(mcode::Register::from_virtual(temp_reg), size);
    } else if (operand.is_register()) {
        mcode::Register reg = lower_reg(operand.get_register());
        
        if (reg.is_stack_slot()) {
            mcode::Register tmp_reg = mcode::Register::from_virtual(get_func().next_virtual_reg());
            emit(mcode::Instruction(X8664Opcode::LEA, {
                mcode::Operand::from_register(tmp_reg, PTR_SIZE),
                mcode::Operand::from_register(reg, size)
            }));
            return mcode::Operand::from_register(tmp_reg, PTR_SIZE);
        } else {
            return mcode::Operand::from_register(reg, size);
        }
    } else if (operand.is_symbol()) {
        const std::string &symbol_name = operand.get_symbol_name();

        mcode::Relocation reloc = mcode::Relocation::NONE;
        if (lang::Config::instance().is_pic() && target->get_descr().is_unix()) {
            if (operand.is_extern_func()) reloc = mcode::Relocation::PLT;
            else if (operand.is_extern_global()) reloc = mcode::Relocation::GOT;
            else ASSERT_UNREACHABLE;
        }

        mcode::Symbol symbol(symbol_name, reloc);

        if (target->get_descr().is_darwin() && (operand.is_func() || operand.is_extern_func())) {
            return mcode::Operand::from_symbol(symbol, PTR_SIZE);
        }
        
        if (target->get_code_model() == CodeModel::SMALL) {
            mcode::Operand src = mcode::Operand::from_symbol_deref(symbol, size);
            mcode::Register temp_reg = mcode::Register::from_virtual(get_func().next_virtual_reg());
            emit(mcode::Instruction(X8664Opcode::LEA, {
                mcode::Operand::from_register(temp_reg, PTR_SIZE), src
            }));
            return mcode::Operand::from_register(temp_reg, PTR_SIZE);
        } else if (target->get_code_model() == CodeModel::LARGE) {
            return read_symbol_addr(symbol);
        } else {
            ASSERT_UNREACHABLE;
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Operand X8664IRLowerer::lower_address(const ir::Operand& operand) {
    return addr_lowering.lower_address(operand); 
}

void X8664IRLowerer::append_mov_and_operation(
    mcode::Opcode machine_opcode,
    ir::VirtualRegister dst,
    ir::Value &lhs,
    ir::Value &rhs
) {
    ir::Type type = lhs.get_type();
    int size = get_size(type);
    mcode::Opcode move_opcode = get_move_opcode(type);
    mcode::Operand reg_val = mcode::Operand::from_register(lower_reg(dst), size);

    emit(mcode::Instruction(move_opcode, {reg_val, lower_value(lhs)}));

    if (rhs.is_register()) {
        ir::InstrIter producer = get_producer(rhs.get_register());
        if (producer->get_opcode() == ir::Opcode::LOAD && get_num_uses(*producer->get_dest()) == 1) {
            emit(mcode::Instruction(machine_opcode, {reg_val, lower_address(producer->get_operand(1))}));
            discard_use(*producer->get_dest());
            return;
        }
    }

    mcode::Operand m_rhs;
    if (reg_val.is_register() && type.is_floating_point() && rhs.is_fp_immediate()) {
        if (rhs.get_type().is_primitive(ir::Primitive::F32) && rhs.get_fp_immediate() == 0.0) {
            m_rhs = mcode::Operand::from_register(lower_reg(get_func().next_virtual_reg()), 4);
            emit(mcode::Instruction(X8664Opcode::XORPS, {m_rhs, m_rhs}));
        } else {
            m_rhs = get_float_addr(rhs.get_fp_immediate(), type);
        }
    } else {
        m_rhs = lower_value(rhs);
    }

    emit(mcode::Instruction(machine_opcode, {reg_val, m_rhs}));
}

bool X8664IRLowerer::lower_stored_operation(ir::Instruction& store) {
    const ir::Operand result = store.get_operand(0);
    if(!result.is_register()) {
        return false;
    }

    ir::InstrIter operation_iter = get_producer(result.get_register());
    if(operation_iter == get_block().end()) {
        return false;
    }

    // int size = get_size(operation_iter->get_operand(0).get_type());

    mcode::Opcode machine_opcode;
    switch(operation_iter->get_opcode()) {
        // case ir::Opcode::ADD: machine_opcode = X8664Opcode::ADD; break;
        // case ir::Opcode::SUB: machine_opcode = X8664Opcode::SUB; break;
        //case ir::Opcode::MUL: machine_opcode = X8664Opcode::IMUL; break;
        //case ir::Opcode::FADD: machine_opcode = size == 8 ? X8664Opcode::ADDSD : X8664Opcode::ADDSS; break;
        //case ir::Opcode::FSUB: machine_opcode = size == 8 ? X8664Opcode::SUBSD : X8664Opcode::SUBSS; break;
        //case ir::Opcode::FMUL: machine_opcode = size == 8 ? X8664Opcode::MULSD : X8664Opcode::MULSS; break;
        //case ir::Opcode::FDIV: machine_opcode = size == 8 ? X8664Opcode::DIVSD : X8664Opcode::DIVSS; break;
        default: return false;
    }
    
    const ir::Operand& lhs = operation_iter->get_operand(0);
    const ir::Operand& rhs = operation_iter->get_operand(1);

    if(lhs.is_register() && rhs.is_register()) {
        ir::InstrIter lhs_producer_iter = get_producer(lhs.get_register());
        ir::InstrIter rhs_producer_iter = get_producer(lhs.get_register());
        if(lhs_producer_iter == get_block().end() || rhs_producer_iter == get_block().end()) {
            return false;
        }

        ir::Instruction& lhs_producer = *lhs_producer_iter;
        ir::Instruction& rhs_producer = *lhs_producer_iter;

        if(lhs_producer.get_opcode() == ir::Opcode::LOAD && rhs_producer.get_opcode() == ir::Opcode::LOAD && lhs_producer.get_operand(1) == store.get_operand(1)) {
            discard_use(*operation_iter->get_dest());
            discard_use(*lhs_producer.get_dest());

            mcode::Operand dest = lower_address(store.get_operand(1));
            if(dest.is_register()) {
                dest = mcode::Operand::from_addr({dest.get_register()});
            }
            dest.set_size(get_size(store.get_operand(0).get_type()));
            emit(mcode::Instruction(machine_opcode, {
                dest,
                mcode::Operand::from_register(lower_reg(*rhs_producer.get_dest()), dest.get_size())
            }));
            
            return true;
        }
    } else if(lhs.is_register() && rhs.is_immediate()) {
        ir::InstrIter lhs_producer_iter = get_producer(lhs.get_register());
        if(lhs_producer_iter == get_block().end()) {
            return false;
        }

        ir::Instruction& lhs_producer = *lhs_producer_iter;

        if(lhs_producer.get_opcode() == ir::Opcode::LOAD && lhs_producer.get_operand(1) == store.get_operand(1)) {
            discard_use(*operation_iter->get_dest());
            discard_use(*lhs_producer.get_dest());
            discard_use(store.get_operand(1).get_register());

            mcode::Operand dst = lower_address(store.get_operand(1));
            if(dst.is_register()) {
                dst = mcode::Operand::from_addr({dst.get_register()});
            }
            dst.set_size(get_size(store.get_operand(0).get_type()));
            emit(mcode::Instruction(machine_opcode, {dst, lower_value(rhs)}));
            
            return true;
        }
    }

    return false;
}

void X8664IRLowerer::lower_load(ir::Instruction& instr) {
    ir::Type type = instr.get_operand(0).get_type();
    int size = get_size(type);

    mcode::Operand src = lower_address(instr.get_operand(1));
    mcode::Operand dst = mcode::Operand::from_register(lower_reg(*instr.get_dest()), size);
    src.set_size(dst.get_size());

    mcode::Opcode opcode = get_move_opcode(type);
    emit(mcode::Instruction(opcode, {dst, src}));
} 

void X8664IRLowerer::lower_store(ir::Instruction& instr) {
    if(lower_stored_operation(instr)) {
        return;
    }

    mcode::Operand dst = lower_address(instr.get_operand(1));
    ir::Type type = instr.get_operand(0).get_type();
    mcode::Instruction m_instr;

    if (instr.get_operand(0).is_immediate() && type.is_primitive(ir::Primitive::F32) && (dst.is_addr() || dst.is_stack_slot())) {
        float val = (float)instr.get_operand(0).get_fp_immediate();
        mcode::Operand src = mcode::Operand::from_immediate(std::to_string(BitOperations::get_bits_32(val)), 4);
        dst.set_size(src.get_size());
        m_instr = mcode::Instruction(X8664Opcode::MOV, {dst, src});
    } else {
        mcode::Operand src = lower_value(instr.get_operand(0));
        dst.set_size(src.get_size());

        if(src.is_symbol()) {
            mcode::Register tmp_reg = mcode::Register::from_virtual(get_func().next_virtual_reg());
            mcode::Operand tmp_val = mcode::Operand::from_register(tmp_reg, src.get_size());
            emit(mcode::Instruction(X8664Opcode::MOV, {tmp_val, src}));
            src = tmp_val;
        }

        mcode::Opcode opcode = get_move_opcode(type);
        m_instr = mcode::Instruction(opcode, {dst, src});
    }

    if(instr.is_flag(ir::Instruction::FLAG_SAVE_ARG)) {
        m_instr.set_flag(mcode::Instruction::FLAG_ARG_STORE);
    }

    emit(m_instr);
}

void X8664IRLowerer::lower_loadarg(ir::Instruction& instr) {
    ir::Type type = instr.get_operand(0).get_type();
    unsigned param = instr.get_operand(1).get_int_immediate().to_u64();
    ir::VirtualRegister dest = instr.get_dest().value();

    int size = get_size(type);

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

    mcode::Opcode opcode = get_move_opcode(type);
    mcode::Instruction machine_instr(opcode, {
        mcode::Operand::from_register(mcode::Register::from_virtual(dest), size),
        mcode::Operand::from_register(reg, size)
    });
    machine_instr.set_flag(mcode::Instruction::FLAG_ARG_STORE);
    emit(machine_instr);
}

void X8664IRLowerer::lower_add(ir::Instruction& instr) {
    ir::Operand lhs = instr.get_operand(0);
    ir::Operand rhs = instr.get_operand(1);
    // int size = get_size(lhs.get_type());

    // Try to use a LEA dst, [lhs + rhs] instruction.
    /*
    if(lhs.is_register() && rhs.is_register()) {
        size = std::max(size, 4);

        emit(mcode::Instruction(X8664Opcode::LEA, {
            mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
            mcode::Operand::from_indirect_addr(mcode::IndirectAddress(
                lower_reg(lhs.get_register()), lower_reg(rhs.get_register()), 1
            ), 8)
        }));

        return;
    }
    */

    append_mov_and_operation(X8664Opcode::ADD, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_sub(ir::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::SUB, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_mul(ir::Instruction& instr) {
    ir::Operand& lhs = instr.get_operand(0);
    ir::Operand& rhs = instr.get_operand(1);

    if(lhs.is_register() && rhs.is_immediate()) {
        unsigned size = get_size(lhs.get_type());

        if((size == 4 || size == 8) && rhs.get_int_immediate() == 3) {
            mcode::Register lhs_reg = lower_reg(lhs.get_register());
            mcode::IndirectAddress addr(lhs_reg, lhs_reg, 2);

            emit(mcode::Instruction(X8664Opcode::LEA, {
                mcode::Operand::from_register(lower_reg(*instr.get_dest()), size),
                mcode::Operand::from_addr(addr, size)
            }));

            return;
        }
    }

    append_mov_and_operation(X8664Opcode::IMUL, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_sdiv(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());
    mcode::Register diviser_reg = mcode::Register::from_physical(X8664Register::RCX);

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RAX), size),
        lower_value(instr.get_operand(0)),
    }));
    
    if(size == 4) {
        emit(mcode::Instruction(X8664Opcode::CDQ));
    } else if(size == 8) {
        emit(mcode::Instruction(X8664Opcode::CQO));
    }

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(diviser_reg, size),
        lower_value(instr.get_operand(1)),
    }));
    emit(mcode::Instruction(X8664Opcode::IDIV, {
        mcode::Operand::from_register(diviser_reg, size)
    }));
    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(lower_reg(instr.get_dest().value()), size),
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RAX), size)
    }));
}

void X8664IRLowerer::lower_srem(ir::Instruction& instr) {
    int size = get_size(instr.get_operand(0).get_type());
    mcode::Register diviser_reg = mcode::Register::from_virtual(get_func().next_virtual_reg());

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RAX), size),
        lower_value(instr.get_operand(0)),
    }));
    
    if(size == 4) {
        emit(mcode::Instruction(X8664Opcode::CDQ));
    } else if(size == 8) {
        emit(mcode::Instruction(X8664Opcode::CQO));
    }

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(diviser_reg, size),
        lower_value(instr.get_operand(1)),
    }));
    emit(mcode::Instruction(X8664Opcode::IDIV, {
        mcode::Operand::from_register(diviser_reg, size)
    }));
    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(lower_reg(instr.get_dest().value()), size),
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RDX), size)
    }));
}

void X8664IRLowerer::lower_udiv(ir::Instruction &instr) {
    // FIXME: yes, this is obviously wrong
    lower_sdiv(instr);
}

void X8664IRLowerer::lower_urem(ir::Instruction &instr) {
    // FIXME: yes, this is obviously wrong
    lower_srem(instr);
}

void X8664IRLowerer::lower_fadd(ir::Instruction& instr) {
    ir::Primitive type = instr.get_operand(0).get_type().get_primitive();
    mcode::Opcode opcode = type == ir::Primitive::F64 ? X8664Opcode::ADDSD : X8664Opcode::ADDSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_fsub(ir::Instruction& instr) {
    ir::Primitive type = instr.get_operand(0).get_type().get_primitive();
    
    if (type == ir::Primitive::F32 && instr.get_operand(0).is_fp_immediate() && instr.get_operand(0).get_fp_immediate() == 0.0) {
        if (!const_neg_zero) {
            const_neg_zero = {"const.neg_zero"};
        }

        mcode::Operand dst = mcode::Operand::from_register(lower_reg(*instr.get_dest()), 4);
        mcode::Operand addr = deref_symbol_addr(*const_neg_zero, 16);

        emit(mcode::Instruction(get_move_opcode(type), {dst, lower_value(instr.get_operand(1))}));
        emit(mcode::Instruction(X8664Opcode::XORPS, {dst.with_size(16), addr}));

        return;
    }

    mcode::Opcode opcode = type == ir::Primitive::F64 ? X8664Opcode::SUBSD : X8664Opcode::SUBSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_fmul(ir::Instruction& instr) {
    ir::Primitive type = instr.get_operand(0).get_type().get_primitive();
    mcode::Opcode opcode = type == ir::Primitive::F64 ? X8664Opcode::MULSD : X8664Opcode::MULSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_fdiv(ir::Instruction& instr) {
    ir::Primitive type = instr.get_operand(0).get_type().get_primitive();
    mcode::Opcode opcode = type == ir::Primitive::F64 ? X8664Opcode::DIVSD : X8664Opcode::DIVSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_and(ir::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::AND, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_or(ir::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::OR, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_xor(ir::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::XOR, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664IRLowerer::lower_shl(ir::Instruction& instr) {
    emit_shift(instr, X8664Opcode::SHL);
}

void X8664IRLowerer::lower_shr(ir::Instruction& instr) {
    emit_shift(instr, X8664Opcode::SHR);
}

void X8664IRLowerer::lower_jmp(ir::Instruction& instr) {
    ir::BranchTarget target = instr.get_operand(0).get_branch_target();
    ir::BasicBlockIter block_iter = target.block;

    move_branch_args(target);

    if(block_iter != get_basic_block_iter().get_next()) {
        emit(mcode::Instruction(X8664Opcode::JMP, {
            mcode::Operand::from_label(block_iter->get_label())
        }));
    }
}

void X8664IRLowerer::lower_cjmp(ir::Instruction& instr) {
    emit_jcc(instr, [this, &instr](){
        ir::VirtualRegister reg = get_func().next_virtual_reg();
        append_mov_and_operation(X8664Opcode::CMP, reg, instr.get_operand(0), instr.get_operand(2));
    });
}

void X8664IRLowerer::lower_fcjmp(ir::Instruction& instr) {
    emit_jcc(instr, [this, &instr](){
        emit(mcode::Instruction(X8664Opcode::UCOMISS, {
            lower_value(instr.get_operand(0)),
            lower_value(instr.get_operand(2))
        }));
    });
}

void X8664IRLowerer::lower_select(ir::Instruction &instr) {
    ir::VirtualRegister dst = *instr.get_dest();
    ir::Operand &cmp_lhs = instr.get_operand(0);
    ir::Comparison cmp = instr.get_operand(1).get_comparison();
    ir::Operand &cmp_rhs = instr.get_operand(2);
    ir::Operand &val_true = instr.get_operand(3);
    ir::Operand &val_false = instr.get_operand(4);

    const ir::Type &type = instr.get_operand(0).get_type();
    unsigned size = std::max(get_size(type), 4u);

    mcode::Operand dst_op = mcode::Operand::from_register(lower_reg(dst), size);

    if (cmp_lhs.get_type().is_floating_point()) {
        unsigned mov_opcode = size == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;

        if (cmp_lhs == val_true && cmp_rhs == val_false) {
            if (cmp == ir::Comparison::FGT) {
                unsigned opcode = size == 4 ? X8664Opcode::MAXSS : X8664Opcode::MAXSD;
                emit(mcode::Instruction(mov_opcode, {dst_op, lower_value(cmp_lhs)}));
                emit(mcode::Instruction(opcode, {dst_op, lower_value(cmp_rhs)}));
                return;
            } else if (cmp == ir::Comparison::FLT) {
                unsigned opcode = size == 4 ? X8664Opcode::MINSS : X8664Opcode::MINSD;
                emit(mcode::Instruction(mov_opcode, {dst_op, lower_value(cmp_lhs)}));
                emit(mcode::Instruction(opcode, {dst_op, lower_value(cmp_rhs)}));
                return;
            }
        }
        
        std::cout << "ERROR: can't handle this floating point select" << std::endl;
        return;
    }

    mcode::Operand tmp_op = mcode::Operand::from_register(create_tmp_reg(), size);

    emit(mcode::Instruction(X8664Opcode::CMP, {lower_value(cmp_lhs), lower_value(cmp_rhs)}));
    emit(mcode::Instruction(X8664Opcode::MOV, {tmp_op, lower_value(val_true)}));
    emit(mcode::Instruction(X8664Opcode::MOV, {dst_op, lower_value(val_false)}));
    emit(mcode::Instruction(get_cmovcc_opcode(cmp), {dst_op, tmp_op}));
}

void X8664IRLowerer::lower_call(ir::Instruction& instr) {
    get_machine_func()->get_calling_conv()->lower_call(*this, instr);
}

void X8664IRLowerer::lower_ret(ir::Instruction& instr) {
    if(!instr.get_operands().empty()) {
        ir::Type type = instr.get_operand(0).get_type();

        mcode::Opcode opcode = get_move_opcode(type);
        long dest_reg = type.is_floating_point() ? X8664Register::XMM0 : X8664Register::RAX;

        emit(mcode::Instruction(opcode, {
            mcode::Operand::from_register(mcode::Register::from_physical(dest_reg), get_size(type)),
            lower_value(instr.get_operands()[0])
        }));
    }

    emit(mcode::Instruction(X8664Opcode::RET));
}

void X8664IRLowerer::lower_uextend(ir::Instruction& instr) {
    mcode::Operand src = lower_value(instr.get_operand(0));
    mcode::Operand dst = mcode::Operand::from_register(lower_reg(*instr.get_dest()), 4);
    mcode::Opcode opcode = src.get_size() < 4 ? X8664Opcode::MOVZX : X8664Opcode::MOV;
    emit(mcode::Instruction(opcode, {dst, src}));
}

void X8664IRLowerer::lower_sextend(ir::Instruction& instr) {
    mcode::Operand src = lower_value(instr.get_operand(0));
    mcode::Operand dst = mcode::Operand::from_register(lower_reg(*instr.get_dest()), 8);
    emit(mcode::Instruction(X8664Opcode::MOVSX, {dst, src}));
}

void X8664IRLowerer::lower_truncate(ir::Instruction& instr) {
    assert(get_size(instr.get_operand(1).get_type()) != 8);

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest()), 4),
        lower_value(instr.get_operand(0))
    }));
}

void X8664IRLowerer::lower_fpromote(ir::Instruction& instr) {
    emit(mcode::Instruction(X8664Opcode::CVTSS2SD, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest())),
        lower_value(instr.get_operand(0))
    }));
}

void X8664IRLowerer::lower_fdemote(ir::Instruction& instr) {
    emit(mcode::Instruction(X8664Opcode::CVTSD2SS, {
        mcode::Operand::from_register(lower_reg(*instr.get_dest())),
        lower_value(instr.get_operand(0))
    }));
}

void X8664IRLowerer::lower_utof(ir::Instruction& instr) {
    // TODO
    lower_stof(instr);
}

void X8664IRLowerer::lower_stof(ir::Instruction& instr) {
    ir::Primitive type = instr.get_operand(1).get_type().get_primitive();
    mcode::Opcode opcode = type == ir::Primitive::F64 ? X8664Opcode::CVTSI2SD : X8664Opcode::CVTSI2SS;
    mcode::Operand reg_operand = mcode::Operand::from_register(lower_reg(*instr.get_dest()));

    emit(mcode::Instruction(X8664Opcode::XORPS, {reg_operand, reg_operand}));
    emit(mcode::Instruction(opcode, {reg_operand, lower_value(instr.get_operand(0))}));
}

void X8664IRLowerer::lower_ftou(ir::Instruction& instr) {
    // TODO
    lower_ftos(instr);
}

void X8664IRLowerer::lower_ftos(ir::Instruction& instr) {
    bool is_double = instr.get_operand(0).get_type().is_primitive(ir::Primitive::F64);
    mcode::Opcode opcode = is_double ? X8664Opcode::CVTSD2SI : X8664Opcode::CVTSS2SI;
    
    mcode::Operand src_operand = lower_value(instr.get_operand(0));
    
    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    dst_size = dst_size == 8 ? 8 : 4;
    mcode::Operand dst_operand = mcode::Operand::from_register(lower_reg(*instr.get_dest()), dst_size);

    emit(mcode::Instruction(opcode, {dst_operand, src_operand}));
}

void X8664IRLowerer::lower_offsetptr(ir::Instruction& instr) {
    // TODO: GLOBAL VARIABLE HACK
    if (!instr.get_operand(0).is_register()) {
        emit(mcode::Instruction(X8664Opcode::MOV, {
            mcode::Operand::from_register(lower_reg(*instr.get_dest()), PTR_SIZE),
            lower_value(instr.get_operand(0))
        }));
        return;
    }

    mcode::Register dest = lower_reg(instr.get_dest().value());
    mcode::IndirectAddress addr = addr_lowering.calc_offsetptr_addr(instr);

    emit(mcode::Instruction(X8664Opcode::LEA, {
        mcode::Operand::from_register(dest, PTR_SIZE),
        mcode::Operand::from_addr(addr)
    }));
}

void X8664IRLowerer::lower_memberptr(ir::Instruction& instr) {
    mcode::Register dest = lower_reg(instr.get_dest().value());
    mcode::IndirectAddress addr = addr_lowering.calc_memberptr_addr(instr);

    emit(mcode::Instruction(X8664Opcode::LEA, {
        mcode::Operand::from_register(dest, PTR_SIZE),
        mcode::Operand::from_addr(addr)
    }));
}

void X8664IRLowerer::lower_copy(ir::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(2).get_type());
    
    if(size <= 64) {
        copy_block_using_movs(instr, size);
        return;
    }

    ir::Instruction call_instr(ir::Opcode::CALL, {
        ir::Operand::from_extern_func("memcpy", ir::Primitive::VOID),
        instr.get_operand(0),
        instr.get_operand(1),
        ir::Operand::from_int_immediate(size, ir::Primitive::I64)
    });

    lower_call(call_instr);
}

void X8664IRLowerer::lower_sqrt(ir::Instruction &instr) {
    mcode::Operand m_src = lower_value(instr.get_operand(0));
    mcode::Operand m_dst = mcode::Operand::from_register(lower_reg(*instr.get_dest()), 0);
    unsigned size = get_size(instr.get_operand(0).get_type());

    emit(mcode::Instruction(X8664Opcode::XORPS, {m_dst, m_dst}));
    emit(mcode::Instruction(size == 4 ? X8664Opcode::SQRTSS : X8664Opcode::SQRTSD, {m_dst, m_src}));
}

mcode::Operand X8664IRLowerer::into_reg_or_addr(ir::Operand &operand) {
    if (operand.is_register()) {
        ir::InstrIter producer = get_producer(operand.get_register());

        if (producer->get_opcode() == ir::Opcode::LOAD && get_num_uses(*producer->get_dest()) == 1) {
            discard_use(*producer->get_dest());
            return lower_address(producer->get_operand(1));
        }
    }

    return lower_value(operand);
}

mcode::InstrIter X8664IRLowerer::move_int_into_reg(mcode::Register reg, mcode::Value &value) {
    unsigned size = value.get_size();
    mcode::Operand reg_operand = mcode::Operand::from_register(reg, size);

    if (value.is_immediate()) {
        const std::string &imm = value.get_immediate();

        if (imm == "0" && !reg.is_stack_slot()) {
            reg_operand.set_size(4);
            return emit(mcode::Instruction(X8664Opcode::XOR, {reg_operand, reg_operand}));
        } else {
            mcode::Operand imm_operand = mcode::Operand::from_immediate(imm, size);
            return emit(mcode::Instruction(X8664Opcode::MOV, {reg_operand, imm_operand}));
        }
    } else if (value.is_stack_slot() || value.is_symbol_deref() || value.is_addr()) {
        return emit(mcode::Instruction(X8664Opcode::LEA, {reg_operand, value}));
    } else {
        return emit(mcode::Instruction(X8664Opcode::MOV, {reg_operand, value}));
    }
}

mcode::InstrIter X8664IRLowerer::move_float_into_reg(mcode::Register reg, mcode::Value &value) {    
    unsigned size = value.get_size();
    mcode::Operand dst = mcode::Operand::from_register(reg, size);
    mcode::Opcode opcode = size == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;
    return emit(mcode::Instruction(opcode, {dst, value}));
}

mcode::Operand X8664IRLowerer::read_symbol_addr(const mcode::Symbol &symbol) {
    if (target->get_code_model() == CodeModel::SMALL) {
        return mcode::Operand::from_symbol(symbol, 8);
    } else if (target->get_code_model() == CodeModel::LARGE) {
        mcode::Register addr_reg = lower_reg(get_func().next_virtual_reg());
        mcode::Operand dst = mcode::Operand::from_register(addr_reg, 8);
        mcode::Operand src = mcode::Operand::from_symbol(symbol, 8);
        emit(mcode::Instruction(X8664Opcode::MOV, {dst, src}));
        return dst;
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Operand X8664IRLowerer::deref_symbol_addr(const mcode::Symbol &symbol, unsigned size) {
if (target->get_code_model() == CodeModel::SMALL) {
        return mcode::Operand::from_symbol_deref(symbol, 8);
    } else if (target->get_code_model() == CodeModel::LARGE) {
        mcode::Register addr_reg = lower_reg(get_func().next_virtual_reg());
        mcode::Operand dst = mcode::Operand::from_register(addr_reg, 8);
        mcode::Operand src = mcode::Operand::from_symbol(symbol, 8);
        emit(mcode::Instruction(X8664Opcode::MOV, {dst, src}));
        return mcode::Operand::from_addr(mcode::IndirectAddress(addr_reg), size);
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Opcode X8664IRLowerer::get_move_opcode(ir::Type type) {
    if(type.is_primitive(ir::Primitive::F32)) return X8664Opcode::MOVSS;
    else if(type.is_primitive(ir::Primitive::F64)) return X8664Opcode::MOVSD;
    else return X8664Opcode::MOV;
}

mcode::CallingConvention* X8664IRLowerer::get_calling_convention(ir::CallingConv calling_conv) {
    switch(calling_conv) {
        case ir::CallingConv::X86_64_SYS_V_ABI: return (mcode::CallingConvention*) &SysVCallingConv::INSTANCE;
        case ir::CallingConv::X86_64_MS_ABI: return (mcode::CallingConvention*) &MSABICallingConv::INSTANCE;
        default: return nullptr;
    }
}

void X8664IRLowerer::copy_block_using_movs(ir::Instruction& instr, unsigned size) {
    unsigned offset = 0;

    for (unsigned mov_size = 8; mov_size != 0; mov_size /= 2) {
        while (size >= mov_size) {
            mcode::IndirectAddress dst_addr(lower_reg(instr.get_operand(0).get_register()), offset, 1);
            mcode::IndirectAddress src_addr(lower_reg(instr.get_operand(1).get_register()), offset, 1);
            mcode::Register tmp_reg = lower_reg(get_func().next_virtual_reg());

            mcode::Operand dst_val = mcode::Operand::from_addr(dst_addr, mov_size);
            mcode::Operand src_val = mcode::Operand::from_addr(src_addr, mov_size);
            mcode::Operand tmp_val = mcode::Operand::from_register(tmp_reg, mov_size);

            emit(mcode::Instruction(X8664Opcode::MOV, {tmp_val, src_val}));
            emit(mcode::Instruction(X8664Opcode::MOV, {dst_val, tmp_val}));

            size -= mov_size;
            offset += mov_size;
        }
    }

    assert(size == 0);
}

void X8664IRLowerer::emit_jcc(ir::Instruction &instr, const std::function<void()> &comparison_emitter) {
    ir::Comparison comparison = instr.get_operand(1).get_comparison();
    ir::BranchTarget &true_target = instr.get_operand(3).get_branch_target();
    ir::BranchTarget &false_target = instr.get_operand(4).get_branch_target();

    ir::BasicBlockIter true_block_iter = true_target.block;
    ir::BasicBlockIter false_block_iter = false_target.block;

    if (true_block_iter == get_basic_block_iter().get_next()) {
        move_branch_args(false_target);
        
        comparison_emitter();
        emit(mcode::Instruction(get_jcc_opcode(ir::invert_comparison(comparison)), {
            mcode::Operand::from_label(false_block_iter->get_label())
        }));

        move_branch_args(true_target);
    } else {
        move_branch_args(true_target);

        comparison_emitter();
        emit(mcode::Instruction(get_jcc_opcode(comparison), {
            mcode::Operand::from_label(true_block_iter->get_label())
        }));

        move_branch_args(false_target);

        if (false_block_iter != get_basic_block_iter().get_next()) {
            emit(mcode::Instruction(X8664Opcode::JMP, {
                mcode::Operand::from_label(false_block_iter->get_label())
            }));
        }
    }
}

mcode::Opcode X8664IRLowerer::get_jcc_opcode(ir::Comparison comparison) {
    return X8664Opcode::JCC + get_condition_code(comparison);
}

mcode::Opcode X8664IRLowerer::get_cmovcc_opcode(ir::Comparison comparison) {
    return X8664Opcode::CMOVCC + get_condition_code(comparison);
}

unsigned X8664IRLowerer::get_condition_code(ir::Comparison comparison) {
    switch(comparison) {
        case ir::Comparison::EQ: return X8664ConditionCode::E;
        case ir::Comparison::NE: return X8664ConditionCode::NE;
        case ir::Comparison::UGT: return X8664ConditionCode::A;
        case ir::Comparison::UGE: return X8664ConditionCode::AE;
        case ir::Comparison::ULT: return X8664ConditionCode::B;
        case ir::Comparison::ULE: return X8664ConditionCode::BE;
        case ir::Comparison::SGT: return X8664ConditionCode::G;
        case ir::Comparison::SGE: return X8664ConditionCode::GE;
        case ir::Comparison::SLT: return X8664ConditionCode::L;
        case ir::Comparison::SLE: return X8664ConditionCode::LE;
        case ir::Comparison::FEQ: return X8664ConditionCode::E;
        case ir::Comparison::FNE: return X8664ConditionCode::NE;
        case ir::Comparison::FGT: return X8664ConditionCode::A;
        case ir::Comparison::FGE: return X8664ConditionCode::AE;
        case ir::Comparison::FLT: return X8664ConditionCode::B;
        case ir::Comparison::FLE: return X8664ConditionCode::BE;
    }
}

mcode::Operand X8664IRLowerer::get_float_addr(double value, const ir::Type &type) {
    bool is_64_bit = type.get_primitive() == ir::Primitive::F64;
    return is_64_bit ? const_lowering.load_f64((double)value) : const_lowering.load_f32((float)value);
}

void X8664IRLowerer::move_branch_args(ir::BranchTarget &target) {
    for (unsigned i = 0; i < target.args.size(); i++) {
        ir::Value &arg = target.args[i];
        ir::VirtualRegister param_reg = target.block->get_param_regs()[i];

        mcode::Opcode move_opcode;
        if (arg.get_type().is_primitive(ir::Primitive::F32)) move_opcode = X8664Opcode::MOVSS;
        else if (arg.get_type().is_primitive(ir::Primitive::F64)) move_opcode = X8664Opcode::MOVSD;
        else move_opcode = X8664Opcode::MOV;

        unsigned size = get_size(arg.get_type());
        mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_virtual(param_reg), size);

        if (arg.is_int_immediate() && arg.get_int_immediate() == 0) {
            emit(mcode::Instruction(X8664Opcode::XOR, {dst, dst}, mcode::Instruction::FLAG_CALL_ARG));
        } else if (arg.is_fp_immediate() && arg.get_fp_immediate() == 0.0) {
            emit(mcode::Instruction(X8664Opcode::XORPS, {dst, dst}, mcode::Instruction::FLAG_CALL_ARG));
        } else {
            mcode::Operand src = lower_value(arg);
            if (src != dst) {
                emit(mcode::Instruction(move_opcode, {dst, src}, mcode::Instruction::FLAG_CALL_ARG));
            }
        }
    }
}

void X8664IRLowerer::emit_shift(ir::Instruction &instr, mcode::Opcode opcode) {
    int size = get_size(instr.get_operand(0).get_type());

    mcode::Register tmp_reg = lower_reg(get_func().next_virtual_reg());
    mcode::Operand op0 = mcode::Operand::from_register(tmp_reg, size);
    emit(mcode::Instruction(X8664Opcode::MOV, {op0, lower_value(instr.get_operand(0))}));

    mcode::Operand op1;

    if (instr.get_operand(1).is_int_immediate()) {
        op1 = mcode::Operand::from_immediate(instr.get_operand(1).get_int_immediate().to_string(), 1);
    } else {
        mcode::Register rcx = mcode::Register::from_physical(X8664Register::RCX);
        mcode::Operand rcx8 = mcode::Operand::from_register(rcx, 8);
        emit(mcode::Instruction(X8664Opcode::MOV, {rcx8, lower_value(instr.get_operand(1)).with_size(8)}));
        op1 = mcode::Operand::from_register(rcx, 1);
    }

    emit(mcode::Instruction(opcode, {op0, op1}));

    mcode::Operand dst = mcode::Operand::from_register(lower_reg(*instr.get_dest()), size);
    emit(mcode::Instruction(X8664Opcode::MOV, {dst, op0}));
}

// clang-format on

} // namespace target

} // namespace banjo
