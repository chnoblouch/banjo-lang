#include "peephole_optimizer.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/structure.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/utils/bit_operations.hpp"

namespace banjo {

namespace passes {

PeepholeOptimizer::PeepholeOptimizer(target::Target *target) : Pass("peephole-opt", target) {}

void PeepholeOptimizer::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func);
    }
}

void PeepholeOptimizer::run(ssa::Function *func) {
    this->func = func;
    stack_slots.clear();

    for (ssa::BasicBlock &block : func->get_basic_blocks()) {
        discard_stack_slot_values();
        run(block, *func);
    }
}

void PeepholeOptimizer::run(ssa::BasicBlock &block, ssa::Function &func) {
    // TODO: canonicalization
    for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        std::optional<ssa::Value> value = Precomputing::precompute_result(*iter);
        if (value) {
            PassUtils::replace_in_block(block, *iter->get_dest(), *value);
            continue;
        }

        switch (iter->get_opcode()) {
            case ssa::Opcode::ALLOCA: process_alloca(iter); break;
            case ssa::Opcode::LOAD: process_load(iter, block, func); break;
            case ssa::Opcode::STORE: process_store(iter); break;
            case ssa::Opcode::ADD:
            case ssa::Opcode::FADD: process_add(iter, block, func); break;
            case ssa::Opcode::SUB:
            case ssa::Opcode::FSUB: process_sub(iter, block, func); break;
            case ssa::Opcode::MUL: process_mul(iter, block); break;
            case ssa::Opcode::UDIV: process_udiv(iter, block); break;
            case ssa::Opcode::FMUL: process_fmul(iter, block, func); break;
            case ssa::Opcode::CALL: process_call(iter, block, func); break;
            case ssa::Opcode::OFFSETPTR: process_offsetptr(iter, block, func); break;
            case ssa::Opcode::MEMBERPTR: process_memberptr(iter, block, func); break;
            default: break;
        }

        if (iter->get_opcode() != ssa::Opcode::LOAD && iter->get_opcode() != ssa::Opcode::STORE &&
            iter->might_access_memory()) {
            discard_stack_slot_values();
        }
    }
}

void PeepholeOptimizer::process_alloca(ssa::InstrIter &iter) {
    StackSlotState state{.members{}};
    collect_members(state, iter->get_operand(0).get_type(), 0);
    stack_slots.emplace(*iter->get_dest(), std::move(state));
}

void PeepholeOptimizer::process_load(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    ssa::Type type = iter->get_operand(0).get_type();
    ssa::Value &addr = iter->get_operand(1);

    if (!addr.is_register()) {
        return;
    }

    StackSlotMemberState *member = find_stack_slot_member(addr.get_register());

    if (!member || type != member->type) {
        return;
    }

    if (member->value) {
        eliminate(iter, *member->value, block, func);
    }
}

void PeepholeOptimizer::process_store(ssa::InstrIter &iter) {
    ssa::Value &value = iter->get_operand(0);
    ssa::Value &addr = iter->get_operand(1);

    if (!addr.is_register()) {
        discard_stack_slot_values();
        return;
    }

    StackSlotMemberState *member = find_stack_slot_member(addr.get_register());

    if (!member || value.get_type() != member->type) {
        discard_stack_slot_values();
        return;
    }

    member->value = value;
}

