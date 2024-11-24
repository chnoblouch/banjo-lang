#include "aapcs_calling_conv.hpp"

#include "aarch64_reg_analyzer.hpp"
#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/codegen/late_reg_alloc.hpp"
#include "banjo/codegen/machine_pass_utils.hpp"
#include "banjo/config/config.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_encoding_info.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_reg_analyzer.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/bit_operations.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

namespace banjo {

namespace target {

using namespace AArch64Register;

AAPCSCallingConv const AAPCSCallingConv::INSTANCE = AAPCSCallingConv();
std::vector<int> const AAPCSCallingConv::GENERAL_ARG_REGS = {R0, R1, R2, R3, R4, R5, R6, R7};
std::vector<int> const AAPCSCallingConv::FLOAT_ARG_REGS = {V0, V1, V2, V3, V4, V5, V6, V7};

AAPCSCallingConv::AAPCSCallingConv() {
    volatile_regs = {
        R0,  R1,  R2,  R3,  R4,  R5, R6, R7, R8, R9, R10, R11, R12, R13,
        R14, R15, R16, R17, R18, V0, V1, V2, V3, V4, V5,  V6,  V7,  SP,
    };
}

void AAPCSCallingConv::lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr) {
    ssa::Operand &func_operand = instr.get_operand(0);

    std::vector<ssa::Type> types;
    for (unsigned int i = 1; i < instr.get_operands().size(); i++) {
        types.push_back(instr.get_operand(i).get_type());
    }

    std::vector<mcode::ArgStorage> arg_storage = get_arg_storage(types);

    for (unsigned int i = 1; i < instr.get_operands().size(); i++) {
        ssa::Operand &operand = instr.get_operand(i);
        int size = lowerer.get_size(operand.get_type());
        bool is_float = operand.get_type().is_floating_point();
        mcode::ArgStorage cur_arg_storage = arg_storage[i - 1];

        mcode::Register reg = mcode::Register::from_virtual(-1);
        if (cur_arg_storage.in_reg) {
            reg = mcode::Register::from_physical(cur_arg_storage.reg);
        } else {
            reg = mcode::Register::from_virtual(lowerer.get_func().next_virtual_reg());
        }

        // TODO: Try moving immediately so the register allocator doesn't have to clean this up.
        lowerer.emit(mcode::Instruction(
            is_float ? AArch64Opcode::FMOV : AArch64Opcode::MOV,
            {mcode::Operand::from_register(reg, size), lowerer.lower_value(operand)},
            mcode::Instruction::FLAG_CALL_ARG
        ));

        if (!cur_arg_storage.in_reg) {
            int size = lowerer.get_size(operand.get_type());
            mcode::Register stack_slot = get_arg_reg(operand, i - 1, lowerer);
            lowerer.emit(mcode::Instruction(
                AArch64Opcode::STR,
                {mcode::Operand::from_register(reg, size), mcode::Operand::from_register(stack_slot, 8)}
            ));
        }
    }

    mcode::Opcode call_opcode;
    mcode::Operand call_operand;

    if (func_operand.is_func()) {
        call_opcode = AArch64Opcode::BL;
        call_operand = mcode::Operand::from_symbol(func_operand.get_func()->get_name(), 8);
    } else if (func_operand.is_symbol()) {
        call_opcode = AArch64Opcode::BL;
        call_operand = mcode::Operand::from_symbol(func_operand.get_symbol_name(), 8);
    } else if (func_operand.is_register()) {
        call_opcode = AArch64Opcode::BLR;
        call_operand = mcode::Operand::from_register(lowerer.lower_reg(func_operand.get_register()), 8);
    } else {
        ASSERT_UNREACHABLE;
    }

    lowerer.emit(mcode::Instruction(call_opcode, {call_operand}, mcode::Instruction::FLAG_CALL));

    if (instr.get_dest()) {
        bool is_fp = instr.get_operand(0).get_type().is_floating_point();
        mcode::Opcode opcode = is_fp ? AArch64Opcode::FMOV : AArch64Opcode::MOV;
        mcode::PhysicalReg return_reg = is_fp ? AArch64Register::V0 : AArch64Register::R0;
        unsigned return_size = lowerer.get_size(instr.get_operand(0).get_type());

        lowerer.emit(mcode::Instruction(
            opcode,
            {mcode::Operand::from_register(mcode::Register::from_virtual(*instr.get_dest()), return_size),
             mcode::Operand::from_register(mcode::Register::from_physical(return_reg), return_size)}
        ));
    }
}

mcode::Register AAPCSCallingConv::get_arg_reg(ssa::Operand &operand, int index, codegen::SSALowerer &lowerer) {
    if (index < GENERAL_ARG_REGS.size()) {
        long id = operand.get_type().is_floating_point() ? FLOAT_ARG_REGS[index] : GENERAL_ARG_REGS[index];
        return mcode::Register::from_physical(id);
    } else {
        int arg_slot_index = index - GENERAL_ARG_REGS.size();
        mcode::StackFrame &stack_frame = lowerer.get_machine_func()->get_stack_frame();

        if (stack_frame.get_call_arg_slot_indices().size() <= arg_slot_index) {
            mcode::StackSlot stack_slot(mcode::StackSlot::Type::CALL_ARG, 8, 1);
            stack_slot.set_call_arg_index(arg_slot_index);
            return mcode::Register::from_stack_slot(stack_frame.new_stack_slot(stack_slot));
        } else {
            int slot_index = stack_frame.get_call_arg_slot_indices()[arg_slot_index];
            return mcode::Register::from_stack_slot(slot_index);
        }
    }
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

    prolog.push_back(mcode::Instruction(
        AArch64Opcode::SUB,
        {mcode::Operand::from_register(sp, 8),
         mcode::Operand::from_register(sp, 8),
         mcode::Operand::from_immediate(std::to_string(size))},
        mcode::Instruction::FLAG_ALLOCA
    ));

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
            mcode::Operand::from_immediate(std::to_string(size))
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

    epilog.push_back(mcode::Instruction(
        AArch64Opcode::ADD,
        {mcode::Operand::from_register(sp, 8),
         mcode::Operand::from_register(sp, 8),
         mcode::Operand::from_immediate(std::to_string(size))}
    ));

    epilog.push_back(mcode::Instruction(
        AArch64Opcode::LDP,
        {mcode::Operand::from_register(fp, 8),
         mcode::Operand::from_register(lr, 8),
         mcode::Operand::from_aarch64_addr(AArch64Address::new_base(sp)),
         mcode::Operand::from_immediate("16")}
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
             mcode::Operand::from_immediate("16")}
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
                mcode::Operand::from_immediate(std::to_string(offset))
            })
        };
    } else {
        return {
            mcode::Instruction(AArch64Opcode::ADD, {
                mcode::Operand::from_register(sp, 8),
                mcode::Operand::from_register(sp, 8),
                mcode::Operand::from_immediate(std::to_string(size))
            }),
            mcode::Instruction(AArch64Opcode::LDP, {
                mcode::Operand::from_register(fp, 8),
                mcode::Operand::from_register(lr, 8),
                mcode::Operand::from_aarch64_addr(addr),
                mcode::Operand::from_immediate(std::to_string(32))
            })
        };
    }
    */
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
             mcode::Operand::from_immediate(std::to_string(offset), 8)}
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

std::vector<mcode::ArgStorage> AAPCSCallingConv::get_arg_storage(const std::vector<ssa::Type> &types) {
    std::vector<mcode::ArgStorage> result;
    result.resize(types.size());

    unsigned int general_reg_index = 0;
    unsigned int float_reg_index = 0;
    unsigned int stack_offset = 0;

    for (unsigned int i = 0; i < types.size(); i++) {
        mcode::ArgStorage storage;
        bool is_fp = types[i].is_floating_point();

        if (is_fp && float_reg_index < FLOAT_ARG_REGS.size()) {
            storage.in_reg = true;
            storage.reg = FLOAT_ARG_REGS[float_reg_index++];
        } else if (!is_fp && general_reg_index < GENERAL_ARG_REGS.size()) {
            storage.in_reg = true;
            storage.reg = GENERAL_ARG_REGS[general_reg_index++];
        } else {
            storage.in_reg = false;
            storage.stack_offset = stack_offset;
            stack_offset += 8;
        }

        result[i] = storage;
    }

    return result;
}

int AAPCSCallingConv::get_implicit_stack_bytes(mcode::Function *func) {
    // TODO
    return 0;
}

} // namespace target

} // namespace banjo
