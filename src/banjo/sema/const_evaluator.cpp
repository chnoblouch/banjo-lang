#include "const_evaluator.hpp"

#include "banjo/sema/meta_expr_evaluator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"
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
        SIR_VISIT_IMPOSSIBLE,                    // ident_expr
        SIR_VISIT_IMPOSSIBLE,                    // star_expr
        SIR_VISIT_IMPOSSIBLE,                    // bracket_expr
        SIR_VISIT_IMPOSSIBLE,                    // dot_expr
        SIR_VISIT_IMPOSSIBLE,                    // pseudo_tpe
        SIR_VISIT_IMPOSSIBLE,                    // meta_access
        return evaluate_meta_field_expr(*inner), // meta_field_expr
        return evaluate_meta_call_expr(*inner),  // meta_call_expr
        SIR_VISIT_IMPOSSIBLE,                    // init_expr
        SIR_VISIT_IMPOSSIBLE,                    // move_expr
        SIR_VISIT_IMPOSSIBLE,                    // deinit_expr
        return nullptr                           // error
    );
}

sir::Expr ConstEvaluator::evaluate_array_literal(sir::ArrayLiteral &array_literal) {
    sir::ArrayLiteral result{
        .ast_node = array_literal.ast_node,
        .type = clone(array_literal.type),
        .values = std::vector<sir::Expr>(array_literal.values.size()),
    };

    for (unsigned i = 0; i < array_literal.values.size(); i++) {
        sir::Expr value = evaluate(array_literal.values[i]);
        if (!value) {
            return nullptr;
        }

        result.values[i] = value;
    }

    return analyzer.create_expr(result);
}

sir::Expr ConstEvaluator::evaluate_symbol_expr(sir::SymbolExpr &symbol_expr) {
    SIR_VISIT_SYMBOL(
        symbol_expr.symbol,
        SIR_VISIT_IMPOSSIBLE,                 // empty
        SIR_VISIT_IMPOSSIBLE,                 // module
        SIR_VISIT_IMPOSSIBLE,                 // func_def
        SIR_VISIT_IMPOSSIBLE,                 // func_decl
        SIR_VISIT_IMPOSSIBLE,                 // native_func_decl
        return clone(evaluate(inner->value)), // const_def
        return &symbol_expr,                  // struct_def
        SIR_VISIT_IMPOSSIBLE,                 // struct_field
        SIR_VISIT_IMPOSSIBLE,                 // var_decl
        SIR_VISIT_IMPOSSIBLE,                 // native_var_decl
        return &symbol_expr,                  // enum_def
        return clone(evaluate(inner->value)), // enum_variant
        return &symbol_expr,                  // union_def
        SIR_VISIT_IMPOSSIBLE,                 // union_case
        SIR_VISIT_IMPOSSIBLE,                 // proto_def
        SIR_VISIT_IMPOSSIBLE,                 // type_alias
        SIR_VISIT_IMPOSSIBLE,                 // use_ident
        SIR_VISIT_IMPOSSIBLE,                 // use_rebind
        return &symbol_expr,                  // local
        return &symbol_expr,                  // param
        SIR_VISIT_IMPOSSIBLE                  // overload_set
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
    sir::Expr value = evaluate(unary_expr.value);

    if (auto int_literal = value.match<sir::IntLiteral>()) {
        if (unary_expr.op == sir::UnaryOp::NEG) return create_int_literal(-int_literal->value, unary_expr.ast_node);
        else ASSERT_UNREACHABLE;
    } else if (auto fp_literal = value.match<sir::FPLiteral>()) {
        if (unary_expr.op == sir::UnaryOp::NEG) return create_fp_literal(-fp_literal->value, unary_expr.ast_node);
        else ASSERT_UNREACHABLE;
    } else if (auto bool_literal = value.match<sir::BoolLiteral>()) {
        if (unary_expr.op == sir::UnaryOp::NOT) return create_bool_literal(!bool_literal->value, unary_expr.ast_node);
        else ASSERT_UNREACHABLE;
    } else {
        ASSERT_UNREACHABLE;
    }
}

sir::Expr ConstEvaluator::evaluate_tuple_expr(sir::TupleExpr &tuple_expr) {
    sir::TupleExpr result{
        .ast_node = tuple_expr.ast_node,
        .type = clone(tuple_expr.type),
        .exprs = std::vector<sir::Expr>(tuple_expr.exprs.size()),
    };

    for (unsigned i = 0; i < tuple_expr.exprs.size(); i++) {
        sir::Expr value = evaluate(tuple_expr.exprs[i]);
        if (!value) {
            return nullptr;
        }

        result.exprs[i] = value;
    }

    return analyzer.create_expr(result);
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

sir::Expr ConstEvaluator::evaluate_non_const(sir::Expr &value) {
    analyzer.report_generator.report_err_compile_time_unknown(value);
    return nullptr;
}

sir::Expr ConstEvaluator::create_int_literal(LargeInt value, ASTNode *ast_node /*= nullptr*/) {
    sir::IntLiteral int_literal{
        .ast_node = ast_node,
        .type = nullptr,
        .value = value,
    };

    return analyzer.create_trivial(int_literal);
}

sir::Expr ConstEvaluator::create_fp_literal(double value, ASTNode *ast_node /*= nullptr*/) {
    sir::FPLiteral fp_literal{
        .ast_node = ast_node,
        .type = nullptr,
        .value = value,
    };

    return analyzer.create_trivial(fp_literal);
}

sir::Expr ConstEvaluator::create_bool_literal(bool value, ASTNode *ast_node /*= nullptr*/) {
    sir::BoolLiteral bool_literal{
        .ast_node = ast_node,
        .type = nullptr,
        .value = value,
    };

    return analyzer.create_trivial(bool_literal);
}

sir::Expr ConstEvaluator::clone(sir::Expr expr) {
    return sir::Cloner(*analyzer.cur_sir_mod).clone_expr(expr);
}

} // namespace sema

} // namespace lang

} // namespace banjo
