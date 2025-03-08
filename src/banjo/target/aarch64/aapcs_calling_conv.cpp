#include "aapcs_calling_conv.hpp"

#include "aarch64_reg_analyzer.hpp"
#include "banjo/codegen/late_reg_alloc.hpp"
#include "banjo/codegen/machine_pass_utils.hpp"
#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/ssa/utils.hpp"
#include "banjo/target/aarch64/aarch64_encoding_info.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_reg_analyzer.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/target/aarch64/aarch64_ssa_lowerer.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

namespace banjo {

namespace target {

using namespace AArch64Register;

AAPCSCallingConv AAPCSCallingConv::INSTANCE_STANDARD(AAPCSCallingConv::Variant::STANDARD);
AAPCSCallingConv AAPCSCallingConv::INSTANCE_APPLE(AAPCSCallingConv::Variant::APPLE);

std::vector<int> const AAPCSCallingConv::ARG_REGS_INT = {R0, R1, R2, R3, R4, R5, R6, R7};
std::vector<int> const AAPCSCallingConv::ARG_REGS_FP = {V0, V1, V2, V3, V4, V5, V6, V7};

AAPCSCallingConv::AAPCSCallingConv(Variant variant) : variant(variant) {
    volatile_regs = {
        R0,  R1,  R2,  R3,  R4,  R5, R6, R7, R8, R9, R10, R11, R12, R13,
        R14, R15, R16, R17, R18, V0, V1, V2, V3, V4, V5,  V6,  V7,  SP,
    };
}

void AAPCSCallingConv::lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr) {
    AArch64SSALowerer &aarch64_ssa_lowerer = static_cast<AArch64SSALowerer &>(lowerer);
    ssa::FunctionType func_type = ssa::get_call_func_type(instr);

    emit_arg_moves(aarch64_ssa_lowerer, func_type, instr);
    emit_call_instr(aarch64_ssa_lowerer, instr);

    if (instr.get_dest()) {
        emit_ret_val_move(aarch64_ssa_lowerer, instr);
    }
}

void AAPCSCallingConv::emit_arg_moves(
    AArch64SSALowerer &lowerer,
    ssa::FunctionType &func_type,
    ssa::Instruction &call_instr
) {
    unsigned gp_reg_index = 0;
    unsigned fp_reg_index = 0;

    for (unsigned i = 1; i < call_instr.get_operands().size(); i++) {
        unsigned index = i - 1;
        ssa::Operand &operand = call_instr.get_operand(i);

        if (variant == Variant::APPLE && func_type.variadic && index >= func_type.first_variadic_index) {
            unsigned arg_slot_index = index - func_type.first_variadic_index;
            emit_stack_arg_move(lowerer, operand, arg_slot_index);
        } else if (index < 8) {
            if (operand.get_type().is_floating_point()) {
                emit_reg_arg_move(lowerer, operand, fp_reg_index);
                fp_reg_index += 1;
            } else {
                emit_reg_arg_move(lowerer, operand, gp_reg_index);
                gp_reg_index += 1;
            }
        } else {
            unsigned arg_slot_index = index - 8;
            emit_stack_arg_move(lowerer, operand, arg_slot_index);
        }
    }
}

void AAPCSCallingConv::emit_reg_arg_move(AArch64SSALowerer &lowerer, ssa::Operand &operand, unsigned index) {
    bool is_fp = operand.get_type().is_floating_point();
    mcode::PhysicalReg m_reg = is_fp ? ARG_REGS_FP[index] : ARG_REGS_INT[index];
    mcode::Register m_dst_reg = mcode::Register::from_physical(m_reg);

    lowerer.lower_as_move_into_reg(m_dst_reg, operand);
}

void AAPCSCallingConv::emit_stack_arg_move(AArch64SSALowerer &lowerer, ssa::Operand &operand, unsigned arg_slot_index) {
    mcode::StackFrame &stack_frame = lowerer.get_machine_func()->get_stack_frame();
    mcode::StackSlotID slot_index = stack_frame.create_call_arg_slot(arg_slot_index, 8, 1);

    unsigned size = lowerer.get_size(operand.get_type());
    mcode::Operand m_dst = mcode::Operand::from_stack_slot(slot_index, size);

    mcode::Operand m_src = lowerer.lower_value(operand);
    lowerer.emit({AArch64Opcode::STR, {m_src, m_dst}});
}

void AAPCSCallingConv::emit_call_instr(AArch64SSALowerer &lowerer, ssa::Instruction &call_instr) {
    ssa::Operand &func_operand = call_instr.get_operand(0);

    mcode::Opcode call_opcode;
    mcode::Operand call_operand;

    if (func_operand.is_func()) {
        call_opcode = AArch64Opcode::BL;
        call_operand = mcode::Operand::from_symbol(func_operand.get_func()->name, 8);
    } else if (func_operand.is_symbol()) {
        call_opcode = AArch64Opcode::BL;
        call_operand = mcode::Operand::from_symbol(func_operand.get_symbol_name(), 8);
    } else if (func_operand.is_register()) {
        call_opcode = AArch64Opcode::BLR;
        call_operand = lowerer.map_vreg_as_operand(func_operand.get_register(), 8);
    } else {
        ASSERT_UNREACHABLE;
    }

    lowerer.emit(mcode::Instruction(call_opcode, {call_operand}, mcode::Instruction::FLAG_CALL));
}

void AAPCSCallingConv::emit_ret_val_move(AArch64SSALowerer &lowerer, ssa::Instruction &call_instr) {
    bool is_fp = call_instr.get_operand(0).get_type().is_floating_point();
    mcode::Opcode opcode = is_fp ? AArch64Opcode::FMOV : AArch64Opcode::MOV;
    mcode::PhysicalReg return_reg = is_fp ? AArch64Register::V0 : AArch64Register::R0;
    unsigned return_size = lowerer.get_size(call_instr.get_operand(0).get_type());

    mcode::Register m_dst_reg = mcode::Register::from_virtual(*call_instr.get_dest());
    mcode::Operand m_dst = mcode::Operand::from_register(m_dst_reg, return_size);

    mcode::Register m_src_reg = mcode::Register::from_physical(return_reg);
    mcode::Operand m_src = mcode::Operand::from_register(m_src_reg, return_size);

    lowerer.emit(mcode::Instruction(opcode, {m_dst, m_src}));
}

void AAPCSCallingConv::create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions) {
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

void AAPCSCallingConv::create_call_arg_region(
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

void AAPCSCallingConv::create_implicit_region(
    mcode::Function *func,
    mcode::StackFrame &frame,
    mcode::StackRegions &regions
) {
    unsigned saved_reg_space_size = 8 * codegen::MachinePassUtils::get_modified_volatile_regs(func).size();
    regions.implicit_region.saved_reg_size = saved_reg_space_size;
    regions.implicit_region.size = get_implicit_stack_bytes(func) + saved_reg_space_size;
}

int AAPCSCallingConv::get_alloca_size(mcode::StackRegions &regions) {
    int arg_store_bytes = regions.arg_store_region.size;
    int generic_bytes = regions.generic_region.size;
    int call_arg_bytes = regions.call_arg_region.size;

    int minimum_size = arg_store_bytes + generic_bytes + call_arg_bytes;
    return Utils::align(minimum_size, 16) + 16;
}

std::vector<mcode::Instruction> AAPCSCallingConv::get_prolog(mcode::Function *func) {
    mcode::Register fp = mcode::Register::from_physical(AArch64Register::R29);
    mcode::Register lr = mcode::Register::from_physical(AArch64Register::R30);
    mcode::Register sp = mcode::Register::from_physical(AArch64Register::SP);
    int size = func->get_stack_frame().get_size();

    std::vector<mcode::Instruction> prolog;

    std::vector<long> modified_regs = codegen::MachinePassUtils::get_modified_volatile_regs(func);
    for (long modified_reg : modified_regs) {
        if (modified_reg == AArch64Register::R29 || modified_reg == AArch64Register::R30) {
            continue;
        }

        prolog.push_back(mcode::Instruction(
            AArch64Opcode::STR,
            {mcode::Operand::from_register(mcode::Register::from_physical(modified_reg), 8),
             mcode::Operand::from_aarch64_addr(AArch64Address::new_base_offset_write(sp, -16))}
        ));
    }

    prolog.push_back(mcode::Instruction(
        AArch64Opcode::STP,
        {mcode::Operand::from_register(fp, 8),
         mcode::Operand::from_register(lr, 8),
         mcode::Operand::from_aarch64_addr(AArch64Address::new_base_offset_write(sp, -16))}
    ));

    modify_sp(AArch64Opcode::SUB, size, [&prolog](mcode::Instruction instr) {
        instr.set_flag(mcode::Instruction::FLAG_ALLOCA);
        prolog.push_back(std::move(instr));
    });

    return prolog;

    /*
    std::vector<mcode::Instruction> prolog;

    // Store frame pointer and link register on stack and allocate at least 16 bytes.
    // If the total space allocated is 512 bytes or less, combine the stack allocation
    // and the STP instruction.
    if(is_mergable) {
        prolog.push_back(mcode::Instruction(AArch64Opcode::STP, {
            mcode::Operand::from_register(fp, 8),
            mcode::Operand::from_register(lr, 8),
            mcode::Operand::from_aarch64_addr(AArch64Address::new_base_offset_write(sp, offset))
        }, mcode::Instruction::FLAG_ALLOCA));
    } else {
        prolog.push_back(mcode::Instruction(AArch64Opcode::STP, {
            mcode::Operand::from_register(fp, 8),
            mcode::Operand::from_register(lr, 8),
            mcode::Operand::from_aarch64_addr(AArch64Address::new_base_offset_write(sp, -32))
        }, mcode::Instruction::FLAG_ALLOCA));

        prolog.push_back(mcode::Instruction(AArch64Opcode::STR, {
            mcode::Operand::from_register(mcode::Register::from_physical(AArch64Register::R28), 8),
            mcode::Operand::from_aarch64_addr(AArch64Address::new_base_offset(sp, 16))
        }, mcode::Instruction::FLAG_ALLOCA));
    }

    // Copy stack pointer into frame pointer.
    prolog.push_back(mcode::Instruction(AArch64Opcode::MOV, {
        mcode::Operand::from_register(fp, 8),
        mcode::Operand::from_register(sp, 8)
    }));

    // Allocate the stack frame if the allocation couldn't be combined with the STP instruction.
    if(!is_mergable) {
        prolog.push_back(mcode::Instruction(AArch64Opcode::SUB, {
            mcode::Operand::from_register(sp, 8),
            mcode::Operand::from_register(sp, 8),
            mcode::Operand::from_int_immediate(size)
        }, mcode::Instruction::FLAG_ALLOCA));
    }

    return prolog;
    */
}

std::vector<mcode::Instruction> AAPCSCallingConv::get_epilog(mcode::Function *func) {
    mcode::Register fp = mcode::Register::from_physical(AArch64Register::R0 + 29);
    mcode::Register lr = mcode::Register::from_physical(AArch64Register::R0 + 30);
    mcode::Register sp = mcode::Register::from_physical(AArch64Register::SP);
    int size = func->get_stack_frame().get_size();

    std::vector<mcode::Instruction> epilog;

    modify_sp(AArch64Opcode::ADD, size, [&epilog](mcode::Instruction instr) { epilog.push_back(std::move(instr)); });

    epilog.push_back(mcode::Instruction(
        AArch64Opcode::LDP,
        {mcode::Operand::from_register(fp, 8),
         mcode::Operand::from_register(lr, 8),
         mcode::Operand::from_aarch64_addr(AArch64Address::new_base(sp)),
         mcode::Operand::from_int_immediate(16)}
    ));

    std::vector<long> modified_regs = codegen::MachinePassUtils::get_modified_volatile_regs(func);
    for (auto it = modified_regs.rbegin(); it != modified_regs.rend(); it++) {
        long modified_reg = *it;

        if (modified_reg == AArch64Register::R29 || modified_reg == AArch64Register::R30) {
            continue;
        }

        epilog.push_back(mcode::Instruction(
            AArch64Opcode::LDR,
            {mcode::Operand::from_register(mcode::Register::from_physical(modified_reg), 8),
             mcode::Operand::from_aarch64_addr(AArch64Address::new_base(sp)),
             mcode::Operand::from_int_immediate(16)}
        ));
    }

    return epilog;

    /*
    if(offset <= 504 && false) {
        return {
            mcode::Instruction(AArch64Opcode::LDP, {
                mcode::Operand::from_register(fp, 8),
                mcode::Operand::from_register(lr, 8),
                mcode::Operand::from_aarch64_addr(addr),
                mcode::Operand::from_int_immediate(offset)
            })
        };
    } else {
        return {
            mcode::Instruction(AArch64Opcode::ADD, {
                mcode::Operand::from_register(sp, 8),
                mcode::Operand::from_register(sp, 8),
                mcode::Operand::from_int_immediate(size)
            }),
            mcode::Instruction(AArch64Opcode::LDP, {
                mcode::Operand::from_register(fp, 8),
                mcode::Operand::from_register(lr, 8),
                mcode::Operand::from_aarch64_addr(addr),
                mcode::Operand::from_int_immediate(32)
            })
        };
    }
    */
}

void AAPCSCallingConv::modify_sp(
    mcode::Opcode opcode,
    unsigned value,
    const std::function<void(mcode::Instruction)> &emit
) {
    mcode::Register sp = mcode::Register::from_physical(AArch64Register::SP);
    mcode::Operand m_sp = mcode::Operand::from_register(sp, 8);

    if (value < 4096) {
        mcode::Operand m_imm = mcode::Operand::from_int_immediate(value);
        emit(mcode::Instruction(opcode, {m_sp, m_sp, m_imm}));
    } else if (value < 4096 * 4096) {
        mcode::Operand m_imm_shifted = mcode::Operand::from_int_immediate(value >> 12);
        mcode::Operand m_shift = mcode::Operand::from_aarch64_left_shift(12);
        emit(mcode::Instruction(opcode, {m_sp, m_sp, m_imm_shifted, m_shift}));

        mcode::Operand m_imm_remainder = mcode::Operand::from_int_immediate(value & 0xFFF);
        emit(mcode::Instruction(opcode, {m_sp, m_sp, m_imm_remainder}));
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::InstrIter AAPCSCallingConv::fix_up_instr(
    mcode::BasicBlock &basic_block,
    mcode::InstrIter iter,
    TargetRegAnalyzer &analyzer
) {
    if (iter->get_opcode() != target::AArch64Opcode::LDR) {
        return iter;
    }

    mcode::Operand dest = iter->get_operand(0);
    mcode::Operand address = iter->get_operand(1);

    if (!address.is_stack_slot()) {
        return iter;
    }

    int index = iter->get_operand(1).get_stack_slot();
    int offset = basic_block.get_func()->get_stack_frame().get_stack_slot(index).get_offset();

    if (AArch64EncodingInfo::is_addr_offset_encodable(offset, dest.get_size())) {
        return iter;
    }

    iter = basic_block.replace(
        iter,
        mcode::Instruction(
            target::AArch64Opcode::MOV,
            {mcode::Operand::from_register(mcode::Register::from_physical(-1), 8),
             mcode::Operand::from_int_immediate(offset, 8)}
        )
    );

    basic_block.insert_after(
        iter,
        mcode::Instruction(
            target::AArch64Opcode::LDR,
            {
                dest,
                mcode::Operand::from_aarch64_addr(
                    target::AArch64Address::new_base_offset(
                        mcode::Register::from_physical(target::AArch64Register::SP),
                        mcode::Register::from_physical(-1)
                    ),
                    8
                ),
            }
        )
    );

    codegen::LateRegAlloc::Range range{.block = basic_block, .start = iter, .end = iter.get_next()};
    codegen::LateRegAlloc alloc(range, AArch64RegClass::GENERAL_PURPOSE, analyzer);
    mcode::PhysicalReg new_reg = AArch64Register::R20;

    iter->get_operand(0).set_to_register(mcode::Register::from_physical(new_reg));
    iter.get_next()->get_operand(1).set_to_aarch64_addr(target::AArch64Address::new_base_offset(
        mcode::Register::from_physical(target::AArch64Register::SP),
        mcode::Register::from_physical(new_reg)
    ));

    return iter.get_next();
}

bool AAPCSCallingConv::is_func_exit(mcode::Opcode opcode) {
    return opcode == AArch64Opcode::RET;
}

std::vector<mcode::ArgStorage> AAPCSCallingConv::get_arg_storage(const ssa::FunctionType &func_type) {
    std::vector<mcode::ArgStorage> result(func_type.params.size());

    unsigned general_reg_index = 0;
    unsigned float_reg_index = 0;
    unsigned stack_offset = 0;

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        mcode::ArgStorage storage;
        bool is_fp = func_type.params[i].is_floating_point();

        if (variant == Variant::APPLE && func_type.variadic && i >= func_type.first_variadic_index) {
            storage.in_reg = false;
            storage.stack_offset = stack_offset;
            stack_offset += 8;
        } else if (is_fp && float_reg_index < ARG_REGS_FP.size()) {
            storage.in_reg = true;
            storage.reg = ARG_REGS_FP[float_reg_index++];
        } else if (!is_fp && general_reg_index < ARG_REGS_INT.size()) {
            storage.in_reg = true;
            storage.reg = ARG_REGS_INT[general_reg_index++];
        } else {
            storage.in_reg = false;
            storage.stack_offset = stack_offset;
            stack_offset += 8;
        }

        result[i] = storage;
    }

    return result;
}

int AAPCSCallingConv::get_implicit_stack_bytes(mcode::Function * /*func*/) {
    // Frame pointer (x29) + link register (x30)
    return 16;
}

} // namespace target

} // namespace banjo
