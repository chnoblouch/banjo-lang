#include "x86_64_ssa_lowerer.hpp"

#include "banjo/config/config.hpp"
#include "banjo/mcode/calling_convention.hpp"
#include "banjo/mcode/global.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"
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

X8664SSALowerer::X8664SSALowerer(Target *target) : SSALowerer(target), addr_lowering(*this), const_lowering(*this) {}

void X8664SSALowerer::init_module(ssa::Module &mod) {
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

mcode::Operand X8664SSALowerer::lower_address(const ssa::Operand& operand) {
    return addr_lowering.lower_address(operand); 
}

void X8664SSALowerer::append_mov_and_operation(
    mcode::Opcode machine_opcode,
    ssa::VirtualRegister dst,
    ssa::Value &lhs,
    ssa::Value &rhs
) {
    unsigned size = get_size(lhs.get_type());
    mcode::Operand m_dst = map_vreg_as_operand(dst, size);

    lower_as_move(m_dst, lhs);
    mcode::Operand m_rhs = lower_as_operand(rhs, {.allow_addrs = !m_dst.is_stack_slot()});
    emit(mcode::Instruction(machine_opcode, {m_dst, m_rhs}));
}

bool X8664SSALowerer::lower_stored_operation(ssa::Instruction& store) {
    const ssa::Operand result = store.get_operand(0);
    if(!result.is_register()) {
        return false;
    }

    ssa::InstrIter operation_iter = get_producer(result.get_register());
    if(operation_iter == get_block().end()) {
        return false;
    }

    // int size = get_size(operation_iter->get_operand(0).get_type());

    mcode::Opcode machine_opcode;
    switch(operation_iter->get_opcode()) {
        // case ssa::Opcode::ADD: machine_opcode = X8664Opcode::ADD; break;
        // case ssa::Opcode::SUB: machine_opcode = X8664Opcode::SUB; break;
        //case ssa::Opcode::MUL: machine_opcode = X8664Opcode::IMUL; break;
        //case ssa::Opcode::FADD: machine_opcode = size == 8 ? X8664Opcode::ADDSD : X8664Opcode::ADDSS; break;
        //case ssa::Opcode::FSUB: machine_opcode = size == 8 ? X8664Opcode::SUBSD : X8664Opcode::SUBSS; break;
        //case ssa::Opcode::FMUL: machine_opcode = size == 8 ? X8664Opcode::MULSD : X8664Opcode::MULSS; break;
        //case ssa::Opcode::FDIV: machine_opcode = size == 8 ? X8664Opcode::DIVSD : X8664Opcode::DIVSS; break;
        default: return false;
    }
    
    const ssa::Operand& lhs = operation_iter->get_operand(0);
    const ssa::Operand& rhs = operation_iter->get_operand(1);

    if(lhs.is_register() && rhs.is_register()) {
        ssa::InstrIter lhs_producer_iter = get_producer(lhs.get_register());
        ssa::InstrIter rhs_producer_iter = get_producer(lhs.get_register());
        if(lhs_producer_iter == get_block().end() || rhs_producer_iter == get_block().end()) {
            return false;
        }

        ssa::Instruction& lhs_producer = *lhs_producer_iter;
        ssa::Instruction& rhs_producer = *lhs_producer_iter;

        if(lhs_producer.get_opcode() == ssa::Opcode::LOAD && rhs_producer.get_opcode() == ssa::Opcode::LOAD && lhs_producer.get_operand(1) == store.get_operand(1)) {
            discard_use(*operation_iter->get_dest());
            discard_use(*lhs_producer.get_dest());

            mcode::Operand dest = lower_address(store.get_operand(1));
            if(dest.is_register()) {
                dest = mcode::Operand::from_addr({dest.get_register()});
            }
            dest.set_size(get_size(store.get_operand(0).get_type()));
            emit(mcode::Instruction(machine_opcode, {
                dest,
                map_vreg_as_operand(*rhs_producer.get_dest(), dest.get_size())
            }));
            
            return true;
        }
    } else if(lhs.is_register() && rhs.is_immediate()) {
        ssa::InstrIter lhs_producer_iter = get_producer(lhs.get_register());
        if(lhs_producer_iter == get_block().end()) {
            return false;
        }

        ssa::Instruction& lhs_producer = *lhs_producer_iter;

        if(lhs_producer.get_opcode() == ssa::Opcode::LOAD && lhs_producer.get_operand(1) == store.get_operand(1)) {
            discard_use(*operation_iter->get_dest());
            discard_use(*lhs_producer.get_dest());
            discard_use(store.get_operand(1).get_register());

            mcode::Operand dst = lower_address(store.get_operand(1));
            if(dst.is_register()) {
                dst = mcode::Operand::from_addr({dst.get_register()});
            }
            dst.set_size(get_size(store.get_operand(0).get_type()));
            emit(mcode::Instruction(machine_opcode, {dst, lower_as_operand(rhs)}));
            
            return true;
        }
    }

    return false;
}

void X8664SSALowerer::lower_load(ssa::Instruction& instr) {
    ssa::Type type = instr.get_operand(0).get_type();
    unsigned size = get_size(type);

    mcode::Opcode opcode = get_move_opcode(type);
    mcode::Operand m_dst = map_vreg_as_operand(*instr.get_dest(), size);
    mcode::Operand m_src = lower_address(instr.get_operand(1)).with_size(size);
    emit(mcode::Instruction(opcode, {m_dst, m_src}));
} 

void X8664SSALowerer::lower_store(ssa::Instruction& instr) {
    if(lower_stored_operation(instr)) {
        return;
    }

    mcode::Operand dst = lower_address(instr.get_operand(1));
    ssa::Type type = instr.get_operand(0).get_type();
    mcode::Instruction m_instr;

    if (instr.get_operand(0).is_immediate() && type.is_primitive(ssa::Primitive::F32) && (dst.is_addr() || dst.is_stack_slot())) {
        float val = (float)instr.get_operand(0).get_fp_immediate();
        mcode::Operand src = mcode::Operand::from_int_immediate(BitOperations::get_bits_32(val), 4);
        dst.set_size(src.get_size());
        m_instr = mcode::Instruction(X8664Opcode::MOV, {dst, src});
    } else {
        mcode::Operand src = lower_as_operand(instr.get_operand(0));
        dst.set_size(src.get_size());

        if(src.is_symbol()) {
            mcode::Register tmp_reg = create_reg();
            mcode::Operand tmp_val = mcode::Operand::from_register(tmp_reg, src.get_size());
            emit(mcode::Instruction(X8664Opcode::MOV, {tmp_val, src}));
            src = tmp_val;
        }

        mcode::Opcode opcode = get_move_opcode(type);
        m_instr = mcode::Instruction(opcode, {dst, src});
    }

    if(instr.is_flag(ssa::Instruction::FLAG_SAVE_ARG)) {
        m_instr.set_flag(mcode::Instruction::FLAG_ARG_STORE);
    }

    emit(m_instr);
}

void X8664SSALowerer::lower_loadarg(ssa::Instruction& instr) {
    ssa::Type type = instr.get_operand(0).get_type();
    unsigned param_index = instr.get_operand(1).get_int_immediate().to_u64();
    unsigned size = get_size(type);

    mcode::CallingConvention* calling_conv = get_machine_func()->get_calling_conv();
    std::vector<mcode::ArgStorage> arg_storage = calling_conv->get_arg_storage(get_func().type.params);
    mcode::ArgStorage cur_arg_storage = arg_storage[param_index];

    mcode::Operand m_src;

    if (cur_arg_storage.in_reg) {
        m_src = mcode::Operand::from_register(mcode::Register::from_physical(cur_arg_storage.reg), size);
    } else {
        mcode::Parameter &param = get_machine_func()->get_parameters()[param_index];
        mcode::StackSlotID slot_index = std::get<mcode::StackSlotID>(param.storage);
        m_src = mcode::Operand::from_stack_slot(slot_index, size);
    }

    mcode::Opcode opcode = get_move_opcode(type);
    mcode::Operand m_dst = map_vreg_dst(instr, size);

    mcode::Instruction m_instr(opcode, {m_dst, m_src});
    m_instr.set_flag(mcode::Instruction::FLAG_ARG_STORE);
    emit(m_instr);
}

void X8664SSALowerer::lower_add(ssa::Instruction& instr) {
    ssa::Operand lhs = instr.get_operand(0);
    ssa::Operand rhs = instr.get_operand(1);
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

void X8664SSALowerer::lower_sub(ssa::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::SUB, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_mul(ssa::Instruction& instr) {
    ssa::Operand& lhs = instr.get_operand(0);
    ssa::Operand& rhs = instr.get_operand(1);

    if (lhs.is_register() && rhs.is_immediate()) {
        unsigned size = get_size(lhs.get_type());

        if ((size == 4 || size == 8) && rhs.get_int_immediate() == 3) {
            mcode::Register lhs_reg = map_vreg_as_reg(lhs.get_register());
            mcode::IndirectAddress addr(lhs_reg, lhs_reg, 2);
            
            mcode::Operand m_dst = map_vreg_dst(instr, size);
            mcode::Operand m_src = mcode::Operand::from_addr(addr, size);
            emit(mcode::Instruction(X8664Opcode::LEA, {m_dst, m_src}));

            return;
        }
    }

    append_mov_and_operation(X8664Opcode::IMUL, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_sdiv(ssa::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());
    mcode::Register divisor_reg = mcode::Register::from_physical(X8664Register::RCX);

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RAX), size),
        lower_as_operand(instr.get_operand(0)),
    }));
    
    if(size == 4) {
        emit(mcode::Instruction(X8664Opcode::CDQ));
    } else if(size == 8) {
        emit(mcode::Instruction(X8664Opcode::CQO));
    }

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(divisor_reg, size),
        lower_as_operand(instr.get_operand(1)),
    }));
    emit(mcode::Instruction(X8664Opcode::IDIV, {
        mcode::Operand::from_register(divisor_reg, size)
    }));
    emit(mcode::Instruction(X8664Opcode::MOV, {
        map_vreg_dst(instr, size),
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RAX), size)
    }));
}

