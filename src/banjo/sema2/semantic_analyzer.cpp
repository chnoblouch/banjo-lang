#include "semantic_analyzer.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

SemanticAnalyzer::SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target) : sir_unit(sir_unit) {
    scopes.push({
        .func_def = nullptr,
        .block = nullptr,
        .symbol_table = sir_unit.block.symbol_table,
    });
}

void SemanticAnalyzer::analyze() {
    analyze_decl_block(sir_unit.block);
}

void SemanticAnalyzer::analyze_decl_block(sir::DeclBlock &decl_block) {
    for (sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) analyze_func_def(*func_def);
        else if (auto native_func_decl = decl.match<sir::NativeFuncDecl>()) analyze_native_func_decl(*native_func_decl);
        else if (auto struct_def = decl.match<sir::StructDef>()) analyze_struct_def(*struct_def);
        else if (auto var_decl = decl.match<sir::VarDecl>()) analyze_var_decl(*var_decl, decl);
        else ASSERT_UNREACHABLE;
    }
}

void SemanticAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    get_scope().symbol_table->symbols.insert({func_def.ident.value, &func_def});

    for (sir::Param &param : func_def.type.params) {
        func_def.block.symbol_table->symbols.insert({param.name.value, &param});
        analyze_expr(param.type);
    }

    analyze_expr(func_def.type.return_type);

    push_scope().func_def = &func_def;
    analyze_block(func_def.block);
    pop_scope();
}

void SemanticAnalyzer::analyze_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    get_scope().symbol_table->symbols.insert({native_func_decl.ident.value, &native_func_decl});

    for (sir::Param &param : native_func_decl.type.params) {
        analyze_expr(param.type);
    }

    analyze_expr(native_func_decl.type.return_type);
}

void SemanticAnalyzer::analyze_struct_def(sir::StructDef &struct_def) {
    get_scope().symbol_table->symbols.insert({struct_def.ident.value, &struct_def});

    push_scope().struct_def = &struct_def;
    analyze_decl_block(struct_def.block);
    pop_scope();
}

void SemanticAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
    analyze_expr(var_decl.type);

    if (get_scope().struct_def) {
        sir::StructDef &struct_def = *get_scope().struct_def;

        out_decl = sir_unit.create_decl(sir::StructField{
            .ast_node = var_decl.ast_node,
            .ident = var_decl.ident,
            .type = var_decl.type,
            .index = static_cast<unsigned>(struct_def.fields.size()),
        });

        struct_def.fields.push_back(&out_decl.as<sir::StructField>());
    }
}

void SemanticAnalyzer::analyze_block(sir::Block &block) {
    Scope &scope = push_scope();
    scope.block = &block;
    scope.symbol_table = block.symbol_table;

    for (sir::Stmt &stmt : block.stmts) {
        analyze_stmt(stmt);
    }

    pop_scope();
}

void SemanticAnalyzer::analyze_stmt(sir::Stmt &stmt) {
    if (auto var_stmt = stmt.match<sir::VarStmt>()) analyze_var_stmt(*var_stmt);
    else if (auto assign_stmt = stmt.match<sir::AssignStmt>()) analyze_assign_stmt(*assign_stmt);
    else if (auto return_stmt = stmt.match<sir::ReturnStmt>()) analyze_return_stmt(*return_stmt);
    else if (auto if_stmt = stmt.match<sir::IfStmt>()) analyze_if_stmt(*if_stmt);
    else if (auto while_stmt = stmt.match<sir::WhileStmt>()) analyze_while_stmt(*while_stmt, stmt);
    else if (auto for_stmt = stmt.match<sir::ForStmt>()) analyze_for_stmt(*for_stmt, stmt);
    else if (auto loop_stmt = stmt.match<sir::LoopStmt>()) analyze_loop_stmt(*loop_stmt);
    else if (auto continue_stmt = stmt.match<sir::ContinueStmt>()) analyze_continue_stmt(*continue_stmt);
    else if (auto break_stmt = stmt.match<sir::BreakStmt>()) analyze_break_stmt(*break_stmt);
    else if (auto expr = stmt.match<sir::Expr>()) analyze_expr(*expr);
    else if (auto block = stmt.match<sir::Block>()) analyze_block(*block);
    else ASSERT_UNREACHABLE;
}

void SemanticAnalyzer::analyze_var_stmt(sir::VarStmt &var_stmt) {
    get_scope().symbol_table->symbols.insert({var_stmt.name.value, &var_stmt});

    if (var_stmt.type) {
        analyze_expr(var_stmt.type);
        analyze_expr(var_stmt.value);
    } else {
        analyze_expr(var_stmt.value);
        var_stmt.type = var_stmt.value.get_type();
    }
}

