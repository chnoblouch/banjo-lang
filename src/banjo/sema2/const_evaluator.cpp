#include "const_evaluator.hpp"

#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/sema2/meta_expr_evaluator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

ConstEvaluator::ConstEvaluator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

LargeInt ConstEvaluator::evaluate_to_int(sir::Expr &expr) {
    sir::Expr result = evaluate(expr);
    if (result) {
        return result.as<sir::IntLiteral>().value;
    } else {
        return 0;
    }
}

bool ConstEvaluator::evaluate_to_bool(sir::Expr &expr) {
    sir::Expr result = evaluate(expr);
    if (result) {
        return result.as<sir::BoolLiteral>().value;
    } else {
        return false;
    };
}

sir::Expr ConstEvaluator::evaluate(sir::Expr &expr) {
    if (auto meta_field_expr = expr.match<sir::MetaFieldExpr>()) {
        return evaluate_meta_field_expr(*meta_field_expr);
    } else if (auto meta_call_expr = expr.match<sir::MetaCallExpr>()) {
        return evaluate_meta_call_expr(*meta_call_expr);
    }

    Result result = ExprAnalyzer(analyzer).analyze(expr);
    if (result != Result::SUCCESS) {
        return nullptr;
    }

    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IMPOSSIBLE,                // empty
        return expr,                         // int_literal
        SIR_VISIT_IMPOSSIBLE,                // fp_literal
        return expr,                         // bool_literal
        SIR_VISIT_IMPOSSIBLE,                // char_literal
        SIR_VISIT_IMPOSSIBLE,                // null_literal
        SIR_VISIT_IMPOSSIBLE,                // none_literal
        SIR_VISIT_IMPOSSIBLE,                // undefined_literal
        SIR_VISIT_IMPOSSIBLE,                // array_literal
        SIR_VISIT_IMPOSSIBLE,                // string_literal
        SIR_VISIT_IMPOSSIBLE,                // struct_literal
        SIR_VISIT_IMPOSSIBLE,                // closure_literal
        return evaluate_symbol_expr(*inner), // symbol_expr
        return evaluate_binary_expr(*inner), // binary_expr
        return evaluate_unary_expr(*inner),  // unary_expr
        SIR_VISIT_IMPOSSIBLE,                // cast_expr
        SIR_VISIT_IMPOSSIBLE,                // index_expr
        SIR_VISIT_IMPOSSIBLE,                // call_expr
        SIR_VISIT_IMPOSSIBLE,                // field_expr
        SIR_VISIT_IMPOSSIBLE,                // range_expr
        return expr,                         // tuple_expr
        return expr,                         // primitive_type
        return expr,                         // pointer_type
        return expr,                         // static_array_type
        return expr,                         // func_type
        return expr,                         // optional_type
        return expr,                         // array_type
        return expr,                         // closure_type
        SIR_VISIT_IMPOSSIBLE,                // ident_expr
        SIR_VISIT_IMPOSSIBLE,                // star_expr
        SIR_VISIT_IMPOSSIBLE,                // bracket_expr
        SIR_VISIT_IMPOSSIBLE,                // dot_expr
        SIR_VISIT_IMPOSSIBLE,                // meta_access
        SIR_VISIT_IMPOSSIBLE,                // meta_field_expr
        SIR_VISIT_IMPOSSIBLE                 // meta_call_expr
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
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE
    );
}

sir::Expr ConstEvaluator::evaluate_binary_expr(sir::BinaryExpr &binary_expr) {
    sir::Expr lhs = evaluate(binary_expr.lhs);
    sir::Expr rhs = evaluate(binary_expr.rhs);

    if (!lhs || !rhs) {
        return nullptr;
    }

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

sir::Expr ConstEvaluator::evaluate_meta_field_expr(sir::MetaFieldExpr &meta_field_expr) {
    sir::Expr dummy_expr = &meta_field_expr;
    MetaExprEvaluator(analyzer).evaluate(meta_field_expr, dummy_expr);
    return dummy_expr;
}

sir::Expr ConstEvaluator::evaluate_meta_call_expr(sir::MetaCallExpr &meta_call_expr) {
    sir::Expr dummy_expr = &meta_call_expr;
    MetaExprEvaluator(analyzer).evaluate(meta_call_expr, dummy_expr);
    return dummy_expr;
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
