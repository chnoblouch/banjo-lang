#ifndef EXPR_ANALYZER_H
#define EXPR_ANALYZER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class ExprAnalyzer {

private:
    SemanticAnalyzer analyzer;

public:
    ExprAnalyzer(SemanticAnalyzer &analyzer);
    void analyze(sir::Expr &expr);
    void analyze(sir::Expr &expr, sir::SymbolTable &symbol_table);

private:
    void analyze_int_literal(sir::IntLiteral &int_literal);
    void analyze_fp_literal(sir::FPLiteral &fp_literal);
    void analyze_bool_literal(sir::BoolLiteral &bool_literal);
    void analyze_char_literal(sir::CharLiteral &char_literal);
    void analyze_string_literal(sir::StringLiteral &string_literal);
    void analyze_struct_literal(sir::StructLiteral &struct_literal);
    void analyze_binary_expr(sir::BinaryExpr &binary_expr);
    void analyze_unary_expr(sir::UnaryExpr &unary_expr);
    void analyze_cast_expr(sir::CastExpr &cast_expr);
    void analyze_call_expr(sir::CallExpr &call_expr, sir::SymbolTable &symbol_table);
    void analyze_dot_expr_callee(sir::DotExpr &dot_expr, sir::SymbolTable &symbol_table, sir::CallExpr &out_call_expr);
    void analyze_dot_expr(sir::DotExpr &dot_expr, sir::SymbolTable &symbol_table, sir::Expr &out_expr);
    void analyze_ident_expr(sir::IdentExpr &ident_expr, sir::SymbolTable &symbol_table, sir::Expr &out_expr);
    void analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr);
    void analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr);

    void analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr);
    void resolve_overload(sir::OverloadSet &overload_set, sir::CallExpr &inout_call_expr);
    bool is_matching_overload(sir::FuncDef &func_def, std::vector<sir::Expr> &args);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif