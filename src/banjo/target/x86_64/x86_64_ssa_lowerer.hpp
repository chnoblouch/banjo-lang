#ifndef BANJO_TARGET_X86_64_SSA_LOWERER_H
#define BANJO_TARGET_X86_64_SSA_LOWERER_H

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/target/x86_64/x86_64_condition.hpp"
#include "banjo/target/x86_64/x86_64_const_lowering.hpp"

#include <functional>
#include <optional>
#include <unordered_map>

namespace banjo::target {

class X8664SSALowerer : public codegen::SSALowerer {

private:
    struct ValueLowerFlags {
        bool allow_addrs = false;
        bool is_callee = false;
    };

    X8664ConstLowering const_lowering;
    std::unordered_map<ssa::VirtualRegister, ssa::VirtualRegister> block_arg_tmps;
    std::optional<std::string> const_neg_zero;

public:
    constexpr static int PTR_SIZE = 8;

    X8664SSALowerer(Target *target);

    mcode::Operand into_reg_or_addr(ssa::Operand &operand);
    mcode::Operand read_symbol_addr(const mcode::Symbol &symbol);

    mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) override;

public:
    void append_mov_and_operation(
        mcode::Opcode machine_opcode,
        ssa::VirtualRegister dst,
        ssa::Value &lhs,
        ssa::Value &rhs
    );
    bool lower_stored_operation(ssa::Instruction &store);

    void init_module(ssa::Module &mod) override;
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
    void lower_copy(ssa::Instruction &instr) override;
    void lower_sqrt(ssa::Instruction &instr) override;

    mcode::Opcode get_move_opcode(ssa::Type type);
    void copy_block_using_movs(ssa::Instruction &instr, unsigned size);
    mcode::Opcode get_cmovcc_opcode(ssa::Comparison comparison);
    
    void lower_shift(mcode::Opcode opcode, ssa::Instruction &instr);
    void lower_cond_branch(mcode::Opcode cmp_opcode, ssa::Instruction &instr);

    X8664Condition lower_condition(ssa::Comparison comparison);
    void move_branch_args(ssa::BranchTarget &target);

    mcode::Operand lower_as_move_into_reg(mcode::Register reg, const ssa::Value &value);
    mcode::Operand lower_as_move(mcode::Operand m_dst, const ssa::Value &value);
    mcode::Operand lower_int_imm_as_move(mcode::Operand m_dst, LargeInt value);
    mcode::Operand lower_fp_imm_as_move(mcode::Operand m_dst, double value);
    mcode::Operand lower_reg_as_move(mcode::Operand m_dst, ssa::VirtualRegister src_reg, ssa::Type type);

    mcode::Operand lower_as_operand(const ssa::Value &value);
    mcode::Operand lower_as_operand(const ssa::Value &value, ValueLowerFlags flags);
    mcode::Operand lower_int_imm_as_operand(LargeInt value, unsigned size);
    mcode::Operand lower_fp_imm_as_operand(double value, unsigned size);
    mcode::Operand lower_reg_as_operand(ssa::VirtualRegister src_reg, unsigned size, ValueLowerFlags flags);
    mcode::Operand lower_symbol_as_operand(const ssa::Value &value, unsigned size, ValueLowerFlags flags);

    mcode::Operand lower_addr_mem_access(AddrComponents addr);

    mcode::Register create_reg();
    mcode::Operand create_fp_const_load(double value, unsigned size);
};

} // namespace banjo::target

#endif
