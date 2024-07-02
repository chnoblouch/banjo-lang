#include "precomputing.hpp"

#include "banjo/passes/pass_utils.hpp"

#include <cmath>

namespace banjo {

namespace passes {

std::optional<ir::Value> precompute_binary_int_operation(
    ir::Instruction &instr,
    std::function<LargeInt(const LargeInt &lhs, const LargeInt &rhs)> compute_func
) {
    const ir::Value &lhs = instr.get_operand(0);
    const ir::Value &rhs = instr.get_operand(1);

    if (lhs.is_int_immediate() && rhs.is_int_immediate()) {
        LargeInt result = compute_func(lhs.get_int_immediate(), rhs.get_int_immediate());
        return ir::Value::from_int_immediate(result, lhs.get_type());
    } else {
        return {};
    }
}

std::optional<ir::Value> precompute_binary_fp_operation(
    ir::Instruction &instr,
    std::function<double(const double &lhs, const double &rhs)> compute_func
) {
    const ir::Value &lhs = instr.get_operand(0);
    const ir::Value &rhs = instr.get_operand(1);

    if (lhs.is_fp_immediate() && rhs.is_fp_immediate()) {
        double result = compute_func(lhs.get_fp_immediate(), rhs.get_fp_immediate());
        return ir::Value::from_fp_immediate(result, lhs.get_type());
    } else {
        return {};
    }
}

void Precomputing::precompute_instrs(ir::Function &func) {
    for (ir::BasicBlock &block : func) {
        for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
            if (iter->get_opcode() == ir::Opcode::CJMP) {
                if (iter->get_operand(0).is_immediate() && iter->get_operand(2).is_immediate()) {
                    bool result = precompute_cmp(
                        iter->get_operand(0),
                        iter->get_operand(2),
                        iter->get_operand(1).get_comparison()
                    );
                    ir::Value target = iter->get_operand(result ? 3 : 4);
                    iter = block.replace(iter, ir::Instruction(ir::Opcode::JMP, {target}));
                }
            } else {
                std::optional<ir::Value> precomputed_result = Precomputing::precompute_result(*iter);
                if (precomputed_result) {
                    PassUtils::replace_in_func(func, *iter->get_dest(), *precomputed_result);
                    ir::InstrIter new_iter = iter.get_prev();
                    block.remove(iter);
                    iter = new_iter;
                    continue;
                }
            }
        }
    }
}

std::optional<ir::Value> Precomputing::precompute_result(ir::Instruction &instr) {
    switch (instr.get_opcode()) {
        case ir::Opcode::ADD: return precompute_add(instr);
        case ir::Opcode::SUB: return precompute_sub(instr);
        case ir::Opcode::MUL: return precompute_mul(instr);
        case ir::Opcode::SDIV: return precompute_sdiv(instr);
        case ir::Opcode::SREM: return precompute_srem(instr);
        case ir::Opcode::UDIV: return precompute_udiv(instr);
        case ir::Opcode::UREM: return precompute_urem(instr);
        case ir::Opcode::FADD: return precompute_fadd(instr);
        case ir::Opcode::FSUB: return precompute_fsub(instr);
        case ir::Opcode::FMUL: return precompute_fmul(instr);
        case ir::Opcode::FDIV: return precompute_fdiv(instr);
        case ir::Opcode::AND: return precompute_and(instr);
        case ir::Opcode::OR: return precompute_or(instr);
        case ir::Opcode::XOR: return precompute_xor(instr);
        case ir::Opcode::SHL: return precompute_shl(instr);
        case ir::Opcode::SHR: return precompute_shr(instr);
        case ir::Opcode::SELECT: return precompute_select(instr);
        case ir::Opcode::SEXTEND: return precompute_extend(instr);
        case ir::Opcode::UEXTEND: return precompute_extend(instr);
        case ir::Opcode::UTOF: return precompute_itof(instr);
        case ir::Opcode::STOF: return precompute_itof(instr);
        case ir::Opcode::SQRT: return precompute_sqrt(instr);
        default: return {};
    }
}

std::optional<ir::Value> Precomputing::precompute_add(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs + rhs; });
}

std::optional<ir::Value> Precomputing::precompute_sub(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs - rhs; });
}

std::optional<ir::Value> Precomputing::precompute_mul(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs * rhs; });
}

std::optional<ir::Value> Precomputing::precompute_sdiv(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs / rhs; });
}

std::optional<ir::Value> Precomputing::precompute_srem(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs % rhs; });
}

std::optional<ir::Value> Precomputing::precompute_udiv(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs / rhs; });
}

std::optional<ir::Value> Precomputing::precompute_urem(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs % rhs; });
}

std::optional<ir::Value> Precomputing::precompute_fadd(ir::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs + rhs; });
}

