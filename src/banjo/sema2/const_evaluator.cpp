#include "const_evaluator.hpp"

#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
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

    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IMPOSSIBLE,
        return expr,
        SIR_VISIT_IMPOSSIBLE,
        return expr,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        return evaluate_symbol_expr(*inner),
        return evaluate_binary_expr(*inner),
        return evaluate_unary_expr(*inner),
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        return expr,
        return expr,
        return expr,
        return expr,
        return expr,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE
    );
}

sir::Expr ConstEvaluator::evaluate_symbol_expr(sir::SymbolExpr &symbol_expr) {
    SIR_VISIT_SYMBOL(
        symbol_expr.symbol,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        return evaluate(inner->value),
        return &symbol_expr,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        return &symbol_expr,
        return evaluate(inner->value),
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE
    );
}

sir::Expr ConstEvaluator::evaluate_binary_expr(sir::BinaryExpr &binary_expr) {
    sir::Expr lhs = evaluate(binary_expr.lhs);
    sir::Expr rhs = evaluate(binary_expr.rhs);

    if (lhs.is<sir::IntLiteral>() && rhs.is<sir::IntLiteral>()) {
        LargeInt lhs_int = lhs.as<sir::IntLiteral>().value;
        LargeInt rhs_int = rhs.as<sir::IntLiteral>().value;

        if (binary_expr.op == sir::BinaryOp::EQ) return create_bool_literal(lhs_int == rhs_int);
        else if (binary_expr.op == sir::BinaryOp::NE) return create_bool_literal(lhs_int != rhs_int);
        else ASSERT_UNREACHABLE;
    } else if (lhs.is<sir::BoolLiteral>() && rhs.is<sir::BoolLiteral>()) {
        bool lhs_bool = lhs.as<sir::BoolLiteral>().value;
        bool rhs_bool = rhs.as<sir::BoolLiteral>().value;

        if (binary_expr.op == sir::BinaryOp::AND) return create_bool_literal(lhs_bool && rhs_bool);
        else if (binary_expr.op == sir::BinaryOp::OR) return create_bool_literal(lhs_bool || rhs_bool);
        else ASSERT_UNREACHABLE;
    } else if (lhs.is_type() && rhs.is_type()) {
        if (binary_expr.op == sir::BinaryOp::EQ) return create_bool_literal(lhs == rhs);
        else if (binary_expr.op == sir::BinaryOp::NE) return create_bool_literal(lhs != rhs);
        else ASSERT_UNREACHABLE;
    } else {
        return create_bool_literal(false);
    }
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
