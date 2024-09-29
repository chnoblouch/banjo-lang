#include "sir_create.hpp"

#include <utility>
#include <vector>

namespace banjo {

namespace lang {

namespace sir {

sir::Expr create_tuple_value(sir::Module &mod, std::vector<sir::Expr> values) {
    std::vector<sir::Expr> types(values.size());

    for (unsigned i = 0; i < values.size(); i++) {
        types[i] = values[i].get_type();
    }

    return mod.create_expr(sir::TupleExpr{
        .ast_node = nullptr,
        .type = mod.create_expr(sir::TupleExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .exprs = types,
        }),
        .exprs = values,
    });
}

sir::Expr create_primitive_type(sir::Module &mod, sir::Primitive primitive) {
    return mod.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = primitive,
    });
}

sir::Expr create_call(sir::Module &mod, sir::FuncDef &func_def, std::vector<sir::Expr> args) {
    return mod.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = func_def.type.return_type,
        .callee = mod.create_expr(sir::SymbolExpr{
            .ast_node = nullptr,
            .type = &func_def.type,
            .symbol = &func_def,
        }),
        .args = std::move(args),
    });
}

sir::Expr create_unary_ref(sir::Module &mod, sir::Expr base_value) {
    return mod.create_expr(sir::UnaryExpr{
        .ast_node = nullptr,
        .type = mod.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = base_value.get_type(),
        }),
        .op = sir::UnaryOp::REF,
        .value = base_value,
    });
}

} // namespace sir

} // namespace lang

} // namespace banjo
