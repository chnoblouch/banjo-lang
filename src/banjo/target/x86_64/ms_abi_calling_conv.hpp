#ifndef MS_ABI_CALLING_CONV_H
#define MS_ABI_CALLING_CONV_H

#include "mcode/calling_convention.hpp"

namespace ir {
class Operand;
} // namespace ir

// TODO: reformat
namespace mcode {
class Operand;
} // namespace mcode

namespace target {

class MSABICallingConv : public mcode::CallingConvention {

public:
    static const MSABICallingConv INSTANCE;
    static const std::vector<int> ARG_REGS_INT;
    static const std::vector<int> ARG_REGS_FLOAT;

public:
    MSABICallingConv();
    void lower_call(codegen::IRLowerer &lowerer, ir::Instruction &instr);
    void create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_call_arg_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_implicit_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    int get_alloca_size(mcode::StackRegions &regions);
    std::vector<mcode::Instruction> get_prolog(mcode::Function *func);
    std::vector<mcode::Instruction> get_epilog(mcode::Function *func);
    bool is_func_exit(mcode::Opcode opcode);

    std::vector<mcode::ArgStorage> get_arg_storage(const std::vector<ir::Type> &types);
    int get_implicit_stack_bytes(mcode::Function *func);

private:
    mcode::Register get_arg_reg(ir::Operand &operand, int index, codegen::IRLowerer &lowerer);
    void append_arg_move(ir::Operand &operand, mcode::Operand &src, mcode::Register reg, codegen::IRLowerer &lowerer);
    void append_call(ir::Operand func_operand, codegen::IRLowerer &lowerer);
    void append_ret_val_move(codegen::IRLowerer &lowerer);
};

} // namespace target

#endif
