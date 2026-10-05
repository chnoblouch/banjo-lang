#ifndef BANJO_SSA_VALIDATOR_H
#define BANJO_SSA_VALIDATOR_H

#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/module.hpp"

#include <ostream>

namespace banjo::ssa {

class Validator {

private:
    std::ostream &stream;

public:
    Validator(std::ostream &stream);
    bool validate(Module &mod);
    bool validate(Module &mod, Function &func);

private:
    bool validate_alloca(Instruction &instr);
    bool validate_load(Instruction &instr);
    bool validate_store(Instruction &instr);
    bool validate_add(Instruction &instr);
    bool validate_sub(Instruction &instr);
    bool validate_mul(Instruction &instr);
    bool validate_sdiv(Instruction &instr);
    bool validate_srem(Instruction &instr);
    bool validate_udiv(Instruction &instr);
    bool validate_urem(Instruction &instr);
    bool validate_fadd(Instruction &instr);
    bool validate_fsub(Instruction &instr);
    bool validate_fmul(Instruction &instr);
    bool validate_fdiv(Instruction &instr);
    bool validate_and(Instruction &instr);
    bool validate_or(Instruction &instr);
    bool validate_xor(Instruction &instr);
    bool validate_lshl(Instruction &instr);
    bool validate_lshr(Instruction &instr);
    bool validate_ashr(Instruction &instr);
    bool validate_jmp(Instruction &instr);
    bool validate_cjmp(Instruction &instr);
    bool validate_fcjmp(Instruction &instr);
    bool validate_select(Instruction &instr);
    bool validate_call(Instruction &instr);
    bool validate_ret(Instruction &instr);
    bool validate_uextend(Instruction &instr);
    bool validate_sextend(Instruction &instr);
    bool validate_truncate(Instruction &instr);
    bool validate_fpromote(Instruction &instr);
    bool validate_fdemote(Instruction &instr);
    bool validate_utof(Instruction &instr);
    bool validate_stof(Instruction &instr);
    bool validate_ftou(Instruction &instr);
    bool validate_ftos(Instruction &instr);
    bool validate_bitcast(Instruction &instr);
    bool validate_atomic_load(Instruction &instr);
    bool validate_atomic_store(Instruction &instr);
    bool validate_atomic_add(Instruction &instr);
    bool validate_atomic_sub(Instruction &instr);
    bool validate_atomic_and(Instruction &instr);
    bool validate_atomic_or(Instruction &instr);
    bool validate_atomic_xor(Instruction &instr);
    bool validate_atomic_swap(Instruction &instr);
    bool validate_atomic_cmpswap(Instruction &instr);
    bool validate_memberptr(Instruction &instr);
    bool validate_offsetptr(Instruction &instr);
    bool validate_copy(Instruction &instr);
    bool validate_sqrt(Instruction &instr);
    bool validate_frame_address(Instruction &instr);

    bool validate_binary_int(Instruction &instr, const std::string &name);
    bool validate_binary_fp(Instruction &instr, const std::string &name);
};

} // namespace banjo::ssa

#endif
