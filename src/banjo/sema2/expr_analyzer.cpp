#include "expr_analyzer.hpp"

#include "banjo/sema2/generics_specializer.hpp"
#include "banjo/sema2/magic_methods.hpp"
#include "banjo/sema2/meta_expansion.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>

namespace banjo {

namespace lang {

namespace sema {

ExprConstraints ExprConstraints::expect_type(sir::Expr type) {
    return {
        .expected_type = type,
    };
}

ExprAnalyzer::ExprAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

ExprAnalyzer::ExprAnalyzer(SemanticAnalyzer &analyzer, ExprConstraints constraints)
  : analyzer(analyzer),
    constraints(constraints) {}

Result ExprAnalyzer::analyze(sir::Expr &expr) {
    Result result = Result::SUCCESS;

    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IMPOSSIBLE,
        result = analyze_int_literal(*inner),
        result = analyze_fp_literal(*inner),
        analyze_bool_literal(*inner),
        analyze_char_literal(*inner),
        result = analyze_null_literal(*inner),
        analyze_array_literal(*inner),
        result = analyze_string_literal(*inner, expr),
        analyze_struct_literal(*inner),
        SIR_VISIT_IGNORE,
        result = analyze_binary_expr(*inner, expr),
        result = analyze_unary_expr(*inner, expr),
        analyze_cast_expr(*inner),
        SIR_VISIT_IGNORE,
        result = analyze_call_expr(*inner),
        SIR_VISIT_IGNORE,
        analyze_range_expr(*inner),
        analyze_tuple_expr(*inner),
        SIR_VISIT_IGNORE,
        SIR_VISIT_IGNORE,
        analyze_static_array_type(*inner),
        analyze_func_type(*inner),
        result = analyze_ident_expr(*inner, expr),
        analyze_star_expr(*inner, expr),
        result = analyze_bracket_expr(*inner, expr),
        analyze_dot_expr(*inner, expr)
    );

    if (result != Result::SUCCESS) {
        return result;
    }

    if (constraints.expected_type && expr.get_type() != constraints.expected_type) {
        analyzer.report_generator.report_err_type_mismatch(expr, constraints.expected_type, expr.get_type());
    }

    return result;
}

Result ExprAnalyzer::analyze_int_literal(sir::IntLiteral &int_literal) {
    if (constraints.expected_type) {
        if (constraints.expected_type.is_int_type()) {
            int_literal.type = constraints.expected_type;
            return Result::SUCCESS;
        } else {
            analyzer.report_generator.report_err_cant_coerce_int_literal(int_literal, constraints.expected_type);
            return Result::ERROR;
        }
    } else {
        int_literal.type = analyzer.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::I32,
        });

        return Result::SUCCESS;
    }
}

Result ExprAnalyzer::analyze_fp_literal(sir::FPLiteral &fp_literal) {
    if (constraints.expected_type) {
        if (constraints.expected_type.is_fp_type()) {
            fp_literal.type = constraints.expected_type;
            return Result::SUCCESS;
        } else {
            analyzer.report_generator.report_err_cant_coerce_fp_literal(fp_literal, constraints.expected_type);
            return Result::ERROR;
        }
    } else {
        fp_literal.type = analyzer.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::F32,
        });

        return Result::SUCCESS;
    }
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

Result ExprAnalyzer::analyze_null_literal(sir::NullLiteral &null_literal) {
    if (constraints.expected_type) {
        if (constraints.expected_type.is_addr_like_type()) {
            null_literal.type = constraints.expected_type;
            return Result::SUCCESS;
        } else {
            analyzer.report_generator.report_err_cant_coerce_null_literal(null_literal, constraints.expected_type);
            return Result::ERROR;
        }
    } else {
        null_literal.type = analyzer.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::ADDR,
        });

        return Result::SUCCESS;
    }
}

void ExprAnalyzer::analyze_array_literal(sir::ArrayLiteral &array_literal) {
    assert(array_literal.values.size() != 0);

    for (sir::Expr &value : array_literal.values) {
        ExprAnalyzer(analyzer).analyze(value);
    }

    array_literal.type = array_literal.values[0].get_type();
}

Result ExprAnalyzer::analyze_string_literal(sir::StringLiteral &string_literal, sir::Expr &out_expr) {
    if (constraints.expected_type) {
        if (constraints.expected_type.is_u8_ptr()) {
            string_literal.type = constraints.expected_type;
            return Result::SUCCESS;
        }

        if (auto symbol_expr = constraints.expected_type.match<sir::SymbolExpr>()) {
            if (symbol_expr->symbol == analyzer.find_std_string()) {
                create_std_string(string_literal, out_expr);
                return Result::SUCCESS;
            }
        }

        analyzer.report_generator.report_err_cant_coerce_string_literal(string_literal, constraints.expected_type);
        return Result::ERROR;
    } else {
        create_std_string(string_literal, out_expr);
        return Result::SUCCESS;
    }
}

