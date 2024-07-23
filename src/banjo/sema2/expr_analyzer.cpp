#include "expr_analyzer.hpp"

#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

ExprAnalyzer::ExprAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void ExprAnalyzer::analyze(sir::Expr &expr) {
    analyze(expr, *analyzer.get_scope().symbol_table);
}

void ExprAnalyzer::analyze(sir::Expr &expr, sir::SymbolTable &symbol_table) {
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

void ExprAnalyzer::analyze_int_literal(sir::IntLiteral &int_literal) {
    int_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::I32,
    });
}

void ExprAnalyzer::analyze_fp_literal(sir::FPLiteral &fp_literal) {
    fp_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::F32,
    });
}

void ExprAnalyzer::analyze_bool_literal(sir::BoolLiteral &bool_literal) {
    bool_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::BOOL,
    });
}

void ExprAnalyzer::analyze_char_literal(sir::CharLiteral &char_literal) {
    char_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::U8,
    });
}

void ExprAnalyzer::analyze_string_literal(sir::StringLiteral &string_literal) {
    sir::Expr base_type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::U8,
    });

    string_literal.type = analyzer.create_expr(sir::PointerType{
        .ast_node = nullptr,
        .base_type = base_type,
    });
}

void ExprAnalyzer::analyze_struct_literal(sir::StructLiteral &struct_literal) {
    analyze(struct_literal.type);

    sir::StructDef &struct_def = struct_literal.type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        for (sir::StructField *field : struct_def.fields) {
            if (field->ident.value == entry.ident.value) {
                entry.field = field;
            }
        }

        assert(entry.field);
        analyze(entry.value);
    }
}

void ExprAnalyzer::analyze_binary_expr(sir::BinaryExpr &binary_expr) {
    analyze(binary_expr.lhs);
    analyze(binary_expr.rhs);

    binary_expr.type = binary_expr.lhs.get_type();
}

void ExprAnalyzer::analyze_unary_expr(sir::UnaryExpr &unary_expr) {
    analyze(unary_expr.value);

    if (unary_expr.op == sir::UnaryOp::NEG) {
        unary_expr.type = unary_expr.value.get_type();
    } else if (unary_expr.op == sir::UnaryOp::REF) {
        unary_expr.type = analyzer.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = unary_expr.value.get_type(),
        });
    } else if (unary_expr.op == sir::UnaryOp::DEREF) {
        // Deref unary operations are handled by `analyze_star_expr`.
        ASSERT_UNREACHABLE;
    } else if (unary_expr.op == sir::UnaryOp::NOT) {
        unary_expr.type = analyzer.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::BOOL,
        });
    }
}

void ExprAnalyzer::analyze_cast_expr(sir::CastExpr &cast_expr) {
    analyze(cast_expr.type);
    analyze(cast_expr.value);
}

void ExprAnalyzer::analyze_call_expr(sir::CallExpr &call_expr, sir::SymbolTable &symbol_table) {
    if (auto dot_expr = call_expr.callee.match<sir::DotExpr>()) {
        analyze_dot_expr_callee(*dot_expr, symbol_table, call_expr);
    } else {
        analyze(call_expr.callee);
    }

    for (sir::Expr &arg : call_expr.args) {
        analyze(arg);
    }

    call_expr.type = call_expr.callee.get_type().as<sir::FuncType>().return_type;
}

void ExprAnalyzer::analyze_dot_expr_callee(
    sir::DotExpr &dot_expr,
    sir::SymbolTable &symbol_table,
    sir::CallExpr &out_call_expr
) {
    analyze(dot_expr.lhs, symbol_table);

    if (auto struct_def = dot_expr.lhs.get_type().match_symbol<sir::StructDef>()) {
        sir::FuncDef &method = struct_def->block.symbol_table->look_up(dot_expr.rhs.value).as<sir::FuncDef>();

        out_call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.rhs.ast_node,
            .type = &method.type,
            .symbol = &method,
        });

        sir::Expr self_arg = analyzer.create_expr(sir::UnaryExpr{
            .ast_node = nullptr,
            .type = analyzer.create_expr(sir::PointerType{
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

            out_call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
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

void ExprAnalyzer::analyze_dot_expr(sir::DotExpr &dot_expr, sir::SymbolTable &symbol_table, sir::Expr &out_expr) {
    analyze(dot_expr.lhs, symbol_table);
    analyze_dot_expr_rhs(dot_expr, out_expr);
}

void ExprAnalyzer::analyze_ident_expr(sir::IdentExpr &ident_expr, sir::SymbolTable &symbol_table, sir::Expr &out_expr) {
    sir::Symbol symbol = symbol_table.look_up(ident_expr.value);
    assert(symbol);

    out_expr = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = ident_expr.ast_node,
        .type = symbol.get_type(),
        .symbol = symbol,
    });
}

void ExprAnalyzer::analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr) {
    analyze(star_expr.value);

    if (star_expr.value.is_type()) {
        out_expr = analyzer.create_expr(sir::PointerType{
            .ast_node = star_expr.ast_node,
            .base_type = star_expr.value,
        });
    } else {
        out_expr = analyzer.create_expr(sir::UnaryExpr{
            .ast_node = star_expr.ast_node,
            .type = star_expr.value.get_type().as<sir::PointerType>().base_type,
            .op = sir::UnaryOp::DEREF,
            .value = star_expr.value,
        });
    }
}

void ExprAnalyzer::analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr) {
    analyze(bracket_expr.lhs);

    for (sir::Expr &expr : bracket_expr.rhs) {
        analyze(expr);
    }

    if (bracket_expr.lhs.is_type()) {
        ASSERT_UNREACHABLE;
    } else {
        out_expr = analyzer.create_expr(sir::IndexExpr{
            .ast_node = bracket_expr.ast_node,
            .type = bracket_expr.lhs.get_type().as<sir::PointerType>().base_type,
            .base = bracket_expr.lhs,
            .index = bracket_expr.rhs[0],
        });
    }
}

void ExprAnalyzer::analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    if (auto mod = dot_expr.lhs.match_symbol<sir::Module>()) {
        sir::Symbol symbol = mod->block.symbol_table->look_up(dot_expr.rhs.value);

        out_expr = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.ast_node,
            .type = symbol.get_type(),
            .symbol = symbol,
        });
    } else if (auto struct_def = dot_expr.lhs.match_symbol<sir::StructDef>()) {
        sir::Symbol symbol = struct_def->block.symbol_table->look_up(dot_expr.rhs.value);

        out_expr = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.ast_node,
            .type = symbol.get_type(),
            .symbol = symbol,
        });
    } else if (dot_expr.lhs.is_value()) {
        sir::Expr lhs_type = dot_expr.lhs.get_type();

        if (auto symbol_expr = lhs_type.match<sir::SymbolExpr>()) {
            sir::StructDef &struct_def = lhs_type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();
            sir::StructField *field = struct_def.find_field(dot_expr.rhs.value);

            out_expr = analyzer.create_expr(sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field->type,
                .base = dot_expr.lhs,
                .field = field,
            });
        } else if (auto pointer_expr = lhs_type.match<sir::PointerType>()) {
            sir::StructDef &struct_def = pointer_expr->base_type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();
            sir::StructField *field = struct_def.find_field(dot_expr.rhs.value);

            out_expr = analyzer.create_expr(sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field->type,
                .base = analyzer.create_expr(sir::UnaryExpr{
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
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