void X8664SSALowerer::lower_srem(ssa::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(0).get_type());
    mcode::Register diviser_reg = create_reg();

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RAX), size),
        lower_as_operand(instr.get_operand(0)),
    }));
    
    if(size == 4) {
        emit(mcode::Instruction(X8664Opcode::CDQ));
    } else if(size == 8) {
        emit(mcode::Instruction(X8664Opcode::CQO));
    }

    emit(mcode::Instruction(X8664Opcode::MOV, {
        mcode::Operand::from_register(diviser_reg, size),
        lower_as_operand(instr.get_operand(1)),
    }));
    emit(mcode::Instruction(X8664Opcode::IDIV, {
        mcode::Operand::from_register(diviser_reg, size)
    }));
    emit(mcode::Instruction(X8664Opcode::MOV, {
        map_vreg_dst(instr, size),
        mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RDX), size)
    }));
}

void X8664SSALowerer::lower_udiv(ssa::Instruction &instr) {
    // FIXME: yes, this is obviously wrong
    lower_sdiv(instr);
}

void X8664SSALowerer::lower_urem(ssa::Instruction &instr) {
    // FIXME: yes, this is obviously wrong
    lower_srem(instr);
}

void X8664SSALowerer::lower_fadd(ssa::Instruction& instr) {
    ssa::Primitive type = instr.get_operand(0).get_type().get_primitive();
    mcode::Opcode opcode = type == ssa::Primitive::F64 ? X8664Opcode::ADDSD : X8664Opcode::ADDSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_fsub(ssa::Instruction& instr) {
    ssa::Primitive type = instr.get_operand(0).get_type().get_primitive();
    
    if (type == ssa::Primitive::F32 && instr.get_operand(0).is_fp_immediate() && instr.get_operand(0).get_fp_immediate() == 0.0) {
        if (!const_neg_zero) {
            const_neg_zero = {"const.neg_zero"};
        }

        mcode::Operand m_dst = map_vreg_dst(instr, 4);
        mcode::Operand m_src = lower_as_operand(instr.get_operand(1));
        mcode::Operand m_const_addr = deref_symbol_addr(*const_neg_zero, 16);

        emit(mcode::Instruction(get_move_opcode(type), {m_dst, m_src}));
        emit(mcode::Instruction(X8664Opcode::XORPS, {m_dst.with_size(16), m_const_addr}));

        return;
    }

    mcode::Opcode opcode = type == ssa::Primitive::F64 ? X8664Opcode::SUBSD : X8664Opcode::SUBSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_fmul(ssa::Instruction& instr) {
    ssa::Primitive type = instr.get_operand(0).get_type().get_primitive();
    mcode::Opcode opcode = type == ssa::Primitive::F64 ? X8664Opcode::MULSD : X8664Opcode::MULSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_fdiv(ssa::Instruction& instr) {
    ssa::Primitive type = instr.get_operand(0).get_type().get_primitive();
    mcode::Opcode opcode = type == ssa::Primitive::F64 ? X8664Opcode::DIVSD : X8664Opcode::DIVSS;
    append_mov_and_operation(opcode, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_and(ssa::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::AND, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_or(ssa::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::OR, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_xor(ssa::Instruction& instr) {
    append_mov_and_operation(X8664Opcode::XOR, *instr.get_dest(), instr.get_operand(0), instr.get_operand(1));
}

void X8664SSALowerer::lower_shl(ssa::Instruction& instr) {
    emit_shift(instr, X8664Opcode::SHL);
}

void X8664SSALowerer::lower_shr(ssa::Instruction& instr) {
    emit_shift(instr, X8664Opcode::SHR);
}

void X8664SSALowerer::lower_jmp(ssa::Instruction& instr) {
    ssa::BranchTarget target = instr.get_operand(0).get_branch_target();
    ssa::BasicBlockIter block_iter = target.block;

    move_branch_args(target);

    if(block_iter != get_basic_block_iter().get_next()) {
        emit(mcode::Instruction(X8664Opcode::JMP, {
            mcode::Operand::from_label(block_iter->get_label())
        }));
    }
}

void X8664SSALowerer::lower_cjmp(ssa::Instruction& instr) {
    emit_jcc(instr, [this, &instr](){
        ssa::VirtualRegister reg = get_func().next_virtual_reg();
        append_mov_and_operation(X8664Opcode::CMP, reg, instr.get_operand(0), instr.get_operand(2));
    });
}

void X8664SSALowerer::lower_fcjmp(ssa::Instruction& instr) {
    emit_jcc(instr, [this, &instr](){
        emit(mcode::Instruction(X8664Opcode::UCOMISS, {
            lower_as_operand(instr.get_operand(0)),
            lower_as_operand(instr.get_operand(2))
        }));
    });
}

void X8664SSALowerer::lower_select(ssa::Instruction &instr) {
    ssa::VirtualRegister dst = *instr.get_dest();
    ssa::Operand &cmp_lhs = instr.get_operand(0);
    ssa::Comparison cmp = instr.get_operand(1).get_comparison();
    ssa::Operand &cmp_rhs = instr.get_operand(2);
    ssa::Operand &val_true = instr.get_operand(3);
    ssa::Operand &val_false = instr.get_operand(4);

    const ssa::Type &type = instr.get_operand(0).get_type();
    unsigned size = std::max(get_size(type), 4u);

    mcode::Operand m_dst = map_vreg_dst(instr, size);

    if (cmp_lhs.get_type().is_floating_point()) {
        unsigned mov_opcode = size == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;

        if (cmp_lhs == val_true && cmp_rhs == val_false) {
            if (cmp == ssa::Comparison::FGT) {
                unsigned opcode = size == 4 ? X8664Opcode::MAXSS : X8664Opcode::MAXSD;
                emit(mcode::Instruction(mov_opcode, {m_dst, lower_as_operand(cmp_lhs)}));
                emit(mcode::Instruction(opcode, {m_dst, lower_as_operand(cmp_rhs)}));
                return;
            } else if (cmp == ssa::Comparison::FLT) {
                unsigned opcode = size == 4 ? X8664Opcode::MINSS : X8664Opcode::MINSD;
                emit(mcode::Instruction(mov_opcode, {m_dst, lower_as_operand(cmp_lhs)}));
                emit(mcode::Instruction(opcode, {m_dst, lower_as_operand(cmp_rhs)}));
                return;
            }
        }
        
        std::cout << "ERROR: can't handle this floating point select" << std::endl;
        return;
    }

    mcode::Operand tmp_op = mcode::Operand::from_register(create_reg(), size);

    emit(mcode::Instruction(X8664Opcode::CMP, {lower_as_operand(cmp_lhs), lower_as_operand(cmp_rhs)}));
    emit(mcode::Instruction(X8664Opcode::MOV, {tmp_op, lower_as_operand(val_true)}));
    emit(mcode::Instruction(X8664Opcode::MOV, {m_dst, lower_as_operand(val_false)}));
    emit(mcode::Instruction(get_cmovcc_opcode(cmp), {m_dst, tmp_op}));
}

void X8664SSALowerer::lower_call(ssa::Instruction& instr) {
    get_machine_func()->get_calling_conv()->lower_call(*this, instr);
}

void X8664SSALowerer::lower_ret(ssa::Instruction& instr) {
    if(!instr.get_operands().empty()) {
        ssa::Type type = instr.get_operand(0).get_type();

        mcode::Opcode opcode = get_move_opcode(type);
        long dest_reg = type.is_floating_point() ? X8664Register::XMM0 : X8664Register::RAX;

        emit(mcode::Instruction(opcode, {
            mcode::Operand::from_register(mcode::Register::from_physical(dest_reg), get_size(type)),
            lower_as_operand(instr.get_operands()[0])
        }));
    }

    emit(mcode::Instruction(X8664Opcode::RET));
}

void X8664SSALowerer::lower_uextend(ssa::Instruction& instr) {
    mcode::Operand m_dst = map_vreg_dst(instr, 4);
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));
    mcode::Opcode opcode = m_src.get_size() < 4 ? X8664Opcode::MOVZX : X8664Opcode::MOV;
    emit(mcode::Instruction(opcode, {m_dst, m_src}));
}

void X8664SSALowerer::lower_sextend(ssa::Instruction& instr) {
    mcode::Operand m_dst = map_vreg_dst(instr, 8);
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));
    emit(mcode::Instruction(X8664Opcode::MOVSX, {m_dst, m_src}));
}

void X8664SSALowerer::lower_truncate(ssa::Instruction& instr) {
    ASSERT(get_size(instr.get_operand(1).get_type()) != 8);

    mcode::Operand m_dst = map_vreg_dst(instr, 4);
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));

    if (m_src.get_size() == 8) {
        m_src.set_size(4);
    }

    emit(mcode::Instruction(X8664Opcode::MOV, {m_dst, m_src}));
}

void X8664SSALowerer::lower_fpromote(ssa::Instruction& instr) {
    mcode::Operand m_dst = map_vreg_dst(instr, 8);
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));
    emit(mcode::Instruction(X8664Opcode::CVTSS2SD, {m_dst, m_src}));
}

