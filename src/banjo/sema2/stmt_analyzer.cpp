#include "stmt_analyzer.hpp"

#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/sema2/magic_methods.hpp"
#include "banjo/sema2/meta_expansion.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"

namespace banjo {

namespace lang {

namespace sema {

StmtAnalyzer::StmtAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void StmtAnalyzer::analyze_block(sir::Block &block) {
    Scope &scope = analyzer.push_scope();
    scope.block = &block;

    for (unsigned i = 0; i < block.stmts.size(); i++) {
        analyze(block, i);
    }

    analyzer.pop_scope();
}

void StmtAnalyzer::analyze(sir::Block &block, unsigned &index) {
    sir::Stmt &stmt = block.stmts[index];

    SIR_VISIT_STMT(
        stmt,
        SIR_VISIT_IMPOSSIBLE,                                         // empty
        analyze_var_stmt(*inner),                                     // var_stmt
        analyze_assign_stmt(*inner),                                  // assign_stmt
        analyze_comp_assign_stmt(*inner, stmt),                       // comp_assign_stmt
        analyze_return_stmt(*inner),                                  // return_stmt
        analyze_if_stmt(*inner),                                      // if_stmt
        analyze_switch_stmt(*inner),                                  // switch_stmt
        analyze_try_stmt(*inner, stmt),                               // try_stmt
        analyze_while_stmt(*inner, stmt),                             // while_stmt
        analyze_for_stmt(*inner, stmt),                               // for_stmt
        analyze_loop_stmt(*inner),                                    // loop_stmt
        analyze_continue_stmt(*inner),                                // continue_stmt
        analyze_break_stmt(*inner),                                   // break_stmt
        MetaExpansion(analyzer).evaluate_meta_if_stmt(block, index),  // meta_if_stmt
        MetaExpansion(analyzer).evaluate_meta_for_stmt(block, index), // meta_for_stmt
        SIR_VISIT_IGNORE,                                             // expanded_meta_stmt
        ExprAnalyzer(analyzer).analyze(*inner),                       // expr_stmt
        analyze_block(*inner)                                         // block_stmt
    )
}

void StmtAnalyzer::analyze_var_stmt(sir::VarStmt &var_stmt) {
    // Variables with empty names can be generated during semantic analysis.
    if (!var_stmt.local.name.value.empty()) {
        insert_symbol(var_stmt.local.name, &var_stmt.local);
    }

    if (var_stmt.local.type) {
        ExprAnalyzer(analyzer).analyze(var_stmt.local.type);

        if (var_stmt.value) {
            ExprAnalyzer(analyzer, ExprConstraints::expect_type(var_stmt.local.type)).analyze(var_stmt.value);
        }
    } else {
        ExprAnalyzer(analyzer).analyze(var_stmt.value);
        var_stmt.local.type = var_stmt.value.get_type();
    }
}

void StmtAnalyzer::analyze_assign_stmt(sir::AssignStmt &assign_stmt) {
    ExprAnalyzer(analyzer).analyze(assign_stmt.lhs);
    ExprAnalyzer(analyzer, ExprConstraints::expect_type(assign_stmt.lhs.get_type())).analyze(assign_stmt.rhs);
}

void StmtAnalyzer::analyze_comp_assign_stmt(sir::CompAssignStmt &comp_assign_stmt, sir::Stmt &out_stmt) {
    // FIXME: error handling for overloaded operators with a bad return type

    sir::AssignStmt *assign_stmt = analyzer.create_stmt(sir::AssignStmt{
        .ast_node = comp_assign_stmt.ast_node,
        .lhs = comp_assign_stmt.lhs,
        .rhs = analyzer.create_expr(sir::BinaryExpr{
            .ast_node = comp_assign_stmt.ast_node,
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
    if (return_stmt.value) {
        sir::Expr return_type = analyzer.get_scope().func_def->type.return_type;
        ExprAnalyzer(analyzer, ExprConstraints::expect_type(return_type)).analyze(return_stmt.value);
    }
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

void StmtAnalyzer::analyze_switch_stmt(sir::SwitchStmt &switch_stmt) {
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(switch_stmt.value);
    if (partial_result != Result::SUCCESS) {
        return;
    }

    for (sir::SwitchCaseBranch &case_branch : switch_stmt.case_branches) {
        insert_symbol(*case_branch.block.symbol_table, case_branch.local.name, &case_branch.local);
        ExprAnalyzer(analyzer).analyze(case_branch.local.type);
        analyze_block(case_branch.block);
    }
}

void StmtAnalyzer::analyze_try_stmt(sir::TryStmt &try_stmt, sir::Stmt &out_stmt) {
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(try_stmt.success_branch.expr);
    if (partial_result != Result::SUCCESS) {
        return;
    }

    sir::Expr type = try_stmt.success_branch.expr.get_type();

    sir::StructField *successful_field;
    sir::StructField *value_field;
    sir::StructField *error_field;

    if (auto specialization = analyzer.as_std_result_specialization(type)) {
        successful_field = specialization->def->find_field("successful");
        value_field = specialization->def->find_field("value");
        error_field = specialization->def->find_field("error");
    } else if (auto specialization = analyzer.as_std_optional_specialization(type)) {
        successful_field = specialization->def->find_field("has_value");
        value_field = specialization->def->find_field("value");
        error_field = nullptr;
    } else {
        analyzer.report_generator.report_err_cannot_use_in_try(try_stmt.success_branch.expr);
        return;
    }

    sir::IfStmt if_stmt{
        .ast_node = nullptr,
        .cond_branches = {},
    };

    sir::Block success_block{
        .ast_node = nullptr,
        .stmts{
            analyzer.create_stmt(sir::VarStmt{
                .ast_node = nullptr,
                .local{
                    .name = try_stmt.success_branch.ident,
                    .type = nullptr,
                },
                .value = analyzer.create_expr(sir::FieldExpr{
                    .ast_node = nullptr,
                    .type = value_field->type,
                    .base = try_stmt.success_branch.expr,
                    .field_index = value_field->index,
                }),
            }),
            analyzer.create_stmt(try_stmt.success_branch.block),
        },
        .symbol_table = analyzer.create_symbol_table(sir::SymbolTable{
            .parent = &analyzer.get_symbol_table(),
            .symbols = {},
        }),
    };

    try_stmt.success_branch.block.symbol_table->parent = success_block.symbol_table;
    analyze_block(success_block);

    if_stmt.cond_branches = {
        sir::IfCondBranch{
            .ast_node = nullptr,
            .condition = analyzer.create_expr(sir::FieldExpr{
                .ast_node = nullptr,
                .type = successful_field->type,
                .base = try_stmt.success_branch.expr,
                .field_index = successful_field->index,
            }),
            .block = success_block,
        },
    };

    if (try_stmt.except_branch) {
        if (error_field) {
            ExprAnalyzer(analyzer).analyze(try_stmt.except_branch->type);

            sir::Block except_block{
                .ast_node = nullptr,
                .stmts{
                    analyzer.create_stmt(sir::VarStmt{
                        .ast_node = nullptr,
                        .local{
                            .name = try_stmt.except_branch->ident,
                            .type = nullptr,
                        },
                        .value = analyzer.create_expr(sir::FieldExpr{
                            .ast_node = nullptr,
                            .type = error_field->type,
                            .base = try_stmt.success_branch.expr,
                            .field_index = error_field->index,
                        }),
                    }),
                    analyzer.create_stmt(try_stmt.except_branch->block),
                },
                .symbol_table = analyzer.create_symbol_table(sir::SymbolTable{
                    .parent = &analyzer.get_symbol_table(),
                    .symbols = {},
                }),
            };

            try_stmt.except_branch->block.symbol_table->parent = except_block.symbol_table;
            analyze_block(except_block);

            if_stmt.else_branch = sir::IfElseBranch{
                .ast_node = nullptr,
                .block = except_block,
            };
        } else {
            analyzer.report_generator.report_err_try_no_error_field(*try_stmt.except_branch);
        }
    } else if (try_stmt.else_branch) {
        sir::Block else_block = try_stmt.else_branch->block;
        analyze_block(else_block);

        if_stmt.else_branch = sir::IfElseBranch{
            .ast_node = nullptr,
            .block = else_block,
        };
    }

    out_stmt = analyzer.create_stmt(if_stmt);
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
    if (for_stmt.range.is<sir::RangeExpr>()) {
        analyze_for_range_stmt(for_stmt, out_stmt);
    } else {
        analyze_for_iter_stmt(for_stmt, out_stmt);
    }
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
    insert_symbol(*analyzer.get_scope().block->symbol_table, ident, symbol);
}

void StmtAnalyzer::insert_symbol(sir::SymbolTable &symbol_table, sir::Ident &ident, sir::Symbol symbol) {
    auto &symbols = symbol_table.symbols;

    auto iter = symbols.find(ident.value);
    if (iter != symbols.end()) {
        analyzer.report_generator.report_err_redefinition(ident, iter->second);
        return;
    }

    symbols.insert({ident.value, symbol});
}

void StmtAnalyzer::analyze_for_range_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt) {
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
        .local{
            .name = for_stmt.ident,
            .type = nullptr,
        },
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

void StmtAnalyzer::analyze_for_iter_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt) {
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(for_stmt.range);
    if (partial_result != Result::SUCCESS) {
        return;
    }

    if (!for_stmt.range.get_type()) {
        return;
    }

    sir::SymbolTable *iterable_symbol_table = for_stmt.range.get_type().as_symbol<sir::StructDef>().block.symbol_table;
    sir::FuncDef &iter_func_def = iterable_symbol_table->look_up(MagicMethods::ITER).as<sir::FuncDef>();
    sir::SymbolTable *iter_symbol_table = iter_func_def.type.return_type.as_symbol<sir::StructDef>().block.symbol_table;
    sir::FuncDef &next_func_def = iter_symbol_table->look_up(MagicMethods::NEXT).as<sir::FuncDef>();

    sir::Block *block = analyzer.create_stmt(sir::Block{
        .ast_node = nullptr,
        .stmts = {},
        .symbol_table = analyzer.create_symbol_table({
            .parent = &analyzer.get_symbol_table(),
            .symbols = {},
        }),
    });

    sir::VarStmt *iter_var_stmt = analyzer.create_stmt(sir::VarStmt{
        .ast_node = nullptr,
        .local{
            .name = create_ident(".iter"),
            .type = nullptr,
        },
        .value = create_method_call(for_stmt.range, iter_func_def),
    });

    sir::SymbolExpr *iter_ref_expr = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = iter_func_def.type.return_type,
        .symbol = &iter_var_stmt->local,
    });

    sir::VarStmt *next_var_stmt = analyzer.create_stmt(sir::VarStmt{
        .ast_node = nullptr,
        .local{
            .name = create_ident(".next"),
            .type = nullptr,
        },
        .value = create_method_call(iter_ref_expr, next_func_def),
    });

    sir::SymbolExpr *next_ref_expr = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = next_func_def.type.return_type,
        .symbol = &next_var_stmt->local,
    });

    sir::DotExpr *loop_condition = analyzer.create_expr(sir::DotExpr{
        .ast_node = nullptr,
        .lhs = next_ref_expr,
        .rhs = create_ident("has_value"),
    });

    sir::Block *inner_loop_block = analyzer.create_stmt(sir::Block{
        .ast_node = for_stmt.block.ast_node,
        .stmts = std::move(for_stmt.block.stmts),
        .symbol_table = for_stmt.block.symbol_table,
    });

    sir::VarStmt *value_var_stmt = analyzer.create_stmt(sir::VarStmt{
        .ast_node = nullptr,
        .local{
            .name = for_stmt.ident,
            .type = nullptr,
        },
        .value = analyzer.create_expr(sir::DotExpr{
            .ast_node = nullptr,
            .lhs = next_ref_expr,
            .rhs = create_ident("value"),
        }),
    });

    sir::Block outer_loop_block{
        .ast_node = nullptr,
        .stmts = {value_var_stmt, inner_loop_block},
        .symbol_table = analyzer.create_symbol_table({
            .parent = block->symbol_table,
            .symbols = {},
        })
    };

    inner_loop_block->symbol_table->parent = outer_loop_block.symbol_table;

    sir::AssignStmt *latch_next_stmt = analyzer.create_stmt(sir::AssignStmt{
        .ast_node = nullptr,
        .lhs = next_ref_expr,
        .rhs = create_method_call(iter_ref_expr, next_func_def),
    });

    sir::Block loop_latch{
        .ast_node = nullptr,
        .stmts = {latch_next_stmt},
        .symbol_table = analyzer.create_symbol_table({
            .parent = block->symbol_table,
            .symbols = {},
        }),
    };

    sir::LoopStmt *loop_stmt = analyzer.create_stmt(sir::LoopStmt{
        .ast_node = nullptr,
        .condition = loop_condition,
        .block = outer_loop_block,
        .latch = loop_latch,
    });

    block->stmts = {iter_var_stmt, next_var_stmt, loop_stmt};

    analyze_block(*block);
    out_stmt = block;
}

sir::Expr StmtAnalyzer::create_method_call(sir::Expr self, sir::FuncDef &method) {
    return analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .callee = analyzer.create_expr(sir::DotExpr{
            .ast_node = nullptr,
            .lhs = self,
            .rhs = method.ident,
        }),
        .args = {},
    });
}

sir::Expr StmtAnalyzer::create_expr(sir::FuncDef &func_def) {
    return analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &func_def.type,
        .symbol = &func_def,
    });
}

sir::Ident StmtAnalyzer::create_ident(std::string value) {
    return sir::Ident{
        .ast_node = nullptr,
        .value = std::move(value),
    };
}

} // namespace sema

} // namespace lang

} // namespace banjo
