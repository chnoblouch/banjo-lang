#include "expr_analyzer.hpp"

#include "banjo/sema2/generics_specializer.hpp"
#include "banjo/sema2/meta_expansion.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>

#define HANDLE_RESULT(result)                                                                                          \
    if ((result) != Result::SUCCESS) {                                                                                 \
        return result;                                                                                                 \
    }

namespace banjo {

namespace lang {

namespace sema {

ExprAnalyzer::ExprAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

Result ExprAnalyzer::analyze(sir::Expr &expr) {
    if (auto int_literal = expr.match<sir::IntLiteral>()) analyze_int_literal(*int_literal);
    else if (auto fp_literal = expr.match<sir::FPLiteral>()) analyze_fp_literal(*fp_literal);
    else if (auto bool_literal = expr.match<sir::BoolLiteral>()) analyze_bool_literal(*bool_literal);
    else if (auto char_literal = expr.match<sir::CharLiteral>()) analyze_char_literal(*char_literal);
    else if (auto null_literal = expr.match<sir::NullLiteral>()) analyze_null_literal(*null_literal);
    else if (auto array_literal = expr.match<sir::ArrayLiteral>()) analyze_array_literal(*array_literal);
    else if (auto string_literal = expr.match<sir::StringLiteral>()) analyze_string_literal(*string_literal);
    else if (auto struct_literal = expr.match<sir::StructLiteral>()) analyze_struct_literal(*struct_literal);
    else if (auto binary_expr = expr.match<sir::BinaryExpr>()) analyze_binary_expr(*binary_expr);
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) analyze_unary_expr(*unary_expr);
    else if (auto cast_expr = expr.match<sir::CastExpr>()) analyze_cast_expr(*cast_expr);
    else if (auto call_expr = expr.match<sir::CallExpr>()) analyze_call_expr(*call_expr);
    else if (auto range_expr = expr.match<sir::RangeExpr>()) analyze_range_expr(*range_expr);
    else if (auto tuple_expr = expr.match<sir::TupleExpr>()) analyze_tuple_expr(*tuple_expr);
    else if (auto static_array_type = expr.match<sir::StaticArrayType>()) analyze_static_array_type(*static_array_type);
    else if (auto dot_expr = expr.match<sir::DotExpr>()) analyze_dot_expr(*dot_expr, expr);
    else if (auto ident_expr = expr.match<sir::IdentExpr>()) return analyze_ident_expr(*ident_expr, expr);
    else if (auto star_expr = expr.match<sir::StarExpr>()) analyze_star_expr(*star_expr, expr);
    else if (auto bracket_expr = expr.match<sir::BracketExpr>()) analyze_bracket_expr(*bracket_expr, expr);

    return Result::SUCCESS;
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

void ExprAnalyzer::analyze_null_literal(sir::NullLiteral &null_literal) {
    null_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::ADDR,
    });
}

void ExprAnalyzer::analyze_array_literal(sir::ArrayLiteral &array_literal) {
    assert(array_literal.values.size() != 0);

    for (sir::Expr &value : array_literal.values) {
        analyze(value);
    }

    array_literal.type = array_literal.values[0].get_type();
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

Result ExprAnalyzer::analyze_struct_literal(sir::StructLiteral &struct_literal) {
    HANDLE_RESULT(analyze(struct_literal.type));

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

    return Result::SUCCESS;
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

void ExprAnalyzer::analyze_call_expr(sir::CallExpr &call_expr) {
    if (auto dot_expr = call_expr.callee.match<sir::DotExpr>()) {
        analyze_dot_expr_callee(*dot_expr, call_expr);
    } else {
        analyze(call_expr.callee);
    }

    for (sir::Expr &arg : call_expr.args) {
        analyze(arg);
    }

    if (auto overload_set = call_expr.callee.match_symbol<sir::OverloadSet>()) {
        resolve_overload(*overload_set, call_expr);
    }

    call_expr.type = call_expr.callee.get_type().as<sir::FuncType>().return_type;
}

void ExprAnalyzer::analyze_dot_expr_callee(sir::DotExpr &dot_expr, sir::CallExpr &out_call_expr) {
    analyze(dot_expr.lhs);

    if (auto struct_def = dot_expr.lhs.get_type().match_symbol<sir::StructDef>()) {
        sir::Symbol method = struct_def->block.symbol_table->look_up(dot_expr.rhs.value);

        out_call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.rhs.ast_node,
            .type = method.get_type(),
            .symbol = method,
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
            sir::Symbol method = struct_def->block.symbol_table->look_up(dot_expr.rhs.value);

            out_call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
                .ast_node = dot_expr.rhs.ast_node,
                .type = method.get_type(),
                .symbol = method,
            });

            out_call_expr.args.insert(out_call_expr.args.begin(), dot_expr.lhs);
        } else {
            analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);
        }
    } else {
        analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);
    }
}

void ExprAnalyzer::analyze_static_array_type(sir::StaticArrayType &static_array_type) {
    analyze(static_array_type.base_type);
    analyze(static_array_type.length);
}

void ExprAnalyzer::analyze_dot_expr(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    analyze(dot_expr.lhs);
    analyze_dot_expr_rhs(dot_expr, out_expr);
}

void ExprAnalyzer::analyze_range_expr(sir::RangeExpr &range_expr) {
    analyze(range_expr.lhs);
    analyze(range_expr.rhs);
}

