#include "precomputing.hpp"

#include "banjo/passes/pass_utils.hpp"

#include <cmath>

namespace banjo {

namespace passes {

std::optional<ssa::Value> precompute_binary_int_operation(
    ssa::Instruction &instr,
    std::function<LargeInt(const LargeInt &lhs, const LargeInt &rhs)> compute_func
) {
    const ssa::Value &lhs = instr.get_operand(0);
    const ssa::Value &rhs = instr.get_operand(1);

    if (lhs.is_int_immediate() && rhs.is_int_immediate()) {
        LargeInt result = compute_func(lhs.get_int_immediate(), rhs.get_int_immediate());
        return ssa::Value::from_int_immediate(result, lhs.get_type());
    } else {
        return {};
    }
}

std::optional<ssa::Value> precompute_binary_fp_operation(
    ssa::Instruction &instr,
    std::function<double(const double &lhs, const double &rhs)> compute_func
) {
    const ssa::Value &lhs = instr.get_operand(0);
    const ssa::Value &rhs = instr.get_operand(1);

    if (lhs.is_fp_immediate() && rhs.is_fp_immediate()) {
        double result = compute_func(lhs.get_fp_immediate(), rhs.get_fp_immediate());
        return ssa::Value::from_fp_immediate(result, lhs.get_type());
    } else {
        return {};
    }
}

void Precomputing::precompute_instrs(ssa::Function &func) {
    for (ssa::BasicBlock &block : func) {
        for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
            if (iter->get_opcode() == ssa::Opcode::CJMP) {
                if (iter->get_operand(0).is_immediate() && iter->get_operand(2).is_immediate()) {
                    bool result = precompute_cmp(
                        iter->get_operand(0),
                        iter->get_operand(2),
                        iter->get_operand(1).get_comparison()
                    );
                    ssa::Value target = iter->get_operand(result ? 3 : 4);
                    iter = block.replace(iter, ssa::Instruction(ssa::Opcode::JMP, {target}));
                }
            } else {
                std::optional<ssa::Value> precomputed_result = Precomputing::precompute_result(*iter);
                if (precomputed_result) {
                    PassUtils::replace_in_func(func, *iter->get_dest(), *precomputed_result);
                    ssa::InstrIter new_iter = iter.get_prev();
                    block.remove(iter);
                    iter = new_iter;
                    continue;
                }
            }
        }
    }
}

std::optional<ssa::Value> Precomputing::precompute_result(ssa::Instruction &instr) {
    switch (instr.get_opcode()) {
        case ssa::Opcode::ADD: return precompute_add(instr);
        case ssa::Opcode::SUB: return precompute_sub(instr);
        case ssa::Opcode::MUL: return precompute_mul(instr);
        case ssa::Opcode::SDIV: return precompute_sdiv(instr);
        case ssa::Opcode::SREM: return precompute_srem(instr);
        case ssa::Opcode::UDIV: return precompute_udiv(instr);
        case ssa::Opcode::UREM: return precompute_urem(instr);
        case ssa::Opcode::FADD: return precompute_fadd(instr);
        case ssa::Opcode::FSUB: return precompute_fsub(instr);
        case ssa::Opcode::FMUL: return precompute_fmul(instr);
        case ssa::Opcode::FDIV: return precompute_fdiv(instr);
        case ssa::Opcode::AND: return precompute_and(instr);
        case ssa::Opcode::OR: return precompute_or(instr);
        case ssa::Opcode::XOR: return precompute_xor(instr);
        case ssa::Opcode::SHL: return precompute_shl(instr);
        case ssa::Opcode::SHR: return precompute_shr(instr);
        case ssa::Opcode::SELECT: return precompute_select(instr);
        case ssa::Opcode::SEXTEND: return precompute_extend(instr);
        case ssa::Opcode::UEXTEND: return precompute_extend(instr);
        case ssa::Opcode::UTOF: return precompute_itof(instr);
        case ssa::Opcode::STOF: return precompute_itof(instr);
        case ssa::Opcode::SQRT: return precompute_sqrt(instr);
        default: return {};
    }
}

std::optional<ssa::Value> Precomputing::precompute_add(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs + rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_sub(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs - rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_mul(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs * rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_sdiv(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs / rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_srem(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs % rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_udiv(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs / rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_urem(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs % rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_fadd(ssa::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs + rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_fsub(ssa::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs - rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_fmul(ssa::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs * rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_fdiv(ssa::Instruction &instr) {
    return precompute_binary_fp_operation(instr, [](double lhs, double rhs) { return lhs / rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_and(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs & rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_or(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs | rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_xor(ssa::Instruction &instr) {
    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) { return lhs ^ rhs; });
}

std::optional<ssa::Value> Precomputing::precompute_shl(ssa::Instruction &instr) {
    // FIXME

    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) {
        return LargeInt{(int)(lhs.to_s64() << rhs.to_s64())};
    });
}

std::optional<ssa::Value> Precomputing::precompute_shr(ssa::Instruction &instr) {
    // FIXME

    return precompute_binary_int_operation(instr, [](LargeInt lhs, LargeInt rhs) {
        return LargeInt{(int)(lhs.to_s64() >> rhs.to_s64())};
    });
}

std::optional<ssa::Value> Precomputing::precompute_select(ssa::Instruction &instr) {
    const ssa::Value &cond_lhs = instr.get_operand(0);
    const ssa::Comparison comparison = instr.get_operand(1).get_comparison();
    const ssa::Value &cond_rhs = instr.get_operand(2);
    const ssa::Value &true_val = instr.get_operand(3);
    const ssa::Value &false_val = instr.get_operand(4);

    if (cond_lhs.is_immediate() && cond_rhs.is_immediate()) {
        bool result = precompute_cmp(cond_lhs, cond_rhs, comparison);
        return result ? true_val : false_val;
    } else {
        return {};
    }
}

std::optional<ssa::Value> Precomputing::precompute_extend(ssa::Instruction &instr) {
    const ssa::Value &value = instr.get_operand(0);
    const ssa::Type &type = instr.get_operand(1).get_type();

    if (value.is_int_immediate()) {
        ssa::Value extended = value;
        extended.set_type(type);
        return extended;
    } else {
        return {};
    }
}

std::optional<ssa::Value> Precomputing::precompute_itof(ssa::Instruction &instr) {
    const ssa::Value &value = instr.get_operand(0);
    const ssa::Type &type = instr.get_operand(1).get_type();

    if (value.is_int_immediate()) {
        double fp_value = (double)value.get_int_immediate().to_s64();
        return ssa::Value::from_fp_immediate(fp_value, type);
    } else {
        return {};
    }
}

std::optional<ssa::Value> Precomputing::precompute_sqrt(ssa::Instruction &instr) {
    const ssa::Value &value = instr.get_operand(0);

    if (value.is_fp_immediate()) {
        double sqrt = std::sqrt(value.get_fp_immediate());
        return ssa::Operand::from_fp_immediate(sqrt, value.get_type());
    } else {
        return {};
    }
}

bool Precomputing::precompute_cmp(const ssa::Value &lhs, const ssa::Value &rhs, ssa::Comparison comparison) {
    switch (comparison) {
        case ssa::Comparison::EQ: return lhs.get_int_immediate() == rhs.get_int_immediate();
        case ssa::Comparison::NE: return lhs.get_int_immediate() != rhs.get_int_immediate();
        case ssa::Comparison::UGT: return lhs.get_int_immediate() > rhs.get_int_immediate();
        case ssa::Comparison::UGE: return lhs.get_int_immediate() >= rhs.get_int_immediate();
        case ssa::Comparison::ULT: return lhs.get_int_immediate() < rhs.get_int_immediate();
        case ssa::Comparison::ULE: return lhs.get_int_immediate() <= rhs.get_int_immediate();
        case ssa::Comparison::SGT: return lhs.get_int_immediate() > rhs.get_int_immediate();
        case ssa::Comparison::SGE: return lhs.get_int_immediate() >= rhs.get_int_immediate();
        case ssa::Comparison::SLT: return lhs.get_int_immediate() < rhs.get_int_immediate();
        case ssa::Comparison::SLE: return lhs.get_int_immediate() <= rhs.get_int_immediate();
        case ssa::Comparison::FEQ: return lhs.get_fp_immediate() == rhs.get_fp_immediate();
        case ssa::Comparison::FNE: return lhs.get_fp_immediate() != rhs.get_fp_immediate();
        case ssa::Comparison::FGT: return lhs.get_fp_immediate() > rhs.get_fp_immediate();
        case ssa::Comparison::FGE: return lhs.get_fp_immediate() >= rhs.get_fp_immediate();
        case ssa::Comparison::FLT: return lhs.get_fp_immediate() < rhs.get_fp_immediate();
        case ssa::Comparison::FLE: return lhs.get_fp_immediate() <= rhs.get_fp_immediate();
    }
}

} // namespace passes

} // namespace banjo
