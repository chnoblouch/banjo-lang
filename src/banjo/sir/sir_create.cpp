#include "sir_create.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/specializer.hpp"

namespace banjo::lang::sir {

Expr create_primitive_type(Module &mod, Primitive primitive) {
    return mod.create(
        PrimitiveType{
            .ast_node = nullptr,
            .primitive = primitive,
        }
    );
}

Expr create_pseudo_type(Module &mod, PseudoTypeKind kind) {
    return mod.create(
        PseudoType{
            .ast_node = nullptr,
            .kind = kind,
        }
    );
}

CallExpr *create_call(Module &mod, Concrete<FuncDef> concrete_func, std::span<Expr> args) {
    if (concrete_func.generic_args.empty()) {
        return create_call(mod, *concrete_func.def, args);
    }

    Specializer specializer{mod.trivial_arena, concrete_func};
    FuncType *func_type = specializer.specialize_func_type(concrete_func.def->type);

    return mod.create(
        CallExpr{
            .ast_node = nullptr,
            .type = func_type->return_type,
            .callee = mod.create(
                SpecializeExpr{
                    .ast_node = nullptr,
                    .type = func_type,
                    .symbol = concrete_func.def,
                    .args = concrete_func.generic_args,
                }
            ),
            .args = args,
        }
    );
}

CallExpr *create_call(Module &mod, FuncDef &func_def, std::span<Expr> args) {
    return mod.create(
        CallExpr{
            .ast_node = nullptr,
            .type = func_def.type.return_type,
            .callee = mod.create(
                SymbolExpr{
                    .ast_node = nullptr,
                    .type = &func_def.type,
                    .symbol = &func_def,
                }
            ),
            .args = args,
        }
    );
}

Expr create_unary_ref(Module &mod, Expr base_value) {
    return mod.create(
        UnaryExpr{
            .ast_node = nullptr,
            .type = mod.create(
                PointerType{
                    .ast_node = nullptr,
                    .base_type = base_value.get_type(),
                }
            ),
            .op = UnaryOp::ADDR,
            .value = base_value,
        }
    );
}

Expr create_error_value(Module &mod, ASTNode *ast_node) {
    return mod.create(
        Error{
            .ast_node = ast_node,
        }
    );
}

Expr create_result_success_void(Module &mod, Concrete<StructDef> concrete_struct) {
    Specializer specializer{mod.trivial_arena, concrete_struct};
    FuncDef &new_func = concrete_struct.def->block.symbol_table->look_up_local("new_success").as<FuncDef>();

    Expr type = mod.create(
        SpecializeExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .symbol = concrete_struct.def,
            .args = concrete_struct.generic_args,
        }
    );

    Expr callee = mod.create(
        SpecializeExpr{
            .ast_node = nullptr,
            .type = specializer.specialize_func_type(new_func.type),
            .symbol = &new_func,
            .args = concrete_struct.generic_args,
        }
    );

    Expr value = mod.create(
        UndefinedLiteral{
            .ast_node = nullptr,
            .type = concrete_struct.generic_args[0],
        }
    );

    return mod.create(
        CallExpr{
            .ast_node = nullptr,
            .type = type,
            .callee = callee,
            .args = mod.create_array<Expr>({value}),
        }
    );
}

Stmt create_return_result_success_void(Module &mod, Concrete<StructDef> concrete_struct) {
    return mod.create(
        ReturnStmt{
            .ast_node = nullptr,
            .value = create_result_success_void(mod, concrete_struct),
        }
    );
}

} // namespace banjo::lang::sir