void X8664SSALowerer::lower_fdemote(ssa::Instruction& instr) {
    mcode::Operand m_dst = map_vreg_dst(instr, 4);
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));
    emit(mcode::Instruction(X8664Opcode::CVTSD2SS, {m_dst, m_src}));
}

void X8664SSALowerer::lower_utof(ssa::Instruction& instr) {
    // TODO
    lower_stof(instr);
}

void X8664SSALowerer::lower_stof(ssa::Instruction& instr) {
    // FIXME: Extend smaller integer sizes

    ssa::Primitive type = instr.get_operand(1).get_type().get_primitive();

    mcode::Opcode opcode = type == ssa::Primitive::F64 ? X8664Opcode::CVTSI2SD : X8664Opcode::CVTSI2SS;
    mcode::Operand m_dst = map_vreg_dst(instr, type == ssa::Primitive::F64 ? 8 : 4);
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));

    emit(mcode::Instruction(X8664Opcode::XORPS, {m_dst, m_dst}));
    emit(mcode::Instruction(opcode, {m_dst, m_src}));
}

void X8664SSALowerer::lower_ftou(ssa::Instruction& instr) {
    // TODO
    lower_ftos(instr);
}

void X8664SSALowerer::lower_ftos(ssa::Instruction& instr) {    
    bool is_double = instr.get_operand(0).get_type().is_primitive(ssa::Primitive::F64);
    
    unsigned dst_size = get_size(instr.get_operand(1).get_type());
    dst_size = dst_size == 8 ? 8 : 4;
    
    mcode::Opcode opcode = is_double ? X8664Opcode::CVTSD2SI : X8664Opcode::CVTSS2SI;
    mcode::Operand m_dst = map_vreg_dst(instr, dst_size);
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));
    emit(mcode::Instruction(opcode, {m_dst, m_src}));
}

