#include "const_evaluator.hpp"

#include "banjo/sema/decl_body_analyzer.hpp"
#include "banjo/sema/meta_expr_evaluator.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

ConstEvaluator::ConstEvaluator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

LargeInt ConstEvaluator::evaluate_to_int(sir::Expr &expr) {
    Output evaluated = evaluate(expr);
    if (evaluated.result != Result::SUCCESS) {
        return 0;
    }

    if (auto int_literal = evaluated.expr.match<sir::IntLiteral>()) {
        return int_literal->value;
    } else {
        return 0;
    }
}

bool ConstEvaluator::evaluate_to_bool(sir::Expr &expr) {
    Output evaluated = evaluate(expr);
    if (evaluated.result != Result::SUCCESS) {
        return 0;
    }

    if (auto bool_literal = evaluated.expr.match<sir::BoolLiteral>()) {
        return bool_literal->value;
    } else {
        return false;
    }
}

ConstEvaluator::Output ConstEvaluator::evaluate(sir::Expr &expr) {
    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IMPOSSIBLE,                    // empty
        return expr,                             // int_literal
        return expr,                             // fp_literal
        return expr,                             // bool_literal
        return expr,                             // char_literal
        return expr,                             // null_literal
        return evaluate_non_const(expr),         // none_literal
        return evaluate_non_const(expr),         // undefined_literal
        return evaluate_array_literal(*inner),   // array_literal
        return expr,                             // string_literal
        return evaluate_non_const(expr),         // struct_literal
        return evaluate_non_const(expr),         // union_case_literal
        return evaluate_non_const(expr),         // map_literal
        return evaluate_non_const(expr),         // closure_literal
        return evaluate_symbol_expr(*inner),     // symbol_expr
        return evaluate_binary_expr(*inner),     // binary_expr
        return evaluate_unary_expr(*inner),      // unary_expr
        return evaluate_non_const(expr),         // cast_expr
        return evaluate_non_const(expr),         // index_expr
        return evaluate_non_const(expr),         // call_expr
        return evaluate_non_const(expr),         // field_expr
        return evaluate_non_const(expr),         // range_expr
        return evaluate_tuple_expr(*inner),      // tuple_expr
        return evaluate_non_const(expr),         // coercion_expr
        return expr,                             // primitive_type
        return expr,                             // pointer_type
        return expr,                             // static_array_type
        return expr,                             // func_type
        return expr,                             // optional_type
        return expr,                             // result_type
        return expr,                             // array_type
        return expr,                             // map_type
        return expr,                             // closure_type
        return expr,                             // reference_type
        return expr,                             // ident_expr
        return expr,                             // star_expr
        return expr,                             // bracket_expr
        return expr,                             // dot_expr
        return expr,                             // pseudo_tpe
        return expr,                             // meta_access
        return evaluate_meta_field_expr(*inner), // meta_field_expr
        return evaluate_meta_call_expr(*inner),  // meta_call_expr
        return expr,                             // init_expr
        return expr,                             // move_expr
        return expr,                             // deinit_expr
        return nullptr                           // error
    );
}

ConstEvaluator::Output ConstEvaluator::evaluate_array_literal(sir::ArrayLiteral &array_literal) {
    sir::ArrayLiteral result{
        .ast_node = array_literal.ast_node,
        .type = clone(array_literal.type),
        .values = analyzer.allocate_array<sir::Expr>(array_literal.values.size()),
    };

    for (unsigned i = 0; i < array_literal.values.size(); i++) {
        Output value = evaluate(array_literal.values[i]);
        if (value.result != Result::SUCCESS) {
            return value.result;
        }

        result.values[i] = value.expr;
    }

    return {Result::SUCCESS, analyzer.create(result)};
}

ConstEvaluator::Output ConstEvaluator::evaluate_symbol_expr(sir::SymbolExpr &symbol_expr) {
    SIR_VISIT_SYMBOL(
        symbol_expr.symbol,
        SIR_VISIT_IMPOSSIBLE,                      // empty
        SIR_VISIT_IMPOSSIBLE,                      // module
        SIR_VISIT_IMPOSSIBLE,                      // func_def
        SIR_VISIT_IMPOSSIBLE,                      // func_decl
        SIR_VISIT_IMPOSSIBLE,                      // native_func_decl
        return evaluate_const_def_value(*inner),   // const_def
        return sir::Expr(&symbol_expr),            // struct_def
        SIR_VISIT_IMPOSSIBLE,                      // struct_field
        SIR_VISIT_IMPOSSIBLE,                      // var_decl
        SIR_VISIT_IMPOSSIBLE,                      // native_var_decl
        return sir::Expr(&symbol_expr),            // enum_def
        return clone(evaluate(inner->value).expr), // enum_variant
        return sir::Expr(&symbol_expr),            // union_def
        SIR_VISIT_IMPOSSIBLE,                      // union_case
        SIR_VISIT_IMPOSSIBLE,                      // proto_def
        SIR_VISIT_IMPOSSIBLE,                      // type_alias
        SIR_VISIT_IMPOSSIBLE,                      // use_ident
        SIR_VISIT_IMPOSSIBLE,                      // use_rebind
        return sir::Expr(&symbol_expr),            // local
        return sir::Expr(&symbol_expr),            // param
        SIR_VISIT_IMPOSSIBLE                       // overload_set
    );
}

ConstEvaluator::Output ConstEvaluator::evaluate_const_def_value(sir::ConstDef &const_def) {
    Result result = DeclBodyAnalyzer(analyzer).visit_const_def(const_def);
    if (result != Result::SUCCESS) {
        return result;
    }

    ConstEvaluator::Output evaluated = evaluate(const_def.value);
    if (evaluated.result != Result::SUCCESS) {
        return evaluated.result;
    }

    return clone(evaluated.expr);
}