void SemanticAnalyzer::analyze_assign_stmt(sir::AssignStmt &assign_stmt) {
    analyze_expr(assign_stmt.lhs);
    analyze_expr(assign_stmt.rhs);
}

void SemanticAnalyzer::analyze_return_stmt(sir::ReturnStmt &return_stmt) {
    analyze_expr(return_stmt.value);
}

void SemanticAnalyzer::analyze_if_stmt(sir::IfStmt &if_stmt) {
    for (sir::IfCondBranch &cond_branch : if_stmt.cond_branches) {
        analyze_expr(cond_branch.condition);
        analyze_block(cond_branch.block);
    }

    if (if_stmt.else_branch) {
        analyze_block(if_stmt.else_branch->block);
    }
}

void SemanticAnalyzer::analyze_while_stmt(sir::WhileStmt &while_stmt, sir::Stmt &out_stmt) {
    sir::LoopStmt *loop_stmt = sir_unit.create_stmt(sir::LoopStmt{
        .ast_node = nullptr,
        .condition = while_stmt.condition,
        .block = std::move(while_stmt.block),
        .latch = {},
    });

    analyze_loop_stmt(*loop_stmt);
    out_stmt = loop_stmt;
}

void SemanticAnalyzer::analyze_for_stmt(sir::ForStmt &for_stmt, sir::Stmt &out_stmt) {
    sir::RangeExpr &range = for_stmt.range.as<sir::RangeExpr>();

    sir::Block *block = sir_unit.create_stmt(sir::Block{
        .ast_node = nullptr,
        .stmts = {},
        .symbol_table = sir_unit.create_symbol_table({
            .parent = get_scope().symbol_table,
            .symbols = {},
        }),
    });

    sir::VarStmt *var_stmt = sir_unit.create_stmt(sir::VarStmt{
        .ast_node = nullptr,
        .name = for_stmt.ident,
        .type = nullptr,
        .value = range.lhs,
    });

    sir::SymbolExpr *var_ref_expr = sir_unit.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .symbol = var_stmt,
    });

    sir::BinaryExpr *loop_condition = sir_unit.create_expr(sir::BinaryExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .op = sir::BinaryOp::LT,
        .lhs = var_ref_expr,
        .rhs = range.rhs,
    });

    sir::Block loop_block{
        .ast_node = for_stmt.block.ast_node,
        .stmts = std::move(for_stmt.block.stmts),
        .symbol_table = sir_unit.create_symbol_table({
            .parent = block->symbol_table,
            .symbols = {},
        }),
    };

    sir::AssignStmt *inc_stmt = sir_unit.create_stmt(sir::AssignStmt{
        .ast_node = nullptr,
        .lhs = var_ref_expr,
        .rhs = sir_unit.create_expr(sir::BinaryExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .op = sir::BinaryOp::ADD,
            .lhs = var_ref_expr,
            .rhs = sir_unit.create_expr(sir::IntLiteral{
                .ast_node = nullptr,
                .type = nullptr,
                .value = 1,
            }),
        }),
    });

    sir::Block loop_latch{
        .ast_node = nullptr,
        .stmts = {inc_stmt},
        .symbol_table = sir_unit.create_symbol_table({
            .parent = block->symbol_table,
            .symbols = {},
        }),
    };

    sir::LoopStmt *loop_stmt = sir_unit.create_stmt(sir::LoopStmt{
        .ast_node = nullptr,
        .condition = loop_condition,
        .block = loop_block,
        .latch = loop_latch,
    });

    block->stmts = {var_stmt, loop_stmt};

    analyze_block(*block);
    out_stmt = block;
}

void SemanticAnalyzer::analyze_loop_stmt(sir::LoopStmt &loop_stmt) {
    analyze_expr(loop_stmt.condition);
    analyze_block(loop_stmt.block);

    if (loop_stmt.latch) {
        analyze_block(*loop_stmt.latch);
    }
}

void SemanticAnalyzer::analyze_continue_stmt(sir::ContinueStmt & /*continue_stmt*/) {}

void SemanticAnalyzer::analyze_break_stmt(sir::BreakStmt & /*break_stmt*/) {}

void SemanticAnalyzer::analyze_expr(sir::Expr &expr) {
    analyze_expr(expr, *get_scope().symbol_table);
}

