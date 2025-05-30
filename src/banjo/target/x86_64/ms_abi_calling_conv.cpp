#include "ms_abi_calling_conv.hpp"

#include "banjo/codegen/machine_pass_utils.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/target/x86_64/x86_64_ssa_lowerer.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

/*
============
Stack Layout
============

+--------------------------+
| Callee argument storage  |
+--------------------------+
| Return address (8 bytes) |
+--------------------------+
| Pushed regs              |
+--------------------------+
| Generic region (RSP)     |
+--------------------------+
| Caller argument storage  |
+--------------------------+

The stack pointer is aligned to 16 bytes.
If the function is not a leaf function, at least 32 bytes of argument storage have to be allocated.
Callees store their register arguments in this argument storage.
Volatile general-purpose registers are pushed to the stack before allocating the generic region.

*/

namespace banjo {

namespace target {

using namespace X8664Register;

MSABICallingConv const MSABICallingConv::INSTANCE = MSABICallingConv();
std::vector<mcode::PhysicalReg> const MSABICallingConv::ARG_REGS_INT = {RCX, RDX, R8, R9};
std::vector<mcode::PhysicalReg> const MSABICallingConv::ARG_REGS_FP = {XMM0, XMM1, XMM2, XMM3};

MSABICallingConv::MSABICallingConv() {
    volatile_regs = {RAX, RCX, RDX, RSP, RBP, R8, R9, R10, R11, XMM0, XMM1, XMM2, XMM3, XMM4, XMM5};
}

void MSABICallingConv::lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr) {
    bool variadic = instr.get_attr() == ssa::Instruction::Attribute::VARIADIC;

    for (unsigned i = 1; i < instr.get_operands().size(); i++) {
        ssa::Operand &operand = instr.get_operands()[i];
        emit_arg_move(lowerer, operand, i - 1, variadic);
    }

    emit_call(lowerer, instr.get_operand(0));

    if (instr.get_dest()) {
        emit_ret_val_move(lowerer);
    }
}

void MSABICallingConv::emit_arg_move(
    codegen::SSALowerer &lowerer,
    ssa::Operand &operand,
    unsigned index,
    bool variadic
) {
    if (index < 4) {
        if (variadic) {
            emit_reg_arg_move_variadic(lowerer, operand, index);
        } else {
            emit_reg_arg_move(lowerer, operand, index);
        }
    } else {
        emit_stack_arg_move(lowerer, operand, index);
    }
}

void MSABICallingConv::emit_reg_arg_move(codegen::SSALowerer &lowerer, ssa::Operand &operand, unsigned index) {
    X8664SSALowerer &x86_64_lowerer = static_cast<X8664SSALowerer &>(lowerer);

    bool is_fp = operand.get_type().is_floating_point();
    mcode::PhysicalReg m_reg = is_fp ? ARG_REGS_FP[index] : ARG_REGS_INT[index];
    mcode::Register m_dst_reg = mcode::Register::from_physical(m_reg);

    x86_64_lowerer.lower_as_move_into_reg(m_dst_reg, operand);
}

void MSABICallingConv::emit_reg_arg_move_variadic(codegen::SSALowerer &lowerer, ssa::Operand &operand, unsigned index) {
    X8664SSALowerer &x86_64_lowerer = static_cast<X8664SSALowerer &>(lowerer);
    bool is_fp = operand.get_type().is_floating_point();

    if (is_fp) {
        ASSERT(operand.get_type() == ssa::Primitive::F64);

        mcode::Register m_dst_reg_fp = mcode::Register::from_physical(ARG_REGS_FP[index]);
        mcode::Operand m_dst_fp = mcode::Operand::from_register(m_dst_reg_fp, 8);
        x86_64_lowerer.lower_as_move_into_reg(m_dst_reg_fp, operand);

        mcode::Register m_dst_reg_gp = mcode::Register::from_physical(ARG_REGS_INT[index]);
        mcode::Operand m_dst_gp = mcode::Operand::from_register(m_dst_reg_gp, 8);
        lowerer.emit({X8664Opcode::MOVQ, {m_dst_gp, m_dst_fp}});
    } else {
        mcode::Register m_dst_reg = mcode::Register::from_physical(ARG_REGS_INT[index]);
        x86_64_lowerer.lower_as_move_into_reg(m_dst_reg, operand);
    }
}

void MSABICallingConv::emit_stack_arg_move(codegen::SSALowerer &lowerer, ssa::Operand &operand, unsigned index) {
    X8664SSALowerer &x86_64_lowerer = static_cast<X8664SSALowerer &>(lowerer);

    unsigned arg_slot_index = index - 4;
    mcode::StackFrame &stack_frame = lowerer.get_machine_func()->get_stack_frame();
    mcode::StackSlotID slot_index;

    if (stack_frame.get_call_arg_slot_indices().size() <= arg_slot_index) {
        mcode::StackSlot stack_slot(mcode::StackSlot::Type::CALL_ARG, 8, 1);
        stack_slot.set_call_arg_index(arg_slot_index);
        slot_index = stack_frame.new_stack_slot(stack_slot);
    } else {
        slot_index = stack_frame.get_call_arg_slot_indices()[arg_slot_index];
    }

    unsigned size = x86_64_lowerer.get_size(operand.get_type());
    mcode::Operand m_dst = mcode::Operand::from_stack_slot(slot_index, size);

    x86_64_lowerer.lower_as_move(m_dst, operand);
}

void MSABICallingConv::emit_call(codegen::SSALowerer &lowerer, const ssa::Operand &func_operand) {
    X8664SSALowerer &x86_64_lowerer = static_cast<X8664SSALowerer &>(lowerer);

    int ptr_size = X8664SSALowerer::PTR_SIZE;

    mcode::Operand operand;
    if (func_operand.is_symbol()) {
        operand = x86_64_lowerer.read_symbol_addr(func_operand.get_symbol_name());
    } else if (func_operand.is_register()) {
        ssa::InstrIter producer = lowerer.get_producer(func_operand.get_register());
        if (producer->get_opcode() == ssa::Opcode::LOAD) {
            operand = x86_64_lowerer.lower_address(producer->get_operand(1));
            lowerer.discard_use(func_operand.get_register());
        } else {
            operand = lowerer.map_vreg_as_operand(func_operand.get_register(), ptr_size);
        }
    }

    lowerer.emit(mcode::Instruction(X8664Opcode::CALL, {operand}, mcode::Instruction::FLAG_CALL));
}

void MSABICallingConv::emit_ret_val_move(codegen::SSALowerer &lowerer) {
    ssa::Instruction &instr = *lowerer.get_instr_iter();

    bool is_floating_point = instr.get_operand(0).get_type().is_floating_point();
    unsigned return_size = lowerer.get_size(instr.get_operand(0).get_type());

    mcode::Opcode opcode;
    mcode::PhysicalReg src_reg;

    if (!is_floating_point) {
        opcode = X8664Opcode::MOV;
        src_reg = X8664Register::RAX;
    } else {
        if (return_size == 4) {
            opcode = X8664Opcode::MOVSS;
        } else if (return_size == 8) {
            opcode = X8664Opcode::MOVSD;
        } else {
            ASSERT_UNREACHABLE;
        }

        src_reg = X8664Register::XMM0;
    }

    lowerer.emit(
        mcode::Instruction(
            opcode,
            {mcode::Operand::from_register(mcode::Register::from_virtual(*instr.get_dest()), return_size),
             mcode::Operand::from_register(mcode::Register::from_physical(src_reg), return_size)}
        )
    );
}

void MSABICallingConv::create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions) {
    mcode::ArgStoreStackRegion &region = regions.arg_store_region;
    region.size = 0;

    // The first argument is below the implicitly allocated region,
    // so we add the size of this region to get to the first argument
    int arg_store_offset = regions.implicit_region.size;

    for (int i = 0; i < frame.get_stack_slots().size(); i++) {
        mcode::StackSlot &slot = frame.get_stack_slots()[i];
        if (!slot.is_defined() && slot.get_type() == mcode::StackSlot::Type::ARG_STORE) {
            region.offsets.insert({i, arg_store_offset});
            arg_store_offset += 8;
        }
    }
}

