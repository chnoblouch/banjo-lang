#ifndef AARCH64_IR_LOWERER_H
#define AARCH64_IR_LOWERER_H

#include "codegen/ir_lowerer.hpp"

namespace banjo {

namespace target {

class AArch64IRLowerer : public codegen::IRLowerer {

private:
    struct Address {
        mcode::Operand base;
        int imm_offset = 0;
        mcode::Register reg_offset = mcode::Register::from_virtual(-1);
        int reg_scale = 0;
    };

    unsigned next_const_index = 0;

public:
    AArch64IRLowerer(Target *target);

public:
    mcode::Operand lower_value(const ir::Operand &operand);
    mcode::Operand lower_address(const ir::Operand &operand);
    mcode::CallingConvention *get_calling_convention(ir::CallingConv calling_conv);

    mcode::Value lower_global_value(ir::Value &value);

    void lower_load(ir::Instruction &instr);
    void lower_store(ir::Instruction &instr);
    void lower_loadarg(ir::Instruction &instr);
    void lower_add(ir::Instruction &instr);
    void lower_sub(ir::Instruction &instr);
    void lower_mul(ir::Instruction &instr);
    void lower_sdiv(ir::Instruction &instr);
    void lower_srem(ir::Instruction &instr);
    void lower_udiv(ir::Instruction &instr);
    void lower_urem(ir::Instruction &instr);
    void lower_fadd(ir::Instruction &instr);
    void lower_fsub(ir::Instruction &instr);
    void lower_fmul(ir::Instruction &instr);
    void lower_fdiv(ir::Instruction &instr);
    void lower_and(ir::Instruction &instr);
    void lower_or(ir::Instruction &instr);
    void lower_xor(ir::Instruction &instr);
    void lower_shl(ir::Instruction &instr);
    void lower_shr(ir::Instruction &instr);
    void lower_jmp(ir::Instruction &instr);
    void lower_cjmp(ir::Instruction &instr);
    void lower_fcjmp(ir::Instruction &instr);
    void lower_select(ir::Instruction &instr);
    void lower_call(ir::Instruction &instr);
    void lower_ret(ir::Instruction &instr);
    void lower_uextend(ir::Instruction &instr);
    void lower_sextend(ir::Instruction &instr);
    void lower_truncate(ir::Instruction &instr);
    void lower_fpromote(ir::Instruction &instr);
    void lower_fdemote(ir::Instruction &instr);
    void lower_utof(ir::Instruction &instr);
    void lower_stof(ir::Instruction &instr);
    void lower_ftou(ir::Instruction &instr);
    void lower_ftos(ir::Instruction &instr);
    void lower_offsetptr(ir::Instruction &instr);
    void lower_memberptr(ir::Instruction &instr);

private:
    void lower_fp_operation(mcode::Opcode opcode, ir::Instruction &instr);
    mcode::Operand lower_reg_val(ir::VirtualRegister virtual_reg, int size);
    mcode::Operand lower_str_val(const std::string &data, int size);
    mcode::Value move_const_into_register(const ir::Value &value, ir::Type type);
    mcode::Value move_int_into_register(LargeInt value, int size);
    mcode::Value move_float_into_register(double fp, int size);
    void move_elements_into_register(mcode::Value value, std::uint16_t *elements, int count);
    mcode::Value move_symbol_into_register(std::string symbol);
    void calculate_address(mcode::Register dst, Address addr);
    mcode::Value create_temp_value(int size);
    AArch64Condition lower_condition(ir::Comparison comparison);
    void move_branch_args(ir::BranchTarget &target);
};

} // namespace target

} // namespace banjo

#endif
