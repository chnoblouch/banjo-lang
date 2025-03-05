#ifndef MS_ABI_CALLING_CONV_H
#define MS_ABI_CALLING_CONV_H

#include "banjo/mcode/calling_convention.hpp"

namespace ssa {
class Operand;
} // namespace ssa

// TODO: reformat
namespace mcode {
class Operand;
} // namespace mcode

namespace banjo {

namespace target {

class MSABICallingConv : public mcode::CallingConvention {

public:
    static const MSABICallingConv INSTANCE;
    static const std::vector<mcode::PhysicalReg> ARG_REGS_INT;
    static const std::vector<mcode::PhysicalReg> ARG_REGS_FP;

public:
    MSABICallingConv();
    void lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr);
    void create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_call_arg_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_implicit_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    int get_alloca_size(mcode::StackRegions &regions);
    std::vector<mcode::Instruction> get_prolog(mcode::Function *func);
    std::vector<mcode::Instruction> get_epilog(mcode::Function *func);
    bool is_func_exit(mcode::Opcode opcode);

    std::vector<mcode::ArgStorage> get_arg_storage(const ssa::FunctionType &func_type);
    int get_implicit_stack_bytes(mcode::Function *func);

private:
    void emit_arg_move(codegen::SSALowerer &lowerer, ssa::Operand &operand, unsigned index, bool variadic);
    void emit_reg_arg_move(codegen::SSALowerer &lowerer, ssa::Operand &operand, unsigned index);
    void emit_reg_arg_move_variadic(codegen::SSALowerer &lowerer, ssa::Operand &operand, unsigned index);
    void emit_stack_arg_move(codegen::SSALowerer &lowerer, ssa::Operand &operand, unsigned index);

    void emit_call(codegen::SSALowerer &lowerer, const ssa::Operand &func_operand);
    void emit_ret_val_move(codegen::SSALowerer &lowerer);
};

} // namespace target

} // namespace banjo

#endif
