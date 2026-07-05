#include "peephole_optimizer.hpp"

#include "banjo/passes/analysis/stack_layout.hpp"
#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/structure.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/utils/bit_operations.hpp"

namespace banjo::passes {

PeepholeOptimizer::PeepholeOptimizer(target::Target *target) : Pass{"peephole-opt", target}, stack_layout{*target} {}

void PeepholeOptimizer::run(ssa::Module &mod) {
    this->mod = &mod;

    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void PeepholeOptimizer::run(ssa::Function &func) {
    this->func = &func;
    stack_layout.build(func);

    for (ssa::BasicBlock &block : func.get_basic_blocks()) {
        discard_stack_values();
        run(block, func);
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
            discard_stack_values();
        }
    }
}

void PeepholeOptimizer::process_load(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    ssa::Type type = iter->get_operand(0).get_type();
    ssa::Value &addr = iter->get_operand(1);

    if (!addr.is_register()) {
        return;
    }

    StackLayout::Member *member = stack_layout.find_member(addr.get_register());

    if (!member || !types_compatible(type, member->type)) {
        return;
    }

    if (std::optional<ssa::Value> value = stack_values[member->index]) {
        eliminate(iter, *value, block, func);
    }
}

void PeepholeOptimizer::process_store(ssa::InstrIter &iter) {
    ssa::Value &value = iter->get_operand(0);
    ssa::Value &addr = iter->get_operand(1);

    if (!addr.is_register()) {
        discard_stack_values();
        return;
    }

    StackLayout::Member *member = stack_layout.find_member(addr.get_register());

    if (!member || !types_compatible(value.get_type(), member->type)) {
        discard_stack_values();
        return;
    }

    stack_values[member->index] = value;
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
        process_memcpy(iter, block, func);
    } else if (callee.get_extern_func()->name == "memmove") {
        process_memmove(iter, block, func);
    } else if (callee.get_extern_func()->name == "sqrtf") {
        // if (iter->get_dest() && iter->get_operands().size() == 2) {
        //     ssa::InstrIter prev = iter.get_prev();
        //     block.replace(iter, ssa::Instruction(ssa::Opcode::SQRT, iter->get_dest(), {iter->get_operand(1)}));
        //     iter = prev;
        // }
    } else if (callee.get_extern_func()->name == "strlen") {
        process_strlen(iter, block, func);
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

void PeepholeOptimizer::process_memcpy(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (iter->get_operands().size() != 4 || iter->get_dest()) {
        return;
    }

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
        ssa::InstrIter store_instr = block.insert_after(load_instr, {ssa::Opcode::STORE, {tmp_reg_operand, dst}});
        iter = store_instr;
        return;
    }

    ssa::InstrIter prev = iter.get_prev();
    block.replace(iter, {ssa::Opcode::COPY, {dst, src, size_as_type}});
    iter = prev;
}

void PeepholeOptimizer::process_memmove(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (iter->get_operands().size() != 4 || iter->get_dest()) {
        return;
    }

    ssa::Value &dst = iter->get_operand(1);
    ssa::Value &src = iter->get_operand(2);
    ssa::Value &size_operand = iter->get_operand(3);

    if (!dst.is_register() || !src.is_register() || !size_operand.is_immediate()) {
        return;
    }

    unsigned size = size_operand.get_int_immediate().to_u64();

    if (!stack_layout.is_non_overlapping(dst.get_register(), src.get_register(), size)) {
        return;
    }

    ssa::FunctionDecl *memcpy_func;

    for (ssa::FunctionDecl *extern_func : mod->get_external_functions()) {
        if (extern_func->name == "memcpy") {
            memcpy_func = extern_func;
            break;
        }
    }

    iter->get_operand(0) = ssa::Operand::from_extern_func(memcpy_func);
    process_memcpy(iter, block, func);
}

void PeepholeOptimizer::process_strlen(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (iter->get_operands().size() != 2 || !iter->get_dest() || !iter->get_operand(1).is_global()) {
        return;
    }

    std::string string = std::get<std::string>(iter->get_operand(1).get_global()->initial_value);
    unsigned string_length = string.size() - 1;
    ssa::Value value = ssa::Value::from_int_immediate(string_length, ssa::Primitive::U64);
    eliminate(iter, value, block, func);
}

void PeepholeOptimizer::eliminate(ssa::InstrIter &iter, ssa::Value val, ssa::BasicBlock &block, ssa::Function &func) {
    PassUtils::replace_in_func(func, *iter->get_dest(), val);

    ssa::InstrIter prev = iter.get_prev();
    block.remove(iter);
    iter = prev;
}

void PeepholeOptimizer::discard_stack_values() {
    stack_values = {stack_layout.get_num_members(), std::optional<ssa::Value>{}};
}

bool PeepholeOptimizer::types_compatible(ssa::Type a, ssa::Type b) {
    // TODO: Move this into the backend.

    if (a.is_struct() || b.is_struct()) {
        return false;
    }

    if (a.get_primitive() == b.get_primitive()) {
        return true;
    }

    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    unsigned size_a = data_layout.get_size(a);
    unsigned size_b = data_layout.get_size(b);
    return size_a == size_b && a.is_floating_point() == b.is_floating_point();
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

} // namespace banjo::passes
