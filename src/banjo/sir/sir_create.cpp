#include "sir_create.hpp"

#include <utility>
#include <vector>

namespace banjo {

namespace lang {

namespace sir {

sir::Expr create_primitive_type(sir::Module &mod, sir::Primitive primitive) {
    return mod.create(
        sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = primitive,
        }
    );
}

sir::Expr create_call(sir::Module &mod, sir::FuncDef &func_def, std::span<sir::Expr> args) {
    return mod.create(
        sir::CallExpr{
            .ast_node = nullptr,
            .type = func_def.type.return_type,
            .callee = mod.create(
                sir::SymbolExpr{
                    .ast_node = nullptr,
                    .type = &func_def.type,
                    .symbol = &func_def,
                }
            ),
            .args = args,
        }
    );
}

sir::Expr create_unary_ref(sir::Module &mod, sir::Expr base_value) {
    return mod.create(
        sir::UnaryExpr{
            .ast_node = nullptr,
            .type = mod.create(
                sir::PointerType{
                    .ast_node = nullptr,
                    .base_type = base_value.get_type(),
                }
            ),
            .op = sir::UnaryOp::REF,
            .value = base_value,
        }
    );
}

sir::Expr create_error_value(sir::Module &mod, ASTNode *ast_node) {
    return mod.create(
        sir::Error{
            .ast_node = ast_node,
        }
    );
}

} // namespace sir

} // namespace lang

} // namespace banjo
