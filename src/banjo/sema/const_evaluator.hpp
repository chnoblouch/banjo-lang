#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/growable_arena.hpp"

namespace banjo {

namespace lang {

namespace sema {

class ConstEvaluator {

private:
    SemanticAnalyzer &analyzer;
    bool use_owned_arena;
    utils::GrowableArena<sir::IntLiteral, 8> int_arena;
    utils::GrowableArena<sir::FPLiteral, 8> fp_arena;
    utils::GrowableArena<sir::BoolLiteral, 8> bool_arena;

public:
    ConstEvaluator(SemanticAnalyzer &analyzer, bool use_owned_arena = true);

    LargeInt evaluate_to_int(sir::Expr &expr);
    bool evaluate_to_bool(sir::Expr &expr);
    sir::Expr evaluate(sir::Expr &expr);

    sir::Expr analyze(sir::Expr &expr);
    sir::Expr analyze_and_evaluate(sir::Expr &expr);
    sir::Expr evaluate_symbol_expr(sir::SymbolExpr &symbol_expr);
    sir::Expr evaluate_binary_expr(sir::BinaryExpr &unary_expr);
    sir::Expr evaluate_unary_expr(sir::UnaryExpr &unary_expr);
    sir::Expr evaluate_meta_field_expr(sir::MetaFieldExpr &meta_field_expr);
    sir::Expr evaluate_meta_call_expr(sir::MetaCallExpr &meta_call_expr);

private:
    sir::Expr create_int_literal(LargeInt value);
    sir::Expr create_fp_literal(double value);
    sir::Expr create_bool_literal(bool value);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif