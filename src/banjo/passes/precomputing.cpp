#include "precomputing.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/primitive.hpp"

#include <cstdint>
#include <functional>
#include <optional>

namespace banjo::passes {

template <typename T>
using ComputeFunc = std::function<T(T lhs, T rhs)>;

static std::optional<ssa::Value> compute_two_operands_bits(
    ssa::Instruction &instr,
    const ComputeFunc<std::uint8_t> &compute8,
    const ComputeFunc<std::uint16_t> &compute16,
    const ComputeFunc<std::uint32_t> &compute32,
    const ComputeFunc<std::uint64_t> &compute64
) {
    ssa::Operand &lhs = instr.get_operand(0);
    ssa::Operand &rhs = instr.get_operand(1);
    ssa::Type type = lhs.get_type();

    if (!type.is_primitive() || !lhs.is_int_immediate() || !rhs.is_int_immediate()) {
        return {};
    }

    std::uint64_t lhs_value = lhs.get_int_immediate().to_bits();
    std::uint64_t rhs_value = rhs.get_int_immediate().to_bits();
    std::optional<LargeInt> result;

    if (type == ssa::Primitive::I8 || type == ssa::Primitive::U8) {
        result = compute8(static_cast<std::uint8_t>(lhs_value), static_cast<std::uint8_t>(rhs_value));
    } else if (type == ssa::Primitive::I16 || type == ssa::Primitive::U16) {
        result = compute16(static_cast<std::uint16_t>(lhs_value), static_cast<std::uint16_t>(rhs_value));
    } else if (type == ssa::Primitive::I32 || type == ssa::Primitive::U32) {
        result = compute32(static_cast<std::uint32_t>(lhs_value), static_cast<std::uint32_t>(rhs_value));
    } else if (type == ssa::Primitive::I64 || type == ssa::Primitive::U64) {
        result = compute64(static_cast<std::uint64_t>(lhs_value), static_cast<std::uint64_t>(rhs_value));
    } else {
        return {};
    }

    if (result) {
        return ssa::Operand::from_int_immediate(*result, type);
    } else {
        return {};
    }
}

static std::optional<ssa::Value> compute_two_operands_signed(
    ssa::Instruction &instr,
    const ComputeFunc<std::int8_t> &compute8,
    const ComputeFunc<std::int16_t> &compute16,
    const ComputeFunc<std::int32_t> &compute32,
    const ComputeFunc<std::int64_t> &compute64
) {
    ssa::Operand &lhs = instr.get_operand(0);
    ssa::Operand &rhs = instr.get_operand(1);
    ssa::Type type = lhs.get_type();

    if (!type.is_primitive() || !lhs.is_int_immediate() || !rhs.is_int_immediate()) {
        return {};
    }

    std::int64_t lhs_value = lhs.get_int_immediate().to_s64();
    std::int64_t rhs_value = rhs.get_int_immediate().to_s64();
    std::optional<LargeInt> result;

    if (type == ssa::Primitive::I8 || type == ssa::Primitive::U8) {
        result = compute8(static_cast<std::int8_t>(lhs_value), static_cast<std::int8_t>(rhs_value));
    } else if (type == ssa::Primitive::I16 || type == ssa::Primitive::U16) {
        result = compute16(static_cast<std::int16_t>(lhs_value), static_cast<std::int16_t>(rhs_value));
    } else if (type == ssa::Primitive::I32 || type == ssa::Primitive::U32) {
        result = compute32(static_cast<std::int32_t>(lhs_value), static_cast<std::int32_t>(rhs_value));
    } else if (type == ssa::Primitive::I64 || type == ssa::Primitive::U64) {
        result = compute64(static_cast<std::int64_t>(lhs_value), static_cast<std::int64_t>(rhs_value));
    } else {
        return {};
    }

    if (result) {
        return ssa::Operand::from_int_immediate(*result, type);
    } else {
        return {};
    }
}

static std::optional<ssa::Value> compute_two_operands_fp(
    ssa::Instruction &instr,
    const ComputeFunc<float> &compute32,
    const ComputeFunc<double> &compute64
) {
    ssa::Operand &lhs = instr.get_operand(0);
    ssa::Operand &rhs = instr.get_operand(1);
    ssa::Type type = lhs.get_type();

    if (!type.is_primitive() || !lhs.is_fp_immediate() || !rhs.is_fp_immediate()) {
        return {};
    }

    double lhs_value = lhs.get_fp_immediate();
    double rhs_value = rhs.get_fp_immediate();
    std::optional<double> result;

    if (type == ssa::Primitive::F32) {
        result = compute32(static_cast<float>(lhs_value), static_cast<float>(rhs_value));
    } else if (type == ssa::Primitive::F64) {
        result = compute64(static_cast<double>(lhs_value), static_cast<double>(rhs_value));
    } else {
        return {};
    }

    if (result) {
        return ssa::Operand::from_fp_immediate(*result, type);
    } else {
        return {};
    }
}

void precompute_instrs(ssa::Function &func) {
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
                std::optional<ssa::Value> precomputed_result = precompute_result(*iter);
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

std::optional<ssa::Value> precompute_result(ssa::Instruction &instr) {
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
        case ssa::Opcode::LSHL: return precompute_lshl(instr);
        case ssa::Opcode::LSHR: return precompute_lshr(instr);
        case ssa::Opcode::ASHR: return precompute_ashr(instr);
        case ssa::Opcode::SELECT: return precompute_select(instr);
        case ssa::Opcode::UEXTEND: return precompute_uextend(instr);
        case ssa::Opcode::SEXTEND: return precompute_sextend(instr);
        default: return {};
    }
}

std::optional<ssa::Value> precompute_add(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs + rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs + rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs + rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs + rhs; }
    );
}

