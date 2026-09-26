#include "sroa_pass_2.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/structure.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/utils/macros.hpp"

#define DEBUG_LOG log()

namespace banjo::passes {

SROAPass2::SROAPass2(target::Target *target) : Pass{"sroa", target} {}

void SROAPass2::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void SROAPass2::run(ssa::Function &func) {
    stack_slots.clear();
    stack_refs.clear();
    stack_ref_instrs.clear();
    stack_accesses.clear();

    for (ssa::BasicBlock &block : func.basic_blocks) {
        collect_stack_slots(block);
    }

    for (ssa::BasicBlock &block : func.basic_blocks) {
        collect_stack_refs(block);
    }

    for (StackSlot &slot : stack_slots) {
        if (!slot.splittable) {
            continue;
        }

        for (Member &member : slot.members) {
            ssa::VirtualRegister reg = func.next_virtual_reg();
            ssa::Operand type_operand = ssa::Operand::from_type(member.type);
            ssa::Instruction member_alloca{ssa::Opcode::ALLOCA, reg, {type_operand}};
            slot.alloca.block.insert_before(slot.alloca.instr, member_alloca);

            member.replacement = reg;
        }
    }

    if (is_logging()) {
        dump(func);
    }

    for (StackSlot &slot : stack_slots) {
        if (slot.splittable) {
            slot.alloca.block.remove(slot.alloca.instr);
        }
    }

    for (StackRefInstr &instr : stack_ref_instrs) {
        StackSlot &slot = stack_slots[instr.reference.slot_index];
        if (!slot.splittable) {
            continue;
        }

        instr.context.block.remove(instr.context.instr);
    }

    for (StackAccess &access : stack_accesses) {
        StackSlot &slot = stack_slots[access.reference.slot_index];
        if (!slot.splittable) {
            continue;
        }

        ssa::VirtualRegister replacement = *slot.members[access.reference.member_index].replacement;
        ssa::Operand addr_operand = ssa::Operand::from_register(replacement, ssa::Primitive::ADDR);
        access.operand = addr_operand;
    }
}

void SROAPass2::collect_stack_slots(ssa::BasicBlock &block) {
    for (ssa::InstrIter instr = block.begin(); instr != block.end(); ++instr) {
        if (instr->get_opcode() != ssa::Opcode::ALLOCA) {
            continue;
        }

        ssa::Type type = instr->get_operand(0).get_type();

        if (!type.is_struct_aggregate()) {
            continue;
        }

        StackSlot slot{
            .alloca{instr, block},
            .members{},
            .splittable = true,
        };

        if (!collect_members(slot, *type.get_struct(), 0)) {
            continue;
        }

        StackReference reference{
            .slot_index = static_cast<unsigned>(stack_slots.size()),
            .member_index = 0,
        };

        stack_slots.push_back(slot);
        stack_refs.insert(*instr->get_dest(), reference);
    }
}

bool SROAPass2::collect_members(StackSlot &slot, ssa::Structure &struct_, unsigned base_offset) {
    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    if (struct_.is_union || struct_.members.empty()) {
        return false;
    }

    for (unsigned i = 0; i < struct_.members.size(); i++) {
        ssa::Type type = struct_.members[i].type;

        if (type.get_array_length() != 1) {
            return false;
        }

        unsigned offset = base_offset + data_layout.get_member_offset(&struct_, i);

        if (type.is_primitive()) {
            slot.members.push_back(Member{.offset = offset, .type = type, .replacement{}});
        } else if (type.is_struct()) {
            if (!collect_members(slot, *type.get_struct(), offset)) {
                return false;
            }
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    return true;
}

void SROAPass2::collect_stack_refs(ssa::BasicBlock &block) {
    for (ssa::InstrIter instr = block.begin(); instr != block.end(); ++instr) {
        if (instr->get_opcode() == ssa::Opcode::MEMBERPTR) {
            collect_memberptr(instr, block);
        } else if (instr->get_opcode() == ssa::Opcode::OFFSETPTR) {
            collect_offsetptr(instr, block);
        } else if (instr->get_opcode() == ssa::Opcode::LOAD) {
            collect_load(*instr);
        } else if (instr->get_opcode() == ssa::Opcode::STORE) {
            collect_store(*instr);
        } else {
            PassUtils::iter_regs(instr->get_operands(), [this](ssa::VirtualRegister reg) {
                if (StackReference *reference = stack_refs.try_find(reg)) {
                    stack_slots[reference->slot_index].splittable = false;
                }
            });
        }
    }
}

void SROAPass2::collect_memberptr(ssa::InstrIter instr, ssa::BasicBlock &block) {
    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    ssa::Operand &base = instr->get_operand(1);
    if (!base.is_register()) {
        return;
    }

    ssa::Structure &struct_ = *instr->get_operand(0).get_type().get_struct();
    unsigned member_index = instr->get_operand(2).get_int_immediate().to_u64();
    unsigned member_offset = data_layout.get_member_offset(&struct_, member_index);
    analyze_sub_pointer({instr, block}, base.get_register(), member_offset);
}

void SROAPass2::collect_offsetptr(ssa::InstrIter instr, ssa::BasicBlock &block) {
    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    ssa::Operand &base = instr->get_operand(0);
    if (!base.is_register()) {
        return;
    }

    ssa::Operand &offset = instr->get_operand(1);
    if (!offset.is_int_immediate()) {
        return;
    }

    ssa::Type base_type = instr->get_operand(2).get_type();
    int member_offset = data_layout.get_size(base_type) * offset.get_int_immediate().to_s32();
    analyze_sub_pointer({instr, block}, base.get_register(), member_offset);
}

void SROAPass2::collect_load(ssa::Instruction &instr) {
    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    ssa::Operand &addr = instr.get_operand(1);
    if (!addr.is_register()) {
        return;
    }

    StackReference *stack_ref = stack_refs.try_find(addr.get_register());
    if (!stack_ref) {
        return;
    }

    stack_accesses.push_back({addr, *stack_ref});

    StackSlot &slot = stack_slots[stack_ref->slot_index];
    Member &member = slot.members[stack_ref->member_index];

    ssa::Type type = instr.get_operand(0).get_type();
    if (data_layout.get_size(type) > data_layout.get_size(member.type)) {
        slot.splittable = false;
    }
}

void SROAPass2::collect_store(ssa::Instruction &instr) {
    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    if (instr.get_operand(0).is_register()) {
        ssa::VirtualRegister reg = instr.get_operand(0).get_register();

        if (StackReference *reference = stack_refs.try_find(reg)) {
            stack_slots[reference->slot_index].splittable = false;
        }
    }

    ssa::Operand &addr = instr.get_operand(1);
    if (!addr.is_register()) {
        return;
    }

    StackReference *stack_ref = stack_refs.try_find(addr.get_register());
    if (!stack_ref) {
        return;
    }

    stack_accesses.push_back({addr, *stack_ref});

    StackSlot &slot = stack_slots[stack_ref->slot_index];
    Member &member = slot.members[stack_ref->member_index];

    ssa::Type type = instr.get_operand(0).get_type();
    if (data_layout.get_size(type) > data_layout.get_size(member.type)) {
        slot.splittable = false;
    }
}

void SROAPass2::analyze_sub_pointer(InstrContext context, ssa::VirtualRegister base, int offset) {
    StackReference *base_ref = stack_refs.try_find(base);
    if (!base_ref) {
        return;
    }

    StackSlot &slot = stack_slots[base_ref->slot_index];
    unsigned total_offset = slot.members[base_ref->member_index].offset + offset;

    for (unsigned i = 0; i < slot.members.size(); i++) {
        if (slot.members[i].offset != total_offset) {
            continue;
        }

        StackReference reference{
            .slot_index = base_ref->slot_index,
            .member_index = i,
        };

        stack_refs.insert(*context.instr->get_dest(), reference);
        stack_ref_instrs.push_back({context, reference});
        return;
    }

    slot.splittable = false;
}

void SROAPass2::dump(ssa::Function &func) {
    DEBUG_LOG << func.name << ":\n";

    for (unsigned i = 0; i < stack_slots.size(); i++) {
        StackSlot &slot = stack_slots[i];

        DEBUG_LOG << "  slot " << i << " {\n";
        DEBUG_LOG << "    alloca: %" << *slot.alloca.instr->get_dest() << "\n";
        DEBUG_LOG << "    splittable: " << (slot.splittable ? "true" : "false") << "\n";
        DEBUG_LOG << "    members: [\n";

        for (Member &member : slot.members) {
            DEBUG_LOG << "      offset: " << member.offset << ", type: ";
            dump_primitive(member.type.get_primitive());

            if (member.replacement) {
                DEBUG_LOG << ", replacement: %" << *member.replacement;
            }

            DEBUG_LOG << "\n";
        }

        DEBUG_LOG << "    ]\n";
        DEBUG_LOG << "  }\n\n";
    }

    DEBUG_LOG << "  references: [\n";

    for (const auto &[reg, ref] : stack_refs) {
        DEBUG_LOG << "    register %" << reg << ": ";
        DEBUG_LOG << "slot " << ref.slot_index << ", ";
        DEBUG_LOG << "member " << ref.member_index << "\n";
    }

    DEBUG_LOG << "  ]\n\n";
}

void SROAPass2::dump_primitive(ssa::Primitive primitive) {
    switch (primitive) {
        case ssa::Primitive::VOID: ASSERT_UNREACHABLE;
        case ssa::Primitive::I8: DEBUG_LOG << "i8"; break;
        case ssa::Primitive::I16: DEBUG_LOG << "i16"; break;
        case ssa::Primitive::I32: DEBUG_LOG << "i32"; break;
        case ssa::Primitive::I64: DEBUG_LOG << "i64"; break;
        case ssa::Primitive::U8: DEBUG_LOG << "u8"; break;
        case ssa::Primitive::U16: DEBUG_LOG << "u16"; break;
        case ssa::Primitive::U32: DEBUG_LOG << "u32"; break;
        case ssa::Primitive::U64: DEBUG_LOG << "u64"; break;
        case ssa::Primitive::F32: DEBUG_LOG << "f32"; break;
        case ssa::Primitive::F64: DEBUG_LOG << "f64"; break;
        case ssa::Primitive::ADDR: DEBUG_LOG << "addr"; break;
    }
}

} // namespace banjo::passes
