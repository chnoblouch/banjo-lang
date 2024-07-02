#ifndef PASSES_PRECOMPUTING_H
#define PASSES_PRECOMPUTING_H

#include "banjo/ir/module.hpp"

#include <functional>
#include <optional>

namespace banjo {

namespace passes {

namespace Precomputing {

void precompute_instrs(ir::Function &func);
std::optional<ir::Value> precompute_result(ir::Instruction &instr);
std::optional<ir::Value> precompute_add(ir::Instruction &instr);
std::optional<ir::Value> precompute_sub(ir::Instruction &instr);
std::optional<ir::Value> precompute_mul(ir::Instruction &instr);
std::optional<ir::Value> precompute_sdiv(ir::Instruction &instr);
std::optional<ir::Value> precompute_srem(ir::Instruction &instr);
std::optional<ir::Value> precompute_udiv(ir::Instruction &instr);
std::optional<ir::Value> precompute_urem(ir::Instruction &instr);
std::optional<ir::Value> precompute_fadd(ir::Instruction &instr);
std::optional<ir::Value> precompute_fsub(ir::Instruction &instr);
std::optional<ir::Value> precompute_fmul(ir::Instruction &instr);
std::optional<ir::Value> precompute_fdiv(ir::Instruction &instr);
std::optional<ir::Value> precompute_and(ir::Instruction &instr);
std::optional<ir::Value> precompute_or(ir::Instruction &instr);
std::optional<ir::Value> precompute_xor(ir::Instruction &instr);
std::optional<ir::Value> precompute_shl(ir::Instruction &instr);
std::optional<ir::Value> precompute_shr(ir::Instruction &instr);
std::optional<ir::Value> precompute_select(ir::Instruction &instr);
std::optional<ir::Value> precompute_extend(ir::Instruction &instr);
std::optional<ir::Value> precompute_itof(ir::Instruction &instr);
std::optional<ir::Value> precompute_sqrt(ir::Instruction &instr);
bool precompute_cmp(const ir::Value &lhs, const ir::Value &rhs, ir::Comparison comparison);

} // namespace Precomputing

} // namespace passes

} // namespace banjo

#endif
