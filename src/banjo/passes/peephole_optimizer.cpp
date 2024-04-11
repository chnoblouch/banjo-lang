#include "peephole_optimizer.hpp"

#include "ir/control_flow_graph.hpp"
#include "passes/pass_utils.hpp"
#include "passes/precomputing.hpp"
#include "utils/bit_operations.hpp"

namespace passes {

PeepholeOptimizer::PeepholeOptimizer(target::Target *target) : Pass("peephole-opt", target) {}

void PeepholeOptimizer::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(func);
    }
}

void PeepholeOptimizer::run(ir::Function *func) {
    for (ir::BasicBlock &block : func->get_basic_blocks()) {
        run(block, *func);
    }
}

void PeepholeOptimizer::run(ir::BasicBlock &block, ir::Function &func) {
    // TODO: canonicalization
    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        std::optional<ir::Value> value = Precomputing::precompute_result(*iter);
        if (value) {
            PassUtils::replace_in_block(block, *iter->get_dest(), *value);
            continue;
        }

        switch (iter->get_opcode()) {
            case ir::Opcode::ADD:
            case ir::Opcode::FADD: optimize_add(iter, block, func); break;
            case ir::Opcode::SUB:
            case ir::Opcode::FSUB: optimize_sub(iter, block, func); break;
            case ir::Opcode::MUL: optimize_mul(iter, block, func); break;
            case ir::Opcode::UDIV: optimize_udiv(iter, block, func); break;
            case ir::Opcode::FMUL: optimize_fmul(iter, block, func); break;
            default: break;
        }

        if (iter->get_opcode() == ir::Opcode::CALL) {
            if (iter->get_operand(0).is_extern_func() && iter->get_operand(0).get_extern_func_name() == "sqrtf") {
                if (iter->get_dest() && iter->get_operands().size() == 2) {
                    ir::InstrIter prev = iter.get_prev();
                    block.replace(iter, ir::Instruction(ir::Opcode::SQRT, iter->get_dest(), {iter->get_operand(1)}));
                    iter = prev;
                }
            }
        }
    }
}

void PeepholeOptimizer::optimize_add(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func) {
    if (is_zero(iter->get_operand(0))) {
        eliminate(iter, iter->get_operand(1), block, func);
    } else if (is_zero(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    }
}

void PeepholeOptimizer::optimize_sub(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func) {
    if (is_zero(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    }
}

void PeepholeOptimizer::optimize_mul(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func) {
    if (is_imm(iter->get_operand(0)) && !is_imm(iter->get_operand(1))) {
        ir::Operand tmp = iter->get_operand(0);
        iter->get_operand(0) = iter->get_operand(1);
        iter->get_operand(1) = tmp;
    }

    if (iter->get_operand(1).is_int_immediate()) {
        std::uint64_t value = iter->get_operand(1).get_int_immediate().to_bits();

        if (BitOperations::is_power_of_two(value)) {
            unsigned shift = BitOperations::get_first_bit_set(value);
            ir::Operand lhs = iter->get_operand(0);
            ir::Operand rhs = ir::Operand::from_int_immediate(shift);
            iter = block.replace(iter, ir::Instruction(ir::Opcode::SHL, *iter->get_dest(), {lhs, rhs}));
        }
    }
}

void PeepholeOptimizer::optimize_udiv(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func) {
    if (iter->get_operand(1).is_int_immediate()) {
        std::uint64_t value = iter->get_operand(1).get_int_immediate().to_bits();

        if (BitOperations::is_power_of_two(value)) {
            unsigned shift = BitOperations::get_first_bit_set(value);
            ir::Operand lhs = iter->get_operand(0);
            ir::Operand rhs = ir::Operand::from_int_immediate(shift);
            iter = block.replace(iter, ir::Instruction(ir::Opcode::SHR, *iter->get_dest(), {lhs, rhs}));
        }
    }
}

void PeepholeOptimizer::optimize_fmul(ir::InstrIter &iter, ir::BasicBlock &block, ir::Function &func) {
    if (is_float_one(iter->get_operand(0))) {
        eliminate(iter, iter->get_operand(1), block, func);
    } else if (is_float_one(iter->get_operand(1))) {
        eliminate(iter, iter->get_operand(0), block, func);
    } else if (is_imm(iter->get_operand(0)) && !is_imm(iter->get_operand(1))) {
        ir::Operand tmp = iter->get_operand(0);
        iter->get_operand(0) = iter->get_operand(1);
        iter->get_operand(1) = tmp;
    }
}

void PeepholeOptimizer::eliminate(ir::InstrIter &iter, ir::Value val, ir::BasicBlock &block, ir::Function &func) {
    PassUtils::replace_in_func(func, *iter->get_dest(), val);

    ir::InstrIter prev = iter.get_prev();
    block.remove(iter);
    iter = prev;
}

bool PeepholeOptimizer::is_imm(ir::Operand &operand) {
    return operand.is_immediate();
}

bool PeepholeOptimizer::is_zero(ir::Operand &operand) {
    if (operand.is_int_immediate()) {
        return operand.get_int_immediate() == 0;
    } else if (operand.is_fp_immediate()) {
        return operand.get_fp_immediate() == 0.0;
    } else {
        return false;
    }
}

bool PeepholeOptimizer::is_float_one(ir::Operand &operand) {
    return operand.is_fp_immediate() && operand.get_fp_immediate() == 1.0;
}

} // namespace passes
