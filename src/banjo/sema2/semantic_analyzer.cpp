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
    else if (auto expr = stmt.match<sir::Expr>()) analyze_expr(*expr);
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

void SemanticAnalyzer::analyze_expr(sir::Expr &expr) {
    if (auto int_literal = expr.match<sir::IntLiteral>()) analyze_int_literal(*int_literal);
    else if (auto fp_literal = expr.match<sir::FPLiteral>()) analyze_fp_literal(*fp_literal);
    else if (auto bool_literal = expr.match<sir::BoolLiteral>()) analyze_bool_literal(*bool_literal);
    else if (auto char_literal = expr.match<sir::CharLiteral>()) analyze_char_literal(*char_literal);
    else if (auto string_literal = expr.match<sir::StringLiteral>()) analyze_string_literal(*string_literal);
    else if (auto struct_literal = expr.match<sir::StructLiteral>()) analyze_struct_literal(*struct_literal);
    else if (auto ident_expr = expr.match<sir::IdentExpr>()) analyze_ident_expr(*ident_expr);
    else if (auto binary_expr = expr.match<sir::BinaryExpr>()) analyze_binary_expr(*binary_expr);
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) analyze_unary_expr(*unary_expr);
    else if (auto call_expr = expr.match<sir::CallExpr>()) analyze_call_expr(*call_expr);
    else if (auto cast_expr = expr.match<sir::CastExpr>()) analyze_cast_expr(*cast_expr);
    else if (auto dot_expr = expr.match<sir::DotExpr>()) analyze_dot_expr(*dot_expr);
    else if (auto star_expr = expr.match<sir::StarExpr>()) analyze_star_expr(*star_expr, expr);
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

    sir::StructDef &struct_def = struct_literal.type.as<sir::IdentExpr>().symbol.as<sir::StructDef>();

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

void SemanticAnalyzer::analyze_ident_expr(sir::IdentExpr &ident_expr) {
    sir::Symbol symbol = get_scope().symbol_table->look_up(ident_expr.value);
    assert(symbol);

    ident_expr.symbol = symbol;
    ident_expr.type = symbol.get_type();
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

void SemanticAnalyzer::analyze_call_expr(sir::CallExpr &call_expr) {
    analyze_expr(call_expr.callee);

    for (sir::Expr &arg : call_expr.args) {
        analyze_expr(arg);
    }

    call_expr.type = call_expr.callee.get_type().as<sir::FuncType>().return_type;
}

void SemanticAnalyzer::analyze_dot_expr(sir::DotExpr &dot_expr) {
    analyze_expr(dot_expr.lhs);

    sir::StructDef &struct_def = dot_expr.lhs.get_type().as<sir::IdentExpr>().symbol.as<sir::StructDef>();

    for (unsigned i = 0; i < struct_def.fields.size(); i++) {
        sir::StructField &field = *struct_def.fields[i];

        if (field.ident.value == dot_expr.rhs.value) {
            dot_expr.type = field.type;
            dot_expr.symbol = &field;
            return;
        }
    }

    ASSERT_UNREACHABLE;
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

} // namespace sema

} // namespace lang

} // namespace banjo
