#ifndef BANJO_SIR_SPECIALIZER_H
#define BANJO_SIR_SPECIALIZER_H

#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"

#include <span>

namespace banjo::sir {

class Specializer {

private:
    utils::Arena &arena;
    std::span<sir::GenericParam *> params;
    std::span<sir::Expr> args;

public:
    Specializer(utils::Arena &arena, std::span<sir::GenericParam *> params, std::span<sir::Expr> args);

    template <typename T>
    Specializer(utils::Arena &arena, sir::Concrete<T> specialization)
      : Specializer{arena, specialization.def->generic_params, specialization.generic_args} {}

    sir::Expr specialize_expr(sir::Expr expr);
    std::span<sir::Expr> specialize_expr_list(std::span<sir::Expr> exprs);
    sir::FuncType *specialize_func_type(sir::FuncType &func_type);

private:
    sir::Expr substitute(sir::Expr expr);
};

} // namespace banjo::sir

#endif
