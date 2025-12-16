#include "const_evaluator.hpp"

#include "banjo/sema/decl_body_analyzer.hpp"
#include "banjo/sema/meta_expr_evaluator.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"
#include "banjo/sir/sir_create.hpp"
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
        return evaluate_range_expr(*inner),      // range_expr
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
        SIR_VISIT_IMPOSSIBLE,                      // overload_set
        SIR_VISIT_IMPOSSIBLE                       // generic_arg
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
        sir::IntLiteral &lhs_int = lhs.expr.as<sir::IntLiteral>();
        sir::IntLiteral &rhs_int = rhs.expr.as<sir::IntLiteral>();
        return evaluate_binary_expr_int(binary_expr, lhs_int, rhs_int);
    } else if (lhs.expr.is<sir::FPLiteral>() && rhs.expr.is<sir::FPLiteral>()) {
        sir::FPLiteral &lhs_fp = lhs.expr.as<sir::FPLiteral>();
        sir::FPLiteral &rhs_fp = rhs.expr.as<sir::FPLiteral>();
        return evaluate_binary_expr_fp(binary_expr, lhs_fp, rhs_fp);
    } else if (lhs.expr.is<sir::BoolLiteral>() && rhs.expr.is<sir::BoolLiteral>()) {
        sir::BoolLiteral &lhs_bool = lhs.expr.as<sir::BoolLiteral>();
        sir::BoolLiteral &rhs_bool = rhs.expr.as<sir::BoolLiteral>();
        return evaluate_binary_expr_bool(binary_expr, lhs_bool, rhs_bool);
    } else if (lhs.expr.is_type() && rhs.expr.is_type()) {
        return evaluate_binary_expr_type(binary_expr, lhs.expr, rhs.expr);
    } else {
        return {&binary_expr};
    }
}

ConstEvaluator::Output ConstEvaluator::evaluate_binary_expr_int(
    sir::BinaryExpr &binary_expr,
    sir::IntLiteral &lhs,
    sir::IntLiteral &rhs
) {
    if (binary_expr.is_arithmetic_op() || binary_expr.is_bitwise_op()) {
        LargeInt result = 0;

        switch (binary_expr.op) {
            case sir::BinaryOp::ADD: result = lhs.value + rhs.value; break;
            case sir::BinaryOp::SUB: result = lhs.value - rhs.value; break;
            case sir::BinaryOp::MUL: result = lhs.value * rhs.value; break;
            case sir::BinaryOp::DIV: result = lhs.value / rhs.value; break;
            case sir::BinaryOp::MOD: result = lhs.value % rhs.value; break;
            case sir::BinaryOp::BIT_AND: result = lhs.value & rhs.value; break;
            case sir::BinaryOp::BIT_OR: result = lhs.value | rhs.value; break;
            case sir::BinaryOp::BIT_XOR: result = lhs.value ^ rhs.value; break;
            // TODO
            case sir::BinaryOp::SHL: return evaluate_non_const(&binary_expr);
            case sir::BinaryOp::SHR: return evaluate_non_const(&binary_expr);
            default: ASSERT_UNREACHABLE;
        }

        sir::IntLiteral output{
            .ast_node = binary_expr.ast_node,
            .type = lhs.type,
            .value = result,
        };

        return {analyzer.create(output)};
    } else if (binary_expr.is_comparison_op()) {
        bool result;

        switch (binary_expr.op) {
            case sir::BinaryOp::EQ: result = lhs.value == rhs.value; break;
            case sir::BinaryOp::NE: result = lhs.value != rhs.value; break;
            case sir::BinaryOp::GT: result = lhs.value > rhs.value; break;
            case sir::BinaryOp::LT: result = lhs.value < rhs.value; break;
            case sir::BinaryOp::GE: result = lhs.value >= rhs.value; break;
            case sir::BinaryOp::LE: result = lhs.value <= rhs.value; break;
            default: ASSERT_UNREACHABLE;
        }

        sir::BoolLiteral output{
            .ast_node = binary_expr.ast_node,
            .type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL),
            .value = result,
        };

        return {analyzer.create(output)};
    } else {
        ASSERT_UNREACHABLE;
    }
}

ConstEvaluator::Output ConstEvaluator::evaluate_binary_expr_fp(
    sir::BinaryExpr &binary_expr,
    sir::FPLiteral &lhs,
    sir::FPLiteral &rhs
) {
    if (binary_expr.is_arithmetic_op() || binary_expr.is_bitwise_op()) {
        return {&binary_expr};
    } else if (binary_expr.is_comparison_op()) {
        bool result;

        switch (binary_expr.op) {
            case sir::BinaryOp::EQ: result = lhs.value == rhs.value; break;
            case sir::BinaryOp::NE: result = lhs.value != rhs.value; break;
            case sir::BinaryOp::GT: result = lhs.value > rhs.value; break;
            case sir::BinaryOp::LT: result = lhs.value < rhs.value; break;
            case sir::BinaryOp::GE: result = lhs.value >= rhs.value; break;
            case sir::BinaryOp::LE: result = lhs.value <= rhs.value; break;
            default: ASSERT_UNREACHABLE;
        }

        sir::BoolLiteral output{
            .ast_node = binary_expr.ast_node,
            .type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL),
            .value = result,
        };

        return {analyzer.create(output)};
    } else {
        ASSERT_UNREACHABLE;
    }
}

