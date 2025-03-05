#ifndef SYSV_CALLING_CONV_H
#define SYSV_CALLING_CONV_H

#include "banjo/mcode/calling_convention.hpp"

namespace ssa {
class Operand;
} // namespace ssa

// TODO: reformat
namespace codegen {
class Operand;
} // namespace codegen

namespace banjo {

namespace target {

class SysVCallingConv : public mcode::CallingConvention {

public:
    static const SysVCallingConv INSTANCE;
    static const std::vector<int> ARG_REGS_INT;
    static const std::vector<int> ARG_REGS_FLOAT;

public:
    SysVCallingConv();
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
    mcode::Operand get_arg_dst(mcode::ArgStorage &storage, codegen::SSALowerer &lowerer);
    void append_call(ssa::Operand func_operand, codegen::SSALowerer &lowerer);
    void append_ret_val_move(codegen::SSALowerer &lowerer);
};

} // namespace target

} // namespace banjo

#endif
