#ifndef SEMA2_SEMANTIC_ANALYZER_H
#define SEMA2_SEMANTIC_ANALYZER_H

#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"

#include <stack>

namespace banjo {

namespace lang {

namespace sema {

struct Scope {
    sir::FuncDef *func_def = nullptr;
    sir::Block *block = nullptr;
    sir::SymbolTable *symbol_table = nullptr;
    sir::StructDef *struct_def = nullptr;
};

class SemanticAnalyzer {

private:
    sir::Unit &sir_unit;
    std::stack<Scope> scopes;

public:
    SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target);
    void analyze();

private:
    Scope &get_scope() { return scopes.top(); }

    Scope &push_scope() {
        scopes.push(scopes.top());
        return get_scope();
    }

    void pop_scope() { scopes.pop(); }

    void analyze_decl_block(sir::DeclBlock &decl_block);
    void analyze_func_def(sir::FuncDef &func_def);
    void analyze_native_func_decl(sir::NativeFuncDecl &native_func_decl);
    void analyze_struct_def(sir::StructDef &struct_def);
    void analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl);

    void analyze_block(sir::Block &block);
    void analyze_stmt(sir::Stmt &stmt);
    void analyze_var_stmt(sir::VarStmt &var_stmt);
    void analyze_assign_stmt(sir::AssignStmt &assign_stmt);
    void analyze_return_stmt(sir::ReturnStmt &return_stmt);
    void analyze_if_stmt(sir::IfStmt &if_stmt);
    void analyze_while_stmt(sir::WhileStmt &while_stmt, sir::Stmt &out_stmt);
    void analyze_for_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt);
    void analyze_loop_stmt(sir::LoopStmt &loop_stmt);
    void analyze_continue_stmt(sir::ContinueStmt &continue_stmt);
    void analyze_break_stmt(sir::BreakStmt &break_stmt);

    void analyze_expr(sir::Expr &expr);
    void analyze_int_literal(sir::IntLiteral &int_literal);
    void analyze_fp_literal(sir::FPLiteral &fp_literal);
    void analyze_bool_literal(sir::BoolLiteral &bool_literal);
    void analyze_char_literal(sir::CharLiteral &char_literal);
    void analyze_string_literal(sir::StringLiteral &string_literal);
    void analyze_struct_literal(sir::StructLiteral &struct_literal);
    void analyze_ident_expr(sir::IdentExpr &ident_expr);
    void analyze_binary_expr(sir::BinaryExpr &binary_expr);
    void analyze_unary_expr(sir::UnaryExpr &unary_expr);
    void analyze_cast_expr(sir::CastExpr &cast_expr);
    void analyze_call_expr(sir::CallExpr &call_expr);
    void analyze_dot_expr(sir::DotExpr &dot_expr);
    void analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
