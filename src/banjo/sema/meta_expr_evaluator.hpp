#ifndef BANJO_SEMA_META_EXPR_EVALUATOR_H
#define BANJO_SEMA_META_EXPR_EVALUATOR_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <span>
#include <string_view>

namespace banjo {

namespace lang {

namespace sema {

class MetaExprEvaluator {

private:
    SemanticAnalyzer &analyzer;

public:
    MetaExprEvaluator(SemanticAnalyzer &analyzer);
    Result evaluate(sir::MetaFieldExpr &meta_field_expr, sir::Expr &out_expr);
    Result evaluate(sir::MetaCallExpr &meta_call_expr, sir::Expr &out_expr);

private:
    sir::Expr compute_name(sir::Expr &type);
    sir::Expr compute_fields(sir::Expr &type);
    sir::Expr compute_is_resource(sir::Expr &type);
    sir::Expr compute_variants(sir::Expr &type);
    sir::Expr compute_array_base(sir::Expr &base);
    sir::Expr compute_array_length(sir::Expr &base);
    sir::Expr compute_tuple_num_fields(sir::Expr &type);
    sir::Expr compute_has_method(sir::Expr &type, std::span<sir::Expr> args);
    sir::Expr compute_field(sir::Expr &base, std::span<sir::Expr> args);
    sir::Expr compute_tuple_field(sir::Expr &base, std::span<sir::Expr> args);

    sir::Expr create_int_literal(LargeInt value);
    sir::Expr create_bool_literal(bool value);
    sir::Expr create_array_literal(std::span<sir::Expr> values);
    sir::Expr create_string_literal(std::string_view value);

    sir::Expr unwrap_expr(sir::Expr expr);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