std::optional<ir::Value> Precomputing::precompute_fsub(ir::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs - rhs; });
}

std::optional<ir::Value> Precomputing::precompute_fmul(ir::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs * rhs; });
}

std::optional<ir::Value> Precomputing::precompute_fdiv(ir::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs / rhs; });
}

std::optional<ir::Value> Precomputing::precompute_and(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs & rhs; });
}

std::optional<ir::Value> Precomputing::precompute_or(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs | rhs; });
}

std::optional<ir::Value> Precomputing::precompute_xor(ir::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs ^ rhs; });
}

std::optional<ir::Value> Precomputing::precompute_shl(ir::Instruction &instr) {
    // FIXME

    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) {
        return LargeInt{(int)(lhs.to_s64() << rhs.to_s64())};
    });
}

std::optional<ir::Value> Precomputing::precompute_shr(ir::Instruction &instr) {
    // FIXME

    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) {
        return LargeInt{(int)(lhs.to_s64() >> rhs.to_s64())};
    });
}

std::optional<ir::Value> Precomputing::precompute_select(ir::Instruction &instr) {
    const ir::Value &cond_lhs = instr.get_operand(0);
    const ir::Comparison comparison = instr.get_operand(1).get_comparison();
    const ir::Value &cond_rhs = instr.get_operand(2);
    const ir::Value &true_val = instr.get_operand(3);
    const ir::Value &false_val = instr.get_operand(4);

    if (cond_lhs.is_immediate() && cond_rhs.is_immediate()) {
        bool result = precompute_cmp(cond_lhs, cond_rhs, comparison);
        return result ? true_val : false_val;
    } else {
        return {};
    }
}

std::optional<ir::Value> Precomputing::precompute_extend(ir::Instruction &instr) {
    const ir::Value &value = instr.get_operand(0);
    const ir::Type &type = instr.get_operand(1).get_type();

    if (value.is_int_immediate()) {
        ir::Value extended = value;
        extended.set_type(type);
        return extended;
    } else {
        return {};
    }
}

std::optional<ir::Value> Precomputing::precompute_itof(ir::Instruction &instr) {
    const ir::Value &value = instr.get_operand(0);
    const ir::Type &type = instr.get_operand(1).get_type();

    if (value.is_int_immediate()) {
        double fp_value = (double)value.get_int_immediate().to_s64();
        return ir::Value::from_fp_immediate(fp_value, type);
    } else {
        return {};
    }
}

std::optional<ir::Value> Precomputing::precompute_sqrt(ir::Instruction &instr) {
    const ir::Value &value = instr.get_operand(0);

    if (value.is_fp_immediate()) {
        double sqrt = std::sqrt(value.get_fp_immediate());
        return ir::Operand::from_fp_immediate(sqrt, value.get_type());
    } else {
        return {};
    }
}

bool Precomputing::precompute_cmp(const ir::Value &lhs, const ir::Value &rhs, ir::Comparison comparison) {
    switch (comparison) {
        case ir::Comparison::EQ: return lhs.get_int_immediate() == rhs.get_int_immediate();
        case ir::Comparison::NE: return lhs.get_int_immediate() != rhs.get_int_immediate();
        case ir::Comparison::UGT: return lhs.get_int_immediate() > rhs.get_int_immediate();
        case ir::Comparison::UGE: return lhs.get_int_immediate() >= rhs.get_int_immediate();
        case ir::Comparison::ULT: return lhs.get_int_immediate() < rhs.get_int_immediate();
        case ir::Comparison::ULE: return lhs.get_int_immediate() <= rhs.get_int_immediate();
        case ir::Comparison::SGT: return lhs.get_int_immediate() > rhs.get_int_immediate();
        case ir::Comparison::SGE: return lhs.get_int_immediate() >= rhs.get_int_immediate();
        case ir::Comparison::SLT: return lhs.get_int_immediate() < rhs.get_int_immediate();
        case ir::Comparison::SLE: return lhs.get_int_immediate() <= rhs.get_int_immediate();
        case ir::Comparison::FEQ: return lhs.get_fp_immediate() == rhs.get_fp_immediate();
        case ir::Comparison::FNE: return lhs.get_fp_immediate() != rhs.get_fp_immediate();
        case ir::Comparison::FGT: return lhs.get_fp_immediate() > rhs.get_fp_immediate();
        case ir::Comparison::FGE: return lhs.get_fp_immediate() >= rhs.get_fp_immediate();
        case ir::Comparison::FLT: return lhs.get_fp_immediate() < rhs.get_fp_immediate();
        case ir::Comparison::FLE: return lhs.get_fp_immediate() <= rhs.get_fp_immediate();
    }
}

} // namespace passes

} // namespace banjo
