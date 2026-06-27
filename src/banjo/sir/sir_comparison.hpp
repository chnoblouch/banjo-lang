#ifndef BANJO_SIR_COMPARISON_H
#define BANJO_SIR_COMPARISON_H

#include "banjo/sir/sir.hpp"

#include <functional>
#include <optional>

namespace banjo::lang::sir {

class Comparison {

private:
    typedef std::function<std::optional<bool>(Expr lhs, Expr rhs)> OverrideFunc;

    std::optional<OverrideFunc> override_func;

public:
    Comparison(std::optional<OverrideFunc> override_func = {});

    bool compare(Expr lhs, Expr rhs);
    bool compare(std::span<Expr> lhs, std::span<Expr> rhs);

    bool compare(IntLiteral &lhs, IntLiteral &rhs);
    bool compare(SymbolExpr &lhs, SymbolExpr &rhs);
    bool compare(TupleExpr &lhs, TupleExpr &rhs);
    bool compare(SpecializeExpr &lhs, SpecializeExpr &rhs);
    bool compare(PrimitiveType &lhs, PrimitiveType &rhs);
    bool compare(PointerType &lhs, PointerType &rhs);
    bool compare(StaticArrayType &lhs, StaticArrayType &rhs);
    bool compare(FuncType &lhs, FuncType &rhs, unsigned first_param = 0);
    bool compare(ClosureType &lhs, ClosureType &rhs);
    bool compare(ReferenceType &lhs, ReferenceType &rhs);
    bool compare(MetaAccess &lhs, MetaAccess &rhs);
    bool compare(MetaFieldExpr &lhs, MetaFieldExpr &rhs);
};

}; // namespace banjo::lang::sir

#endif
