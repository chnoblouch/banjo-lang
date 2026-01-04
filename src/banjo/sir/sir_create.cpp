#include "sir_create.hpp"

#include "banjo/sir/sir.hpp"

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

sir::Expr create_pseudo_type(sir::Module &mod, sir::PseudoTypeKind kind) {
    return mod.create(
        sir::PseudoType{
            .ast_node = nullptr,
            .kind = kind,
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

sir::Stmt create_return_result_success_void(sir::Module &mod, sir::StructDef &result_def) {
    sir::Symbol new_func = result_def.block.symbol_table->look_up_local("new_success");

    sir::Expr type = mod.create(
        sir::SymbolExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .symbol = &result_def,
        }
    );

    sir::Expr callee = mod.create(
        sir::SymbolExpr{
            .ast_node = nullptr,
            .type = &new_func.as<sir::FuncDef>().type,
            .symbol = new_func,
        }
    );

    sir::Expr value = mod.create(
        sir::UndefinedLiteral{
            .ast_node = nullptr,
            .type = result_def.parent_specialization->args[0],
        }
    );

    sir::Expr call_expr = mod.create(
        sir::CallExpr{
            .ast_node = nullptr,
            .type = type,
            .callee = callee,
            .args = mod.create_array<sir::Expr>({value}),
        }
    );

    return mod.create(
        sir::ReturnStmt{
            .ast_node = nullptr,
            .value = call_expr,
        }
    );
}

} // namespace sir

} // namespace lang

} // namespace banjo
