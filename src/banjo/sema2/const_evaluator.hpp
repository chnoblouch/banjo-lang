#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class ConstEvaluator {

private:
    SemanticAnalyzer analyzer;

public:
    ConstEvaluator(SemanticAnalyzer &analyzer);
    LargeInt evaluate_to_int(const sir::Expr &expr);

private:
    LargeInt evaluate_unary_expr(const sir::UnaryExpr &unary_expr);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
