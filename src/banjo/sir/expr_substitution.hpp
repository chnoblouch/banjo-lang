#ifndef BANJO_SIR_EXPR_SUBSTITUTION_H
#define BANJO_SIR_EXPR_SUBSTITUTION_H

#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"

#include <functional>
#include <span>

namespace banjo::sir {

Expr concretize_type(utils::Arena &arena, Expr expr, Expr self_type);

class ExprSubstitution {

public:
    typedef std::function<Expr(Expr expr)> SubstituteFunc;

private:
    utils::Arena &arena;
    SubstituteFunc func;

public:
    ExprSubstitution(utils::Arena &arena, SubstituteFunc func);
    Expr apply(Expr expr);
    std::span<sir::Expr> apply_list(std::span<Expr> exprs);

private:
    Expr visit_symbol_expr(SymbolExpr &symbol_expr);
    Expr visit_tuple_expr(TupleExpr &tuple_expr);
    Expr visit_specialize_expr(SpecializeExpr &specialize_expr);
    Expr visit_pointer_type(PointerType &pointer_type);
    Expr visit_static_array_type(StaticArrayType &static_array_type);
    Expr visit_func_type(FuncType &func_type);
    Expr visit_closure_type(ClosureType &closure_type);
    Expr visit_reference_type(ReferenceType &reference_type);

    FuncType visit_func_type_inline(FuncType &func_type);
    std::span<Param> visit_param_list(std::span<Param> params);
};

} // namespace banjo::sir

#endif
