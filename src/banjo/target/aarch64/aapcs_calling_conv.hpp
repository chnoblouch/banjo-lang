#ifndef AAPCS_CALLING_CONV_H
#define AAPCS_CALLING_CONV_H

#include "mcode/calling_convention.hpp"
#include "mcode/register.hpp"

namespace target {

class AAPCSCallingConv : public mcode::CallingConvention {

public:
    static const AAPCSCallingConv INSTANCE;
    static const std::vector<int> GENERAL_ARG_REGS;
    static const std::vector<int> FLOAT_ARG_REGS;

public:
    AAPCSCallingConv();
    void lower_call(codegen::IRLowerer &lowerer, ir::Instruction &instr);
    void create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_call_arg_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    void create_implicit_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions);
    int get_alloca_size(mcode::StackRegions &regions);
    std::vector<mcode::Instruction> get_prolog(mcode::Function *func);
    std::vector<mcode::Instruction> get_epilog(mcode::Function *func);

    mcode::InstrIter fix_up_instr(mcode::BasicBlock &block, mcode::InstrIter iter, TargetRegAnalyzer &analyzer);

    bool is_func_exit(mcode::Opcode opcode);

    std::vector<mcode::ArgStorage> get_arg_storage(const std::vector<ir::Type> &types);
    int get_implicit_stack_bytes(mcode::Function *func);

private:
    mcode::Register get_arg_reg(ir::Operand &operand, int index, codegen::IRLowerer &lowerer);
};

} // namespace target

#endif