void ExprAnalyzer::analyze_tuple_expr(sir::TupleExpr &tuple_expr) {
    assert(!tuple_expr.exprs.empty());

    for (sir::Expr &expr : tuple_expr.exprs) {
        analyze(expr);
    }

    if (tuple_expr.exprs[0].is_type()) {
        return;
    }

    std::vector<sir::Expr> types;
    types.resize(tuple_expr.exprs.size());

    for (unsigned i = 0; i < tuple_expr.exprs.size(); i++) {
        types[i] = tuple_expr.exprs[i].get_type();
    }

    tuple_expr.type = analyzer.create_expr(sir::TupleExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .exprs = types,
    });
}

Result ExprAnalyzer::analyze_ident_expr(sir::IdentExpr &ident_expr, sir::Expr &out_expr) {
    sir::SymbolTable &symbol_table = analyzer.get_symbol_table();
    const std::unordered_map<std::string_view, sir::Expr> &generic_args = analyzer.get_scope().generic_args;

    if (!generic_args.empty()) {
        auto iter = generic_args.find(ident_expr.value);
        if (iter != generic_args.end()) {
            out_expr = iter->second;
            return Result::SUCCESS;
        }
    }

    sir::Symbol symbol = symbol_table.look_up(ident_expr.value);

    if (!symbol && !symbol_table.complete && !analyzer.is_in_stmt_block()) {
        MetaExpansion(analyzer).run_on_decl_block(*analyzer.get_scope().decl_block);
        symbol = symbol_table.look_up(ident_expr.value);
    }

    if (!symbol) {
        analyzer.report_generator.report_err_symbol_not_found(ident_expr);
        return Result::ERROR;
    }

    out_expr = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = ident_expr.ast_node,
        .type = symbol.get_type(),
        .symbol = symbol,
    });

    return Result::SUCCESS;
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

    if (auto func_def = bracket_expr.lhs.match_symbol<sir::FuncDef>()) {
        if (func_def->is_generic()) {
            sir::FuncDef *specialization = GenericsSpecializer(analyzer).specialize(*func_def, bracket_expr.rhs);

            out_expr = analyzer.create_expr(sir::SymbolExpr{
                .ast_node = bracket_expr.ast_node,
                .type = &specialization->type,
                .symbol = specialization,
            });

            return;
        }
    } else if (auto struct_def = bracket_expr.lhs.match_symbol<sir::StructDef>()) {
        if (struct_def->is_generic()) {
            sir::StructDef *specialization = GenericsSpecializer(analyzer).specialize(*struct_def, bracket_expr.rhs);

            out_expr = analyzer.create_expr(sir::SymbolExpr{
                .ast_node = bracket_expr.ast_node,
                .type = nullptr,
                .symbol = specialization,
            });

            return;
        }
    }

    const sir::Expr &lhs_type = bracket_expr.lhs.get_type();

    if (auto pointer_type = lhs_type.match<sir::PointerType>()) {
        out_expr = analyzer.create_expr(sir::IndexExpr{
            .ast_node = bracket_expr.ast_node,
            .type = pointer_type->base_type,
            .base = bracket_expr.lhs,
            .index = bracket_expr.rhs[0],
        });
    } else if (auto static_array_type = lhs_type.match<sir::StaticArrayType>()) {
        out_expr = analyzer.create_expr(sir::IndexExpr{
            .ast_node = bracket_expr.ast_node,
            .type = static_array_type->base_type,
            .base = bracket_expr.lhs,
            .index = bracket_expr.rhs[0],
        });
    } else {
        ASSERT_UNREACHABLE;
    }
}

void ExprAnalyzer::analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    sir::DeclBlock *decl_block = dot_expr.lhs.get_decl_block();
    if (decl_block) {
        sir::Symbol symbol = decl_block->symbol_table->look_up(dot_expr.rhs.value);

        if (!symbol && !decl_block->symbol_table->complete) {
            MetaExpansion(analyzer).run_on_decl_block(*analyzer.get_scope().decl_block);
            symbol = decl_block->symbol_table->look_up(dot_expr.rhs.value);
        }

        assert(symbol);

        out_expr = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.ast_node,
            .type = symbol.get_type(),
            .symbol = symbol,
        });

        return;
    }

    if (dot_expr.lhs.is_value()) {
        sir::Expr lhs_type = dot_expr.lhs.get_type();

        if (auto symbol_expr = lhs_type.match<sir::SymbolExpr>()) {
            sir::StructDef &struct_def = lhs_type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();
            sir::StructField *field = struct_def.find_field(dot_expr.rhs.value);

            out_expr = analyzer.create_expr(sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field->type,
                .base = dot_expr.lhs,
                .field_index = field->index,
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
                .field_index = field->index,
            });
        } else if (auto tuple_expr = lhs_type.match<sir::TupleExpr>()) {
            unsigned field_index = std::stoul(dot_expr.rhs.value);

            out_expr = analyzer.create_expr(sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = tuple_expr->exprs[field_index],
                .base = dot_expr.lhs,
                .field_index = field_index,
            });
        } else {
            ASSERT_UNREACHABLE;
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

void ExprAnalyzer::resolve_overload(sir::OverloadSet &overload_set, sir::CallExpr &inout_call_expr) {
    for (sir::FuncDef *func_def : overload_set.func_defs) {
        if (is_matching_overload(*func_def, inout_call_expr.args)) {
            inout_call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
                .ast_node = nullptr,
                .type = &func_def->type,
                .symbol = func_def,
            });

            return;
        }
    }

    ASSERT_UNREACHABLE;
}

bool ExprAnalyzer::is_matching_overload(sir::FuncDef &func_def, std::vector<sir::Expr> &args) {
    if (func_def.type.params.size() != args.size()) {
        return false;
    }

    for (unsigned i = 0; i < args.size(); i++) {
        if (args[i].get_type() != func_def.type.params[i].type) {
            return false;
        }
    }

    return true;
}

} // namespace sema

} // namespace lang

} // namespace banjo