void PeepholeOptimizer::process_add(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (is_zero(iter->get_operand(0))) {
        eliminate(iter, iter->get_operand(1), block, func);
    } else if (is_zero(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    }
}

void PeepholeOptimizer::process_sub(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (is_zero(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    }
}

void PeepholeOptimizer::process_mul(ssa::InstrIter &iter, ssa::BasicBlock &block) {
    if (is_imm(iter->get_operand(0)) && !is_imm(iter->get_operand(1))) {
        ssa::Operand tmp = iter->get_operand(0);
        iter->get_operand(0) = iter->get_operand(1);
        iter->get_operand(1) = tmp;
    }

    if (iter->get_operand(1).is_int_immediate()) {
        std::uint64_t value = iter->get_operand(1).get_int_immediate().to_bits();

        if (BitOperations::is_power_of_two(value)) {
            unsigned shift = BitOperations::get_first_bit_set(value);
            ssa::Operand lhs = iter->get_operand(0);
            ssa::Operand rhs = ssa::Operand::from_int_immediate(shift, lhs.get_type());
            iter = block.replace(iter, ssa::Instruction(ssa::Opcode::SHL, *iter->get_dest(), {lhs, rhs}));
        }
    }
}

void PeepholeOptimizer::process_udiv(ssa::InstrIter &iter, ssa::BasicBlock &block) {
    if (iter->get_operand(1).is_int_immediate()) {
        std::uint64_t value = iter->get_operand(1).get_int_immediate().to_bits();

        if (BitOperations::is_power_of_two(value)) {
            unsigned shift = BitOperations::get_first_bit_set(value);
            ssa::Operand lhs = iter->get_operand(0);
            ssa::Operand rhs = ssa::Operand::from_int_immediate(shift, lhs.get_type());
            iter = block.replace(iter, ssa::Instruction(ssa::Opcode::SHR, *iter->get_dest(), {lhs, rhs}));
        }
    }
}

void PeepholeOptimizer::process_fmul(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (is_float_one(iter->get_operand(0))) {
        eliminate(iter, iter->get_operand(1), block, func);
    } else if (is_float_one(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    } else if (is_imm(iter->get_operand(0)) && !is_imm(iter->get_operand(1))) {
        ssa::Operand tmp = iter->get_operand(0);
        iter->get_operand(0) = iter->get_operand(1);
        iter->get_operand(1) = tmp;
    }
}

void PeepholeOptimizer::process_call(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    ssa::Value &callee = iter->get_operand(0);
    if (!callee.is_extern_func()) {
        return;
    }

    if (callee.get_extern_func()->name == "memcpy") {
        if (iter->get_operands().size() == 4 && !iter->get_dest()) {
            ssa::Operand dst = iter->get_operand(1);
            ssa::Operand src = iter->get_operand(2);
            ssa::Operand size_operand = iter->get_operand(3);

            if (!dst.is_register() || !src.is_register()) {
                return;
            }

            if (!size_operand.is_int_immediate() || size_operand.get_int_immediate().is_negative()) {
                return;
            }

            unsigned size = static_cast<unsigned>(size_operand.get_int_immediate().to_u64());

            unsigned usize = get_target()->get_data_layout().get_register_size();
            ssa::Operand size_as_type = ssa::Operand::from_type({ssa::Primitive::U8, size});

            if (size < usize && (size == 1 || size == 2 || size == 4 || size == 8)) {
                ssa::VirtualRegister tmp_reg = func.next_virtual_reg();
                ssa::Operand tmp_reg_operand = ssa::Operand::from_register(tmp_reg, size_as_type.get_type());

                ssa::InstrIter load_instr = block.replace(iter, {ssa::Opcode::LOAD, tmp_reg, {size_as_type, src}});
                ssa::InstrIter store_instr =
                    block.insert_after(load_instr, {ssa::Opcode::STORE, {tmp_reg_operand, dst}});
                iter = store_instr;
                return;
            }

            ssa::InstrIter prev = iter.get_prev();
            block.replace(iter, {ssa::Opcode::COPY, {dst, src, size_as_type}});
            iter = prev;
        }
    } else if (callee.get_extern_func()->name == "sqrtf") {
        // if (iter->get_dest() && iter->get_operands().size() == 2) {
        //     ssa::InstrIter prev = iter.get_prev();
        //     block.replace(iter, ssa::Instruction(ssa::Opcode::SQRT, iter->get_dest(), {iter->get_operand(1)}));
        //     iter = prev;
        // }
    } else if (callee.get_extern_func()->name == "strlen") {
        if (!iter->get_operand(1).is_global()) {
            return;
        }

        std::string string = std::get<std::string>(iter->get_operand(1).get_global()->initial_value);
        unsigned string_length = string.size() - 1;
        ssa::Value value = ssa::Value::from_int_immediate(string_length, ssa::Primitive::U64);
        eliminate(iter, value, block, func);
    }
}

void PeepholeOptimizer::process_offsetptr(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    // ssa::Value &base = iter->get_operand(0);
    // ssa::Value &offset = iter->get_operand(1);

    // if (offset.is_int_immediate() && offset.get_int_immediate() == 0) {
    //     eliminate(iter, base, block, func);
    // }
}

void PeepholeOptimizer::process_memberptr(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    // ssa::Value &base = iter->get_operand(1);
    // unsigned index = iter->get_operand(2).get_int_immediate().to_u64();

    // if (index == 0) {
    //     eliminate(iter, base, block, func);
    // }
}

void PeepholeOptimizer::eliminate(ssa::InstrIter &iter, ssa::Value val, ssa::BasicBlock &block, ssa::Function &func) {
    PassUtils::replace_in_func(func, *iter->get_dest(), val);

    ssa::InstrIter prev = iter.get_prev();
    block.remove(iter);
    iter = prev;
}

void PeepholeOptimizer::collect_members(StackSlotState &slot_state, ssa::Type type, unsigned base_offset) {
    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    if (type.is_struct()) {
        ssa::Structure &struct_ = *type.get_struct();

        for (unsigned i = 0; i < struct_.members.size(); i++) {
            ssa::Type type = struct_.members[i].type;
            unsigned offset = base_offset + data_layout.get_member_offset(&struct_, i);
            collect_members(slot_state, type, offset);
        }
    } else {
        slot_state.members.push_back(
            StackSlotMemberState{
                .offset = base_offset,
                .type = type,
                .value{},
            }
        );
    }
}

PeepholeOptimizer::StackSlotMemberState *PeepholeOptimizer::find_stack_slot_member(ssa::VirtualRegister reg) {
    target::TargetDataLayout &data_layout = get_target()->get_data_layout();
    unsigned current_offset = 0;

    while (true) {
        auto stack_slot = stack_slots.find(reg);

        if (stack_slot != stack_slots.end()) {
            for (StackSlotMemberState &member : stack_slot->second.members) {
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
            current_offset += offset.get_int_immediate().to_unsigned() * type_size;
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

void PeepholeOptimizer::discard_stack_slot_values() {
    for (auto &iter : stack_slots) {
        for (StackSlotMemberState &member : iter.second.members) {
            member.value = {};
        }
    }
}

bool PeepholeOptimizer::is_imm(ssa::Operand &operand) {
    return operand.is_immediate();
}

bool PeepholeOptimizer::is_zero(ssa::Operand &operand) {
    if (operand.is_int_immediate()) {
        return operand.get_int_immediate() == 0;
    } else if (operand.is_fp_immediate()) {
        return operand.get_fp_immediate() == 0.0;
    } else {
        return false;
    }
}

bool PeepholeOptimizer::is_float_one(ssa::Operand &operand) {
    return operand.is_fp_immediate() && operand.get_fp_immediate() == 1.0;
}

} // namespace passes

} // namespace banjo
