#include "const_evaluator.hpp"

#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

ConstEvaluator::ConstEvaluator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

LargeInt ConstEvaluator::evaluate_to_int(sir::Expr &expr) {
    return evaluate(expr).as<sir::IntLiteral>().value;
}

bool ConstEvaluator::evaluate_to_bool(sir::Expr &expr) {
    return evaluate(expr).as<sir::BoolLiteral>().value;
}

sir::Expr ConstEvaluator::evaluate(sir::Expr &expr) {
    ExprAnalyzer(analyzer).analyze(expr);

    if (auto int_literal = expr.match<sir::IntLiteral>()) return expr;
    else if (auto symbol_expr = expr.match<sir::SymbolExpr>()) return evaluate_symbol_expr(*symbol_expr);
    else if (auto bool_literal = expr.match<sir::BoolLiteral>()) return expr;
    else if (auto binary_expr = expr.match<sir::BinaryExpr>()) return evaluate_binary_expr(*binary_expr);
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) return evaluate_unary_expr(*unary_expr);
    else ASSERT_UNREACHABLE;
}

sir::Expr ConstEvaluator::evaluate_symbol_expr(sir::SymbolExpr &symbol_expr) {
    return evaluate(symbol_expr.symbol.as<sir::ConstDef>().value);
}

sir::Expr ConstEvaluator::evaluate_binary_expr(sir::BinaryExpr &binary_expr) {
    LargeInt lhs = evaluate_to_int(binary_expr.lhs);
    LargeInt rhs = evaluate_to_int(binary_expr.rhs);

    if (binary_expr.op == sir::BinaryOp::EQ) return create_bool_literal(lhs == rhs);
    else if (binary_expr.op == sir::BinaryOp::NE) return create_bool_literal(lhs != rhs);
    else ASSERT_UNREACHABLE;
}

sir::Expr ConstEvaluator::evaluate_unary_expr(sir::UnaryExpr &unary_expr) {
    LargeInt value = evaluate_to_int(unary_expr.value);

    if (unary_expr.op == sir::UnaryOp::NEG) return create_int_literal(-value);
    else ASSERT_UNREACHABLE;
}

sir::Expr ConstEvaluator::create_int_literal(LargeInt value) {
    return int_arena.create(sir::IntLiteral{
        .ast_node = nullptr,
        .type = nullptr,
        .value = value,
    });
}

sir::Expr ConstEvaluator::create_bool_literal(bool value) {
    return bool_arena.create(sir::BoolLiteral{
        .ast_node = nullptr,
        .type = nullptr,
        .value = value,
    });
}

} // namespace sema

} // namespace lang

} // namespace banjo