void SemanticAnalyzer::analyze_expr(sir::Expr &expr, sir::SymbolTable &symbol_table) {
    if (auto int_literal = expr.match<sir::IntLiteral>()) analyze_int_literal(*int_literal);
    else if (auto fp_literal = expr.match<sir::FPLiteral>()) analyze_fp_literal(*fp_literal);
    else if (auto bool_literal = expr.match<sir::BoolLiteral>()) analyze_bool_literal(*bool_literal);
    else if (auto char_literal = expr.match<sir::CharLiteral>()) analyze_char_literal(*char_literal);
    else if (auto string_literal = expr.match<sir::StringLiteral>()) analyze_string_literal(*string_literal);
    else if (auto struct_literal = expr.match<sir::StructLiteral>()) analyze_struct_literal(*struct_literal);
    else if (auto binary_expr = expr.match<sir::BinaryExpr>()) analyze_binary_expr(*binary_expr);
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) analyze_unary_expr(*unary_expr);
    else if (auto call_expr = expr.match<sir::CallExpr>()) analyze_call_expr(*call_expr, symbol_table);
    else if (auto cast_expr = expr.match<sir::CastExpr>()) analyze_cast_expr(*cast_expr);
    else if (auto dot_expr = expr.match<sir::DotExpr>()) analyze_dot_expr(*dot_expr, symbol_table, expr);
    else if (auto ident_expr = expr.match<sir::IdentExpr>()) analyze_ident_expr(*ident_expr, symbol_table, expr);
    else if (auto star_expr = expr.match<sir::StarExpr>()) analyze_star_expr(*star_expr, expr);
    else if (auto bracket_expr = expr.match<sir::BracketExpr>()) analyze_bracket_expr(*bracket_expr, expr);
}

void SemanticAnalyzer::analyze_int_literal(sir::IntLiteral &int_literal) {
    int_literal.type = sir_unit.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::I32,
    });
}

void SemanticAnalyzer::analyze_fp_literal(sir::FPLiteral &fp_literal) {
    fp_literal.type = sir_unit.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::F32,
    });
}

void SemanticAnalyzer::analyze_bool_literal(sir::BoolLiteral &bool_literal) {
    bool_literal.type = sir_unit.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::BOOL,
    });
}

void SemanticAnalyzer::analyze_char_literal(sir::CharLiteral &char_literal) {
    char_literal.type = sir_unit.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::U8,
    });
}

void SemanticAnalyzer::analyze_string_literal(sir::StringLiteral &string_literal) {
    sir::Expr base_type = sir_unit.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::U8,
    });

    string_literal.type = sir_unit.create_expr(sir::PointerType{
        .ast_node = nullptr,
        .base_type = base_type,
    });
}

void SemanticAnalyzer::analyze_struct_literal(sir::StructLiteral &struct_literal) {
    analyze_expr(struct_literal.type);

    sir::StructDef &struct_def = struct_literal.type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        for (sir::StructField *field : struct_def.fields) {
            if (field->ident.value == entry.ident.value) {
                entry.field = field;
            }
        }

        assert(entry.field);
        analyze_expr(entry.value);
    }
}

void SemanticAnalyzer::analyze_binary_expr(sir::BinaryExpr &binary_expr) {
    analyze_expr(binary_expr.lhs);
    analyze_expr(binary_expr.rhs);

    binary_expr.type = binary_expr.lhs.get_type();
}

void SemanticAnalyzer::analyze_unary_expr(sir::UnaryExpr &unary_expr) {
    analyze_expr(unary_expr.value);

    if (unary_expr.op == sir::UnaryOp::NEG) {
        unary_expr.type = unary_expr.value.get_type();
    } else if (unary_expr.op == sir::UnaryOp::REF) {
        unary_expr.type = sir_unit.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = unary_expr.value.get_type(),
        });
    } else if (unary_expr.op == sir::UnaryOp::DEREF) {
        // Deref unary operations are handled by `analyze_star_expr`.
        ASSERT_UNREACHABLE;
    } else if (unary_expr.op == sir::UnaryOp::NOT) {
        unary_expr.type = sir_unit.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::BOOL,
        });
    }
}

void SemanticAnalyzer::analyze_cast_expr(sir::CastExpr &cast_expr) {
    analyze_expr(cast_expr.type);
    analyze_expr(cast_expr.value);
}

void SemanticAnalyzer::analyze_call_expr(sir::CallExpr &call_expr, sir::SymbolTable &symbol_table) {
    if (auto dot_expr = call_expr.callee.match<sir::DotExpr>()) {
        analyze_dot_expr_callee(*dot_expr, symbol_table, call_expr);
    } else {
        analyze_expr(call_expr.callee);
    }

    for (sir::Expr &arg : call_expr.args) {
        analyze_expr(arg);
    }

    call_expr.type = call_expr.callee.get_type().as<sir::FuncType>().return_type;
}

