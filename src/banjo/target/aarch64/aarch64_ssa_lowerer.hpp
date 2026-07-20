#ifndef BANJO_TARGET_AARCH64_SSA_LOWERER_H
#define BANJO_TARGET_AARCH64_SSA_LOWERER_H

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <unordered_map>

namespace banjo::target {

class AArch64SSALowerer : public codegen::SSALowerer {

private:
    unsigned next_const_index = 0;
    std::unordered_map<ssa::VirtualRegister, ssa::VirtualRegister> block_arg_tmps;

public:
    AArch64SSALowerer(Target *target);

public:
    mcode::Operand lower_value(const ssa::Operand &operand);
    
    mcode::Operand lower_addr_mem_access(AddrComponents addr, unsigned size);
    mcode::Operand lower_addr_value(AddrComponents addr);
    std::variant<mcode::Register, mcode::StackSlotID> lower_addr_base(ssa::Operand &base);

    mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) override;

    void init_func(ssa::Function &func) override;
    void emit_block_prologue(ssa::BasicBlock &block) override;

    void lower_load(ssa::Instruction &instr) override;
    void lower_store(ssa::Instruction &instr) override;
    void lower_loadarg(ssa::Instruction &instr) override;
    void lower_add(ssa::Instruction &instr) override;
    void lower_sub(ssa::Instruction &instr) override;
    void lower_mul(ssa::Instruction &instr) override;
    void lower_sdiv(ssa::Instruction &instr) override;
    void lower_srem(ssa::Instruction &instr) override;
    void lower_udiv(ssa::Instruction &instr) override;
    void lower_urem(ssa::Instruction &instr) override;
    void lower_fadd(ssa::Instruction &instr) override;
    void lower_fsub(ssa::Instruction &instr) override;
    void lower_fmul(ssa::Instruction &instr) override;
    void lower_fdiv(ssa::Instruction &instr) override;
    void lower_and(ssa::Instruction &instr) override;
    void lower_or(ssa::Instruction &instr) override;
    void lower_xor(ssa::Instruction &instr) override;
    void lower_lshl(ssa::Instruction &instr) override;
    void lower_lshr(ssa::Instruction &instr) override;
    void lower_ashr(ssa::Instruction &instr) override;
    void lower_jmp(ssa::Instruction &instr) override;
    void lower_cjmp(ssa::Instruction &instr) override;
    void lower_fcjmp(ssa::Instruction &instr) override;
    void lower_select(ssa::Instruction &instr) override;
    void lower_call(ssa::Instruction &instr) override;
    void lower_ret(ssa::Instruction &instr) override;
    void lower_uextend(ssa::Instruction &instr) override;
    void lower_sextend(ssa::Instruction &instr) override;
    void lower_truncate(ssa::Instruction &instr) override;
    void lower_fpromote(ssa::Instruction &instr) override;
    void lower_fdemote(ssa::Instruction &instr) override;
    void lower_utof(ssa::Instruction &instr) override;
    void lower_stof(ssa::Instruction &instr) override;
    void lower_ftou(ssa::Instruction &instr) override;
    void lower_ftos(ssa::Instruction &instr) override;
    void lower_offsetptr(ssa::Instruction &instr) override;
    void lower_memberptr(ssa::Instruction &instr) override;

    void lower_fp_operation(mcode::Opcode opcode, ssa::Instruction &instr);
    void lower_cond_branch(mcode::Opcode cmp_opcode, ssa::Instruction &instr);

    mcode::Operand lower_reg_val(ssa::VirtualRegister virtual_reg, unsigned size);
    mcode::Value move_const_into_register(const ssa::Value &value, ssa::Type type);
    mcode::Value move_int_into_register(LargeInt value, unsigned size);
    mcode::Value move_float_into_register(double fp, unsigned size);
    void move_elements_into_register(mcode::Value value, std::uint16_t *elements, unsigned count);
    mcode::Register move_symbol_into_register(const std::string &symbol);
    void build_address(const mcode::Operand &m_dst, AddrComponents addr);
    mcode::Value create_temp_value(int size);
    AArch64Condition lower_condition(ssa::Comparison comparison);
    void move_branch_args(ssa::BranchTarget &target);

    mcode::Operand lower_as_move_into_reg(mcode::Register reg, const ssa::Value &value);
    mcode::Operand emit_add_scaled(mcode::Operand m_base, mcode::Register reg, unsigned scale);
    mcode::Operand emit_add_imm(mcode::Operand m_lhs, LargeInt immediate);
};

} // namespace banjo::target

#endif
