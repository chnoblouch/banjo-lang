#include "specializer.hpp"

#include "banjo/sir/expr_substitution.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <span>

namespace banjo::sir {

Specializer::Specializer(utils::Arena &arena, std::span<sir::GenericParam *> params, std::span<sir::Expr> args)
  : arena{arena},
    params{params},
    args{args} {
    ASSERT(params.size() == args.size());
}

sir::Expr Specializer::specialize_expr(sir::Expr expr) {
    auto func = [this](sir::Expr expr) { return substitute(expr); };
    return sir::ExprSubstitution{arena, func}.apply(expr);
}

std::span<sir::Expr> Specializer::specialize_expr_list(std::span<sir::Expr> exprs) {
    auto func = [this](sir::Expr expr) { return substitute(expr); };
    return sir::ExprSubstitution{arena, func}.apply_list(exprs);
}

sir::FuncType *Specializer::specialize_func_type(sir::FuncType &func_type) {
    return &specialize_expr(&func_type).as<sir::FuncType>();
}

sir::Expr Specializer::substitute(sir::Expr expr) {
    if (auto generic_param = expr.match_symbol<sir::GenericParam>()) {
        for (unsigned i = 0; i < params.size(); i++) {
            if (params[i] == generic_param) {
                return args[i];
            }
        }

        // TODO: This should actually be unreachable, but we can't completely
        // specialize when comparing constraints of generic parameters in
        // sir::type_constraints
        // ASSERT_UNREACHABLE;
    }

    return nullptr;
}

} // namespace banjo::sir