void ExprAnalyzer::analyze_struct_literal(sir::StructLiteral &struct_literal) {
    if (struct_literal.type) {
        ExprAnalyzer(analyzer).analyze(struct_literal.type);
    } else if (constraints.expected_type) {
        struct_literal.type = constraints.expected_type;
    } else {
        ASSERT_UNREACHABLE;
    }

    sir::StructDef &struct_def = struct_literal.type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        for (sir::StructField *field : struct_def.fields) {
            if (field->ident.value == entry.ident.value) {
                entry.field = field;
            }
        }

        assert(entry.field);
        ExprAnalyzer(analyzer, ExprConstraints::expect_type(entry.field->type)).analyze(entry.value);
    }
}

Result ExprAnalyzer::analyze_binary_expr(sir::BinaryExpr &binary_expr, sir::Expr &out_expr) {
    ExprAnalyzer(analyzer).analyze(binary_expr.lhs);
    ExprAnalyzer(analyzer).analyze(binary_expr.rhs);

    if (auto struct_def = binary_expr.lhs.get_type().match_symbol<sir::StructDef>()) {
        std::string_view impl_name = MagicMethods::look_up(binary_expr.op);
        sir::Symbol symbol = struct_def->block.symbol_table->look_up(impl_name);

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(binary_expr);
            return Result::ERROR;
        }

        return analyze_operator_overload_call(symbol, binary_expr.lhs, binary_expr.rhs, out_expr);
    }

    switch (binary_expr.op) {
        case sir::BinaryOp::ADD:
        case sir::BinaryOp::SUB:
        case sir::BinaryOp::MUL:
        case sir::BinaryOp::DIV:
        case sir::BinaryOp::MOD:
        case sir::BinaryOp::BIT_AND:
        case sir::BinaryOp::BIT_OR:
        case sir::BinaryOp::BIT_XOR:
        case sir::BinaryOp::SHL:
        case sir::BinaryOp::SHR: binary_expr.type = binary_expr.lhs.get_type(); break;
        case sir::BinaryOp::EQ:
        case sir::BinaryOp::NE:
        case sir::BinaryOp::GT:
        case sir::BinaryOp::LT:
        case sir::BinaryOp::GE:
        case sir::BinaryOp::LE:
        case sir::BinaryOp::AND:
        case sir::BinaryOp::OR:
            binary_expr.type = analyzer.create_expr(sir::PrimitiveType{
                .ast_node = nullptr,
                .primitive = sir::Primitive::BOOL,
            });
            break;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_unary_expr(sir::UnaryExpr &unary_expr, sir::Expr &out_expr) {
    // Deref unary operations are handled by `analyze_star_expr`.
    ASSUME(unary_expr.op != sir::UnaryOp::DEREF);

    ExprAnalyzer(analyzer).analyze(unary_expr.value);

    if (unary_expr.op == sir::UnaryOp::REF) {
        unary_expr.type = analyzer.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = unary_expr.value.get_type(),
        });

        return Result::SUCCESS;
    }

    if (auto struct_def = unary_expr.value.get_type().match_symbol<sir::StructDef>()) {
        std::string_view impl_name = MagicMethods::look_up(unary_expr.op);
        sir::Symbol symbol = struct_def->block.symbol_table->look_up(impl_name);

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(unary_expr);
            return Result::ERROR;
        }

        return analyze_operator_overload_call(symbol, unary_expr.value, nullptr, out_expr);
    }

    if (unary_expr.op == sir::UnaryOp::NEG) {
        unary_expr.type = unary_expr.value.get_type();
    } else if (unary_expr.op == sir::UnaryOp::NOT) {
        unary_expr.type = analyzer.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::BOOL,
        });
    } else {
        ASSERT_UNREACHABLE;
    }

    return Result::SUCCESS;
}

void ExprAnalyzer::analyze_cast_expr(sir::CastExpr &cast_expr) {
    ExprAnalyzer(analyzer).analyze(cast_expr.type);
    ExprAnalyzer(analyzer).analyze(cast_expr.value);
}

