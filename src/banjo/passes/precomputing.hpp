#ifndef BANJO_PASSES_PRECOMPUTING_H
#define BANJO_PASSES_PRECOMPUTING_H

#include "banjo/ssa/function.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"

#include <optional>

namespace banjo::passes {

void precompute_instrs(ssa::Function &func);
std::optional<ssa::Value> precompute_result(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_add(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_sub(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_mul(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_sdiv(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_srem(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_udiv(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_urem(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_fadd(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_fsub(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_fmul(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_fdiv(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_and(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_or(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_xor(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_lshl(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_lshr(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_ashr(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_select(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_uextend(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_sextend(ssa::Instruction &instr);

std::optional<bool> try_precompute_cmp(ssa::Value &lhs, ssa::Value &rhs, ssa::Comparison comparison);
bool precompute_cmp(const ssa::Value &lhs, const ssa::Value &rhs, ssa::Comparison comparison);

} // namespace banjo::passes

#endif