void MSABICallingConv::create_call_arg_region(
    mcode::Function *func,
    mcode::StackFrame &frame,
    mcode::StackRegions &regions
) {
    mcode::CallArgStackRegion &region = regions.call_arg_region;

    bool has_call_instr = false;
    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (mcode::Instruction &instr : basic_block) {
            if (instr.get_opcode() == X8664Opcode::CALL) {
                has_call_instr = true;
                break;
            }
        }
    }

    if (has_call_instr) {
        region.size = 32;
    } else {
        region.size = 0;
        return;
    }

    for (int index : frame.get_call_arg_slot_indices()) {
        mcode::StackSlot &slot = frame.get_stack_slot(index);
        slot.set_offset(32 + 8 * slot.get_call_arg_index());
        region.size += 8;
    }
}

void MSABICallingConv::create_implicit_region(
    mcode::Function *func,
    mcode::StackFrame &frame,
    mcode::StackRegions &regions
) {
    unsigned saved_reg_space_size = 0;
    for (mcode::PhysicalReg reg : codegen::MachinePassUtils::get_modified_volatile_regs(func)) {
        if (reg >= RAX && reg <= R15) {
            saved_reg_space_size += 8;
        } else {
            unsigned index = frame.new_stack_slot({mcode::StackSlot::Type::GENERIC, 8, 8});
            frame.get_reg_save_slot_indices().push_back(index);
        }
    }

    regions.implicit_region.saved_reg_size = saved_reg_space_size;
    regions.implicit_region.size = get_implicit_stack_bytes(func) + saved_reg_space_size;
}

