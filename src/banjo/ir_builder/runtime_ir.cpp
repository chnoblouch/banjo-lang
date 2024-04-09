#include "runtime_ir.hpp"

namespace ir_builder {

void RuntimeIR::insert(IRBuilderContext &context) {
    context.get_current_mod()->add(ir::FunctionDecl(
        "memcpy",
        {ir::Type(ir::Primitive::VOID, 1), ir::Type(ir::Primitive::VOID, 1), ir::Type(ir::Primitive::I64)},
        ir::Type(ir::Primitive::VOID),
        ir::CallingConv::X86_64_MS_ABI
    ));

    context.get_current_mod()->add(ir::FunctionDecl(
        "malloc",
        {ir::Type(ir::Primitive::I64)},
        ir::Type(ir::Primitive::VOID, 1),
        ir::CallingConv::X86_64_MS_ABI
    ));

    context.get_current_mod()->add(ir::FunctionDecl(
        "realloc",
        {ir::Type(ir::Primitive::VOID, 1), ir::Type(ir::Primitive::I64)},
        ir::Type(ir::Primitive::VOID, 1),
        ir::CallingConv::X86_64_MS_ABI
    ));
}

} // namespace ir_builder