void X8664SSALowerer::lower_offsetptr(ssa::Instruction& instr) {
    // TODO: GLOBAL VARIABLE HACK
    if (!instr.get_operand(0).is_register()) {
        mcode::Operand m_dst = map_vreg_dst(instr, 8);
        mcode::Operand m_src = lower_as_operand(instr.get_operand(0));
        emit(mcode::Instruction(X8664Opcode::MOV, {m_dst, m_src}));
        return;
    }

    mcode::Operand m_dst = map_vreg_dst(instr, 8);
    mcode::Operand m_src = mcode::Operand::from_addr(addr_lowering.calc_offsetptr_addr(instr));
    emit(mcode::Instruction(X8664Opcode::LEA, {m_dst, m_src}));
}

void X8664SSALowerer::lower_memberptr(ssa::Instruction& instr) {
    mcode::Operand m_dst = map_vreg_dst(instr, 8);
    mcode::Operand m_src = mcode::Operand::from_addr(addr_lowering.calc_memberptr_addr(instr));
    emit(mcode::Instruction(X8664Opcode::LEA, {m_dst, m_src}));
}

void X8664SSALowerer::lower_copy(ssa::Instruction& instr) {
    unsigned size = get_size(instr.get_operand(2).get_type());
    
    if(size <= 64) {
        copy_block_using_movs(instr, size);
        return;
    }

    ssa::Instruction call_instr(ssa::Opcode::CALL, {
        ssa::Operand::from_extern_func(memcpy_func, ssa::Primitive::VOID),
        instr.get_operand(0),
        instr.get_operand(1),
        ssa::Operand::from_int_immediate(size, ssa::Primitive::I64)
    });

    lower_call(call_instr);
}