int MSABICallingConv::get_alloca_size(mcode::StackRegions &regions) {
    int arg_store_bytes = regions.arg_store_region.size;
    int generic_bytes = regions.generic_region.size;
    int call_arg_bytes = regions.call_arg_region.size;
    int implicit_bytes = regions.implicit_region.size;

    int minimum_size = arg_store_bytes + generic_bytes + call_arg_bytes;

    // If this is a leaf function, alignment is not required.
    if (call_arg_bytes == 0) {
        return minimum_size;
    }

    return Utils::align(minimum_size + implicit_bytes, 16) - implicit_bytes;
}

std::vector<mcode::Instruction> MSABICallingConv::get_prolog(mcode::Function *func) {
    std::vector<mcode::Instruction> prolog;
    std::vector<long> modified_volatile_regs = codegen::MachinePassUtils::get_modified_volatile_regs(func);

    // Push modified non-volatile general-purpose registers.
    for (mcode::PhysicalReg reg : modified_volatile_regs) {
        if (reg >= RAX && reg <= R15) {
            mcode::Operand operand = mcode::Operand::from_register(mcode::Register::from_physical(reg), 8);
            prolog.push_back(mcode::Instruction(X8664Opcode::PUSH, {operand}));
            prolog.push_back(mcode::Instruction(mcode::PseudoOpcode::EH_PUSHREG, {operand}));
        }
    }

    // Allocate stack frame.
    if (func->get_stack_frame().get_size() > 0) {
        prolog.push_back(
            mcode::Instruction(
                X8664Opcode::SUB,
                {
                    mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RSP), 8),
                    mcode::Operand::from_int_immediate(func->get_stack_frame().get_size()),
                },
                mcode::Instruction::FLAG_ALLOCA
            )
        );
    }

    // Push modified non-volatile SSE registers.
    unsigned sse_slot_index = 0;
    for (mcode::PhysicalReg reg : modified_volatile_regs) {
        if (reg >= XMM0 && reg <= XMM15) {
            unsigned slot_index = func->get_stack_frame().get_reg_save_slot_indices()[sse_slot_index++];

            mcode::Operand m_dst = mcode::Operand::from_stack_slot(slot_index, 8);
            mcode::Operand m_src = mcode::Operand::from_register(mcode::Register::from_physical(reg), 8);
            prolog.push_back(mcode::Instruction(X8664Opcode::MOVSD, {m_dst, m_src}));
        }
    }

    return prolog;
}

std::vector<mcode::Instruction> MSABICallingConv::get_epilog(mcode::Function *func) {
    std::vector<mcode::Instruction> epilog;
    std::vector<long> modified_volatile_regs = codegen::MachinePassUtils::get_modified_volatile_regs(func);

    // Pop modified non-volatile SSE registers.
    // TODO: reverse order for aesthetics?

    unsigned sse_slot_index = 0;
    for (mcode::PhysicalReg reg : modified_volatile_regs) {
        if (reg >= XMM0 && reg <= XMM15) {
            unsigned slot_index = func->get_stack_frame().get_reg_save_slot_indices()[sse_slot_index++];

            mcode::Operand m_dst = mcode::Operand::from_register(mcode::Register::from_physical(reg), 8);
            mcode::Operand m_src = mcode::Operand::from_stack_slot(slot_index, 8);
            epilog.push_back(mcode::Instruction(X8664Opcode::MOVSD, {m_dst, m_src}));
        }
    }

    // Deallocate stack frame.
    if (func->get_stack_frame().get_size() > 0) {
        epilog.push_back(
            mcode::Instruction(
                X8664Opcode::ADD,
                {
                    mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RSP), 8),
                    mcode::Operand::from_int_immediate(func->get_stack_frame().get_size()),
                }
            )
        );
    }

    // Pop modified non-volatile general-purpose registers.
    for (int i = modified_volatile_regs.size() - 1; i >= 0; i--) {
        mcode::PhysicalReg reg = modified_volatile_regs[i];
        if (reg >= RAX && reg <= R15) {
            epilog.push_back(
                mcode::Instruction(
                    X8664Opcode::POP,
                    {
                        mcode::Operand::from_register(mcode::Register::from_physical(reg), 8),
                    }
                )
            );
        }
    }

    return epilog;
}

bool MSABICallingConv::is_func_exit(mcode::Opcode opcode) {
    return opcode == X8664Opcode::RET;
}

std::vector<mcode::ArgStorage> MSABICallingConv::get_arg_storage(const ssa::FunctionType &func_type) {
    std::vector<mcode::ArgStorage> result(func_type.params.size());

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        mcode::ArgStorage &storage = result[i];

        if (i < ARG_REGS_INT.size()) {
            storage.in_reg = true;
            storage.reg = func_type.params[i].is_floating_point() ? ARG_REGS_FP[i] : ARG_REGS_INT[i];
        } else {
            storage.in_reg = false;
            storage.stack_offset = 8 * i;
        }
    }

    return result;
}

int MSABICallingConv::get_implicit_stack_bytes(mcode::Function *func) {
    return 8; // CALL instruction return address
}

} // namespace target

} // namespace banjo