Result ExprAnalyzer::analyze_call_expr(sir::CallExpr &call_expr) {
    Result result;

    if (auto dot_expr = call_expr.callee.match<sir::DotExpr>()) {
        result = analyze_dot_expr_callee(*dot_expr, call_expr);
    } else {
        result = ExprAnalyzer(analyzer).analyze(call_expr.callee);
    }

    if (result != Result::SUCCESS) {
        return result;
    }

    for (unsigned i = 0; i < call_expr.args.size(); i++) {
        sir::Expr &arg = call_expr.args[i];

        ExprConstraints constraints;

        if (auto func_def = call_expr.callee.match_symbol<sir::FuncDef>()) {
            constraints.expected_type = func_def->type.params[i].type;
        } else if (auto native_func_decl = call_expr.callee.match_symbol<sir::NativeFuncDecl>()) {
            constraints.expected_type = native_func_decl->type.params[i].type;
        }

        ExprAnalyzer(analyzer, constraints).analyze(arg);
    }

    if (auto overload_set = call_expr.callee.match_symbol<sir::OverloadSet>()) {
        sir::FuncDef *func_def = resolve_overload(*overload_set, call_expr.args);

        call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = nullptr,
            .type = &func_def->type,
            .symbol = func_def,
        });
    }

    sir::Expr callee_type = call_expr.callee.get_type();

    if (auto func_type = callee_type.match<sir::FuncType>()) {
        call_expr.type = callee_type.as<sir::FuncType>().return_type;
    } else {
        analyzer.report_generator.report_cannot_call(call_expr.callee);
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_dot_expr_callee(sir::DotExpr &dot_expr, sir::CallExpr &out_call_expr) {
    Result result;

    result = ExprAnalyzer(analyzer).analyze(dot_expr.lhs);
    if (result != Result::SUCCESS) {
        return result;
    }

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
            result = analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);
            return result;
        }
    } else {
        result = analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);
        return result;
    }

    return Result::SUCCESS;
}

void ExprAnalyzer::analyze_static_array_type(sir::StaticArrayType &static_array_type) {
    ExprAnalyzer(analyzer).analyze(static_array_type.base_type);
    ExprAnalyzer(analyzer).analyze(static_array_type.length);
}

void ExprAnalyzer::analyze_func_type(sir::FuncType &func_type) {
    for (sir::Param &param : func_type.params) {
        ExprAnalyzer(analyzer).analyze(param.type);
    }

    ExprAnalyzer(analyzer).analyze(func_type.return_type);
}

Result ExprAnalyzer::analyze_dot_expr(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    Result result = ExprAnalyzer(analyzer).analyze(dot_expr.lhs);
    if (result == Result::ERROR) {
        return result;
    }

    result = analyze_dot_expr_rhs(dot_expr, out_expr);
    return result;
}

void ExprAnalyzer::analyze_range_expr(sir::RangeExpr &range_expr) {
    ExprAnalyzer(analyzer).analyze(range_expr.lhs);
    ExprAnalyzer(analyzer).analyze(range_expr.rhs);
}

