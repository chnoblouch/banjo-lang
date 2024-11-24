#ifndef AARCH64_SSA_LOWERER_H
#define AARCH64_SSA_LOWERER_H

#include "banjo/codegen/ssa_lowerer.hpp"

namespace banjo {

namespace target {

class AArch64SSALowerer : public codegen::SSALowerer {

private:
    struct Address {
        mcode::Operand base;
        int imm_offset = 0;
        mcode::Register reg_offset = mcode::Register::from_virtual(-1);
        int reg_scale = 0;
    };

    unsigned next_const_index = 0;

public:
    AArch64SSALowerer(Target *target);

public:
    mcode::Operand lower_value(const ssa::Operand &operand);
    mcode::Operand lower_address(const ssa::Operand &operand);
    mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv);

    mcode::Value lower_global_value(ssa::Value &value);

    void lower_load(ssa::Instruction &instr);
    void lower_store(ssa::Instruction &instr);
    void lower_loadarg(ssa::Instruction &instr);
    void lower_add(ssa::Instruction &instr);
    void lower_sub(ssa::Instruction &instr);
    void lower_mul(ssa::Instruction &instr);
    void lower_sdiv(ssa::Instruction &instr);
    void lower_srem(ssa::Instruction &instr);
    void lower_udiv(ssa::Instruction &instr);
    void lower_urem(ssa::Instruction &instr);
    void lower_fadd(ssa::Instruction &instr);
    void lower_fsub(ssa::Instruction &instr);
    void lower_fmul(ssa::Instruction &instr);
    void lower_fdiv(ssa::Instruction &instr);
    void lower_and(ssa::Instruction &instr);
    void lower_or(ssa::Instruction &instr);
    void lower_xor(ssa::Instruction &instr);
    void lower_shl(ssa::Instruction &instr);
    void lower_shr(ssa::Instruction &instr);
    void lower_jmp(ssa::Instruction &instr);
    void lower_cjmp(ssa::Instruction &instr);
    void lower_fcjmp(ssa::Instruction &instr);
    void lower_select(ssa::Instruction &instr);
    void lower_call(ssa::Instruction &instr);
    void lower_ret(ssa::Instruction &instr);
    void lower_uextend(ssa::Instruction &instr);
    void lower_sextend(ssa::Instruction &instr);
    void lower_truncate(ssa::Instruction &instr);
    void lower_fpromote(ssa::Instruction &instr);
    void lower_fdemote(ssa::Instruction &instr);
    void lower_utof(ssa::Instruction &instr);
    void lower_stof(ssa::Instruction &instr);
    void lower_ftou(ssa::Instruction &instr);
    void lower_ftos(ssa::Instruction &instr);
    void lower_offsetptr(ssa::Instruction &instr);
    void lower_memberptr(ssa::Instruction &instr);

private:
    void lower_fp_operation(mcode::Opcode opcode, ssa::Instruction &instr);
    mcode::Operand lower_reg_val(ssa::VirtualRegister virtual_reg, int size);
    mcode::Value move_const_into_register(const ssa::Value &value, ssa::Type type);
    mcode::Value move_int_into_register(LargeInt value, int size);
    mcode::Value move_float_into_register(double fp, int size);
    void move_elements_into_register(mcode::Value value, std::uint16_t *elements, int count);
    mcode::Value move_symbol_into_register(std::string symbol);
    void calculate_address(mcode::Register dst, Address addr);
    mcode::Value create_temp_value(int size);
    AArch64Condition lower_condition(ssa::Comparison comparison);
    void move_branch_args(ssa::BranchTarget &target);
};

} // namespace target

} // namespace banjo

#endif
