#ifndef BANJO_TARGET_AARCH64_AAPCS_CALLING_CONV_H
#define BANJO_TARGET_AARCH64_AAPCS_CALLING_CONV_H

#include "banjo/mcode/calling_convention.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/target/aarch64/aarch64_ssa_lowerer.hpp"

#include <cstdint>
#include <functional>

namespace banjo {

namespace target {

class AAPCSCallingConv : public mcode::CallingConvention {

public:
    enum Variant : std::uint8_t {
        STANDARD,
        APPLE,
    };

public:
    static AAPCSCallingConv INSTANCE_STANDARD;
    static AAPCSCallingConv INSTANCE_APPLE;

    static const std::vector<int> ARG_REGS_INT;
    static const std::vector<int> ARG_REGS_FP;

private:
    Variant variant;

public:
    AAPCSCallingConv(Variant variant);
    void lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr);
    void create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_call_arg_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_implicit_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    int get_alloca_size(mcode::StackRegions &regions);
    std::vector<mcode::Instruction> get_prolog(mcode::Function *func);
    std::vector<mcode::Instruction> get_epilog(mcode::Function *func);

    mcode::InstrIter fix_up_instr(mcode::BasicBlock &block, mcode::InstrIter iter, TargetRegAnalyzer &analyzer);

    bool is_func_exit(mcode::Opcode opcode);

    std::vector<mcode::ArgStorage> get_arg_storage(const ssa::FunctionType &func_type);
    int get_implicit_stack_bytes(mcode::Function *func);

private:
    void emit_arg_moves(AArch64SSALowerer &lowerer, ssa::FunctionType &func_type, ssa::Instruction &call_instr);
    void emit_reg_arg_move(AArch64SSALowerer &lowerer, ssa::Operand &operand, unsigned index);
    void emit_stack_arg_move(AArch64SSALowerer &lowerer, ssa::Operand &operand, unsigned arg_slot_index);
    void emit_call_instr(AArch64SSALowerer &lowerer, ssa::Instruction &call_instr);
    void emit_ret_val_move(AArch64SSALowerer &lowerer, ssa::Instruction &call_instr);

    void modify_sp(mcode::Opcode opcode, unsigned value, const std::function<void(mcode::Instruction)> &emit);
};

} // namespace target

} // namespace banjo

#endif
