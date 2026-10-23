#ifndef BANJO_TARGET_X86_64_SSA_LOWERER_H
#define BANJO_TARGET_X86_64_SSA_LOWERER_H

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/x86_64/x86_64_condition.hpp"
#include "banjo/target/x86_64/x86_64_const_lowering.hpp"

#include <optional>
#include <unordered_map>

namespace banjo::target {

class X8664SSALowerer : public codegen::SSALowerer {

private:
    enum class OperandKind {
        REG,
        REG_ADDR,
        REG_IMM,
        REG_ADDR_IMM,
    };

    X8664ConstLowering const_lowering;
    std::unordered_map<ssa::VirtualRegister, ssa::VirtualRegister> block_arg_tmps;
    std::optional<mcode::Symbol> const_neg_zero_64;
    std::optional<mcode::Symbol> const_neg_zero_32;

public:
    X8664SSALowerer(Target *target);

    mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) override;

public:
    void init_func(ssa::Function &func) override;
    void emit_block_prologue(ssa::BasicBlock &block) override;

    void lower_load(ssa::Instruction &instr) override;
    void lower_store(ssa::Instruction &instr) override;
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
    void lower_bitcast(ssa::Instruction &instr) override;
    void lower_atomic_load(ssa::Instruction &instr) override;
    void lower_atomic_store(ssa::Instruction &instr) override;
    void lower_atomic_swap(ssa::Instruction &instr) override;
    void lower_atomic_cmpswap(ssa::Instruction &instr) override;
    void lower_atomic_add(ssa::Instruction &instr) override;
    void lower_atomic_sub(ssa::Instruction &instr) override;
    void lower_atomic_and(ssa::Instruction &instr) override;
    void lower_atomic_or(ssa::Instruction &instr) override;
    void lower_atomic_xor(ssa::Instruction &instr) override;
    void lower_offsetptr(ssa::Instruction &instr) override;
    void lower_memberptr(ssa::Instruction &instr) override;
    void lower_copy(ssa::Instruction &instr) override;
    void lower_frame_address(ssa::Instruction &instr) override;

    bool lower_stored_operation(ssa::Instruction &store);
    void lower_in_place_operation(mcode::Opcode m_opcode, ssa::Operand &addr, ssa::Value &rhs);
    void lower_in_place_shift(mcode::Opcode m_opcode, ssa::Operand &addr, ssa::Value &rhs);

    void lower_into_div(mcode::PhysicalReg result, ssa::Instruction &instr);
    void lower_into_idiv(mcode::PhysicalReg result, ssa::Instruction &instr);
    void lower_bitwise(mcode::Opcode opcode, ssa::Instruction &instr);
    void lower_shift(mcode::Opcode opcode, ssa::Instruction &instr);
    void lower_cond_branch(ssa::Instruction &instr);
    void lower_atomic_cas_loop(mcode::Opcode opcode, ssa::Instruction &instr);

    void lower_to_copy_into(mcode::Register reg, ssa::Value &value);
    void lower_to_copy_into(mcode::Operand &m_dst, ssa::Value &value);

    void emit_branch_arg_copies(ssa::BranchTarget &target);
    void emit_memory_copy_using_movs(ssa::Instruction &instr, unsigned size);

    mcode::Operand lower_value(ssa::Value &value, OperandKind kind);
    mcode::Operand lower_reg(ssa::VirtualRegister reg, unsigned size, OperandKind kind);
    mcode::Operand lower_int_imm(LargeInt value, unsigned size, OperandKind kind);
    mcode::Operand lower_fp_imm(double value, unsigned size, OperandKind kind);
    X8664Condition lower_comparison(ssa::Operand *lhs, ssa::Operand *rhs, ssa::Comparison comparison);

    mcode::Operand lower_addr_mem_access(AddrComponents addr);
    mcode::Operand lower_callee(ssa::Value &value);
    mcode::Operand lower_symbol(ssa::Value &value, unsigned size, bool is_callee);

    mcode::Register create_reg();
    mcode::Operand create_fp_const_load(double value, unsigned size);
    mcode::Symbol &create_const_neg_zero_64();
    mcode::Symbol &create_const_neg_zero_32();

    mcode::Opcode mov_opcode(ssa::Type type);
    bool should_swap(ssa::Value &lhs, ssa::Value &rhs);
    bool is_loaded(ssa::Value &value);
};

} // namespace banjo::target

#endif
