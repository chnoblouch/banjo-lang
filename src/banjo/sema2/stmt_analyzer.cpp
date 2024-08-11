#include "stmt_analyzer.hpp"

#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

StmtAnalyzer::StmtAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void StmtAnalyzer::analyze(sir::Stmt &stmt) {
    if (auto var_stmt = stmt.match<sir::VarStmt>()) analyze_var_stmt(*var_stmt);
    else if (auto assign_stmt = stmt.match<sir::AssignStmt>()) analyze_assign_stmt(*assign_stmt);
    else if (auto comp_assign_stmt = stmt.match<sir::CompAssignStmt>())
        analyze_comp_assign_stmt(*comp_assign_stmt, stmt);
    else if (auto return_stmt = stmt.match<sir::ReturnStmt>()) analyze_return_stmt(*return_stmt);
    else if (auto if_stmt = stmt.match<sir::IfStmt>()) analyze_if_stmt(*if_stmt);
    else if (auto while_stmt = stmt.match<sir::WhileStmt>()) analyze_while_stmt(*while_stmt, stmt);
    else if (auto for_stmt = stmt.match<sir::ForStmt>()) analyze_for_stmt(*for_stmt, stmt);
    else if (auto loop_stmt = stmt.match<sir::LoopStmt>()) analyze_loop_stmt(*loop_stmt);
    else if (auto continue_stmt = stmt.match<sir::ContinueStmt>()) analyze_continue_stmt(*continue_stmt);
    else if (auto break_stmt = stmt.match<sir::BreakStmt>()) analyze_break_stmt(*break_stmt);
    else if (auto expr = stmt.match<sir::Expr>()) ExprAnalyzer(analyzer).analyze(*expr);
    else if (auto block = stmt.match<sir::Block>()) analyze_block(*block);
    else ASSERT_UNREACHABLE;
}

void StmtAnalyzer::analyze_block(sir::Block &block) {
    Scope &scope = analyzer.push_scope();
    scope.block = &block;

    for (sir::Stmt &stmt : block.stmts) {
        analyze(stmt);
    }

    analyzer.pop_scope();
}

void StmtAnalyzer::analyze_var_stmt(sir::VarStmt &var_stmt) {
    insert_symbol(var_stmt.name, &var_stmt);

    if (var_stmt.type) {
        ExprAnalyzer(analyzer).analyze(var_stmt.type);
        ExprAnalyzer(analyzer).analyze(var_stmt.value);
    } else {
        ExprAnalyzer(analyzer).analyze(var_stmt.value);
        var_stmt.type = var_stmt.value.get_type();
    }
}

void StmtAnalyzer::analyze_assign_stmt(sir::AssignStmt &assign_stmt) {
    ExprAnalyzer(analyzer).analyze(assign_stmt.lhs);
    ExprAnalyzer(analyzer).analyze(assign_stmt.rhs);
}

void StmtAnalyzer::analyze_comp_assign_stmt(sir::CompAssignStmt &comp_assign_stmt, sir::Stmt &out_stmt) {
    sir::AssignStmt *assign_stmt = analyzer.create_stmt(sir::AssignStmt{
        .ast_node = comp_assign_stmt.ast_node,
        .lhs = comp_assign_stmt.lhs,
        .rhs = analyzer.create_expr(sir::BinaryExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .op = comp_assign_stmt.op,
            .lhs = comp_assign_stmt.lhs,
            .rhs = comp_assign_stmt.rhs,
        }),
    });

    analyze_assign_stmt(*assign_stmt);
    out_stmt = assign_stmt;
}

void StmtAnalyzer::analyze_return_stmt(sir::ReturnStmt &return_stmt) {
    ExprAnalyzer(analyzer).analyze(return_stmt.value);
}

void StmtAnalyzer::analyze_if_stmt(sir::IfStmt &if_stmt) {
    for (sir::IfCondBranch &cond_branch : if_stmt.cond_branches) {
        ExprAnalyzer(analyzer).analyze(cond_branch.condition);
        analyze_block(cond_branch.block);
    }

    if (if_stmt.else_branch) {
        analyze_block(if_stmt.else_branch->block);
    }
}

