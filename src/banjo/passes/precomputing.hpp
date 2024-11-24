#ifndef PASSES_PRECOMPUTING_H
#define PASSES_PRECOMPUTING_H

#include "banjo/ssa/module.hpp"

#include <functional>
#include <optional>

namespace banjo {

namespace passes {

namespace Precomputing {

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
std::optional<ssa::Value> precompute_shl(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_shr(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_select(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_extend(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_itof(ssa::Instruction &instr);
std::optional<ssa::Value> precompute_sqrt(ssa::Instruction &instr);
bool precompute_cmp(const ssa::Value &lhs, const ssa::Value &rhs, ssa::Comparison comparison);

} // namespace Precomputing

} // namespace passes

} // namespace banjo

#endif
