#include "const_evaluator.hpp"

#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

ConstEvaluator::ConstEvaluator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

LargeInt ConstEvaluator::evaluate_to_int(const sir::Expr &expr) {
    if (auto int_literal = expr.match<sir::IntLiteral>()) return int_literal->value;
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) return evaluate_unary_expr(*unary_expr);
    else ASSERT_UNREACHABLE;
}

LargeInt ConstEvaluator::evaluate_unary_expr(const sir::UnaryExpr &unary_expr) {
    LargeInt value = evaluate_to_int(unary_expr.value);

    if (unary_expr.op == sir::UnaryOp::NEG) return -value;
    else ASSERT_UNREACHABLE;
}

} // namespace sema

} // namespace lang

} // namespace banjo
