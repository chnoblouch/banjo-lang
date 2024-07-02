#ifndef X86_64_IR_LOWERER_H
#define X86_64_IR_LOWERER_H

#include "banjo/codegen/ir_lowerer.hpp"
#include "banjo/ir/instruction.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/target/x86_64/x86_64_addr_lowering.hpp"
#include "banjo/target/x86_64/x86_64_const_lowering.hpp"

#include <functional>
#include <optional>

namespace banjo {

namespace target {

class X8664IRLowerer : public codegen::IRLowerer {

private:
    X8664AddrLowering addr_lowering;
    X8664ConstLowering const_lowering;
    std::optional<std::string> const_neg_zero;

public:
    constexpr static int PTR_SIZE = 8;

    X8664IRLowerer(Target *target);

    mcode::Operand into_reg_or_addr(ir::Operand &operand);
    mcode::InstrIter move_int_into_reg(mcode::Register reg, mcode::Value &value);
    mcode::InstrIter move_float_into_reg(mcode::Register reg, mcode::Value &value);

private:
    mcode::Operand lower_value(const ir::Operand &operand) override;
    mcode::Operand lower_address(const ir::Operand &operand) override;
    mcode::CallingConvention *get_calling_convention(ir::CallingConv calling_conv) override;

    void append_mov_and_operation(
        mcode::Opcode machine_opcode,
        ir::VirtualRegister dst,
        ir::Value &lhs,
        ir::Value &rhs
    );
    bool lower_stored_operation(ir::Instruction &store);

    void init_module(ir::Module &mod) override;
    mcode::Value lower_global_value(ir::Value &value) override;

    void lower_load(ir::Instruction &instr) override;
    void lower_store(ir::Instruction &instr) override;
    void lower_loadarg(ir::Instruction &instr) override;
    void lower_add(ir::Instruction &instr) override;
    void lower_sub(ir::Instruction &instr) override;
    void lower_mul(ir::Instruction &instr) override;
    void lower_sdiv(ir::Instruction &instr) override;
    void lower_srem(ir::Instruction &instr) override;
    void lower_udiv(ir::Instruction &instr) override;
    void lower_urem(ir::Instruction &instr) override;
    void lower_fadd(ir::Instruction &instr) override;
    void lower_fsub(ir::Instruction &instr) override;
    void lower_fmul(ir::Instruction &instr) override;
    void lower_fdiv(ir::Instruction &instr) override;
    void lower_and(ir::Instruction &instr) override;
    void lower_or(ir::Instruction &instr) override;
    void lower_xor(ir::Instruction &instr) override;
    void lower_shl(ir::Instruction &instr) override;
    void lower_shr(ir::Instruction &instr) override;
    void lower_jmp(ir::Instruction &instr) override;
    void lower_cjmp(ir::Instruction &instr) override;
    void lower_fcjmp(ir::Instruction &instr) override;
    void lower_select(ir::Instruction &instr) override;
    void lower_call(ir::Instruction &instr) override;
    void lower_ret(ir::Instruction &instr) override;
    void lower_uextend(ir::Instruction &instr) override;
    void lower_sextend(ir::Instruction &instr) override;
    void lower_truncate(ir::Instruction &instr) override;
    void lower_fpromote(ir::Instruction &instr) override;
    void lower_fdemote(ir::Instruction &instr) override;
    void lower_utof(ir::Instruction &instr) override;
    void lower_stof(ir::Instruction &instr) override;
    void lower_ftou(ir::Instruction &instr) override;
    void lower_ftos(ir::Instruction &instr) override;
    void lower_offsetptr(ir::Instruction &instr) override;
    void lower_memberptr(ir::Instruction &instr) override;
    void lower_copy(ir::Instruction &instr) override;
    void lower_sqrt(ir::Instruction &instr) override;

    mcode::Opcode get_move_opcode(ir::Type type);
    void copy_block_using_movs(ir::Instruction &instr, unsigned size);
    void emit_jcc(ir::Instruction &instr, const std::function<void()> &comparison_emitter);
    mcode::Opcode get_jcc_opcode(ir::Comparison comparison);
    mcode::Opcode get_cmovcc_opcode(ir::Comparison comparison);
    unsigned get_condition_code(ir::Comparison comparison);
    mcode::Operand get_float_addr(double value, const ir::Type &type);
    void move_branch_args(ir::BranchTarget &target);
    void emit_shift(ir::Instruction &instr, mcode::Opcode opcode);
};

} // namespace target

} // namespace banjo

#endif
