#ifndef META_EXPR_EVALUATOR_H
#define META_EXPR_EVALUATOR_H

#include "banjo/sema2/semantic_analyzer.hpp"
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
    sir::Expr has_method(sir::Expr &type, const std::string &name);

    sir::Expr create_bool_literal(bool value);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
