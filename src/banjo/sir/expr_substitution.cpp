#include "expr_substitution.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"

#include <optional>
#include <span>
#include <utility>

namespace banjo::sir {

Expr concretize_type(utils::Arena &arena, Expr expr, Expr self_type) {
    auto substitute = [self_type](Expr expr) {
        if (expr.is_pseudo_type(PseudoTypeKind::SELF_TYPE)) {
            return self_type;
        } else {
            return Expr{nullptr};
        }
    };

    return ExprSubstitution{arena, substitute}.apply(expr);
}

ExprSubstitution::ExprSubstitution(utils::Arena &arena, SubstituteFunc func) : arena{arena}, func{std::move(func)} {}

Expr ExprSubstitution::apply(Expr expr) {
    Expr replacement = func(expr);
    if (replacement) {
        return replacement;
    }

    if (auto symbol_expr = expr.match<SymbolExpr>()) {
        return visit_symbol_expr(*symbol_expr);
    } else if (auto tuple_expr = expr.match<TupleExpr>()) {
        return visit_tuple_expr(*tuple_expr);
    } else if (auto specialize_expr = expr.match<SpecializeExpr>()) {
        return visit_specialize_expr(*specialize_expr);
    } else if (auto pointer_type = expr.match<PointerType>()) {
        return visit_pointer_type(*pointer_type);
    } else if (auto static_array_type = expr.match<StaticArrayType>()) {
        return visit_static_array_type(*static_array_type);
    } else if (auto func_type = expr.match<FuncType>()) {
        return visit_func_type(*func_type);
    } else if (auto closure_type = expr.match<ClosureType>()) {
        return visit_closure_type(*closure_type);
    } else if (auto reference_type = expr.match<ReferenceType>()) {
        return visit_reference_type(*reference_type);
    } else {
        return expr;
    }
}

std::span<sir::Expr> ExprSubstitution::apply_list(std::span<Expr> exprs) {
    std::optional<std::span<Expr>> replacements;

    for (unsigned i = 0; i < exprs.size(); i++) {
        Expr expr = exprs[i];
        Expr replacement = apply(expr);

        if (replacement == expr) {
            continue;
        }

        if (!replacements) {
            replacements = arena.allocate_array<Expr>(exprs.size());

            for (unsigned i = 0; i < exprs.size(); i++) {
                (*replacements)[i] = exprs[i];
            }
        }

        (*replacements)[i] = replacement;
    }

    return replacements ? *replacements : exprs;
}

Expr ExprSubstitution::visit_symbol_expr(SymbolExpr &symbol_expr) {
    Expr type_replacement = apply(symbol_expr.type);
    if (type_replacement == symbol_expr.type) {
        return &symbol_expr;
    }

    return arena.create<SymbolExpr>({
        .ast_node = symbol_expr.ast_node,
        .type = type_replacement,
        .symbol = symbol_expr.symbol,
    });
}

Expr ExprSubstitution::visit_tuple_expr(TupleExpr &tuple_expr) {
    ASSERT(!tuple_expr.type);

    std::span<Expr> replacements = apply_list(tuple_expr.exprs);
    if (replacements.data() == tuple_expr.exprs.data()) {
        return &tuple_expr;
    }

    return arena.create<TupleExpr>({
        .ast_node = tuple_expr.ast_node,
        .type = nullptr,
        .exprs = replacements,
    });
}

Expr ExprSubstitution::visit_specialize_expr(SpecializeExpr &specialize_expr) {
    ASSERT(!specialize_expr.type);

    std::span<Expr> replacements = apply_list(specialize_expr.args);
    if (replacements.data() == specialize_expr.args.data()) {
        return &specialize_expr;
    }

    return arena.create<SpecializeExpr>({
        .ast_node = specialize_expr.ast_node,
        .type = nullptr,
        .symbol = specialize_expr.symbol,
        .args = replacements,
    });
}

Expr ExprSubstitution::visit_pointer_type(PointerType &pointer_type) {
    Expr base_replacement = apply(pointer_type.base_type);
    if (base_replacement == pointer_type.base_type) {
        return &pointer_type;
    }

    return arena.create<PointerType>({
        .ast_node = pointer_type.ast_node,
        .base_type = base_replacement,
    });
}

Expr ExprSubstitution::visit_static_array_type(StaticArrayType &static_array_type) {
    // TODO: Specialize length (e.g. if it's a meta expression)

    Expr base_replacement = apply(static_array_type.base_type);
    if (base_replacement == static_array_type.base_type) {
        return &static_array_type;
    }

    return arena.create<StaticArrayType>({
        .ast_node = static_array_type.ast_node,
        .base_type = base_replacement,
        .length = static_array_type.length,
    });
}

Expr ExprSubstitution::visit_func_type(FuncType &func_type) {
    std::span<Param> param_replacements = visit_param_list(func_type.params);
    Expr return_replacement = apply(func_type.return_type);

    if (param_replacements.data() == func_type.params.data() && return_replacement == func_type.return_type) {
        return &func_type;
    }

    return arena.create<FuncType>({
        .ast_node = func_type.ast_node,
        .params = param_replacements,
        .return_type = return_replacement,
    });
}

Expr ExprSubstitution::visit_closure_type(ClosureType &closure_type) {
    FuncType &func_type = closure_type.func_type;
    std::span<Param> param_replacements = visit_param_list(func_type.params);
    Expr return_replacement = apply(func_type.return_type);

    if (param_replacements.data() == func_type.params.data() && return_replacement == func_type.return_type) {
        return &closure_type;
    }

    return arena.create<ClosureType>({
        .ast_node = closure_type.ast_node,
        .func_type{
            .ast_node = func_type.ast_node,
            .params = param_replacements,
            .return_type = return_replacement,
        },
        .underlying_struct = closure_type.underlying_struct,
    });
}

Expr ExprSubstitution::visit_reference_type(ReferenceType &reference_type) {
    Expr base_replacement = apply(reference_type.base_type);
    if (base_replacement == reference_type.base_type) {
        return &reference_type;
    }

    return arena.create<ReferenceType>({
        .ast_node = reference_type.ast_node,
        .mut = reference_type.mut,
        .base_type = base_replacement,
    });
}

FuncType ExprSubstitution::visit_func_type_inline(FuncType &func_type) {
    return FuncType{
        .ast_node = func_type.ast_node,
        .params = visit_param_list(func_type.params),
        .return_type = apply(func_type.return_type),
    };
}

std::span<Param> ExprSubstitution::visit_param_list(std::span<Param> params) {
    std::optional<std::span<Param>> replacements;

    for (unsigned i = 0; i < params.size(); i++) {
        Expr type = params[i].type;
        Expr type_replacement = apply(type);

        if (type_replacement == type) {
            continue;
        }

        if (!replacements) {
            replacements = arena.allocate_array<Param>(params.size());

            for (unsigned i = 0; i < params.size(); i++) {
                (*replacements)[i] = params[i];
            }
        }

        (*replacements)[i].type = type_replacement;
    }

    return replacements ? *replacements : params;
}

} // namespace banjo::sir
