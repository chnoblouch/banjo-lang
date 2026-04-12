#ifndef BANJO_SIR_SPECIALIZER_H
#define BANJO_SIR_SPECIALIZER_H

#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"

#include <span>

namespace banjo::lang::sir {

class Specializer {

private:
    utils::Arena<2048> &arena;
    std::span<sir::GenericParam *> params;
    std::span<sir::Expr> args;

public:
    Specializer(utils::Arena<2048> &arena, std::span<sir::GenericParam *> params, std::span<sir::Expr> args);
    sir::Expr specialize_expr(sir::Expr expr);
    std::span<sir::Expr> specialize_expr_list(std::span<sir::Expr> exprs);
    sir::FuncType specialize_func_type_directly(sir::FuncType &func_type);

    sir::Expr specialize_symbol_expr(sir::SymbolExpr &symbol_expr);
    sir::Expr specialize_specialize_expr(sir::SpecializeExpr &specialize_expr);
    sir::Expr specialize_pointer_type(sir::PointerType &pointer_type);
    sir::FuncType *specialize_func_type(sir::FuncType &func_type);
    sir::Expr specialize_reference_type(sir::ReferenceType &reference_type);
};

} // namespace banjo::lang::sir

#endif
