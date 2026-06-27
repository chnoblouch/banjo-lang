#include "sir_comparison.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"

#include <utility>

namespace banjo::lang::sir {

Comparison::Comparison(std::optional<OverrideFunc> override_func /* = {} */)
  : override_func{std::move(override_func)} {}

bool Comparison::compare(Expr lhs, Expr rhs) {
    if (override_func) {
        if (auto result = (*override_func)(lhs, rhs)) {
            return *result;
        }
    }

    if (lhs.kind.index() != rhs.kind.index()) {
        return false;
    }

    SIR_VISIT_EXPR(
        lhs,
        return true,                                       // empty
        return compare(*inner, rhs.as<IntLiteral>()),      // int_literal
        return false,                                      // fp_literal
        return false,                                      // bool_literal
        return false,                                      // char_literal
        return false,                                      // null_literal
        return false,                                      // none_literal
        return false,                                      // undefined_literal
        return false,                                      // array_literal
        return false,                                      // string_literal
        return false,                                      // struct_literal
        return false,                                      // union_case_literal
        return false,                                      // map_literal
        return false,                                      // closure_literal
        return compare(*inner, rhs.as<SymbolExpr>()),      // symbol_expr
        return false,                                      // binary_expr
        return false,                                      // unary_expr
        return false,                                      // cast_expr
        return false,                                      // index_expr
        return false,                                      // call_expr
        return false,                                      // field_expr
        return false,                                      // range_expr
        return false,                                      // try_expr
        return compare(*inner, rhs.as<TupleExpr>()),       // tuple_expr
        return false,                                      // coercion_expr
        return compare(*inner, rhs.as<SpecializeExpr>()),  // specialize_expr
        return compare(*inner, rhs.as<PrimitiveType>()),   // primitive_type
        return compare(*inner, rhs.as<PointerType>()),     // pointer_type
        return compare(*inner, rhs.as<StaticArrayType>()), // static_array_type
        return compare(*inner, rhs.as<FuncType>()),        // func_type
        return false,                                      // optional_type
        return false,                                      // result_type
        return false,                                      // array_type
        return false,                                      // map_type
        return compare(*inner, rhs.as<ClosureType>()),     // closure_type
        return compare(*inner, rhs.as<ReferenceType>()),   // reference_type
        return false,                                      // ident_expr
        return false,                                      // star_expr
        return false,                                      // bracket_expr
        return false,                                      // dot_expr
        return false,                                      // pseudo_type
        return compare(*inner, rhs.as<MetaAccess>()),      // meta_access
        return compare(*inner, rhs.as<MetaFieldExpr>()),   // meta_field_expr
        return false,                                      // meta_call_expr
        return false,                                      // init_expr
        return false,                                      // move_expr
        return false,                                      // deinit_expr
        return false,                                      // type_guard_expr
        return false,                                      // placeholder_expr
        return false                                       // error
    );
}

bool Comparison::compare(std::span<Expr> lhs, std::span<Expr> rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (unsigned i = 0; i < lhs.size(); i++) {
        if (!compare(lhs[i], rhs[i])) {
            return false;
        }
    }

    return true;
}

bool Comparison::compare(IntLiteral &lhs, IntLiteral &rhs) {
    return lhs.value == rhs.value;
}

bool Comparison::compare(SymbolExpr &lhs, SymbolExpr &rhs) {
    return lhs.symbol == rhs.symbol;
}

bool Comparison::compare(TupleExpr &lhs, TupleExpr &rhs) {
    return compare(lhs.exprs, rhs.exprs);
}

bool Comparison::compare(SpecializeExpr &lhs, SpecializeExpr &rhs) {
    return lhs.symbol == rhs.symbol && compare(lhs.args, rhs.args);
}

bool Comparison::compare(PrimitiveType &lhs, PrimitiveType &rhs) {
    return lhs.primitive == rhs.primitive;
}

bool Comparison::compare(PointerType &lhs, PointerType &rhs) {
    return compare(lhs.base_type, rhs.base_type);
}

bool Comparison::compare(StaticArrayType &lhs, StaticArrayType &rhs) {
    return compare(lhs.base_type, rhs.base_type) && compare(lhs.length, rhs.length);
}

bool Comparison::compare(FuncType &lhs, FuncType &rhs, unsigned first_param /* = 0 */) {
    if (lhs.params.size() != rhs.params.size()) {
        return false;
    }

    for (unsigned i = first_param; i < lhs.params.size(); i++) {
        if (!compare(lhs.params[i].type, rhs.params[i].type)) {
            return false;
        }
    }

    return compare(lhs.return_type, rhs.return_type);
}

bool Comparison::compare(ClosureType &lhs, ClosureType &rhs) {
    return compare(lhs.func_type, rhs.func_type);
}

bool Comparison::compare(ReferenceType &lhs, ReferenceType &rhs) {
    return lhs.mut == rhs.mut && compare(lhs.base_type, rhs.base_type);
}

bool Comparison::compare(MetaAccess &lhs, MetaAccess &rhs) {
    return compare(lhs.expr, rhs.expr);
}

bool Comparison::compare(MetaFieldExpr &lhs, MetaFieldExpr &rhs) {
    return compare(lhs.base, rhs.base) && lhs.field == rhs.field;
}

} // namespace banjo::lang::sir