void X8664SSALowerer::lower_sqrt(ssa::Instruction &instr) {
    mcode::Operand m_src = lower_as_operand(instr.get_operand(0));
    mcode::Operand m_dst = map_vreg_dst(instr, m_src.get_size());
    mcode::Opcode opcode = m_src.get_size() == 4 ? X8664Opcode::SQRTSS : X8664Opcode::SQRTSD;

    emit(mcode::Instruction(X8664Opcode::XORPS, {m_dst, m_dst}));
    emit(mcode::Instruction(opcode, {m_dst, m_src}));
}

mcode::Operand X8664SSALowerer::into_reg_or_addr(ssa::Operand &operand) {
    if (operand.is_register()) {
        ssa::InstrIter producer = get_producer(operand.get_register());

        if (producer->get_opcode() == ssa::Opcode::LOAD && get_num_uses(*producer->get_dest()) == 1) {
            discard_use(*producer->get_dest());
            return lower_address(producer->get_operand(1));
        }
    }

    return lower_as_operand(operand);
}

mcode::Operand X8664SSALowerer::read_symbol_addr(const mcode::Symbol &symbol) {
    if (target->get_code_model() == CodeModel::SMALL) {
        return mcode::Operand::from_symbol(symbol, 8);
    } else if (target->get_code_model() == CodeModel::LARGE) {
        mcode::Register addr_reg = create_reg();
        mcode::Operand dst = mcode::Operand::from_register(addr_reg, 8);
        mcode::Operand src = mcode::Operand::from_symbol(symbol, 8);
        emit(mcode::Instruction(X8664Opcode::MOV, {dst, src}));
        return dst;
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Operand X8664SSALowerer::deref_symbol_addr(const mcode::Symbol &symbol, unsigned size) {
    if (target->get_code_model() == CodeModel::SMALL) {
        return mcode::Operand::from_symbol_deref(symbol, 8);
    } else if (target->get_code_model() == CodeModel::LARGE) {
        mcode::Register addr_reg = create_reg();
        mcode::Operand dst = mcode::Operand::from_register(addr_reg, 8);
        mcode::Operand src = mcode::Operand::from_symbol(symbol, 8);
        emit(mcode::Instruction(X8664Opcode::MOV, {dst, src}));
        return mcode::Operand::from_addr(mcode::IndirectAddress(addr_reg), size);
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Opcode X8664SSALowerer::get_move_opcode(ssa::Type type) {
    if(type.is_primitive(ssa::Primitive::F32)) return X8664Opcode::MOVSS;
    else if(type.is_primitive(ssa::Primitive::F64)) return X8664Opcode::MOVSD;
    else return X8664Opcode::MOV;
}

mcode::CallingConvention* X8664SSALowerer::get_calling_convention(ssa::CallingConv calling_conv) {
    switch(calling_conv) {
        case ssa::CallingConv::X86_64_SYS_V_ABI: return (mcode::CallingConvention*) &SysVCallingConv::INSTANCE;
        case ssa::CallingConv::X86_64_MS_ABI: return (mcode::CallingConvention*) &MSABICallingConv::INSTANCE;
        default: return nullptr;
    }
}

void X8664SSALowerer::copy_block_using_movs(ssa::Instruction& instr, unsigned size) {
    unsigned offset = 0;

    std::variant<mcode::Register, mcode::StackSlotID> dst_base = map_vreg(instr.get_operand(0).get_register());
    std::variant<mcode::Register, mcode::StackSlotID> src_base = map_vreg(instr.get_operand(1).get_register());

    for (unsigned mov_size = 8; mov_size != 0; mov_size /= 2) {
        while (size >= mov_size) {
            mcode::IndirectAddress dst_addr(dst_base, offset, 1);
            mcode::IndirectAddress src_addr(src_base, offset, 1);
            mcode::Register tmp_reg = create_reg();

            mcode::Operand dst_val = mcode::Operand::from_addr(dst_addr, mov_size);
            mcode::Operand src_val = mcode::Operand::from_addr(src_addr, mov_size);
            mcode::Operand tmp_val = mcode::Operand::from_register(tmp_reg, mov_size);

            emit(mcode::Instruction(X8664Opcode::MOV, {tmp_val, src_val}));
            emit(mcode::Instruction(X8664Opcode::MOV, {dst_val, tmp_val}));

            size -= mov_size;
            offset += mov_size;
        }
    }

    ASSERT(size == 0);
}

void X8664SSALowerer::emit_jcc(ssa::Instruction &instr, const std::function<void()> &comparison_emitter) {
    ssa::Comparison comparison = instr.get_operand(1).get_comparison();
    ssa::BranchTarget &true_target = instr.get_operand(3).get_branch_target();
    ssa::BranchTarget &false_target = instr.get_operand(4).get_branch_target();

    ssa::BasicBlockIter true_block_iter = true_target.block;
    ssa::BasicBlockIter false_block_iter = false_target.block;

    if (true_block_iter == get_basic_block_iter().get_next()) {
        move_branch_args(false_target);
        
        comparison_emitter();
        emit(mcode::Instruction(get_jcc_opcode(ssa::invert_comparison(comparison)), {
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

mcode::Opcode X8664SSALowerer::get_jcc_opcode(ssa::Comparison comparison) {
    return X8664Opcode::JCC + get_condition_code(comparison);
}

mcode::Opcode X8664SSALowerer::get_cmovcc_opcode(ssa::Comparison comparison) {
    return X8664Opcode::CMOVCC + get_condition_code(comparison);
}

unsigned X8664SSALowerer::get_condition_code(ssa::Comparison comparison) {
    switch(comparison) {
        case ssa::Comparison::EQ: return X8664ConditionCode::E;
        case ssa::Comparison::NE: return X8664ConditionCode::NE;
        case ssa::Comparison::UGT: return X8664ConditionCode::A;
        case ssa::Comparison::UGE: return X8664ConditionCode::AE;
        case ssa::Comparison::ULT: return X8664ConditionCode::B;
        case ssa::Comparison::ULE: return X8664ConditionCode::BE;
        case ssa::Comparison::SGT: return X8664ConditionCode::G;
        case ssa::Comparison::SGE: return X8664ConditionCode::GE;
        case ssa::Comparison::SLT: return X8664ConditionCode::L;
        case ssa::Comparison::SLE: return X8664ConditionCode::LE;
        case ssa::Comparison::FEQ: return X8664ConditionCode::E;
        case ssa::Comparison::FNE: return X8664ConditionCode::NE;
        case ssa::Comparison::FGT: return X8664ConditionCode::A;
        case ssa::Comparison::FGE: return X8664ConditionCode::AE;
        case ssa::Comparison::FLT: return X8664ConditionCode::B;
        case ssa::Comparison::FLE: return X8664ConditionCode::BE;
    }
}

void X8664SSALowerer::move_branch_args(ssa::BranchTarget &target) {
    for (unsigned i = 0; i < target.args.size(); i++) {
        ssa::Value &arg = target.args[i];
        ssa::VirtualRegister param_reg = target.block->get_param_regs()[i];

        mcode::Opcode move_opcode;
        if (arg.get_type().is_primitive(ssa::Primitive::F32)) move_opcode = X8664Opcode::MOVSS;
        else if (arg.get_type().is_primitive(ssa::Primitive::F64)) move_opcode = X8664Opcode::MOVSD;
        else move_opcode = X8664Opcode::MOV;

        unsigned size = get_size(arg.get_type());
        mcode::Operand dst = mcode::Operand::from_register(mcode::Register::from_virtual(param_reg), size);

        if (arg.is_int_immediate() && arg.get_int_immediate() == 0) {
            emit(mcode::Instruction(X8664Opcode::XOR, {dst, dst}, mcode::Instruction::FLAG_CALL_ARG));
        } else if (arg.is_fp_immediate() && arg.get_fp_immediate() == 0.0) {
            emit(mcode::Instruction(X8664Opcode::XORPS, {dst, dst}, mcode::Instruction::FLAG_CALL_ARG));
        } else {
            mcode::Operand src = lower_as_operand(arg);
            if (src != dst) {
                emit(mcode::Instruction(move_opcode, {dst, src}, mcode::Instruction::FLAG_CALL_ARG));
            }
        }
    }
}

void X8664SSALowerer::emit_shift(ssa::Instruction &instr, mcode::Opcode opcode) {
    unsigned size = get_size(instr.get_operand(0).get_type());

    mcode::Register tmp_reg = create_reg();
    mcode::Operand op0 = mcode::Operand::from_register(tmp_reg, size);
    emit(mcode::Instruction(X8664Opcode::MOV, {op0, lower_as_operand(instr.get_operand(0))}));

    mcode::Operand op1;

    if (instr.get_operand(1).is_int_immediate()) {
        op1 = mcode::Operand::from_int_immediate(instr.get_operand(1).get_int_immediate(), 1);
    } else {
        mcode::Register rcx = mcode::Register::from_physical(X8664Register::RCX);
        mcode::Operand rcx8 = mcode::Operand::from_register(rcx, 8);
        emit(mcode::Instruction(X8664Opcode::MOV, {rcx8, lower_as_operand(instr.get_operand(1)).with_size(8)}));
        op1 = mcode::Operand::from_register(rcx, 1);
    }

    emit(mcode::Instruction(opcode, {op0, op1}));

    mcode::Operand dst = map_vreg_dst(instr, size);
    emit(mcode::Instruction(X8664Opcode::MOV, {dst, op0}));
}

// clang-format on

mcode::Operand X8664SSALowerer::lower_as_move_into_reg(mcode::Register reg, const ssa::Value &value) {
    unsigned size = get_size(value.get_type());
    mcode::Operand m_dst = mcode::Operand::from_register(reg, size);
    return lower_as_move(m_dst, value);
}

mcode::Operand X8664SSALowerer::lower_as_move(mcode::Operand m_dst, const ssa::Value &value) {
    if (value.is_int_immediate()) {
        return lower_int_imm_as_move(m_dst, value.get_int_immediate());
    } else if (value.is_fp_immediate()) {
        return lower_fp_imm_as_move(m_dst, value.get_fp_immediate());
    } else if (value.is_register()) {
        return lower_reg_as_move(m_dst, value.get_register(), value.get_type());
    } else if (value.is_symbol()) {
        emit({X8664Opcode::MOV, {m_dst, lower_as_operand(value)}});
        return m_dst;
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Operand X8664SSALowerer::lower_int_imm_as_move(mcode::Operand m_dst, LargeInt value) {
    // Use `XOR dst, dst` for zeroing registers to save code size and also because modern CPUs recognize and
    // optimize this idiom.
    if (value == 0 && m_dst.is_register()) {
        // Use the 32-bit variant even when operating on 64-bit registers so we can potentially omit the REX prefix.
        m_dst.set_size(std::min(m_dst.get_size(), 4));

        emit({X8664Opcode::XOR, {m_dst, m_dst}});
        return m_dst;
    }

    mcode::Operand m_src = mcode::Operand::from_int_immediate(value, m_dst.get_size());
    emit({X8664Opcode::MOV, {m_dst, m_src}});
    return m_dst;
}

mcode::Operand X8664SSALowerer::lower_fp_imm_as_move(mcode::Operand m_dst, double value) {
    // Use `XORP[S,D] dst, dst` to generate a zero floating-point number so we don't have to load it from memory.
    if (value == 0.0) {
        mcode::Opcode m_opcode = m_dst.get_size() == 4 ? X8664Opcode::XORPS : X8664Opcode::XORPD;

        if (m_dst.is_register()) {
            emit({m_opcode, {m_dst, m_dst}});
        } else {
            mcode::Operand m_tmp = mcode::Operand::from_register(create_reg(), m_dst.get_size());
            mcode::Opcode m_mov_opcode = m_dst.get_size() == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;

            emit({m_opcode, {m_tmp, m_tmp}});
            emit({m_mov_opcode, {m_dst, m_tmp}});
        }

        return m_dst;
    }

    mcode::Opcode m_opcode = m_dst.get_size() == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;
    mcode::Operand m_src = create_fp_const_load(value, m_dst.get_size());

    if (!m_dst.is_aarch64_addr() && m_src.is_symbol_deref()) {
        mcode::Operand m_tmp = mcode::Operand::from_register(create_reg(), m_dst.get_size());
        emit({m_opcode, {m_tmp, m_src}});
        emit({m_opcode, {m_dst, m_tmp}});
    } else {
        emit({m_opcode, {m_dst, m_src}});
    }

    return m_dst;
}

mcode::Operand X8664SSALowerer::lower_reg_as_move(mcode::Operand m_dst, ssa::VirtualRegister src_reg, ssa::Type type) {
    mcode::Operand m_src = map_vreg_as_operand(src_reg, m_dst.get_size());

    if (m_src.is_stack_slot()) {
        if (m_dst.is_register()) {
            emit({X8664Opcode::LEA, {m_dst, m_src}});
        } else {
            mcode::Operand m_tmp = mcode::Operand::from_register(create_reg(), m_dst.get_size());
            emit({X8664Opcode::LEA, {m_tmp, m_src}});
            emit({X8664Opcode::MOV, {m_dst, m_tmp}});
        }
    } else if (type.is_floating_point()) {
        mcode::Opcode m_opcode = m_dst.get_size() == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;
        emit({m_opcode, {m_dst, m_src}});
    } else {
        emit({X8664Opcode::MOV, {m_dst, m_src}});
    }

    return m_dst;
}

mcode::Operand X8664SSALowerer::lower_as_operand(const ssa::Value &value) {
    return lower_as_operand(value, {});
}

mcode::Operand X8664SSALowerer::lower_as_operand(const ssa::Value &value, ValueLowerFlags flags) {
    unsigned size = get_size(value.get_type());

    if (value.is_int_immediate()) {
        return lower_int_imm_as_operand(value.get_int_immediate(), size);
    } else if (value.is_fp_immediate()) {
        return lower_fp_imm_as_operand(value.get_fp_immediate(), size);
    } else if (value.is_register()) {
        return lower_reg_as_operand(value.get_register(), size, flags);
    } else if (value.is_symbol()) {
        return lower_symbol_as_operand(value, size, flags);
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Operand X8664SSALowerer::lower_int_imm_as_operand(LargeInt value, unsigned size) {
    // Integer immediates that are larger than 32 bits cannot be encoded as an operand, so move them
    // into a register using an extra `MOV` instruction.
    if (size == 8 && value.to_bits() >= (1ull << 32)) {
        mcode::Register m_reg = create_reg();
        mcode::Operand m_dst = mcode::Operand::from_register(m_reg, 8);
        mcode::Operand m_src = mcode::Operand::from_int_immediate(value, 8);
        emit({X8664Opcode::MOV, {m_dst, m_src}});
        return m_dst;
    }

    return mcode::Operand::from_int_immediate(value, size);
}

mcode::Operand X8664SSALowerer::lower_fp_imm_as_operand(double value, unsigned size) {
    mcode::Register m_reg = create_reg();
    mcode::Operand m_dst = mcode::Operand::from_register(m_reg, size);

    // Use `XORP[S,D] dst, dst` to generate a zero floating-point number so we don't have to load it
    // from memory.
    if (value == 0.0) {
        mcode::Opcode m_opcode = size == 4 ? X8664Opcode::XORPS : X8664Opcode::XORPD;
        emit({m_opcode, {m_dst, m_dst}});
        return m_dst;
    }

    mcode::Opcode m_opcode = size == 4 ? X8664Opcode::MOVSS : X8664Opcode::MOVSD;
    mcode::Operand m_src = create_fp_const_load(value, size);
    emit({m_opcode, {m_dst, m_src}});
    return m_dst;
}

mcode::Operand X8664SSALowerer::lower_reg_as_operand(
    ssa::VirtualRegister src_reg,
    unsigned size,
    ValueLowerFlags flags
) {
    mcode::Operand m_src = map_vreg_as_operand(src_reg, size);

    if (m_src.is_stack_slot()) {
        mcode::Operand m_dst = mcode::Operand::from_register(create_reg(), size);

        emit(mcode::Instruction(X8664Opcode::LEA, {m_dst, m_src}));
        return m_dst;
    }

    if (flags.allow_addrs) {
        ssa::InstrIter producer = get_producer(src_reg);

        // If this register was produced by a load, use the loaded value directly instead of first
        // loading it into a register.
        if (producer->get_opcode() == ssa::Opcode::LOAD && get_num_uses(*producer->get_dest()) == 1) {
            mcode::Operand m_load_dst = lower_address(producer->get_operand(1));
            discard_use(*producer->get_dest());
            return m_load_dst;
        }
    }

    return m_src;
}

mcode::Operand X8664SSALowerer::lower_symbol_as_operand(const ssa::Value &value, unsigned size, ValueLowerFlags flags) {
    const std::string &symbol_name = value.get_symbol_name();

    mcode::Relocation reloc = mcode::Relocation::NONE;
    if (target->get_descr().is_unix()) {
        if (value.is_extern_func()) {
            // Actually, the PLT doesn't seem to be that popular nowadays, so we might want to not
            // use it? Rust disabled it back in 2018: https://github.com/rust-lang/rust/pull/54592
            reloc = mcode::Relocation::PLT;
        } else if (value.is_extern_global()) {
            reloc = mcode::Relocation::GOT;
        }
    }

    mcode::Symbol symbol(symbol_name, reloc);

    if (target->get_descr().is_darwin() && (value.is_func() || value.is_extern_func())) {
        return mcode::Operand::from_symbol(symbol, PTR_SIZE);
    }

    if (flags.is_callee) {
        return read_symbol_addr(symbol);
    }

    if (target->get_code_model() == CodeModel::SMALL) {
        mcode::Operand m_src = mcode::Operand::from_symbol_deref(symbol, size);
        mcode::Operand m_dst = mcode::Operand::from_register(create_reg(), PTR_SIZE);
        emit({X8664Opcode::LEA, {m_dst, m_src}});
        return m_dst;
    } else if (target->get_code_model() == CodeModel::LARGE) {
        return read_symbol_addr(symbol);
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Register X8664SSALowerer::create_reg() {
    return mcode::Register::from_virtual(get_func().next_virtual_reg());
}

mcode::Operand X8664SSALowerer::create_fp_const_load(double value, unsigned size) {
    if (size == 4) {
        return const_lowering.load_f32(static_cast<float>(value));
    } else if (size == 8) {
        return const_lowering.load_f64(value);
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace target

} // namespace banjo
