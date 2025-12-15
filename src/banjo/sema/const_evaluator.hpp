#ifndef BANJO_SEMA_CONST_EVALUATOR_H
#define BANJO_SEMA_CONST_EVALUATOR_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <cstddef>

namespace banjo {

namespace lang {

namespace sema {

class ConstEvaluator {

public:
    struct Output {
        Result result;
        sir::Expr expr;

        Output(Result result, sir::Expr expr) : result{result}, expr{expr} {}
        Output(sir::Expr expr) : result{Result::SUCCESS}, expr{expr} {}
        Output(Result result) : result{result}, expr{nullptr} {}
        Output(std::nullptr_t) : result{Result::ERROR}, expr{nullptr} {}
    };

private:
    SemanticAnalyzer &analyzer;

public:
    ConstEvaluator(SemanticAnalyzer &analyzer);

    LargeInt evaluate_to_int(sir::Expr &expr);
    bool evaluate_to_bool(sir::Expr &expr);
    Output evaluate(sir::Expr &expr);

    Output evaluate_array_literal(sir::ArrayLiteral &array_literal);
    Output evaluate_symbol_expr(sir::SymbolExpr &symbol_expr);
    Output evaluate_const_def_value(sir::ConstDef &const_def);
    Output evaluate_binary_expr(sir::BinaryExpr &binary_expr);
    Output evaluate_binary_expr_int(sir::BinaryExpr &binary_expr, sir::IntLiteral &lhs, sir::IntLiteral &rhs);
    Output evaluate_binary_expr_fp(sir::BinaryExpr &binary_expr, sir::FPLiteral &lhs, sir::FPLiteral &rhs);
    Output evaluate_binary_expr_bool(sir::BinaryExpr &binary_expr, sir::BoolLiteral &lhs, sir::BoolLiteral &rhs);
    Output evaluate_binary_expr_type(sir::BinaryExpr &binary_expr, sir::Expr lhs, sir::Expr rhs);
    Output evaluate_unary_expr(sir::UnaryExpr &unary_expr);
    Output evaluate_unary_expr_int(sir::UnaryExpr &unary_expr, sir::IntLiteral &value);
    Output evaluate_unary_expr_fp(sir::UnaryExpr &unary_expr, sir::FPLiteral &value);
    Output evaluate_unary_expr_bool(sir::UnaryExpr &unary_expr, sir::BoolLiteral &value);
    Output evaluate_range_expr(sir::RangeExpr &range_expr);
    Output evaluate_tuple_expr(sir::TupleExpr &tuple_expr);
    Output evaluate_meta_field_expr(sir::MetaFieldExpr &meta_field_expr);
    Output evaluate_meta_call_expr(sir::MetaCallExpr &meta_call_expr);
    Output evaluate_non_const(sir::Expr value);

private:
    sir::Expr create_int_literal(LargeInt value, ASTNode *ast_node = nullptr);
    sir::Expr create_fp_literal(double value, ASTNode *ast_node = nullptr);
    sir::Expr create_bool_literal(bool value, ASTNode *ast_node = nullptr);
    sir::Expr clone(sir::Expr expr);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