void SemanticAnalyzer::analyze_dot_expr_callee(
    sir::DotExpr &dot_expr,
    sir::SymbolTable &symbol_table,
    sir::CallExpr &out_call_expr
) {
    analyze_expr(dot_expr.lhs, symbol_table);

    if (auto struct_def = dot_expr.lhs.get_type().match_symbol<sir::StructDef>()) {
        sir::FuncDef &method = struct_def->block.symbol_table->look_up(dot_expr.rhs.value).as<sir::FuncDef>();

        out_call_expr.callee = sir_unit.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.rhs.ast_node,
            .type = &method.type,
            .symbol = &method,
        });

        sir::Expr self_arg = sir_unit.create_expr(sir::UnaryExpr{
            .ast_node = nullptr,
            .type = sir_unit.create_expr(sir::PointerType{
                .ast_node = nullptr,
                .base_type = dot_expr.lhs.get_type(),
            }),
            .op = sir::UnaryOp::REF,
            .value = dot_expr.lhs,
        });

        out_call_expr.args.insert(out_call_expr.args.begin(), self_arg);
    } else if (auto pointer_type = dot_expr.lhs.get_type().match<sir::PointerType>()) {
        if (auto struct_def = pointer_type->base_type.match_symbol<sir::StructDef>()) {
            sir::FuncDef &method = struct_def->block.symbol_table->look_up(dot_expr.rhs.value).as<sir::FuncDef>();

            out_call_expr.callee = sir_unit.create_expr(sir::SymbolExpr{
                .ast_node = dot_expr.rhs.ast_node,
                .type = &method.type,
                .symbol = &method,
            });

            out_call_expr.args.insert(out_call_expr.args.begin(), dot_expr.lhs);
        } else {
            analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);
        }
    } else {
        analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);
    }
}

void SemanticAnalyzer::analyze_dot_expr(sir::DotExpr &dot_expr, sir::SymbolTable &symbol_table, sir::Expr &out_expr) {
    analyze_expr(dot_expr.lhs, symbol_table);
    analyze_dot_expr_rhs(dot_expr, out_expr);
}

void SemanticAnalyzer::analyze_ident_expr(
    sir::IdentExpr &ident_expr,
    sir::SymbolTable &symbol_table,
    sir::Expr &out_expr
) {
    sir::Symbol symbol = symbol_table.look_up(ident_expr.value);
    assert(symbol);

    out_expr = sir_unit.create_expr(sir::SymbolExpr{
        .ast_node = ident_expr.ast_node,
        .type = symbol.get_type(),
        .symbol = symbol,
    });
}

void SemanticAnalyzer::analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr) {
    analyze_expr(star_expr.value);

    if (star_expr.value.is_type()) {
        out_expr = sir_unit.create_expr(sir::PointerType{
            .ast_node = star_expr.ast_node,
            .base_type = star_expr.value,
        });
    } else {
        out_expr = sir_unit.create_expr(sir::UnaryExpr{
            .ast_node = star_expr.ast_node,
            .type = star_expr.value.get_type().as<sir::PointerType>().base_type,
            .op = sir::UnaryOp::DEREF,
            .value = star_expr.value,
        });
    }
}

void SemanticAnalyzer::analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr) {
    analyze_expr(bracket_expr.lhs);

    for (sir::Expr &expr : bracket_expr.rhs) {
        analyze_expr(expr);
    }

    if (bracket_expr.lhs.is_type()) {
        ASSERT_UNREACHABLE;
    } else {
        out_expr = sir_unit.create_expr(sir::IndexExpr{
            .ast_node = bracket_expr.ast_node,
            .type = bracket_expr.lhs.get_type().as<sir::PointerType>().base_type,
            .base = bracket_expr.lhs,
            .index = bracket_expr.rhs[0],
        });
    }
}

void SemanticAnalyzer::analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    if (dot_expr.lhs.is_value()) {
        sir::Expr lhs_type = dot_expr.lhs.get_type();

        if (auto symbol_expr = lhs_type.match<sir::SymbolExpr>()) {
            sir::StructDef &struct_def = lhs_type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();
            sir::StructField *field = struct_def.find_field(dot_expr.rhs.value);

            out_expr = sir_unit.create_expr(sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field->type,
                .base = dot_expr.lhs,
                .field = field,
            });
        } else if (auto pointer_expr = lhs_type.match<sir::PointerType>()) {
            sir::StructDef &struct_def = pointer_expr->base_type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();
            sir::StructField *field = struct_def.find_field(dot_expr.rhs.value);

            out_expr = sir_unit.create_expr(sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field->type,
                .base = sir_unit.create_expr(sir::UnaryExpr{
                    .ast_node = nullptr,
                    .type = pointer_expr->base_type,
                    .op = sir::UnaryOp::DEREF,
                    .value = dot_expr.lhs,
                }),
                .field = field,
            });
        } else {
            ASSERT_UNREACHABLE;
        }
    } else if (auto struct_def = dot_expr.lhs.as<sir::SymbolExpr>().symbol.match<sir::StructDef>()) {
        sir::Symbol symbol = struct_def->block.symbol_table->look_up(dot_expr.rhs.value);

        out_expr = sir_unit.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.ast_node,
            .type = symbol.get_type(),
            .symbol = symbol,
        });
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