std::optional<ssa::Value> precompute_sub(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs - rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs - rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs - rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs - rhs; }
    );
}

std::optional<ssa::Value> precompute_mul(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs * rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs * rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs * rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs * rhs; }
    );
}

std::optional<ssa::Value> precompute_sdiv(ssa::Instruction &instr) {
    return compute_two_operands_signed(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs / rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs / rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs / rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs / rhs; }
    );
}

std::optional<ssa::Value> precompute_srem(ssa::Instruction &instr) {
    return compute_two_operands_signed(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs % rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs % rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs % rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs % rhs; }
    );
}

std::optional<ssa::Value> precompute_udiv(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs / rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs / rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs / rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs / rhs; }
    );
}

std::optional<ssa::Value> precompute_urem(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs % rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs % rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs % rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs % rhs; }
    );
}

std::optional<ssa::Value> precompute_fadd(ssa::Instruction &instr) {
    return compute_two_operands_fp(
        instr,
        [](float lhs, float rhs) { return lhs + rhs; },
        [](double lhs, double rhs) { return lhs + rhs; }
    );
}

std::optional<ssa::Value> precompute_fsub(ssa::Instruction &instr) {
    return compute_two_operands_fp(
        instr,
        [](float lhs, float rhs) { return lhs - rhs; },
        [](double lhs, double rhs) { return lhs - rhs; }
    );
}

std::optional<ssa::Value> precompute_fmul(ssa::Instruction &instr) {
    return compute_two_operands_fp(
        instr,
        [](float lhs, float rhs) { return lhs * rhs; },
        [](double lhs, double rhs) { return lhs * rhs; }
    );
}

std::optional<ssa::Value> precompute_fdiv(ssa::Instruction &instr) {
    return compute_two_operands_fp(
        instr,
        [](float lhs, float rhs) { return lhs / rhs; },
        [](double lhs, double rhs) { return lhs / rhs; }
    );
}

std::optional<ssa::Value> precompute_and(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs & rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs & rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs & rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs & rhs; }
    );
}

std::optional<ssa::Value> precompute_or(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs | rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs | rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs | rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs | rhs; }
    );
}

std::optional<ssa::Value> precompute_xor(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs ^ rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs ^ rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs ^ rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs ^ rhs; }
    );
}

std::optional<ssa::Value> precompute_lshl(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs << rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs << rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs << rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs << rhs; }
    );
}

