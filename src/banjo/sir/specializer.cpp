#include "specializer.hpp"
#include "banjo/sir/sir.hpp"

#include <span>

namespace banjo::lang::sir {

Specializer::Specializer(utils::Arena<2048> &arena, std::span<sir::GenericParam *> params, std::span<sir::Expr> args)
  : arena{arena},
    params{params},
    args{args} {}

sir::Expr Specializer::specialize_expr(sir::Expr expr) {
    if (auto symbol_expr = expr.match<sir::SymbolExpr>()) {
        return specialize_symbol_expr(*symbol_expr);
    } else if (auto specialize_expr = expr.match<sir::SpecializeExpr>()) {
        return specialize_specialize_expr(*specialize_expr);
    } else if (auto pointer_type = expr.match<sir::PointerType>()) {
        return specialize_pointer_type(*pointer_type);
    } else if (auto func_type = expr.match<sir::FuncType>()) {
        return specialize_func_type(*func_type);
    } else if (auto reference_type = expr.match<sir::ReferenceType>()) {
        return specialize_reference_type(*reference_type);
    } else {
        return expr;
    }
}

std::span<sir::Expr> Specializer::specialize_expr_list(std::span<sir::Expr> exprs) {
    std::span<sir::Expr> clone = arena.allocate_array<sir::Expr>(exprs.size());

    for (unsigned i = 0; i < clone.size(); i++) {
        clone[i] = specialize_expr(exprs[i]);
    }

    return clone;
}

sir::FuncType Specializer::specialize_func_type_directly(sir::FuncType &func_type) {
    std::span<sir::Param> params = arena.allocate_array<sir::Param>(func_type.params.size());

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        const sir::Param &param = func_type.params[i];

        params[i] = sir::Param{
            .ast_node = param.ast_node,
            .name = param.name,
            .type = specialize_expr(param.type),
            .attrs = param.attrs,
        };
    }

    return sir::FuncType{
        .ast_node = func_type.ast_node,
        .params = params,
        .return_type = specialize_expr(func_type.return_type),
    };
}

sir::Expr Specializer::specialize_symbol_expr(sir::SymbolExpr &symbol_expr) {
    if (auto generic_param = symbol_expr.symbol.match<sir::GenericParam>()) {
        for (unsigned i = 0; i < params.size(); i++) {
            if (params[i] == generic_param) {
                return args[i];
            }
        }

        ASSERT_UNREACHABLE;
    } else {
        return &symbol_expr;
    }
}

sir::Expr Specializer::specialize_specialize_expr(sir::SpecializeExpr &specialize_expr) {
    std::span<sir::Expr> args = arena.allocate_array<sir::Expr>(specialize_expr.args.size());

    for (unsigned i = 0; i < specialize_expr.args.size(); i++) {
        args[i] = this->specialize_expr(specialize_expr.args[i]);
    }

    sir::SpecializeExpr specialization{
        .ast_node = specialize_expr.ast_node,
        .type = this->specialize_expr(specialize_expr.type),
        .symbol = specialize_expr.symbol,
        .args = args,
    };

    return arena.create<sir::SpecializeExpr>(specialization);
}

sir::Expr Specializer::specialize_pointer_type(sir::PointerType &pointer_type) {
    sir::Expr base_type = specialize_expr(pointer_type.base_type);

    if (base_type == pointer_type.base_type) {
        return &pointer_type;
    } else {
        sir::PointerType specialization{
            .ast_node = pointer_type.ast_node,
            .base_type = base_type,
        };

        return arena.create<sir::PointerType>(specialization);
    }
}

sir::Expr Specializer::specialize_func_type(sir::FuncType &func_type) {
    return arena.create<FuncType>(specialize_func_type_directly(func_type));
}

sir::Expr Specializer::specialize_reference_type(sir::ReferenceType &reference_type) {
    sir::Expr base_type = specialize_expr(reference_type.base_type);

    if (base_type == reference_type.base_type) {
        return &reference_type;
    } else {
        sir::ReferenceType specialization{
            .ast_node = reference_type.ast_node,
            .mut = reference_type.mut,
            .base_type = base_type,
        };

        return arena.create<sir::ReferenceType>(specialization);
    }
}

} // namespace banjo::lang::sir
