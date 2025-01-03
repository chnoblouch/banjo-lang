#include "sys_v_calling_conv.hpp"

#include "banjo/codegen/machine_pass_utils.hpp"
#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/target/x86_64/x86_64_ssa_lowerer.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

namespace banjo {

namespace target {

using namespace X8664Register;

SysVCallingConv const SysVCallingConv::INSTANCE = SysVCallingConv();
std::vector<int> const SysVCallingConv::ARG_REGS_INT = {RDI, RSI, RDX, RCX, R8, R9};
std::vector<int> const SysVCallingConv::ARG_REGS_FLOAT = {XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8};

SysVCallingConv::SysVCallingConv() {
    volatile_regs = {
        RAX,  RDI,  RSI,  RDX,  RCX,  RSP,  RBP,  R8,    R9,    R10,   R11,   XMM0,  XMM1,  XMM2,
        XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
    };
}

void SysVCallingConv::lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr) {
    X8664SSALowerer &x86_64_lowerer = static_cast<X8664SSALowerer &>(lowerer);

    std::vector<ssa::Type> types;
    for (unsigned i = 1; i < instr.get_operands().size(); i++) {
        types.push_back(instr.get_operand(i).get_type());
    }
    std::vector<mcode::ArgStorage> arg_storage = get_arg_storage(types);

    for (int i = 1; i < instr.get_operands().size(); i++) {
        ssa::Operand &operand = instr.get_operands()[i];
        mcode::Register reg = get_arg_reg(instr, i - 1, lowerer);
        x86_64_lowerer.lower_as_move_into_reg(reg, operand);
    }

    append_call(instr.get_operand(0), lowerer);

    if (instr.get_dest().has_value()) {
        append_ret_val_move(lowerer);
    }
}

mcode::Register SysVCallingConv::get_arg_reg(ssa::Instruction &instr, unsigned index, codegen::SSALowerer &lowerer) {
    mcode::StackFrame &stack_frame = lowerer.get_machine_func()->get_stack_frame();

    std::vector<ssa::Type> types;
    for (unsigned i = 1; i < instr.get_operands().size(); i++) {
        types.push_back(instr.get_operand(i).get_type());
    }

    mcode::ArgStorage storage = get_arg_storage(types)[index];
    if (storage.in_reg) {
        return mcode::Register::from_physical(storage.reg);
    } else if (stack_frame.get_call_arg_slot_indices().size() <= storage.arg_slot_index) {
        mcode::StackSlot stack_slot(mcode::StackSlot::Type::CALL_ARG, 8, 1);
        stack_slot.set_call_arg_index(storage.arg_slot_index);
        return mcode::Register::from_stack_slot(stack_frame.new_stack_slot(stack_slot));
    } else {
        mcode::StackSlotID slot_index = stack_frame.get_call_arg_slot_indices()[storage.arg_slot_index];
        return mcode::Register::from_stack_slot(slot_index);
    }
}

void SysVCallingConv::append_call(ssa::Operand func_operand, codegen::SSALowerer &lowerer) {
    X8664SSALowerer &x86_64_lowerer = static_cast<X8664SSALowerer &>(lowerer);

    int ptr_size = X8664SSALowerer::PTR_SIZE;

    mcode::Operand m_callee;
    if (func_operand.is_symbol()) {
        m_callee = x86_64_lowerer.lower_as_operand(func_operand, {.is_callee = true});
    } else if (func_operand.is_register()) {
        ssa::InstrIter producer = lowerer.get_producer(func_operand.get_register());
        if (producer->get_opcode() == ssa::Opcode::LOAD) {
            m_callee = x86_64_lowerer.lower_address(producer->get_operand(1));
            lowerer.discard_use(func_operand.get_register());
        } else {
            m_callee = mcode::Operand::from_register(lowerer.lower_reg(func_operand.get_register()), ptr_size);
        }
    }

    mcode::InstrIter iter =
        lowerer.emit(mcode::Instruction(X8664Opcode::CALL, {m_callee}, mcode::Instruction::FLAG_CALL));

    iter->add_reg_op(X8664Register::RAX, mcode::RegUsage::DEF);
    iter->add_reg_op(X8664Register::XMM0, mcode::RegUsage::DEF);
}

void SysVCallingConv::append_ret_val_move(codegen::SSALowerer &lowerer) {
    ssa::Instruction &instr = *lowerer.get_instr_iter();

    bool is_floating_point = instr.get_operand(0).get_type().is_floating_point();
    int return_size = lowerer.get_size(instr.get_operand(0).get_type());

    mcode::Opcode opcode;
    long src_reg;

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

    lowerer.emit(mcode::Instruction(
        opcode,
        {mcode::Operand::from_register(mcode::Register::from_virtual(*instr.get_dest()), return_size),
         mcode::Operand::from_register(mcode::Register::from_physical(src_reg), return_size)}
    ));
}

void SysVCallingConv::create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions) {
    mcode::ArgStoreStackRegion &region = regions.arg_store_region;
    region.size = 0;

    for (int i = 0; i < frame.get_stack_slots().size(); i++) {
        mcode::StackSlot &slot = frame.get_stack_slots()[i];
        if (!slot.is_defined() && slot.get_type() == mcode::StackSlot::Type::ARG_STORE) {
            region.size -= 8;
            region.offsets.insert({i, region.size});
        }
    }
}

