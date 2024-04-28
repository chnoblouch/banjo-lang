#include "runtime_ir.hpp"

namespace ir_builder {

void RuntimeIR::insert(IRBuilderContext &context) {
    context.get_current_mod()->add(ir::FunctionDecl(
        "memcpy",
        {ir::Primitive::ADDR, ir::Primitive::ADDR, ir::Primitive::I64},
        ir::Primitive::VOID,
        ir::CallingConv::X86_64_MS_ABI
    ));

    context.get_current_mod()->add(
        ir::FunctionDecl("malloc", {ir::Primitive::I64}, ir::Primitive::ADDR, ir::CallingConv::X86_64_MS_ABI)
    );

    context.get_current_mod()->add(ir::FunctionDecl(
        "realloc",
        {ir::Primitive::ADDR, ir::Primitive::I64},
        ir::Primitive::ADDR,
        ir::CallingConv::X86_64_MS_ABI
    ));
}

} // namespace ir_builder
