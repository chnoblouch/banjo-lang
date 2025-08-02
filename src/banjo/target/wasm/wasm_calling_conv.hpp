#ifndef BANJO_TARGET_WASM_CALLING_CONV_H
#define BANJO_TARGET_WASM_CALLING_CONV_H

#include "banjo/mcode/calling_convention.hpp"

namespace banjo::target {

class WasmCallingConv final : public mcode::CallingConvention {

public:
    static WasmCallingConv INSTANCE;

    void lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr) override;
    void create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions) override;
    void create_call_arg_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions) override;
    void create_implicit_region(mcode::Function *func, mcode::StackFrame &frame, mcode::StackRegions &regions) override;
    int get_alloca_size(mcode::StackRegions &regions) override;
    std::vector<mcode::Instruction> get_prolog(mcode::Function *func) override;
    std::vector<mcode::Instruction> get_epilog(mcode::Function *func) override;
    bool is_func_exit(mcode::Opcode opcode) override;
    std::vector<mcode::ArgStorage> get_arg_storage(const ssa::FunctionType &func_type) override;
    int get_implicit_stack_bytes(mcode::Function *func) override;
};

} // namespace banjo::target

#endif