void SysVCallingConv::create_call_arg_region(
    mcode::Function *func,
    mcode::StackFrame &frame,
    mcode::StackRegions &regions
) {
    mcode::CallArgStackRegion &region = regions.call_arg_region;
    region.size = 0;

    for (int index : frame.get_call_arg_slot_indices()) {
        mcode::StackSlot &slot = frame.get_stack_slot(index);
        slot.set_offset(8 * slot.get_call_arg_index());
        region.size += 8;
    }
}

void SysVCallingConv::create_implicit_region(
    mcode::Function *func,
    mcode::StackFrame &frame,
    mcode::StackRegions &regions
) {
    unsigned saved_reg_space_size = 0;
    for (mcode::PhysicalReg reg : codegen::MachinePassUtils::get_modified_volatile_regs(func)) {
        if (reg >= RAX && reg <= R15) {
            saved_reg_space_size += 8;
        } else {
            unsigned index = frame.new_stack_slot({mcode::StackSlot::Type::GENERIC, 8, 0});
            frame.get_reg_save_slot_indices().push_back(index);
        }
    }

    regions.implicit_region.saved_reg_size = saved_reg_space_size;
    regions.implicit_region.size = get_implicit_stack_bytes(func) + saved_reg_space_size;
}

int SysVCallingConv::get_alloca_size(mcode::StackRegions &regions) {
    int arg_store_bytes = regions.arg_store_region.size;
    int generic_bytes = regions.generic_region.size;
    int call_arg_bytes = regions.call_arg_region.size;
    int saved_reg_bytes = regions.implicit_region.saved_reg_size;

    int minimum_size = arg_store_bytes + generic_bytes + call_arg_bytes;
    return Utils::align(minimum_size + saved_reg_bytes, 16) - saved_reg_bytes;
}

std::vector<mcode::Instruction> SysVCallingConv::get_prolog(mcode::Function *func) {
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

    // Push frame pointer.
    prolog.push_back(mcode::Instruction(
        X8664Opcode::PUSH,
        {mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RBP), 8)}
    ));

    // Set frame pointer to stack pointer.
    prolog.push_back(mcode::Instruction(
        X8664Opcode::MOV,
        {mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RBP), 8),
         mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RSP), 8)}
    ));

    // Allocate stack frame.
    prolog.push_back(mcode::Instruction(
        X8664Opcode::SUB,
        {mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RSP), 8),
         mcode::Operand::from_int_immediate(func->get_stack_frame().get_size())},
        mcode::Instruction::FLAG_ALLOCA
    ));

    // Push modified non-volatile SSE registers.
    unsigned sse_slot_index = 0;
    for (mcode::PhysicalReg reg : modified_volatile_regs) {
        if (reg >= XMM0 && reg <= XMM15) {
            unsigned slot_index = func->get_stack_frame().get_reg_save_slot_indices()[sse_slot_index++];

            prolog.push_back(mcode::Instruction(
                X8664Opcode::MOVSD,
                {
                    mcode::Operand::from_register(mcode::Register::from_stack_slot(slot_index), 8),
                    mcode::Operand::from_register(mcode::Register::from_physical(reg), 8),
                }
            ));
        }
    }

    return prolog;
}