std::optional<ssa::Value> precompute_lshr(ssa::Instruction &instr) {
    return compute_two_operands_bits(
        instr,
        [](std::uint8_t lhs, std::uint8_t rhs) { return lhs >> rhs; },
        [](std::uint16_t lhs, std::uint16_t rhs) { return lhs >> rhs; },
        [](std::uint32_t lhs, std::uint32_t rhs) { return lhs >> rhs; },
        [](std::uint64_t lhs, std::uint64_t rhs) { return lhs >> rhs; }
    );
}

std::optional<ssa::Value> precompute_ashr(ssa::Instruction &instr) {
    return compute_two_operands_signed(
        instr,
        [](std::int8_t lhs, std::int8_t rhs) { return lhs >> rhs; },
        [](std::int16_t lhs, std::int16_t rhs) { return lhs >> rhs; },
        [](std::int32_t lhs, std::int32_t rhs) { return lhs >> rhs; },
        [](std::int64_t lhs, std::int64_t rhs) { return lhs >> rhs; }
    );
}

std::optional<ssa::Value> precompute_select(ssa::Instruction &instr) {
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

std::optional<ssa::Value> precompute_uextend(ssa::Instruction &instr) {
    ssa::Value &value = instr.get_operand(0);
    ssa::Type dst_type = instr.get_operand(1).get_type();
    ssa::Type src_type = value.get_type();

    if (!value.is_int_immediate()) {
        return {};
    }

    std::uint64_t bits = value.get_int_immediate().to_bits();
    LargeInt result = 0;

    if (src_type == ssa::Primitive::I8 || src_type == ssa::Primitive::I8) {
        result = LargeInt{static_cast<std::uint8_t>(bits)};
    } else if (src_type == ssa::Primitive::I16 || src_type == ssa::Primitive::U16) {
        result = LargeInt{static_cast<std::uint16_t>(bits)};
    } else if (src_type == ssa::Primitive::I32 || src_type == ssa::Primitive::U32) {
        result = LargeInt{static_cast<std::uint32_t>(bits)};
    } else {
        return {};
    }

    return ssa::Operand::from_int_immediate(result, dst_type);
}

std::optional<ssa::Value> precompute_sextend(ssa::Instruction &instr) {
    ssa::Value &value = instr.get_operand(0);
    ssa::Type dst_type = instr.get_operand(1).get_type();
    ssa::Type src_type = value.get_type();

    if (!value.is_int_immediate()) {
        return {};
    }

    std::uint64_t bits = value.get_int_immediate().to_bits();
    LargeInt result = 0;

    if (src_type == ssa::Primitive::I8 || src_type == ssa::Primitive::I8) {
        result = LargeInt{static_cast<std::int8_t>(bits)};
    } else if (src_type == ssa::Primitive::I16 || src_type == ssa::Primitive::U16) {
        result = LargeInt{static_cast<std::int16_t>(bits)};
    } else if (src_type == ssa::Primitive::I32 || src_type == ssa::Primitive::U32) {
        result = LargeInt{static_cast<std::int32_t>(bits)};
    } else {
        return {};
    }

    return ssa::Operand::from_int_immediate(result, dst_type);
}

std::optional<ssa::Value> precompute_itof(ssa::Instruction &instr) {
    const ssa::Value &value = instr.get_operand(0);
    const ssa::Type &type = instr.get_operand(1).get_type();

    if (value.is_int_immediate()) {
        double fp_value = (double)value.get_int_immediate().to_s64();
        return ssa::Value::from_fp_immediate(fp_value, type);
    } else {
        return {};
    }
}

std::optional<bool> try_precompute_cmp(ssa::Value &lhs, ssa::Value &rhs, ssa::Comparison comparison) {
    if (comparison < ssa::Comparison::FEQ) {
        if (lhs.is_int_immediate() && rhs.is_int_immediate()) {
            return precompute_cmp(lhs, rhs, comparison);
        }
    } else {
        if (lhs.is_fp_immediate() && rhs.is_fp_immediate()) {
            return precompute_cmp(lhs, rhs, comparison);
        }
    }

    return {};
}

bool precompute_cmp(const ssa::Value &lhs, const ssa::Value &rhs, ssa::Comparison comparison) {
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

} // namespace banjo::passes
