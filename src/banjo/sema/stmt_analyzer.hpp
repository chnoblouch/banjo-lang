#ifndef STMT_ANALYZER_H
#define STMT_ANALYZER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class StmtAnalyzer {

private:
    SemanticAnalyzer &analyzer;

public:
    StmtAnalyzer(SemanticAnalyzer &analyzer);
    void analyze_block(sir::Block &block);
    void analyze(sir::Block &block, unsigned &index);

private:
    void analyze_var_stmt(sir::VarStmt &var_stmt);
    void analyze_assign_stmt(sir::AssignStmt &assign_stmt);
    void analyze_comp_assign_stmt(sir::CompAssignStmt &comp_assign_stmt, sir::Stmt &out_stmt);
    void analyze_return_stmt(sir::ReturnStmt &return_stmt);
    void analyze_if_stmt(sir::IfStmt &if_stmt);
    void analyze_switch_stmt(sir::SwitchStmt &switch_stmt);
    void analyze_try_stmt(sir::TryStmt &try_stmt, sir::Stmt &out_stmt);
    void analyze_while_stmt(sir::WhileStmt &while_stmt, sir::Stmt &out_stmt);
    void analyze_for_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt);
    void analyze_loop_stmt(sir::LoopStmt &loop_stmt);
    void analyze_continue_stmt(sir::ContinueStmt &continue_stmt);
    void analyze_break_stmt(sir::BreakStmt &break_stmt);

    void insert_symbol(sir::Ident &ident, sir::Symbol symbol);
    void insert_symbol(sir::SymbolTable &symbol_table, sir::Ident &ident, sir::Symbol symbol);
    void analyze_for_range_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt);
    void analyze_for_iter_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt);

    sir::Expr create_method_call(sir::Expr self, sir::FuncDef &method);
    sir::Expr create_expr(sir::FuncDef &func_def);
    sir::Ident create_ident(std::string value);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