std::vector<mcode::Instruction> SysVCallingConv::get_epilog(mcode::Function *func) {
    std::vector<mcode::Instruction> epilog;
    std::vector<long> modified_volatile_regs = codegen::MachinePassUtils::get_modified_volatile_regs(func);

    // Pop modified non-volatile SSE registers.
    // TODO: reverse order for aesthetics?

    unsigned sse_slot_index = 0;
    for (mcode::PhysicalReg reg : modified_volatile_regs) {
        if (reg >= XMM0 && reg <= XMM15) {
            unsigned slot_index = func->get_stack_frame().get_reg_save_slot_indices()[sse_slot_index++];

            epilog.push_back(mcode::Instruction(
                X8664Opcode::MOVSD,
                {
                    mcode::Operand::from_register(mcode::Register::from_physical(reg), 8),
                    mcode::Operand::from_register(mcode::Register::from_stack_slot(slot_index), 8),
                }
            ));
        }
    }

    // Deallocate stack frame.
    epilog.push_back(mcode::Instruction(
        X8664Opcode::ADD,
        {mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RSP), 8),
         mcode::Operand::from_int_immediate(func->get_stack_frame().get_size())}
    ));

    // Pop frame pointer.
    epilog.push_back(mcode::Instruction(
        X8664Opcode::POP,
        {mcode::Operand::from_register(mcode::Register::from_physical(X8664Register::RBP), 8)}
    ));

    // Pop modified non-volatile general-purpose registers.
    for (int i = modified_volatile_regs.size() - 1; i >= 0; i--) {
        mcode::PhysicalReg reg = modified_volatile_regs[i];
        if (reg >= RAX && reg <= R15) {
            epilog.push_back(mcode::Instruction(
                X8664Opcode::POP,
                {mcode::Operand::from_register(mcode::Register::from_physical(reg), 8)}
            ));
        }
    }

    return epilog;
}

bool SysVCallingConv::is_func_exit(mcode::Opcode opcode) {
    return opcode == X8664Opcode::RET;
}

std::vector<mcode::ArgStorage> SysVCallingConv::get_arg_storage(const std::vector<ssa::Type> &types) {
    std::vector<mcode::ArgStorage> result;
    result.resize(types.size());

    unsigned int general_reg_index = 0;
    unsigned int float_reg_index = 0;
    unsigned int arg_slot_index = 0;

    for (unsigned int i = 0; i < types.size(); i++) {
        mcode::ArgStorage storage;
        bool is_fp = types[i].is_floating_point();

        if (is_fp && float_reg_index < ARG_REGS_FLOAT.size()) {
            storage.in_reg = true;
            storage.reg = ARG_REGS_FLOAT[float_reg_index++];
        } else if (!is_fp && general_reg_index < ARG_REGS_INT.size()) {
            storage.in_reg = true;
            storage.reg = ARG_REGS_INT[general_reg_index++];
        } else {
            storage.in_reg = false;
            storage.arg_slot_index = arg_slot_index++;
            storage.stack_offset = 8 * arg_slot_index;
        }

        result[i] = storage;
    }

    return result;
}

int SysVCallingConv::get_implicit_stack_bytes(mcode::Function *func) {
    return 16; // CALL instruction return address + RBP
}

} // namespace target

} // namespace banjo
