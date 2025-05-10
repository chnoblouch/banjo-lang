#ifndef META_EXPR_EVALUATOR_H
#define META_EXPR_EVALUATOR_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <string>

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
    sir::Expr compute_has_method(sir::Expr &type, const std::vector<sir::Expr> &args);
    sir::Expr compute_field(sir::Expr &base, const std::vector<sir::Expr> &args);

    sir::Expr create_bool_literal(bool value);
    sir::Expr create_array_literal(std::vector<sir::Expr> values);
    sir::Expr create_string_literal(std::string value);

    sir::Expr unwrap_expr(sir::Expr expr);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