void StmtAnalyzer::analyze_while_stmt(sir::WhileStmt &while_stmt, sir::Stmt &out_stmt) {
    sir::LoopStmt *loop_stmt = analyzer.create_stmt(sir::LoopStmt{
        .ast_node = nullptr,
        .condition = while_stmt.condition,
        .block = std::move(while_stmt.block),
        .latch = {},
    });

    analyze_loop_stmt(*loop_stmt);
    out_stmt = loop_stmt;
}

void StmtAnalyzer::analyze_for_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt) {
    sir::RangeExpr &range = for_stmt.range.as<sir::RangeExpr>();

    sir::Block *block = analyzer.create_stmt(sir::Block{
        .ast_node = nullptr,
        .stmts = {},
        .symbol_table = analyzer.create_symbol_table({
            .parent = &analyzer.get_symbol_table(),
            .symbols = {},
        }),
    });

    sir::VarStmt *var_stmt = analyzer.create_stmt(sir::VarStmt{
        .ast_node = nullptr,
        .name = for_stmt.ident,
        .type = nullptr,
        .value = range.lhs,
    });

    sir::IdentExpr *var_ref_expr = analyzer.create_expr(sir::IdentExpr{
        .ast_node = nullptr,
        .value = for_stmt.ident.value,
    });

    sir::BinaryExpr *loop_condition = analyzer.create_expr(sir::BinaryExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .op = sir::BinaryOp::LT,
        .lhs = var_ref_expr,
        .rhs = range.rhs,
    });

    for_stmt.block.symbol_table->parent = block->symbol_table;

    sir::Block loop_block{
        .ast_node = for_stmt.block.ast_node,
        .stmts = std::move(for_stmt.block.stmts),
        .symbol_table = for_stmt.block.symbol_table,
    };

    sir::AssignStmt *inc_stmt = analyzer.create_stmt(sir::AssignStmt{
        .ast_node = nullptr,
        .lhs = var_ref_expr,
        .rhs = analyzer.create_expr(sir::BinaryExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .op = sir::BinaryOp::ADD,
            .lhs = var_ref_expr,
            .rhs = analyzer.create_expr(sir::IntLiteral{
                .ast_node = nullptr,
                .type = nullptr,
                .value = 1,
            }),
        }),
    });

    sir::Block loop_latch{
        .ast_node = nullptr,
        .stmts = {inc_stmt},
        .symbol_table = analyzer.create_symbol_table({
            .parent = block->symbol_table,
            .symbols = {},
        }),
    };

    sir::LoopStmt *loop_stmt = analyzer.create_stmt(sir::LoopStmt{
        .ast_node = nullptr,
        .condition = loop_condition,
        .block = loop_block,
        .latch = loop_latch,
    });

    block->stmts = {var_stmt, loop_stmt};

    analyze_block(*block);
    out_stmt = block;
}

void StmtAnalyzer::analyze_loop_stmt(sir::LoopStmt &loop_stmt) {
    ExprAnalyzer(analyzer).analyze(loop_stmt.condition);
    analyze_block(loop_stmt.block);

    if (loop_stmt.latch) {
        analyze_block(*loop_stmt.latch);
    }
}

void StmtAnalyzer::analyze_continue_stmt(sir::ContinueStmt & /*continue_stmt*/) {}

void StmtAnalyzer::analyze_break_stmt(sir::BreakStmt & /*break_stmt*/) {}

void StmtAnalyzer::insert_symbol(sir::Ident &ident, sir::Symbol symbol) {
    auto &symbols = analyzer.get_scope().block->symbol_table->symbols;

    auto iter = symbols.find(ident.value);
    if (iter != symbols.end()) {
        analyzer.report_generator.report_err_redefinition(ident, iter->second);
        return;
    }

    symbols.insert({ident.value, symbol});
}

} // namespace sema

} // namespace lang

} // namespace banjo
