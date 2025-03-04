#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class ConstEvaluator {

private:
    SemanticAnalyzer &analyzer;

public:
    ConstEvaluator(SemanticAnalyzer &analyzer);

    LargeInt evaluate_to_int(sir::Expr &expr);
    bool evaluate_to_bool(sir::Expr &expr);
    sir::Expr evaluate(sir::Expr &expr);

    sir::Expr evaluate_array_literal(sir::ArrayLiteral &array_literal);
    sir::Expr evaluate_symbol_expr(sir::SymbolExpr &symbol_expr);
    sir::Expr evaluate_binary_expr(sir::BinaryExpr &unary_expr);
    sir::Expr evaluate_unary_expr(sir::UnaryExpr &unary_expr);
    sir::Expr evaluate_tuple_expr(sir::TupleExpr &tuple_expr);
    sir::Expr evaluate_meta_field_expr(sir::MetaFieldExpr &meta_field_expr);
    sir::Expr evaluate_meta_call_expr(sir::MetaCallExpr &meta_call_expr);
    sir::Expr evaluate_non_const(sir::Expr &value);

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
