#include "expr_analyzer.hpp"

#include "banjo/sema/const_evaluator.hpp"
#include "banjo/sema/decl_body_analyzer.hpp"
#include "banjo/sema/decl_interface_analyzer.hpp"
#include "banjo/sema/expr_finalizer.hpp"
#include "banjo/sema/generic_arg_inference.hpp"
#include "banjo/sema/generics_specializer.hpp"
#include "banjo/sema/meta_expansion.hpp"
#include "banjo/sema/meta_expr_evaluator.hpp"
#include "banjo/sema/resource_analyzer.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/use_resolver.hpp"
#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

ExprConstraints ExprConstraints::expect_type(sir::Expr type) {
    return {
        .expected_type = type,
    };
}

ExprAnalyzer::ExprAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

Result ExprAnalyzer::analyze(sir::Expr &expr, ExprConstraints constraints /*= {}*/) {
    Result result;

    result = analyze_uncoerced(expr);
    if (result != Result::SUCCESS) {
        return result;
    }

    if (constraints.expected_type) {
        return ExprFinalizer(analyzer).finalize_by_coercion(expr, constraints.expected_type);
    } else {
        return ExprFinalizer(analyzer).finalize(expr);
    }
}

Result ExprAnalyzer::analyze_uncoerced(sir::Expr &expr) {
    Result result = Result::SUCCESS;

    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IMPOSSIBLE,                                        // empty
        result = analyze_int_literal(*inner),                        // int_literal
        result = analyze_fp_literal(*inner),                         // fp_literal
        result = analyze_bool_literal(*inner),                       // bool_literal
        result = analyze_char_literal(*inner),                       // char_literal
        result = Result::SUCCESS,                                    // null_literal
        result = Result::SUCCESS,                                    // none_literal
        result = Result::SUCCESS,                                    // undefined_literal
        result = analyze_array_literal(*inner, expr),                // array_literal
        result = analyze_string_literal(*inner),                     // string_literal
        result = analyze_struct_literal(*inner),                     // struct_literal
        SIR_VISIT_IGNORE,                                            // union_case_literal
        result = analyze_map_literal(*inner, expr),                  // map_literal
        result = analyze_closure_literal(*inner, expr),              // closure_literal
        SIR_VISIT_IGNORE,                                            // symbol_expr
        result = analyze_binary_expr(*inner, expr),                  // binary_expr
        result = analyze_unary_expr(*inner, expr),                   // unary_expr
        result = analyze_cast_expr(*inner),                          // cast_expr
        SIR_VISIT_IGNORE,                                            // index_expr
        result = analyze_call_expr(*inner, expr),                    // call_expr
        SIR_VISIT_IGNORE,                                            // field_expr
        analyze_range_expr(*inner),                                  // range_expr
        analyze_tuple_expr(*inner),                                  // tuple_expr
        SIR_VISIT_IGNORE,                                            // coercion_expr
        SIR_VISIT_IGNORE,                                            // primitive_type
        SIR_VISIT_IGNORE,                                            // pointer_type
        result = analyze_static_array_type(*inner),                  // static_array_type
        analyze_func_type(*inner),                                   // func_type
        result = analyze_optional_type(*inner, expr),                // optional_type
        result = analyze_result_type(*inner, expr),                  // result_type
        result = analyze_array_type(*inner, expr),                   // array_type
        result = analyze_map_type(*inner, expr),                     // map_type
        result = analyze_closure_type(*inner),                       // closure_type
        result = analyze_ident_expr(*inner, expr),                   // ident_expr
        analyze_star_expr(*inner, expr),                             // star_expr
        result = analyze_bracket_expr(*inner, expr),                 // bracket_expr
        result = analyze_dot_expr(*inner, expr),                     // dot_expr
        SIR_VISIT_IMPOSSIBLE,                                        // pseudo_tpe
        result = Result::SUCCESS,                                    // meta_access
        result = MetaExprEvaluator(analyzer).evaluate(*inner, expr), // meta_field_expr
        result = MetaExprEvaluator(analyzer).evaluate(*inner, expr), // meta_call_expr
        SIR_VISIT_IMPOSSIBLE,                                        // init_expr
        SIR_VISIT_IMPOSSIBLE,                                        // move_expr
        SIR_VISIT_IMPOSSIBLE,                                        // deinit_expr
        result = Result::ERROR,                                      // error
        result = analyze_completion_token(*inner, expr)              // completion_token
    );

    if (result != Result::SUCCESS) {
        return result;
    }

    while (auto type_alias = expr.match_symbol<sir::TypeAlias>()) {
        expr = type_alias->type;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_int_literal(sir::IntLiteral &int_literal) {
    int_literal.type = analyzer.create_expr(sir::PseudoType{
        .ast_node = nullptr,
        .kind = sir::PseudoTypeKind::INT_LITERAL,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_fp_literal(sir::FPLiteral &fp_literal) {
    fp_literal.type = analyzer.create_expr(sir::PseudoType{
        .ast_node = nullptr,
        .kind = sir::PseudoTypeKind::FP_LITERAL,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_bool_literal(sir::BoolLiteral &bool_literal) {
    bool_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::BOOL,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_char_literal(sir::CharLiteral &char_literal) {
    char_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::U8,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_array_literal(sir::ArrayLiteral &array_literal, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::Expr &value : array_literal.values) {
        partial_result = ExprAnalyzer(analyzer).analyze_uncoerced(value);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (array_literal.values.size() == 1 && array_literal.values[0].is_type()) {
        out_expr = analyzer.create_expr(sir::ArrayType{
            .ast_node = array_literal.ast_node,
            .base_type = array_literal.values[0],
        });

        return analyze_array_type(out_expr.as<sir::ArrayType>(), out_expr);
    } else {
        array_literal.type = analyzer.create_expr(sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::ARRAY_LITERAL,
        });
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_string_literal(sir::StringLiteral &string_literal) {
    string_literal.type = analyzer.create_expr(sir::PseudoType{
        .ast_node = nullptr,
        .kind = sir::PseudoTypeKind::STRING_LITERAL,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_struct_literal(sir::StructLiteral &struct_literal) {
    Result result;

    if (struct_literal.type) {
        result = ExprAnalyzer(analyzer).analyze(struct_literal.type);
        if (result != Result::SUCCESS) {
            return result;
        }
    }

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        result = ExprAnalyzer(analyzer).analyze_uncoerced(entry.value);
        if (result != Result::SUCCESS) {
            return result;
        }
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_map_literal(sir::MapLiteral &map_literal, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::MapLiteralEntry &entry : map_literal.entries) {
        partial_result = ExprAnalyzer(analyzer).analyze_uncoerced(entry.key);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }

        partial_result = ExprAnalyzer(analyzer).analyze_uncoerced(entry.value);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (map_literal.entries.size() == 1 && map_literal.entries[0].key.is_type()) {
        out_expr = analyzer.create_expr(sir::MapType{
            .ast_node = map_literal.ast_node,
            .key_type = map_literal.entries[0].key,
            .value_type = map_literal.entries[0].value,
        });

        return analyze_map_type(out_expr.as<sir::MapType>(), out_expr);
    } else {
        map_literal.type = analyzer.create_expr(sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::MAP_LITERAL,
        });
    }

    return result;
}

Result ExprAnalyzer::analyze_closure_literal(sir::ClosureLiteral &closure_literal, sir::Expr &out_expr) {
    sir::TupleExpr *data_type = analyzer.create_expr(sir::TupleExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .exprs = {},
    });

    sir::Param data_ptr_param{
        .ast_node = nullptr,
        .name = {},
        .type = analyzer.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::ADDR,
        }),
    };

    sir::FuncType generated_func_type = closure_literal.func_type;
    generated_func_type.params.insert(generated_func_type.params.begin(), data_ptr_param);

    sir::FuncDef *generated_func = analyzer.create_decl(sir::FuncDef{
        .ast_node = nullptr,
        .ident{
            .ast_node = nullptr,
            .value = "",
        },
        .type = generated_func_type,
        .block = closure_literal.block,
        .attrs = nullptr,
        .generic_params = {},
        .specializations = {},
        .parent_specialization = nullptr,
    });

    generated_func->block.symbol_table->parent = analyzer.get_scope().decl.get_symbol_table();

    ClosureContext closure_ctx{
        .captured_vars = {},
        .data_type = data_type,
        .parent_block = analyzer.get_scope().block,
    };

    analyzer.push_scope().closure_ctx = &closure_ctx;

    DeclInterfaceAnalyzer(analyzer).analyze_func_def(*generated_func);
    DeclBodyAnalyzer(analyzer).analyze_func_def(*generated_func);
    ResourceAnalyzer(analyzer).analyze_func_def(*generated_func);

    analyzer.pop_scope();

    analyzer.cur_sir_mod->block.decls.push_back(generated_func);

    data_type->exprs.resize(closure_ctx.captured_vars.size());
    std::vector<sir::Expr> capture_values(closure_ctx.captured_vars.size());

    for (unsigned i = 0; i < closure_ctx.captured_vars.size(); i++) {
        sir::Symbol &captured_var = closure_ctx.captured_vars[i];

        sir::Expr type = captured_var.get_type();
        sir::Expr value = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = nullptr,
            .type = type,
            .symbol = captured_var,
        });

        data_type->exprs[i] = type;
        capture_values[i] = value;
    }

    sir::TupleExpr *data = analyzer.create_expr(sir::TupleExpr{
        .ast_node = nullptr,
        .type = data_type,
        .exprs = capture_values,
    });

    sir::StructDef &std_closure_def = analyzer.find_std_closure().as<sir::StructDef>();

    sir::FuncDef &new_def_generic = std_closure_def.block.symbol_table->look_up("new").as<sir::FuncDef>();
    sir::FuncDef &new_def = *GenericsSpecializer(analyzer).specialize(new_def_generic, {data_type});

    sir::Expr callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &new_def.type,
        .symbol = &new_def,
    });

    sir::Expr addr_type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::ADDR,
    });

    sir::Expr func_ptr = analyzer.create_expr(sir::CastExpr{
        .ast_node = nullptr,
        .type = addr_type,
        .value = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = nullptr,
            .type = &generated_func->type,
            .symbol = generated_func,
        }),
    });

    out_expr = analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = analyzer.create_expr(sir::ClosureType{
            .ast_node = nullptr,
            .func_type = closure_literal.func_type,
            .underlying_struct = &std_closure_def,
        }),
        .callee = callee,
        .args = {func_ptr, data},
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_binary_expr(sir::BinaryExpr &binary_expr, sir::Expr &out_expr) {
    Result lhs_result;
    Result rhs_result;

    lhs_result = analyze_uncoerced(binary_expr.lhs);
    rhs_result = analyze_uncoerced(binary_expr.rhs);

    if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    bool is_operator_overload = false;

    if (auto pseudo_type = binary_expr.lhs.get_type().match<sir::PseudoType>()) {
        if (pseudo_type->is_struct_by_default()) {
            lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);
            rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);

            if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
                return Result::ERROR;
            }

            is_operator_overload = true;
        }
    } else if (binary_expr.lhs.get_type().is_symbol<sir::StructDef>()) {
        lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);
        rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);

        if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        is_operator_overload = true;
    }

    if (is_operator_overload) {
        sir::StructDef &struct_def = binary_expr.lhs.get_type().as_symbol<sir::StructDef>();
        std::string_view impl_name = sir::MagicMethods::look_up(binary_expr.op);
        sir::Symbol symbol = struct_def.block.symbol_table->look_up(impl_name);

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(binary_expr);
            return Result::ERROR;
        }

        return analyze_operator_overload_call(symbol, binary_expr.lhs, binary_expr.rhs, out_expr);
    }

    sir::Expr lhs_type = binary_expr.lhs.get_type();
    sir::Expr rhs_type = binary_expr.rhs.get_type();

    if (lhs_type.is<sir::PseudoType>() && !rhs_type.is<sir::PseudoType>()) {
        rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);

        if (rhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        lhs_result = ExprFinalizer(analyzer).finalize_by_coercion(binary_expr.lhs, binary_expr.rhs.get_type());
    } else if (rhs_type.is<sir::PseudoType>() && !lhs_type.is<sir::PseudoType>()) {
        lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);

        if (lhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        rhs_result = ExprFinalizer(analyzer).finalize_by_coercion(binary_expr.rhs, binary_expr.lhs.get_type());
    } else {
        lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);
        rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);
    }

    if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (binary_expr.lhs.is_type() ||
        (binary_expr.lhs.is<sir::BoolLiteral>() && binary_expr.rhs.is<sir::BoolLiteral>())) {
        out_expr = ConstEvaluator(analyzer, false).evaluate_binary_expr(binary_expr);
        return analyze_uncoerced(out_expr);
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
    Result partial_result;

    // Deref unary operations are handled by `analyze_star_expr`.
    ASSUME(unary_expr.op != sir::UnaryOp::DEREF);

    partial_result = analyze_uncoerced(unary_expr.value);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (unary_expr.op == sir::UnaryOp::REF) {
        ExprFinalizer(analyzer).finalize(unary_expr.value);

        unary_expr.type = analyzer.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = unary_expr.value.get_type(),
        });

        return Result::SUCCESS;
    }

    if (auto struct_def = unary_expr.value.get_type().match_symbol<sir::StructDef>()) {
        ExprFinalizer(analyzer).finalize(unary_expr.value);

        std::string_view impl_name = sir::MagicMethods::look_up(unary_expr.op);
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
        ExprFinalizer(analyzer).finalize(unary_expr.value);

        unary_expr.type = analyzer.create_expr(sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::BOOL,
        });
    } else {
        ASSERT_UNREACHABLE;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_cast_expr(sir::CastExpr &cast_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(cast_expr.type);
    if (partial_result != Result::SUCCESS) {
        result = partial_result;
    }

    partial_result = ExprAnalyzer(analyzer).analyze(cast_expr.value);
    if (partial_result != Result::SUCCESS) {
        result = partial_result;
    }

    if (result != Result::SUCCESS) {
        return result;
    }

    sir::Expr from = cast_expr.value.get_type();
    sir::Expr to = cast_expr.type;
    bool is_cast_possible;

    if (from.is_int_type()) {
        is_cast_possible =
            to.is_int_type() || to.is_fp_type() || to.is_addr_like_type() || to.is_symbol<sir::EnumDef>();
    } else if (from.is_fp_type()) {
        is_cast_possible = to.is_int_type() || to.is_fp_type();
    } else if (from.is_addr_like_type()) {
        is_cast_possible = to.is_int_type() || to.is_addr_like_type();
    } else if (from.is_symbol<sir::EnumDef>()) {
        is_cast_possible = from == to || to.is_int_type();
    } else {
        is_cast_possible = false;
    }

    if (!is_cast_possible) {
        analyzer.report_generator.report_err_cannot_cast(cast_expr);
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_call_expr(sir::CallExpr &call_expr, sir::Expr &out_expr) {
    Result partial_result;
    Result result = Result::SUCCESS;
    bool is_method = false;

    // TODO: move this into some kind of builtin module.
    if (auto ident_expr = call_expr.callee.match<sir::IdentExpr>()) {
        if (ident_expr->value == "__builtin_deinit") {
            analyze(call_expr.args[0]);

            out_expr = analyzer.create_expr(sir::DeinitExpr{
                .ast_node = nullptr,
                .type = call_expr.args[0].get_type(),
                .value = call_expr.args[0],
                .resource = nullptr,
            });

            return Result::SUCCESS;
        }
    }

    if (auto dot_expr = call_expr.callee.match<sir::DotExpr>()) {
        partial_result = analyze_dot_expr_callee(*dot_expr, call_expr, is_method);
    } else {
        partial_result = ExprAnalyzer(analyzer).analyze(call_expr.callee);
    }

    if (partial_result != Result::SUCCESS) {
        return partial_result;
    }

    if (call_expr.callee.is_symbol<sir::UnionCase>()) {
        return analyze_union_case_literal(call_expr, out_expr);
    }

    unsigned first_arg_to_analyze = is_method ? 1 : 0;

    for (unsigned i = first_arg_to_analyze; i < call_expr.args.size(); i++) {
        sir::Expr &arg = call_expr.args[i];

        ExprConstraints constraints;

        if (auto func_def = call_expr.callee.match_symbol<sir::FuncDef>()) {
            if (!func_def->is_generic() && call_expr.args.size() == func_def->type.params.size()) {
                constraints = ExprConstraints::expect_type(func_def->type.params[i].type);
            }
        } else if (auto native_func_decl = call_expr.callee.match_symbol<sir::NativeFuncDecl>()) {
            if (call_expr.args.size() == native_func_decl->type.params.size()) {
                constraints = ExprConstraints::expect_type(native_func_decl->type.params[i].type);
            }
        }

        partial_result = analyze(arg, constraints);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::FuncDef *callee_func_def = nullptr;

    if (auto func_def = call_expr.callee.match_symbol<sir::FuncDef>()) {
        if (func_def->is_generic()) {
            std::vector<sir::Expr> generic_args;

            partial_result = GenericArgInference(analyzer, &call_expr, *func_def).infer(call_expr.args, generic_args);
            if (partial_result != Result::SUCCESS) {
                return Result::ERROR;
            }

            partial_result = specialize(*func_def, generic_args, call_expr.callee);
            if (partial_result != Result::SUCCESS) {
                return partial_result;
            }

            if (func_def->generic_params.back().kind == sir::GenericParamKind::SEQUENCE) {
                unsigned num_non_sequence_args = func_def->type.params.size() - 1;
                unsigned num_sequence_args = call_expr.args.size() - num_non_sequence_args;

                std::vector<sir::Expr> sequence_args(num_sequence_args);
                for (unsigned i = 0; i < num_sequence_args; i++) {
                    sequence_args[i] = call_expr.args[num_non_sequence_args + i];
                }

                call_expr.args.resize(num_non_sequence_args + 1);

                call_expr.args[num_non_sequence_args] = analyzer.create_expr(sir::TupleExpr{
                    .ast_node = nullptr,
                    .type = generic_args.back(),
                    .exprs = sequence_args,
                });
            }
        }

        callee_func_def = &call_expr.callee.as_symbol<sir::FuncDef>();
    } else if (auto overload_set = call_expr.callee.match_symbol<sir::OverloadSet>()) {
        callee_func_def = resolve_overload(*overload_set, call_expr.args);

        if (!callee_func_def) {
            analyzer.report_generator.report_err_no_matching_overload(call_expr.callee, *overload_set);
            return Result::ERROR;
        }

        call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
            .ast_node = nullptr,
            .type = &callee_func_def->type,
            .symbol = callee_func_def,
        });
    }

    sir::Expr callee_type = call_expr.callee.get_type();
    sir::FuncType *callee_func_type;

    if (auto func_type = callee_type.match<sir::FuncType>()) {
        callee_func_type = func_type;
    } else if (auto closure_type = callee_type.match<sir::ClosureType>()) {
        callee_func_type = &closure_type->func_type;
    } else {
        analyzer.report_generator.report_err_cannot_call(call_expr.callee);
        return Result::ERROR;
    }

    if (call_expr.args.size() != callee_func_type->params.size()) {
        analyzer.report_generator
            .report_err_unexpected_arg_count(call_expr, callee_func_type->params.size(), callee_func_def);
        return Result::ERROR;
    }

    for (unsigned i = 0; i < call_expr.args.size(); i++) {
        ExprFinalizer(analyzer).finalize_by_coercion(call_expr.args[i], callee_func_type->params[i].type);
    }

    call_expr.type = callee_func_type->return_type;

    if (auto closure_type = callee_type.match<sir::ClosureType>()) {
        sir::Expr func_ptr = analyzer.create_expr(sir::FieldExpr{
            .ast_node = nullptr,
            .type = closure_type->underlying_struct->fields[0]->type,
            .base = call_expr.callee,
            .field_index = 0,
        });

        sir::Expr data_ptr = analyzer.create_expr(sir::FieldExpr{
            .ast_node = nullptr,
            .type = closure_type->underlying_struct->fields[1]->type,
            .base = call_expr.callee,
            .field_index = 1,
        });

        call_expr.callee = analyzer.create_expr(sir::CastExpr{
            .ast_node = nullptr,
            .type = &closure_type->func_type,
            .value = func_ptr,
        });

        call_expr.args.insert(call_expr.args.begin(), data_ptr);
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_dot_expr_callee(sir::DotExpr &dot_expr, sir::CallExpr &out_call_expr, bool &is_method) {
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(dot_expr.lhs);
    if (partial_result != Result::SUCCESS) {
        return partial_result;
    }

    sir::Expr lhs = dot_expr.lhs;
    sir::Expr lhs_type = dot_expr.lhs.get_type();

    while (auto pointer_type = lhs_type.match<sir::PointerType>()) {
        if (pointer_type->base_type.match_symbol<sir::ProtoDef>()) {
            break;
        }

        lhs = analyzer.create_expr(sir::UnaryExpr{
            .ast_node = nullptr,
            .type = pointer_type->base_type,
            .op = sir::UnaryOp::DEREF,
            .value = lhs,
        });

        lhs_type = pointer_type->base_type;
    }

    if (auto struct_def = lhs_type.match_symbol<sir::StructDef>()) {
        sir::Symbol method = struct_def->block.symbol_table->look_up(dot_expr.rhs.value);

        if (method) {
            create_method_call(out_call_expr, lhs, dot_expr.rhs, method, false);
            is_method = true;
            return Result::SUCCESS;
        }

        sir::StructField *field = struct_def->find_field(dot_expr.rhs.value);

        if (field) {
            analyzer.add_symbol_use(dot_expr.rhs.ast_node, field);

            out_call_expr.callee = analyzer.create_expr(sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field->type,
                .base = lhs,
                .field_index = field->index,
            });

            return Result::SUCCESS;
        }

        analyzer.report_generator.report_err_no_method(dot_expr.rhs, *struct_def);
        return Result::ERROR;
    } else if (auto union_def = lhs_type.match_symbol<sir::UnionDef>()) {
        sir::Symbol method = union_def->block.symbol_table->look_up(dot_expr.rhs.value);

        if (method) {
            create_method_call(out_call_expr, lhs, dot_expr.rhs, method, false);
            is_method = true;
            return Result::SUCCESS;
        }

        analyzer.report_generator.report_err_no_method(dot_expr.rhs, *struct_def);
        return Result::ERROR;
    } else if (auto proto_def = lhs_type.match_proto_ptr()) {
        sir::Symbol method = proto_def->block.symbol_table->look_up(dot_expr.rhs.value);

        if (method) {
            create_method_call(out_call_expr, lhs, dot_expr.rhs, method, true);
            is_method = true;
            return Result::SUCCESS;
        }

        // analyzer.report_generator.report_err_no_method(dot_expr.rhs, *struct_def);
        return Result::ERROR;
    } else {
        partial_result = analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);

        is_method = false;
        return partial_result;
    }
}

Result ExprAnalyzer::analyze_union_case_literal(sir::CallExpr &call_expr, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    sir::UnionCase &union_case = call_expr.callee.as_symbol<sir::UnionCase>();

    for (unsigned i = 0; i < call_expr.args.size(); i++) {
        sir::Expr expected_type = union_case.fields[i].type;
        partial_result = analyze(call_expr.args[i], ExprConstraints::expect_type(expected_type));
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    out_expr = analyzer.create_expr(sir::UnionCaseLiteral{
        .ast_node = nullptr,
        .type = call_expr.callee,
        .args = std::move(call_expr.args),
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_static_array_type(sir::StaticArrayType &static_array_type) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(static_array_type.base_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    partial_result = ExprAnalyzer(analyzer).analyze(static_array_type.length);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    static_array_type.length = ConstEvaluator(analyzer).evaluate(static_array_type.length);

    return result;
}

void ExprAnalyzer::analyze_func_type(sir::FuncType &func_type) {
    for (sir::Param &param : func_type.params) {
        ExprAnalyzer(analyzer).analyze(param.type);
    }

    ExprAnalyzer(analyzer).analyze(func_type.return_type);
}

Result ExprAnalyzer::analyze_optional_type(sir::OptionalType &optional_type, sir::Expr &out_expr) {
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(optional_type.base_type);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::StructDef &struct_def = analyzer.find_std_optional().as<sir::StructDef>();
    specialize(struct_def, {optional_type.base_type}, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_result_type(sir::ResultType &result_type, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(result_type.value_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    partial_result = ExprAnalyzer(analyzer).analyze(result_type.error_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::StructDef &struct_def = analyzer.find_std_result().as<sir::StructDef>();
    specialize(struct_def, {result_type.value_type, result_type.error_type}, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_array_type(sir::ArrayType &array_type, sir::Expr &out_expr) {
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(array_type.base_type);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::StructDef &struct_def = analyzer.find_std_array().as<sir::StructDef>();
    specialize(struct_def, {array_type.base_type}, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_map_type(sir::MapType &map_type, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = ExprAnalyzer(analyzer).analyze(map_type.key_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    partial_result = ExprAnalyzer(analyzer).analyze(map_type.value_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::StructDef &struct_def = analyzer.find_std_map().as<sir::StructDef>();
    specialize(struct_def, {map_type.key_type, map_type.value_type}, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_closure_type(sir::ClosureType &closure_type) {
    sir::StructDef &std_closure_def = analyzer.find_std_closure().as<sir::StructDef>();
    closure_type.underlying_struct = &std_closure_def;
    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_dot_expr(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    Result result = ExprAnalyzer(analyzer).analyze(dot_expr.lhs);
    if (result != Result::SUCCESS) {
        return result;
    }

    if (analyzer.mode == Mode::COMPLETION && dot_expr.rhs.value == "[completion]") {
        analyzer.completion_context = CompleteAfterDot{
            .lhs = dot_expr.lhs,
        };

        return Result::SUCCESS;
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
        ExprAnalyzer(analyzer).analyze_uncoerced(expr);
    }

    if (tuple_expr.exprs[0].is_type()) {
        return;
    }

    std::vector<sir::Expr> types(tuple_expr.exprs.size());

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
    ClosureContext *closure_ctx = analyzer.get_scope().closure_ctx;

    if (!generic_args.empty()) {
        auto iter = generic_args.find(ident_expr.value);
        if (iter != generic_args.end()) {
            if (iter->second.is<sir::StringLiteral>()) {
                out_expr = sir::Cloner(*analyzer.cur_sir_mod).clone_expr(iter->second);
            } else {
                out_expr = iter->second;
            }

            return Result::SUCCESS;
        }
    }

    if (analyzer.in_meta_expansion) {
        sir::DeclBlock &decl_block = *analyzer.get_scope().decl.get_decl_block();
        UseResolver(analyzer).resolve_in_block(decl_block);
        MetaExpansion(analyzer).run_on_decl_block(decl_block);
    }

    sir::Symbol symbol = symbol_table.look_up(ident_expr.value);

    if (!symbol && closure_ctx) {
        symbol = closure_ctx->parent_block->symbol_table->look_up(ident_expr.value);

        if (symbol) {
            unsigned captured_var_index = 0;
            bool captured_var_exists = false;

            for (unsigned i = 0; i < closure_ctx->captured_vars.size(); i++) {
                if (closure_ctx->captured_vars[i] == symbol) {
                    captured_var_index = i;
                    captured_var_exists = true;
                    break;
                }
            }

            if (!captured_var_exists) {
                captured_var_index = closure_ctx->captured_vars.size();
                closure_ctx->captured_vars.push_back(symbol);
            }

            sir::FuncDef &func_def = analyzer.get_scope().decl.as<sir::FuncDef>();
            sir::Symbol data_ptr_param = &func_def.type.params[0];

            out_expr = analyzer.create_expr(sir::FieldExpr{
                .ast_node = nullptr,
                .type = symbol.get_type(),
                .base = analyzer.create_expr(sir::UnaryExpr{
                    .ast_node = nullptr,
                    .type = closure_ctx->data_type,
                    .op = sir::UnaryOp::DEREF,
                    .value = analyzer.create_expr(sir::CastExpr{
                        .ast_node = nullptr,
                        .type = analyzer.create_expr(sir::PointerType{
                            .ast_node = nullptr,
                            .base_type = closure_ctx->data_type,
                        }),
                        .value = analyzer.create_expr(sir::SymbolExpr{
                            .ast_node = nullptr,
                            .type = data_ptr_param.get_type(),
                            .symbol = data_ptr_param,
                        })
                    }),
                }),
                .field_index = captured_var_index,
            });

            return Result::SUCCESS;
        }
    }

    if (!symbol) {
        auto iter = analyzer.preamble_symbols.find(ident_expr.value);
        symbol = iter == analyzer.preamble_symbols.end() ? nullptr : iter->second;
    }

    if (!symbol) {
        analyzer.report_generator.report_err_symbol_not_found(ident_expr);
        return Result::ERROR;
    }

    analyzer.add_symbol_use(ident_expr.ast_node, symbol);

    out_expr = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = ident_expr.ast_node,
        .type = symbol.get_type(),
        .symbol = symbol,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr) {
    Result result;

    result = ExprAnalyzer(analyzer).analyze(star_expr.value);
    if (result != Result::SUCCESS) {
        return result;
    }

    if (star_expr.value.is_type()) {
        out_expr = analyzer.create_expr(sir::PointerType{
            .ast_node = star_expr.ast_node,
            .base_type = star_expr.value,
        });
    } else {
        sir::Expr value_type = star_expr.value.get_type();

        if (auto struct_def = value_type.match_symbol<sir::StructDef>()) {
            std::string_view impl_name = sir::MagicMethods::look_up(sir::UnaryOp::DEREF);
            sir::Symbol symbol = struct_def->block.symbol_table->look_up(impl_name);

            if (!symbol) {
                analyzer.report_generator.report_err_operator_overload_not_found(star_expr);
                return Result::ERROR;
            }

            return analyze_operator_overload_call(symbol, star_expr.value, nullptr, out_expr);
        }

        if (auto pointer_type = value_type.match<sir::PointerType>()) {
            out_expr = analyzer.create_expr(sir::UnaryExpr{
                .ast_node = star_expr.ast_node,
                .type = pointer_type->base_type,
                .op = sir::UnaryOp::DEREF,
                .value = star_expr.value,
            });
        } else {
            analyzer.report_generator.report_err_cannot_deref(star_expr.value);
            return Result::ERROR;
        }
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr) {
    Result result;

    result = ExprAnalyzer(analyzer).analyze(bracket_expr.lhs);
    if (result != Result::SUCCESS) {
        return result;
    }

    for (sir::Expr &expr : bracket_expr.rhs) {
        ExprAnalyzer(analyzer).analyze(expr);
    }

    if (auto func_def = bracket_expr.lhs.match_symbol<sir::FuncDef>()) {
        if (func_def->is_generic()) {
            if (bracket_expr.rhs.size() != func_def->generic_params.size()) {
                analyzer.report_generator.report_err_unexpected_generic_arg_count(bracket_expr, *func_def);
                return Result::ERROR;
            }

            return specialize(*func_def, bracket_expr.rhs, out_expr);
        }
    } else if (auto struct_def = bracket_expr.lhs.match_symbol<sir::StructDef>()) {
        if (struct_def->is_generic()) {
            if (bracket_expr.rhs.size() != struct_def->generic_params.size()) {
                analyzer.report_generator.report_err_unexpected_generic_arg_count(bracket_expr, *struct_def);
                return Result::ERROR;
            }

            return specialize(*struct_def, bracket_expr.rhs, out_expr);
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
        ASSERT(bracket_expr.rhs.size() == 1);

        sir::Symbol symbol = struct_def->block.symbol_table->look_up(sir::MagicMethods::OP_INDEX);

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(bracket_expr);
            return Result::ERROR;
        }

        result = analyze_operator_overload_call(symbol, bracket_expr.lhs, bracket_expr.rhs[0], out_expr);

        if (result == Result::SUCCESS) {
            out_expr = analyzer.create_expr(sir::UnaryExpr{
                .ast_node = nullptr,
                .type = out_expr.get_type().as<sir::PointerType>().base_type,
                .op = sir::UnaryOp::DEREF,
                .value = out_expr,
            });
        }

        return result;
    } else {
        analyzer.report_generator.report_err_expected_generic_or_indexable(bracket_expr.lhs);
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_completion_token(sir::CompletionToken &completion_token, sir::Expr &out_expr) {
    if (analyzer.is_in_stmt_block()) {
        analyzer.completion_context = CompleteInBlock{
            .block = analyzer.get_scope().block,
        };
    } else if (analyzer.is_in_decl()) {
        analyzer.completion_context = CompleteInDeclBlock{
            .decl_block = analyzer.get_scope().decl.get_decl_block(),
        };
    } else {
        analyzer.completion_context = CompleteInDeclBlock{
            .decl_block = &analyzer.cur_sir_mod->block,
        };
    }

    out_expr = analyzer.create_expr(sir::IdentExpr{
        .ast_node = nullptr,
        .value = "[completion]",
    });

    // Return an error so as not to continue analysing expressions that contain
    // a completion token and cannot be valid anyway.
    return Result::ERROR;
}

void ExprAnalyzer::create_method_call(
    sir::CallExpr &call_expr,
    sir::Expr lhs,
    sir::Ident rhs,
    sir::Symbol method,
    bool lhs_is_already_pointer
) {
    analyzer.add_symbol_use(rhs.ast_node, method);

    sir::Expr callee_type = method.get_type();

    call_expr.callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = rhs.ast_node,
        .type = callee_type,
        .symbol = method,
    });

    if (lhs_is_already_pointer) {
        call_expr.args.insert(call_expr.args.begin(), lhs);
    } else {
        if (auto func_type = callee_type.match<sir::FuncType>()) {
            sir::Param &self_param = func_type->params[0];

            if (self_param.attrs && self_param.attrs->byval) {
                call_expr.args.insert(call_expr.args.begin(), lhs);
                return;
            }
        }

        sir::Expr self_arg = analyzer.create_expr(sir::UnaryExpr{
            .ast_node = nullptr,
            .type = analyzer.create_expr(sir::PointerType{
                .ast_node = nullptr,
                .base_type = lhs.get_type(),
            }),
            .op = sir::UnaryOp::REF,
            .value = lhs,
        });

        call_expr.args.insert(call_expr.args.begin(), self_arg);
    }
}

Result ExprAnalyzer::analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    sir::DeclBlock *decl_block = dot_expr.lhs.get_decl_block();
    if (decl_block) {
        sir::Symbol symbol = decl_block->symbol_table->look_up(dot_expr.rhs.value);

        if (!symbol) {
            auto iter = analyzer.incomplete_decl_blocks.find(decl_block);

            if (iter != analyzer.incomplete_decl_blocks.end()) {
                analyzer.push_scope() = iter->second;
                MetaExpansion(analyzer).run_on_decl_block(*decl_block);
                analyzer.pop_scope();

                symbol = decl_block->symbol_table->look_up(dot_expr.rhs.value);
            }
        }

        if (symbol) {
            analyzer.add_symbol_use(dot_expr.rhs.ast_node, symbol);

            out_expr = analyzer.create_expr(sir::SymbolExpr{
                .ast_node = dot_expr.ast_node,
                .type = symbol.get_type(),
                .symbol = symbol,
            });

            return Result::SUCCESS;
        }

        if (auto mod = dot_expr.lhs.match_symbol<sir::Module>()) {
            ModulePath sub_mod_path = mod->path;
            sub_mod_path.append(dot_expr.rhs.value);
            sir::Module *sub_mod = analyzer.sir_unit.mods_by_path[sub_mod_path];

            if (sub_mod) {
                analyzer.add_symbol_use(dot_expr.rhs.ast_node, sub_mod);

                out_expr = analyzer.create_expr(sir::SymbolExpr{
                    .ast_node = dot_expr.ast_node,
                    .type = nullptr,
                    .symbol = sub_mod,
                });

                return Result::SUCCESS;
            }
        }

        analyzer.report_generator.report_err_symbol_not_found(dot_expr.rhs);
        return Result::ERROR;
    }

    sir::Expr lhs = dot_expr.lhs;
    sir::Expr lhs_type = dot_expr.lhs.get_type();

    while (auto pointer_type = lhs_type.match<sir::PointerType>()) {
        lhs = analyzer.create_expr(sir::UnaryExpr{
            .ast_node = nullptr,
            .type = pointer_type->base_type,
            .op = sir::UnaryOp::DEREF,
            .value = lhs,
        });

        lhs_type = pointer_type->base_type;
    }

    if (!lhs_type) {
        return Result::ERROR;
    } else if (auto struct_def = lhs_type.match_symbol<sir::StructDef>()) {
        sir::StructField *field = struct_def->find_field(dot_expr.rhs.value);

        if (!field) {
            analyzer.report_generator.report_err_no_field(dot_expr.rhs, *struct_def);
            return Result::ERROR;
        }

        analyzer.add_symbol_use(dot_expr.rhs.ast_node, field);

        out_expr = analyzer.create_expr(sir::FieldExpr{
            .ast_node = dot_expr.ast_node,
            .type = field->type,
            .base = lhs,
            .field_index = field->index,
        });
    } else if (auto union_case = lhs_type.match_symbol<sir::UnionCase>()) {
        std::optional<unsigned> field_index = union_case->find_field(dot_expr.rhs.value);

        if (!field_index) {
            // analyzer.report_generator.report_err_no_field(dot_expr.rhs, *union_case);
            return Result::ERROR;
        }

        sir::UnionCaseField &field = union_case->fields[*field_index];

        out_expr = analyzer.create_expr(sir::FieldExpr{
            .ast_node = dot_expr.ast_node,
            .type = field.type,
            .base = lhs,
            .field_index = *field_index,
        });

        // analyzer.add_symbol_use(dot_expr.rhs.ast_node, field);
    } else if (auto tuple_expr = lhs_type.match<sir::TupleExpr>()) {
        unsigned field_index = std::stoul(dot_expr.rhs.value);

        out_expr = analyzer.create_expr(sir::FieldExpr{
            .ast_node = dot_expr.ast_node,
            .type = tuple_expr->exprs[field_index],
            .base = lhs,
            .field_index = field_index,
        });
    } else {
        analyzer.report_generator.report_err_no_members(dot_expr);
    }

    return Result::SUCCESS;
}

sir::FuncDef *ExprAnalyzer::resolve_overload(sir::OverloadSet &overload_set, const std::vector<sir::Expr> &args) {
    for (sir::FuncDef *func_def : overload_set.func_defs) {
        if (is_matching_overload(*func_def, args)) {
            return func_def;
        }
    }

    return nullptr;
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
    sir::Expr &inout_expr
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

        if (!impl) {
            analyzer.report_generator.report_err_no_matching_overload(inout_expr, *overload_set);
            return Result::ERROR;
        }
    } else {
        ASSERT_UNREACHABLE;
    }

    ASSERT(!(impl->type.params[0].attrs && impl->type.params[0].attrs->byval));

    sir::Expr callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &impl->type,
        .symbol = impl,
    });

    analyzer.add_symbol_use(inout_expr.get_ast_node(), impl);

    inout_expr = analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = impl->type.return_type,
        .callee = callee,
        .args = args,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::specialize(
    sir::FuncDef &func_def,
    const std::vector<sir::Expr> &generic_args,
    sir::Expr &inout_expr
) {
    sir::FuncDef *specialization = GenericsSpecializer(analyzer).specialize(func_def, generic_args);

    inout_expr = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = inout_expr.get_ast_node(),
        .type = &specialization->type,
        .symbol = specialization,
    });

    return Result::SUCCESS;
}

Result ExprAnalyzer::specialize(
    sir::StructDef &struct_def,
    const std::vector<sir::Expr> &generic_args,
    sir::Expr &inout_expr
) {
    sir::StructDef *specialization = GenericsSpecializer(analyzer).specialize(struct_def, generic_args);

    inout_expr = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = inout_expr.get_ast_node(),
        .type = nullptr,
        .symbol = specialization,
    });

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