ConstEvaluator::Output ConstEvaluator::evaluate_binary_expr_bool(
    sir::BinaryExpr &binary_expr,
    sir::BoolLiteral &lhs,
    sir::BoolLiteral &rhs
) {
    bool result;

    switch (binary_expr.op) {
        case sir::BinaryOp::AND: result = lhs.value && rhs.value; break;
        case sir::BinaryOp::OR: result = lhs.value || rhs.value; break;
        default: return evaluate_non_const(&binary_expr);
    }

    sir::BoolLiteral output{
        .ast_node = binary_expr.ast_node,
        .type = lhs.type,
        .value = result,
    };

    return {analyzer.create(output)};
}

ConstEvaluator::Output ConstEvaluator::evaluate_binary_expr_type(
    sir::BinaryExpr &binary_expr,
    sir::Expr lhs,
    sir::Expr rhs
) {
    bool result;

    switch (binary_expr.op) {
        case sir::BinaryOp::EQ: result = lhs == rhs; break;
        case sir::BinaryOp::NE: result = lhs != rhs; break;
        default: return evaluate_non_const(&binary_expr);
    }

    sir::BoolLiteral output{
        .ast_node = binary_expr.ast_node,
        .type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL),
        .value = result,
    };

    return {analyzer.create(output)};
}

ConstEvaluator::Output ConstEvaluator::evaluate_unary_expr(sir::UnaryExpr &unary_expr) {
    Output value = evaluate(unary_expr.value);
    if (value.result != Result::SUCCESS) {
        return value.result;
    }

    if (auto int_literal = value.expr.match<sir::IntLiteral>()) {
        return evaluate_unary_expr_int(unary_expr, *int_literal);
    } else if (auto fp_literal = value.expr.match<sir::FPLiteral>()) {
        return evaluate_unary_expr_fp(unary_expr, *fp_literal);
    } else if (auto bool_literal = value.expr.match<sir::BoolLiteral>()) {
        return evaluate_unary_expr_bool(unary_expr, *bool_literal);
    } else {
        ASSERT_UNREACHABLE;
    }
}

ConstEvaluator::Output ConstEvaluator::evaluate_unary_expr_int(sir::UnaryExpr &unary_expr, sir::IntLiteral &value) {
    LargeInt result = 0;

    switch (unary_expr.op) {
        case sir::UnaryOp::NEG: result = -value.value; break;
        default: return evaluate_non_const(&unary_expr);
    }

    sir::IntLiteral output{
        .ast_node = value.ast_node,
        .type = value.type,
        .value = result,
    };

    return {analyzer.create(output)};
}

ConstEvaluator::Output ConstEvaluator::evaluate_unary_expr_fp(sir::UnaryExpr &unary_expr, sir::FPLiteral &value) {
    double result;

    switch (unary_expr.op) {
        case sir::UnaryOp::NEG: result = -value.value; break;
        default: return evaluate_non_const(&unary_expr);
    }

    sir::FPLiteral output{
        .ast_node = value.ast_node,
        .type = value.type,
        .value = result,
    };

    return {analyzer.create(output)};
}

ConstEvaluator::Output ConstEvaluator::evaluate_unary_expr_bool(sir::UnaryExpr &unary_expr, sir::BoolLiteral &value) {
    bool result;

    switch (unary_expr.op) {
        case sir::UnaryOp::NOT: result = !value.value; break;
        default: return evaluate_non_const(&unary_expr);
    }

    sir::BoolLiteral output{
        .ast_node = value.ast_node,
        .type = value.type,
        .value = result,
    };

    return {analyzer.create(output)};
}

ConstEvaluator::Output ConstEvaluator::evaluate_range_expr(sir::RangeExpr &range_expr) {
    ConstEvaluator::Output lhs = evaluate(range_expr.lhs);
    ConstEvaluator::Output rhs = evaluate(range_expr.rhs);

    if (lhs.result != Result::SUCCESS || rhs.result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::RangeExpr result{
        .ast_node = range_expr.ast_node,
        .lhs = lhs.expr,
        .rhs = rhs.expr,
    };

    return sir::Expr{analyzer.create(result)};
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

ConstEvaluator::Output ConstEvaluator::evaluate_non_const(sir::Expr value) {
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
