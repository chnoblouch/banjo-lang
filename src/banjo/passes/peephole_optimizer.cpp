#include "peephole_optimizer.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/utils/bit_operations.hpp"

namespace banjo {

namespace passes {

PeepholeOptimizer::PeepholeOptimizer(target::Target *target) : Pass("peephole-opt", target) {}

void PeepholeOptimizer::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func, mod);
    }
}

void PeepholeOptimizer::run(ssa::Function *func, ssa::Module &mod) {
    for (ssa::BasicBlock &block : func->get_basic_blocks()) {
        run(block, *func, mod);
    }
}

void PeepholeOptimizer::run(ssa::BasicBlock &block, ssa::Function &func, ssa::Module &mod) {
    // TODO: canonicalization
    for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        std::optional<ssa::Value> value = Precomputing::precompute_result(*iter);
        if (value) {
            PassUtils::replace_in_block(block, *iter->get_dest(), *value);
            continue;
        }

        switch (iter->get_opcode()) {
            case ssa::Opcode::ADD:
            case ssa::Opcode::FADD: optimize_add(iter, block, func); break;
            case ssa::Opcode::SUB:
            case ssa::Opcode::FSUB: optimize_sub(iter, block, func); break;
            case ssa::Opcode::MUL: optimize_mul(iter, block, func); break;
            case ssa::Opcode::UDIV: optimize_udiv(iter, block, func); break;
            case ssa::Opcode::FMUL: optimize_fmul(iter, block, func); break;
            case ssa::Opcode::CALL: optimize_call(iter, block, func, mod); break;
            default: break;
        }
    }
}

void PeepholeOptimizer::optimize_add(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (is_zero(iter->get_operand(0))) {
        eliminate(iter, iter->get_operand(1), block, func);
    } else if (is_zero(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    }
}

void PeepholeOptimizer::optimize_sub(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (is_zero(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    }
}

void PeepholeOptimizer::optimize_mul(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
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
            ssa::Operand rhs = ssa::Operand::from_int_immediate(shift);
            iter = block.replace(iter, ssa::Instruction(ssa::Opcode::SHL, *iter->get_dest(), {lhs, rhs}));
        }
    }
}

void PeepholeOptimizer::optimize_udiv(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
    if (iter->get_operand(1).is_int_immediate()) {
        std::uint64_t value = iter->get_operand(1).get_int_immediate().to_bits();

        if (BitOperations::is_power_of_two(value)) {
            unsigned shift = BitOperations::get_first_bit_set(value);
            ssa::Operand lhs = iter->get_operand(0);
            ssa::Operand rhs = ssa::Operand::from_int_immediate(shift);
            iter = block.replace(iter, ssa::Instruction(ssa::Opcode::SHR, *iter->get_dest(), {lhs, rhs}));
        }
    }
}

void PeepholeOptimizer::optimize_fmul(ssa::InstrIter &iter, ssa::BasicBlock &block, ssa::Function &func) {
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

void PeepholeOptimizer::optimize_call(
    ssa::InstrIter &iter,
    ssa::BasicBlock &block,
    ssa::Function &func,
    ssa::Module &mod
) {
    ssa::Value &callee = iter->get_operand(0);
    if (!callee.is_extern_func()) {
        return;
    }

    if (callee.get_extern_func()->name == "memcpy") {
        if (iter->get_operands().size() == 4 && !iter->get_dest()) {
            ssa::Operand dst = iter->get_operand(1);
            ssa::Operand src = iter->get_operand(2);
            ssa::Operand size = iter->get_operand(3);

            if (!dst.is_register() || !src.is_register() || !size.is_int_immediate()) {
                return;
            }

            ssa::Operand size_as_type = ssa::Operand::from_type({
                ssa::Primitive::U8,
                static_cast<unsigned>(size.get_int_immediate().to_u64()),
            });

            ssa::InstrIter prev = iter.get_prev();
            block.replace(iter, {ssa::Opcode::COPY, {dst, src, size_as_type}});
            iter = prev;
        }
    } else if (callee.get_extern_func()->name == "sqrtf") {
        if (iter->get_dest() && iter->get_operands().size() == 2) {
            ssa::InstrIter prev = iter.get_prev();
            block.replace(iter, ssa::Instruction(ssa::Opcode::SQRT, iter->get_dest(), {iter->get_operand(1)}));
            iter = prev;
        }
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

void PeepholeOptimizer::eliminate(ssa::InstrIter &iter, ssa::Value val, ssa::BasicBlock &block, ssa::Function &func) {
    PassUtils::replace_in_func(func, *iter->get_dest(), val);

    ssa::InstrIter prev = iter.get_prev();
    block.remove(iter);
    iter = prev;
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