ConstEvaluator::Output ConstEvaluator::evaluate_binary_expr(sir::BinaryExpr &binary_expr) {
    ConstEvaluator::Output lhs = evaluate(binary_expr.lhs);
    ConstEvaluator::Output rhs = evaluate(binary_expr.rhs);

    if (lhs.result != Result::SUCCESS || rhs.result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (lhs.expr.is<sir::IntLiteral>() && rhs.expr.is<sir::IntLiteral>()) {
        LargeInt lhs_int = lhs.expr.as<sir::IntLiteral>().value;
        LargeInt rhs_int = rhs.expr.as<sir::IntLiteral>().value;

        if (binary_expr.op == sir::BinaryOp::EQ) return create_bool_literal(lhs_int == rhs_int);
        else if (binary_expr.op == sir::BinaryOp::NE) return create_bool_literal(lhs_int != rhs_int);
        else ASSERT_UNREACHABLE;
    } else if (lhs.expr.is<sir::BoolLiteral>() && rhs.expr.is<sir::BoolLiteral>()) {
        bool lhs_bool = lhs.expr.as<sir::BoolLiteral>().value;
        bool rhs_bool = rhs.expr.as<sir::BoolLiteral>().value;

        if (binary_expr.op == sir::BinaryOp::AND) return create_bool_literal(lhs_bool && rhs_bool);
        else if (binary_expr.op == sir::BinaryOp::OR) return create_bool_literal(lhs_bool || rhs_bool);
        else ASSERT_UNREACHABLE;
    } else if (lhs.expr.is_type() && rhs.expr.is_type()) {
        if (binary_expr.op == sir::BinaryOp::EQ) return create_bool_literal(lhs.expr == rhs.expr);
        else if (binary_expr.op == sir::BinaryOp::NE) return create_bool_literal(lhs.expr != rhs.expr);
        else ASSERT_UNREACHABLE;
    } else {
        return create_bool_literal(false);
    }
}

ConstEvaluator::Output ConstEvaluator::evaluate_unary_expr(sir::UnaryExpr &unary_expr) {
    Output value = evaluate(unary_expr.value);
    if (value.result != Result::SUCCESS) {
        return value.result;
    }

    if (auto int_literal = value.expr.match<sir::IntLiteral>()) {
        if (unary_expr.op == sir::UnaryOp::NEG) return create_int_literal(-int_literal->value, unary_expr.ast_node);
        else ASSERT_UNREACHABLE;
    } else if (auto fp_literal = value.expr.match<sir::FPLiteral>()) {
        if (unary_expr.op == sir::UnaryOp::NEG) return create_fp_literal(-fp_literal->value, unary_expr.ast_node);
        else ASSERT_UNREACHABLE;
    } else if (auto bool_literal = value.expr.match<sir::BoolLiteral>()) {
        if (unary_expr.op == sir::UnaryOp::NOT) return create_bool_literal(!bool_literal->value, unary_expr.ast_node);
        else ASSERT_UNREACHABLE;
    } else {
        ASSERT_UNREACHABLE;
    }
}

ConstEvaluator::Output ConstEvaluator::evaluate_tuple_expr(sir::TupleExpr &tuple_expr) {
    sir::TupleExpr result{
        .ast_node = tuple_expr.ast_node,
        .type = clone(tuple_expr.type),
        .exprs = analyzer.allocate_array<sir::Expr>(tuple_expr.exprs.size()),
    };

    for (unsigned i = 0; i < tuple_expr.exprs.size(); i++) {
        Output value = evaluate(tuple_expr.exprs[i]);
        if (value.result != Result::SUCCESS) {
            return value.result;
        }

        result.exprs[i] = value.expr;
    }

    return sir::Expr{analyzer.create(result)};
}

ConstEvaluator::Output ConstEvaluator::evaluate_meta_field_expr(sir::MetaFieldExpr &meta_field_expr) {
    sir::Expr dummy_expr = &meta_field_expr;
    MetaExprEvaluator(analyzer).evaluate(meta_field_expr, dummy_expr);
    return dummy_expr;
}

ConstEvaluator::Output ConstEvaluator::evaluate_meta_call_expr(sir::MetaCallExpr &meta_call_expr) {
    sir::Expr dummy_expr = &meta_call_expr;
    MetaExprEvaluator(analyzer).evaluate(meta_call_expr, dummy_expr);
    return dummy_expr;
}

ConstEvaluator::Output ConstEvaluator::evaluate_non_const(sir::Expr &value) {
    analyzer.report_generator.report_err_compile_time_unknown(value);
    return Result::ERROR;
}

sir::Expr ConstEvaluator::create_int_literal(LargeInt value, ASTNode *ast_node /*= nullptr*/) {
    sir::IntLiteral int_literal{
        .ast_node = ast_node,
        .type = nullptr,
        .value = value,
    };

    return analyzer.create(int_literal);
}

sir::Expr ConstEvaluator::create_fp_literal(double value, ASTNode *ast_node /*= nullptr*/) {
    sir::FPLiteral fp_literal{
        .ast_node = ast_node,
        .type = nullptr,
        .value = value,
    };

    return analyzer.create(fp_literal);
}

sir::Expr ConstEvaluator::create_bool_literal(bool value, ASTNode *ast_node /*= nullptr*/) {
    sir::BoolLiteral bool_literal{
        .ast_node = ast_node,
        .type = nullptr,
        .value = value,
    };

    return analyzer.create(bool_literal);
}

sir::Expr ConstEvaluator::clone(sir::Expr expr) {
    return sir::Cloner(analyzer.get_mod()).clone_expr(expr);
}

} // namespace sema

} // namespace lang

} // namespace banjo