void ExprAnalyzer::analyze_tuple_expr(sir::TupleExpr &tuple_expr) {
    assert(!tuple_expr.exprs.empty());

    for (sir::Expr &expr : tuple_expr.exprs) {
        ExprAnalyzer(analyzer).analyze(expr);
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

Result ExprAnalyzer::analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr) {
    ExprAnalyzer(analyzer).analyze(star_expr.value);

    if (star_expr.value.is_type()) {
        out_expr = analyzer.create_expr(sir::PointerType{
            .ast_node = star_expr.ast_node,
            .base_type = star_expr.value,
        });
    } else {
        if (auto struct_def = star_expr.value.get_type().match_symbol<sir::StructDef>()) {
            std::string_view impl_name = MagicMethods::look_up(sir::UnaryOp::DEREF);
            sir::Symbol symbol = struct_def->block.symbol_table->look_up(impl_name);

            if (!symbol) {
                analyzer.report_generator.report_err_operator_overload_not_found(star_expr);
                return Result::ERROR;
            }

            return analyze_operator_overload_call(symbol, star_expr.value, nullptr, out_expr);
        }

        out_expr = analyzer.create_expr(sir::UnaryExpr{
            .ast_node = star_expr.ast_node,
            .type = star_expr.value.get_type().as<sir::PointerType>().base_type,
            .op = sir::UnaryOp::DEREF,
            .value = star_expr.value,
        });
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr) {
    ExprAnalyzer(analyzer).analyze(bracket_expr.lhs);

    for (sir::Expr &expr : bracket_expr.rhs) {
        ExprAnalyzer(analyzer).analyze(expr);
    }

    if (auto func_def = bracket_expr.lhs.match_symbol<sir::FuncDef>()) {
        if (func_def->is_generic()) {
            sir::FuncDef *specialization = GenericsSpecializer(analyzer).specialize(*func_def, bracket_expr.rhs);

            out_expr = analyzer.create_expr(sir::SymbolExpr{
                .ast_node = bracket_expr.ast_node,
                .type = &specialization->type,
                .symbol = specialization,
            });

            return Result::SUCCESS;
        }
    } else if (auto struct_def = bracket_expr.lhs.match_symbol<sir::StructDef>()) {
        if (struct_def->is_generic()) {
            sir::StructDef *specialization = GenericsSpecializer(analyzer).specialize(*struct_def, bracket_expr.rhs);

            out_expr = analyzer.create_expr(sir::SymbolExpr{
                .ast_node = bracket_expr.ast_node,
                .type = nullptr,
                .symbol = specialization,
            });

            return Result::SUCCESS;
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
    } else if (auto struct_def = lhs_type.match_symbol<sir::StructDef>()) {
        // FIXME: error handling
        ASSUME(bracket_expr.rhs.size() == 1);

        sir::Symbol symbol = struct_def->block.symbol_table->look_up(MagicMethods::OP_INDEX);

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(bracket_expr);
            return Result::ERROR;
        }

        return analyze_operator_overload_call(symbol, bracket_expr.lhs, bracket_expr.rhs[0], out_expr);
    } else {
        // FIXME: error handling
        ASSERT_UNREACHABLE;
    }

    return Result::SUCCESS;
}

void ExprAnalyzer::create_std_string(sir::StringLiteral &string_literal, sir::Expr &out_expr) {
    sir::StructDef &struct_def = analyzer.find_std_string().as<sir::StructDef>();
    sir::FuncDef &func_def = struct_def.block.symbol_table->look_up("from_cstr").as<sir::FuncDef>();

    sir::Expr callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &func_def.type,
        .symbol = &func_def,
    });

    sir::Expr arg = analyzer.create_expr(sir::StringLiteral{
        .ast_node = string_literal.ast_node,
        .type = func_def.type.params[0].type,
        .value = std::move(string_literal.value),
    });

    out_expr = analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = func_def.type.return_type,
        .callee = callee,
        .args = {arg},
    });
}

Result ExprAnalyzer::analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    sir::DeclBlock *decl_block = dot_expr.lhs.get_decl_block();
    if (decl_block) {
        sir::Symbol symbol = decl_block->symbol_table->look_up(dot_expr.rhs.value);

        if (!symbol && !decl_block->symbol_table->complete) {
            MetaExpansion(analyzer).run_on_decl_block(*analyzer.get_scope().decl_block);
            symbol = decl_block->symbol_table->look_up(dot_expr.rhs.value);
        }

        if (!symbol) {
            analyzer.report_generator.report_err_symbol_not_found(dot_expr.rhs);
            return Result::ERROR;
        }

        out_expr = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = dot_expr.ast_node,
            .type = symbol.get_type(),
            .symbol = symbol,
        });

        return Result::SUCCESS;
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
        std::cout << "maybe broken?" << std::endl;
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

sir::FuncDef *ExprAnalyzer::resolve_overload(sir::OverloadSet &overload_set, const std::vector<sir::Expr> &args) {
    for (sir::FuncDef *func_def : overload_set.func_defs) {
        if (is_matching_overload(*func_def, args)) {
            return func_def;
        }
    }

    ASSERT_UNREACHABLE;
}

bool ExprAnalyzer::is_matching_overload(sir::FuncDef &func_def, const std::vector<sir::Expr> &args) {
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

Result ExprAnalyzer::analyze_operator_overload_call(
    sir::Symbol symbol,
    sir::Expr self,
    sir::Expr arg,
    sir::Expr &out_expr
) {
    sir::Expr self_ref = analyzer.create_expr(sir::UnaryExpr{
        .ast_node = nullptr,
        .type = analyzer.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = self.get_type(),
        }),
        .op = sir::UnaryOp::REF,
        .value = self,
    });

    std::vector<sir::Expr> args;
    if (arg) {
        args = {self_ref, arg};
    } else {
        args = {self_ref};
    }

    sir::FuncDef *impl;

    if (auto func_def = symbol.match<sir::FuncDef>()) {
        impl = func_def;
    } else if (auto overload_set = symbol.match<sir::OverloadSet>()) {
        impl = resolve_overload(*overload_set, args);
    } else {
        ASSERT_UNREACHABLE;
    }

    sir::Expr callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &impl->type,
        .symbol = impl,
    });

    out_expr = analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = impl->type.return_type,
        .callee = callee,
        .args = args,
    });

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
