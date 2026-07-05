#include "stack_layout.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

namespace banjo::passes {

StackLayout::StackLayout(target::Target &target) : target{target} {}

void StackLayout::build(ssa::Function &func) {
    this->func = &func;
    slots.clear();
    num_members = 0;

    for (ssa::BasicBlock &block : func) {
        for (ssa::Instruction &instr : block) {
            if (instr.get_opcode() != ssa::Opcode::ALLOCA) {
                continue;
            }

            ssa::VirtualRegister reg = *instr.get_dest();

            Slot slot{.members{}};
            collect_members(reg, slot, instr.get_operand(0).get_type(), 0);
            slots.emplace(reg, std::move(slot));
        }
    }
}

void StackLayout::collect_members(ssa::VirtualRegister slot_reg, Slot &slot, ssa::Type type, unsigned base_offset) {
    target::TargetDataLayout &data_layout = target.get_data_layout();

    if (type.is_struct()) {
        ssa::Structure &struct_ = *type.get_struct();

        for (unsigned i = 0; i < struct_.members.size(); i++) {
            ssa::Type type = struct_.members[i].type;
            unsigned offset = base_offset + data_layout.get_member_offset(&struct_, i);
            collect_members(slot_reg, slot, type, offset);
        }
    } else {
        slot.members.push_back(
            Member{
                .index = num_members,
                .slot = slot_reg,
                .offset = base_offset,
                .type = type,
            }
        );

        num_members += 1;
    }
}

StackLayout::Member *StackLayout::find_member(ssa::VirtualRegister reg) {
    target::TargetDataLayout &data_layout = target.get_data_layout();
    unsigned current_offset = 0;

    while (true) {
        auto slot = slots.find(reg);

        if (slot != slots.end()) {
            for (Member &member : slot->second.members) {
                if (member.offset == current_offset) {
                    return &member;
                }
            }
        }

        ssa::InstrIter def = PassUtils::find_def(*func, reg);
        if (!def) {
            break;
        }

        if (def->get_opcode() == ssa::Opcode::OFFSETPTR) {
            ssa::Value &base = def->get_operand(0);
            ssa::Value &offset = def->get_operand(1);
            ssa::Type type = def->get_operand(2).get_type();

            if (!base.is_register() || !offset.is_int_immediate()) {
                break;
            }

            unsigned type_size = static_cast<unsigned>(data_layout.get_size(type));
            current_offset += offset.get_int_immediate().to_s64() * type_size;
            reg = base.get_register();
        } else if (def->get_opcode() == ssa::Opcode::MEMBERPTR) {
            ssa::Structure &struct_ = *def->get_operand(0).get_type().get_struct();
            ssa::Value &base = def->get_operand(1);
            unsigned index = def->get_operand(2).get_int_immediate().to_u64();

            if (!base.is_register()) {
                break;
            }

            current_offset += static_cast<unsigned>(data_layout.get_member_offset(&struct_, index));
            reg = base.get_register();
        } else {
            break;
        }
    }

    return nullptr;
}

std::optional<ssa::VirtualRegister> StackLayout::find_base_slot(ssa::VirtualRegister reg) {
    while (!slots.contains(reg)) {
        ssa::InstrIter def = PassUtils::find_def(*func, reg);
        if (!def) {
            return {};
        }

        if (def->get_opcode() == ssa::Opcode::OFFSETPTR) {
            ssa::Operand &base = def->get_operand(0);
            if (!base.is_register()) {
                return {};
            }

            reg = base.get_register();
        } else if (def->get_opcode() == ssa::Opcode::MEMBERPTR) {
            ssa::Operand &base = def->get_operand(1);
            if (!base.is_register()) {
                return {};
            }

            reg = base.get_register();
        } else {
            return {};
        }
    }

    return reg;
}

} // namespace banjo::passes
