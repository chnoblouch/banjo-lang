#include "expr_analyzer.hpp"

#include "banjo/sema/const_evaluator.hpp"
#include "banjo/sema/decl_body_analyzer.hpp"
#include "banjo/sema/decl_interface_analyzer.hpp"
#include "banjo/sema/expr_finalizer.hpp"
#include "banjo/sema/expr_property_analyzer.hpp"
#include "banjo/sema/generic_arg_inference.hpp"
#include "banjo/sema/meta_expr_evaluator.hpp"
#include "banjo/sema/overload_resolver.hpp"
#include "banjo/sema/resource_analyzer.hpp"
#include "banjo/sema/result_macros.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/symbol_collector.hpp"
#include "banjo/sema/symbol_context.hpp"
#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"
#include "banjo/sir/sir_create.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/sir/specializer.hpp"
#include "banjo/sir/type_constraints.hpp"
#include "banjo/source/module_path.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

ExprAnalyzer::ExprAnalyzer(SemanticAnalyzer &analyzer, unsigned flags /* = 0x00000000 */)
  : analyzer{analyzer},
    flags{flags} {}

Result ExprAnalyzer::analyze_value(sir::Expr &expr) {
    Result result = analyze_value_uncoerced(expr);
    if (result != Result::SUCCESS) {
        return result;
    }

    return ExprFinalizer(analyzer).finalize(expr);
}

Result ExprAnalyzer::analyze_value(sir::Expr &expr, sir::Expr expected_type) {
    Result result = analyze_value_uncoerced(expr);
    if (result != Result::SUCCESS) {
        return result;
    }

    return ExprFinalizer(analyzer).finalize_by_coercion(expr, expected_type);
}

Result ExprAnalyzer::analyze_value_uncoerced(sir::Expr &expr) {
    Result result = analyze_uncoerced(expr);
    if (result != Result::SUCCESS) {
        return result;
    }

    sir::ExprCategory category = expr.get_category();

    if (category != sir::ExprCategory::VALUE && category != sir::ExprCategory::VALUE_OR_TYPE) {
        analyzer.report_generator.report_err_expr_category(expr, sir::ExprCategory::VALUE);
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_type(sir::Expr &expr) {
    Result result = analyze(expr);
    if (result != Result::SUCCESS) {
        return result;
    }

    sir::ExprCategory category = expr.get_category();

    if (category != sir::ExprCategory::TYPE && category != sir::ExprCategory::VALUE_OR_TYPE) {
        analyzer.report_generator.report_err_expr_category(expr, sir::ExprCategory::TYPE);
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze(sir::Expr &expr) {
    Result result = analyze_uncoerced(expr);
    if (result != Result::SUCCESS) {
        return result;
    }

    return ExprFinalizer(analyzer).finalize(expr);
}

Result ExprAnalyzer::analyze(sir::Expr &expr, sir::Expr expected_type) {
    Result result = analyze_uncoerced(expr);
    if (result != Result::SUCCESS) {
        return result;
    }

    return ExprFinalizer(analyzer).finalize_by_coercion(expr, expected_type);
}

Result ExprAnalyzer::analyze_uncoerced(sir::Expr &expr) {
    Result result = Result::SUCCESS;

    // FIXME: Hack because range expressions get analyzed twice.
    if (analyzer.get_resolved_type(expr).is<sir::ReferenceType>()) {
        return Result::SUCCESS;
    }

    bool is_ref = false;

    if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        is_ref = unary_expr->op == sir::UnaryOp::REF || unary_expr->op == sir::UnaryOp::REF_MUT;
    }

    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IMPOSSIBLE,                           // empty
        result = analyze_int_literal(*inner),           // int_literal
        result = analyze_fp_literal(*inner),            // fp_literal
        result = analyze_bool_literal(*inner),          // bool_literal
        result = analyze_char_literal(*inner),          // char_literal
        result = analyze_null_literal(*inner),          // null_literal
        result = analyze_none_literal(*inner),          // none_literal
        result = analyze_undefined_literal(*inner),     // undefined_literal
        result = analyze_array_literal(*inner, expr),   // array_literal
        result = analyze_string_literal(*inner),        // string_literal
        result = analyze_struct_literal(*inner),        // struct_literal
        SIR_VISIT_IGNORE,                               // union_case_literal
        result = analyze_map_literal(*inner, expr),     // map_literal
        result = analyze_closure_literal(*inner, expr), // closure_literal
        SIR_VISIT_IGNORE,                               // symbol_expr
        result = analyze_binary_expr(*inner, expr),     // binary_expr
        result = analyze_unary_expr(*inner, expr),      // unary_expr
        result = analyze_cast_expr(*inner),             // cast_expr
        SIR_VISIT_IGNORE,                               // index_expr
        result = analyze_call_expr(*inner, expr),       // call_expr
        SIR_VISIT_IGNORE,                               // field_expr
        analyze_range_expr(*inner),                     // range_expr
        result = analyze_try_expr(*inner),              // try_expr
        analyze_tuple_expr(*inner),                     // tuple_expr
        SIR_VISIT_IGNORE,                               // coercion_expr
        SIR_VISIT_IGNORE,                               // specialize_expr
        SIR_VISIT_IGNORE,                               // primitive_type
        SIR_VISIT_IGNORE,                               // pointer_type
        result = analyze_static_array_type(*inner),     // static_array_type
        analyze_func_type(*inner),                      // func_type
        result = analyze_optional_type(*inner, expr),   // optional_type
        result = analyze_result_type(*inner, expr),     // result_type
        result = analyze_array_type(*inner, expr),      // array_type
        result = analyze_map_type(*inner, expr),        // map_type
        result = analyze_closure_type(*inner),          // closure_type
        result = analyze_reference_type(*inner),        // reference_type
        result = analyze_ident_expr(*inner, expr),      // ident_expr
        result = analyze_star_expr(*inner, expr),       // star_expr
        result = analyze_bracket_expr(*inner, expr),    // bracket_expr
        result = analyze_dot_expr(*inner, expr),        // dot_expr
        SIR_VISIT_IGNORE,                               // pseudo_tpe
        result = analyze_meta_access(*inner),           // meta_access
        result = analyze_meta_field_expr(*inner),       // meta_field_expr
        result = analyze_meta_call_expr(*inner, expr),  // meta_call_expr
        SIR_VISIT_IMPOSSIBLE,                           // init_expr
        SIR_VISIT_IMPOSSIBLE,                           // move_expr
        SIR_VISIT_IMPOSSIBLE,                           // deinit_expr
        SIR_VISIT_IMPOSSIBLE,                           // type_guard_expr
        SIR_VISIT_IMPOSSIBLE,                           // placeholder_expr
        result = Result::ERROR                          // error
    );

    if (result != Result::SUCCESS) {
        return result;
    }

    while (auto type_alias = expr.match_symbol<sir::TypeAlias>()) {
        expr = type_alias->type;
    }

    if (!is_ref) {
        if (auto reference_type = analyzer.get_resolved_type(expr).match<sir::ReferenceType>()) {
            expr = analyzer.create(
                sir::UnaryExpr{
                    .ast_node = expr.get_ast_node(),
                    .type = reference_type->base_type,
                    .op = sir::UnaryOp::DEREF,
                    .value = expr,
                }
            );
        }
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_int_literal(sir::IntLiteral &int_literal) {
    int_literal.type = analyzer.create(
        sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::INT_LITERAL,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_fp_literal(sir::FPLiteral &fp_literal) {
    fp_literal.type = analyzer.create(
        sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::FP_LITERAL,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_bool_literal(sir::BoolLiteral &bool_literal) {
    bool_literal.type = analyzer.create(
        sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::BOOL,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_char_literal(sir::CharLiteral &char_literal) {
    // TODO: Error handling for characters with size != 1

    char_literal.type = analyzer.create(
        sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::U8,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_null_literal(sir::NullLiteral &null_literal) {
    null_literal.type = analyzer.create(
        sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::NULL_LITERAL,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_none_literal(sir::NoneLiteral &none_literal) {
    none_literal.type = analyzer.create(
        sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::NONE_LITERAL,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_undefined_literal(sir::UndefinedLiteral &undefined_literal) {
    undefined_literal.type = analyzer.create(
        sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::UNDEFINED_LITERAL,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_array_literal(sir::ArrayLiteral &array_literal, sir::Expr &out_expr) {
    // FIXME: Error handling for fixing values and types

    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::Expr &value : array_literal.values) {
        partial_result = analyze_uncoerced(value);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (array_literal.values.size() == 1 && array_literal.values[0].is_type()) {
        out_expr = analyzer.create(
            sir::ArrayType{
                .ast_node = array_literal.ast_node,
                .base_type = array_literal.values[0],
            }
        );

        return analyze_array_type(out_expr.as<sir::ArrayType>(), out_expr);
    } else {
        array_literal.type = analyzer.create(
            sir::PseudoType{
                .ast_node = nullptr,
                .kind = sir::PseudoTypeKind::ARRAY_LITERAL,
            }
        );
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_string_literal(sir::StringLiteral &string_literal) {
    string_literal.type = analyzer.create(
        sir::PseudoType{
            .ast_node = nullptr,
            .kind = sir::PseudoTypeKind::STRING_LITERAL,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_struct_literal(sir::StructLiteral &struct_literal) {
    Result result;

    if (struct_literal.type) {
        result = analyze_type(struct_literal.type);
        if (result != Result::SUCCESS) {
            return result;
        }
    }

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        if (entry.ident.is_completion_token()) {
            analyzer.completion_context = CompleteInStructLiteral{
                .struct_literal = &struct_literal,
            };

            continue;
        }

        result = analyze_value_uncoerced(entry.value);
        if (result != Result::SUCCESS) {
            return result;
        }
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_map_literal(sir::MapLiteral &map_literal, sir::Expr &out_expr) {
    // FIXME: Error handling for fixing values and types

    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::MapLiteralEntry &entry : map_literal.entries) {
        partial_result = analyze_uncoerced(entry.key);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }

        partial_result = analyze_uncoerced(entry.value);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::ExprCategory first_key_category = map_literal.entries[0].key.get_category();
    sir::ExprCategory first_value_category = map_literal.entries[0].value.get_category();

    if (first_key_category == sir::ExprCategory::VALUE && first_value_category == sir::ExprCategory::VALUE) {
        map_literal.type = analyzer.create(
            sir::PseudoType{
                .ast_node = nullptr,
                .kind = sir::PseudoTypeKind::MAP_LITERAL,
            }
        );

        return Result::SUCCESS;
    } else if (first_key_category == sir::ExprCategory::TYPE && first_value_category == sir::ExprCategory::TYPE) {
        out_expr = analyzer.create(
            sir::MapType{
                .ast_node = map_literal.ast_node,
                .key_type = map_literal.entries[0].key,
                .value_type = map_literal.entries[0].value,
            }
        );

        return analyze_map_type(out_expr.as<sir::MapType>(), out_expr);
    } else {
        // TODO: Report error
        return Result::ERROR;
    }
}

Result ExprAnalyzer::analyze_closure_literal(sir::ClosureLiteral &closure_literal, sir::Expr &out_expr) {
    sir::TupleExpr *data_type = analyzer.create(
        sir::TupleExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .exprs = {},
        }
    );

    sir::Param data_ptr_param{
        .ast_node = nullptr,
        .name = {},
        .type = analyzer.create(
            sir::PrimitiveType{
                .ast_node = nullptr,
                .primitive = sir::Primitive::ADDR,
            }
        ),
    };

    unsigned num_args = closure_literal.func_type.params.size() + 1;

    std::span<sir::Param> generated_params = analyzer.allocate_array<sir::Param>(num_args);
    generated_params[0] = data_ptr_param;

    for (unsigned i = 0; i < closure_literal.func_type.params.size(); i++) {
        generated_params[i + 1] = closure_literal.func_type.params[i];
    }

    sir::FuncType generated_func_type{
        .ast_node = closure_literal.func_type.ast_node,
        .params = generated_params,
        .return_type = closure_literal.func_type.return_type,
    };

    sir::FuncDef *generated_func = analyzer.create(
        sir::FuncDef{
            .ast_node = closure_literal.ast_node,
            .ident{
                .ast_node = nullptr,
                .value = "",
            },
            .type = generated_func_type,
            .block = *closure_literal.block,
            .attrs = nullptr,
            .generic_params = {},
        }
    );

    generated_func->block.symbol_table->parent = analyzer.get_decl_block().symbol_table;

    ClosureContext closure_ctx{
        .captured_vars{},
        .data_type = data_type,
        .parent_block = &analyzer.get_block(),
    };

    SymbolCollector(analyzer).collect_closure_def(*generated_func);
    DeclInterfaceAnalyzer(analyzer).visit_func_def(*generated_func);
    DeclBodyAnalyzer(analyzer).visit_closure_def(*generated_func, closure_ctx);
    ResourceAnalyzer(analyzer).visit_func_def(*generated_func);

    analyzer.get_mod().block.decls.push_back(generated_func);

    data_type->exprs = analyzer.allocate_array<sir::Expr>(closure_ctx.captured_vars.size());
    std::span<sir::Expr> capture_values = analyzer.allocate_array<sir::Expr>(closure_ctx.captured_vars.size());

    for (unsigned i = 0; i < closure_ctx.captured_vars.size(); i++) {
        sir::Symbol captured_var = closure_ctx.captured_vars[i];
        sir::Expr type = captured_var.get_type();

        sir::Expr value = analyzer.create(
            sir::SymbolExpr{
                .ast_node = nullptr,
                .type = type,
                .symbol = captured_var,
            }
        );

        data_type->exprs[i] = type;
        capture_values[i] = value;
    }

    sir::TupleExpr *data = analyzer.create(
        sir::TupleExpr{
            .ast_node = nullptr,
            .type = data_type,
            .exprs = capture_values,
        }
    );

    sir::StructDef &std_closure_def = *analyzer.std_closure_def;
    sir::FuncDef &new_def_generic = std_closure_def.block.symbol_table->look_up_local("new").as<sir::FuncDef>();
    std::span<sir::Expr> generic_args = analyzer.create_array<sir::Expr>({data_type});

    sir::Expr callee = analyzer.create(
        sir::SpecializeExpr{
            .ast_node = nullptr,
            .type = sir::Specializer{analyzer.mod->trivial_arena, new_def_generic.generic_params, generic_args}
                        .specialize_func_type(new_def_generic.type),
            .symbol = &new_def_generic,
            .args = generic_args,
        }
    );

    sir::Expr addr_type = analyzer.create(
        sir::PrimitiveType{
            .ast_node = nullptr,
            .primitive = sir::Primitive::ADDR,
        }
    );

    sir::Expr func_ptr = analyzer.create(
        sir::CastExpr{
            .ast_node = nullptr,
            .type = addr_type,
            .value = analyzer.create(
                sir::SymbolExpr{
                    .ast_node = nullptr,
                    .type = &generated_func->type,
                    .symbol = generated_func,
                }
            ),
        }
    );

    analyze_func_type(closure_literal.func_type);

    out_expr = analyzer.create(
        sir::CallExpr{
            .ast_node = nullptr,
            .type = analyzer.create(
                sir::ClosureType{
                    .ast_node = nullptr,
                    .func_type = closure_literal.func_type,
                    .underlying_struct = &std_closure_def,
                }
            ),
            .callee = callee,
            .args = analyzer.create_array<sir::Expr>({func_ptr, data}),
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_binary_expr(sir::BinaryExpr &binary_expr, sir::Expr &out_expr) {
    // FIXME: Don't allow mod and bitwise operations on floats.

    BinaryOpType op_type = get_binary_op_type(binary_expr.op);

    Result lhs_result;
    Result rhs_result;

    if (op_type == BinaryOpType::EQUALITY_COMP) {
        lhs_result = analyze_uncoerced(binary_expr.lhs);
        rhs_result = analyze_uncoerced(binary_expr.rhs);
    } else {
        lhs_result = analyze_value_uncoerced(binary_expr.lhs);
        rhs_result = analyze_value_uncoerced(binary_expr.rhs);
    }

    if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (binary_expr.lhs.is_symbol<sir::GenericParam>() && binary_expr.rhs.is_type()) {
        return create_type_guard(binary_expr, out_expr);
    }

    if (binary_expr.lhs.is_type()) {
        return create_type_comparison(binary_expr, out_expr);
    }

    sir::Expr lhs_type = analyzer.get_resolved_type(binary_expr.lhs);
    sir::Expr rhs_type = analyzer.get_resolved_type(binary_expr.rhs);

    if (auto generic_param = lhs_type.match_symbol<sir::GenericParam>()) {
        sir::ProtoDef *proto_def;

        switch (binary_expr.op) {
            case sir::BinaryOp::ADD: proto_def = analyzer.std_add_def; break;
            case sir::BinaryOp::SUB: proto_def = analyzer.std_sub_def; break;
            case sir::BinaryOp::MUL: proto_def = analyzer.std_mul_def; break;
            case sir::BinaryOp::DIV: proto_def = analyzer.std_div_def; break;
            case sir::BinaryOp::MOD: proto_def = analyzer.std_mod_def; break;
            case sir::BinaryOp::BIT_AND: proto_def = analyzer.std_bit_and_def; break;
            case sir::BinaryOp::BIT_OR: proto_def = analyzer.std_bit_or_def; break;
            case sir::BinaryOp::BIT_XOR: proto_def = analyzer.std_bit_xor_def; break;
            case sir::BinaryOp::SHL: proto_def = analyzer.std_shl_def; break;
            case sir::BinaryOp::SHR: proto_def = analyzer.std_shr_def; break;
            case sir::BinaryOp::EQ: proto_def = analyzer.std_compare_def; break;
            case sir::BinaryOp::NE: proto_def = analyzer.std_compare_def; break;
            case sir::BinaryOp::GT: proto_def = analyzer.std_order_def; break;
            case sir::BinaryOp::LT: proto_def = analyzer.std_order_def; break;
            case sir::BinaryOp::GE: proto_def = analyzer.std_order_def; break;
            case sir::BinaryOp::LE: proto_def = analyzer.std_order_def; break;
            case sir::BinaryOp::AND: proto_def = nullptr; break;
            case sir::BinaryOp::OR: proto_def = nullptr; break;
        }

        for (sir::Expr component : generic_param->constraint.components) {
            if (auto concrete_proto = component.match_concrete<sir::ProtoDef>()) {
                if (concrete_proto->def == proto_def && concrete_proto->generic_args[0] == rhs_type) {
                    lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);
                    rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);

                    RESULT_RETURN_ON_ERROR(lhs_result)
                    RESULT_RETURN_ON_ERROR(rhs_result)

                    sir::Expr return_type = concrete_proto->def->func_decls[0].get_type().return_type;
                    sir::Specializer specializer{analyzer.mod->trivial_arena, *concrete_proto};
                    return_type = specializer.specialize_expr(return_type);

                    out_expr = analyzer.create(
                        sir::PlaceholderExpr{
                            .ast_node = nullptr,
                            .type = return_type,
                            .kind = sir::PlaceholderExpr::BinaryExpr{
                                .op = binary_expr.op,
                                .lhs = binary_expr.lhs,
                                .rhs = binary_expr.rhs,
                            },
                        }
                    );

                    return Result::SUCCESS;
                }
            }
        }
    }

    bool is_operator_built_in = false;
    bool is_operator_overload = false;

    if (auto primitive_type = lhs_type.match<sir::PrimitiveType>()) {
        switch (primitive_type->primitive) {
            case sir::Primitive::I8:
            case sir::Primitive::I16:
            case sir::Primitive::I32:
            case sir::Primitive::I64:
            case sir::Primitive::U8:
            case sir::Primitive::U16:
            case sir::Primitive::U32:
            case sir::Primitive::U64:
            case sir::Primitive::USIZE:
            case sir::Primitive::F32:
            case sir::Primitive::F64:
            case sir::Primitive::ADDR:
                is_operator_built_in = op_type == BinaryOpType::ARITHMETIC || op_type == BinaryOpType::EQUALITY_COMP ||
                                       op_type == BinaryOpType::ORDER_COMP;
                break;
            case sir::Primitive::BOOL:
                is_operator_built_in = op_type == BinaryOpType::EQUALITY_COMP || op_type == BinaryOpType::LOGICAL;
                break;
            case sir::Primitive::VOID: break;
        }
    } else if (auto pseudo_type = lhs_type.match<sir::PseudoType>()) {
        switch (pseudo_type->kind) {
            case sir::PseudoTypeKind::INT_LITERAL:
            case sir::PseudoTypeKind::FP_LITERAL:
                is_operator_built_in = op_type == BinaryOpType::ARITHMETIC || op_type == BinaryOpType::EQUALITY_COMP ||
                                       op_type == BinaryOpType::ORDER_COMP;
                break;
            case sir::PseudoTypeKind::NULL_LITERAL:
                is_operator_built_in = op_type == BinaryOpType::EQUALITY_COMP || binary_expr.op == sir::BinaryOp::ADD;
                break;
            case sir::PseudoTypeKind::NONE_LITERAL: break;
            case sir::PseudoTypeKind::UNDEFINED_LITERAL: break;
            case sir::PseudoTypeKind::STRING_LITERAL: is_operator_overload = true; break;
            case sir::PseudoTypeKind::ARRAY_LITERAL: is_operator_overload = true; break;
            case sir::PseudoTypeKind::MAP_LITERAL: is_operator_overload = true; break;
            case sir::PseudoTypeKind::SELF_TYPE: is_operator_overload = true; break;
        }
    } else if (auto symbol_expr = lhs_type.match<sir::SymbolExpr>()) {
        if (symbol_expr->symbol.is<sir::StructDef>()) {
            is_operator_overload = true;
        } else if (symbol_expr->symbol.is<sir::EnumDef>()) {
            is_operator_built_in = true;
        }
    } else if (lhs_type.is<sir::PointerType>()) {
        is_operator_built_in = op_type == BinaryOpType::EQUALITY_COMP || binary_expr.op == sir::BinaryOp::ADD ||
                               binary_expr.op == sir::BinaryOp::SUB;
    }

    if (is_operator_overload) {
        lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);
        if (lhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        sir::StructDef &struct_def = analyzer.get_resolved_type(binary_expr.lhs).as_symbol<sir::StructDef>();
        std::string_view impl_name = sir::MagicMethods::look_up(binary_expr.op);
        sir::Symbol symbol = struct_def.block.symbol_table->look_up_local(impl_name);

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(binary_expr);
            return Result::ERROR;
        }

        std::span<sir::Expr> call_args = analyzer.create_array<sir::Expr>({binary_expr.lhs, binary_expr.rhs});
        return analyze_operator_overload_call(symbol, call_args, out_expr);
    } else if (!is_operator_built_in) {
        analyzer.report_generator.report_err_cannot_apply_operator(binary_expr);
        return Result::ERROR;
    }

    bool can_lhs_be_coerced = can_be_coerced(binary_expr.lhs);
    bool can_rhs_be_coerced = can_be_coerced(binary_expr.rhs);

    if (!can_lhs_be_coerced && !can_rhs_be_coerced) {
        lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);
        rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);
    } else if (can_lhs_be_coerced && !can_rhs_be_coerced) {
        rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);

        if (rhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        lhs_result = ExprFinalizer(analyzer).finalize_by_coercion(binary_expr.lhs, rhs_type);
    } else if (can_rhs_be_coerced && !can_lhs_be_coerced) {
        lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);

        if (lhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        rhs_result = ExprFinalizer(analyzer).finalize_by_coercion(binary_expr.rhs, lhs_type);
    } else {
        if (binary_expr.is_arithmetic_op() || binary_expr.is_bitwise_op()) {
            binary_expr.type = analyzer.get_resolved_type(binary_expr.lhs);
            return Result::SUCCESS;
        }

        lhs_result = ExprFinalizer(analyzer).finalize(binary_expr.lhs);
        rhs_result = ExprFinalizer(analyzer).finalize(binary_expr.rhs);
    }

    if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    lhs_type = analyzer.get_resolved_type(binary_expr.lhs);
    rhs_type = analyzer.get_resolved_type(binary_expr.rhs);
    bool types_equal = lhs_type == rhs_type;

    if (lhs_type != rhs_type) {
        if (lhs_type.is_addr_like_type()) {
            binary_expr.rhs = create_isize_cast(binary_expr.rhs);
            types_equal = true;
        } else if (rhs_type.is_addr_like_type()) {
            binary_expr.lhs = create_isize_cast(binary_expr.lhs);
            types_equal = true;
        }
    }

    if (binary_expr.is_arithmetic_op() || binary_expr.is_bitwise_op()) {
        if (!types_equal) {
            analyzer.report_generator.report_err_type_mismatch(binary_expr.rhs, lhs_type, rhs_type);
            return Result::ERROR;
        }

        binary_expr.type = lhs_type;
    } else if (binary_expr.is_comparison_op()) {
        if (!types_equal) {
            analyzer.report_generator.report_err_type_mismatch(binary_expr.rhs, lhs_type, rhs_type);
            return Result::ERROR;
        }

        binary_expr.type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL);
    } else if (binary_expr.is_logical_op()) {
        if (!lhs_type.is_primitive_type(sir::Primitive::BOOL)) {
            analyzer.report_generator.report_err_expected_bool(binary_expr.lhs);
            lhs_result = Result::ERROR;
        }

        if (!rhs_type.is_primitive_type(sir::Primitive::BOOL)) {
            analyzer.report_generator.report_err_expected_bool(binary_expr.rhs);
            rhs_result = Result::ERROR;
        }

        if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        binary_expr.type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL);
    } else {
        ASSERT_UNREACHABLE;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::create_type_guard(sir::BinaryExpr &binary_expr, sir::Expr &out_expr) {
    out_expr = analyzer.create(
        sir::TypeGuardExpr{
            .ast_node = binary_expr.ast_node,
            .type = sir::create_primitive_type(*analyzer.mod, sir::Primitive::BOOL),
            .generic_param = &binary_expr.lhs.as_symbol<sir::GenericParam>(),
            .constraint = binary_expr.rhs,
        }
    );

    type_narrowing = sir::TypeNarrowing{
        .generic_param = &binary_expr.lhs.as_symbol<sir::GenericParam>(),
        .constraint = binary_expr.rhs,
    };

    return Result::SUCCESS;
}

Result ExprAnalyzer::create_type_comparison(sir::BinaryExpr &binary_expr, sir::Expr &out_expr) {
    ConstEvaluator::Output evaluated = ConstEvaluator{analyzer}.evaluate_binary_expr(binary_expr);
    if (evaluated.result != Result::SUCCESS) {
        return evaluated.result;
    }

    out_expr = evaluated.expr;
    return analyze_uncoerced(out_expr);
}

Result ExprAnalyzer::analyze_unary_expr(sir::UnaryExpr &unary_expr, sir::Expr &out_expr) {
    Result partial_result;

    if (unary_expr.op == sir::UnaryOp::DEREF) {
        return Result::SUCCESS;
    }

    // Deref unary operations are handled by `analyze_star_expr`.
    // FIXME: This assumption can currently fail due to the meta expression evaulator analyzing
    // expressions twice.
    ASSUME(unary_expr.op != sir::UnaryOp::DEREF);

    partial_result = analyze_uncoerced(unary_expr.value);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::Expr value_type = analyzer.get_resolved_type(unary_expr.value);

    if (unary_expr.op == sir::UnaryOp::ADDR) {
        ExprFinalizer(analyzer).finalize(unary_expr.value);

        unary_expr.type = analyzer.create(
            sir::PointerType{
                .ast_node = nullptr,
                .base_type = value_type,
            }
        );

        ExprProperties props = ExprPropertyAnalyzer().analyze(unary_expr.value);

        if (props.mutability == Mutability::IMMUTABLE_REF) {
            analyzer.report_generator.report_err_cannot_create_pointer_to_immut(&unary_expr, props.base_value);
            return Result::ERROR;
        }

        return Result::SUCCESS;
    }

    if (unary_expr.op == sir::UnaryOp::REF || unary_expr.op == sir::UnaryOp::REF_MUT) {
        partial_result = ExprFinalizer(analyzer).finalize(unary_expr.value);
        if (partial_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        bool mut = unary_expr.op == sir::UnaryOp::REF_MUT;

        if (unary_expr.value.is_type()) {
            out_expr = analyzer.create(
                sir::ReferenceType{
                    .ast_node = nullptr,
                    .mut = mut,
                    .base_type = unary_expr.value,
                }
            );
        } else {
            unary_expr.op = sir::UnaryOp::ADDR;

            unary_expr.type = analyzer.create(
                sir::ReferenceType{
                    .ast_node = nullptr,
                    .mut = mut,
                    .base_type = analyzer.get_resolved_type(unary_expr.value),
                }
            );
        }

        return Result::SUCCESS;
    } else if (unary_expr.op == sir::UnaryOp::SHARE) {
        partial_result = ExprFinalizer(analyzer).finalize(unary_expr.value);
        if (partial_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        sir::StructDef &struct_def = *analyzer.std_shared_def;

        if (unary_expr.value.is_type()) {
            std::span<sir::Expr> generic_args = analyzer.create_array({unary_expr.value});
            specialize(struct_def, generic_args, out_expr);
        } else {
            sir::Expr type = analyzer.get_resolved_type(value_type);
            std::span<sir::Expr> generic_args = analyzer.create_array({type});

            // FIXME: Checked generics
            sir::StructDef *specialization =
                nullptr; // GenericsSpecializer(analyzer).specialize(struct_def, generic_args);

            out_expr = sir::create_call(
                analyzer.get_mod(),
                specialization->block.symbol_table->look_up_local("new").as<sir::FuncDef>(),
                analyzer.create_array({unary_expr.value})
            );
        }

        return Result::SUCCESS;
    }

    if (auto struct_def = value_type.match_symbol<sir::StructDef>()) {
        ExprFinalizer(analyzer).finalize(unary_expr.value);

        std::string_view impl_name = sir::MagicMethods::look_up(unary_expr.op);
        sir::Symbol symbol = struct_def->block.symbol_table->look_up_local(impl_name);

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(unary_expr);
            return Result::ERROR;
        }

        std::span<sir::Expr> call_args = analyzer.create_array<sir::Expr>({unary_expr.value});
        return analyze_operator_overload_call(symbol, call_args, out_expr);
    }

    if (unary_expr.op == sir::UnaryOp::NEG) {
        if (value_type.is_signed_type() || value_type.is_fp_type() ||
            value_type.is_pseudo_type(sir::PseudoTypeKind::INT_LITERAL) ||
            value_type.is_pseudo_type(sir::PseudoTypeKind::FP_LITERAL)) {
            unary_expr.type = value_type;
        } else if (value_type.is_unsigned_type()) {
            analyzer.report_generator.report_err_cannot_negate_unsigned(unary_expr);
            return Result::ERROR;
        } else {
            analyzer.report_generator.report_err_cannot_negate(unary_expr);
            return Result::ERROR;
        }
    } else if (unary_expr.op == sir::UnaryOp::BIT_NOT) {
        if (!value_type.is_int_type()) {
            analyzer.report_generator.report_err_expected_integer(unary_expr.value);
            return Result::ERROR;
        }

        unary_expr.type = value_type;
    } else if (unary_expr.op == sir::UnaryOp::NOT) {
        partial_result = ExprFinalizer(analyzer).finalize(unary_expr.value);
        if (partial_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        value_type = analyzer.get_resolved_type(unary_expr.value);

        if (!value_type.is_primitive_type(sir::Primitive::BOOL)) {
            analyzer.report_generator.report_err_expected_bool(unary_expr.value);
            return Result::ERROR;
        }

        unary_expr.type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL);
    } else {
        ASSERT_UNREACHABLE;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_cast_expr(sir::CastExpr &cast_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = analyze_value(cast_expr.value);
    if (partial_result != Result::SUCCESS) {
        result = partial_result;
    }

    partial_result = analyze_type(cast_expr.type);
    if (partial_result != Result::SUCCESS) {
        result = partial_result;
    }

    if (result != Result::SUCCESS) {
        return result;
    }

    sir::Expr from = analyzer.get_resolved_type(cast_expr.value);
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
            analyze_value(call_expr.args[0]);

            out_expr = analyzer.create(
                sir::DeinitExpr{
                    .ast_node = nullptr,
                    .type = analyzer.get_resolved_type(call_expr.args[0]),
                    .value = call_expr.args[0],
                    .resource = nullptr,
                }
            );

            return Result::SUCCESS;
        } else if (ident_expr->value == "__builtin_pointer_to") {
            analyze_value(call_expr.args[0]);

            out_expr = analyzer.create(
                sir::UnaryExpr{
                    .ast_node = nullptr,
                    .type = analyzer.create(
                        sir::PointerType{
                            .ast_node = nullptr,
                            .base_type = analyzer.get_resolved_type(call_expr.args[0]),
                        }
                    ),
                    .op = sir::UnaryOp::ADDR,
                    .value = call_expr.args[0],
                }
            );

            return Result::SUCCESS;
        }
    }

    if (auto dot_expr = call_expr.callee.match<sir::DotExpr>()) {
        partial_result = analyze_dot_expr_callee(*dot_expr, call_expr, is_method);
    } else {
        partial_result = analyze_value(call_expr.callee);
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

        partial_result = analyze_value_uncoerced(arg);
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
            partial_result = Result::SUCCESS;

            for (unsigned i = first_arg_to_analyze; i < call_expr.args.size(); i++) {
                sir::Expr &arg = call_expr.args[i];

                partial_result = ExprFinalizer(analyzer).finalize(arg);
                if (partial_result != Result::SUCCESS) {
                    result = Result::ERROR;
                }
            }

            if (partial_result != Result::SUCCESS) {
                return Result::ERROR;
            }

            std::span<sir::Expr> generic_args;

            partial_result = GenericArgInference(analyzer, &call_expr, *func_def).infer(call_expr.args, generic_args);
            if (partial_result != Result::SUCCESS) {
                return Result::ERROR;
            }

            Result result = Result::SUCCESS;

            for (unsigned i = 0; i < func_def->generic_params.size(); i++) {
                ASTNode *ast_node = call_expr.callee.get_ast_node();
                RESULT_MERGE(result, check_type_constraint(ast_node, func_def->generic_params, generic_args, i));
            }

            RESULT_RETURN_ON_ERROR(result);

            partial_result = specialize(*func_def, generic_args, call_expr.callee);
            if (partial_result != Result::SUCCESS) {
                return partial_result;
            }

            if (func_def->generic_params.back()->kind == sir::GenericParamKind::SEQUENCE) {
                unsigned num_non_sequence_args = func_def->type.params.size() - 1;
                unsigned num_sequence_args = call_expr.args.size() - num_non_sequence_args;

                std::span<sir::Expr> sequence_args = analyzer.allocate_array<sir::Expr>(num_sequence_args);
                for (unsigned i = 0; i < num_sequence_args; i++) {
                    sequence_args[i] = call_expr.args[num_non_sequence_args + i];
                }

                std::span<sir::Expr> args = analyzer.allocate_array<sir::Expr>(num_non_sequence_args + 1);
                for (unsigned i = 0; i < num_non_sequence_args; i++) {
                    args[i] = call_expr.args[i];
                }

                args[num_non_sequence_args] = analyzer.create(
                    sir::TupleExpr{
                        .ast_node = nullptr,
                        .type = generic_args.back(),
                        .exprs = sequence_args,
                    }
                );

                call_expr.args = args;
            }
        }

        callee_func_def = func_def;
    } else if (auto overload_set = call_expr.callee.match_symbol<sir::OverloadSet>()) {
        callee_func_def = OverloadResolver(analyzer).resolve(*overload_set, call_expr.args);

        if (!callee_func_def) {
            analyzer.report_generator.report_err_no_matching_overload(call_expr.callee, *overload_set);
            return Result::ERROR;
        }

        analyzer.add_symbol_use(call_expr.callee.as<sir::SymbolExpr>().ast_node, callee_func_def);

        call_expr.callee = analyzer.create(
            sir::SymbolExpr{
                .ast_node = nullptr,
                .type = &callee_func_def->type,
                .symbol = callee_func_def,
            }
        );
    }

    sir::Expr callee_type = analyzer.get_resolved_type(call_expr.callee);
    sir::FuncType *callee_func_type;

    if (auto func_type = callee_type.match<sir::FuncType>()) {
        callee_func_type = func_type;
    } else if (auto closure_type = callee_type.match<sir::ClosureType>()) {
        callee_func_type = &closure_type->func_type;
    } else {
        if (callee_type) {
            analyzer.report_generator.report_err_cannot_call(call_expr.callee);
        }

        return Result::ERROR;
    }

    partial_result = finalize_call_expr_args(call_expr, *callee_func_type, callee_func_def);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    call_expr.type = callee_func_type->return_type;

    if (auto closure_type = callee_type.match<sir::ClosureType>()) {
        sir::Expr func_ptr = analyzer.create(
            sir::FieldExpr{
                .ast_node = nullptr,
                .type = closure_type->underlying_struct->fields[0]->type,
                .base = call_expr.callee,
                .field_index = 0,
            }
        );

        sir::Expr data_ptr = analyzer.create(
            sir::FieldExpr{
                .ast_node = nullptr,
                .type = closure_type->underlying_struct->fields[1]->type,
                .base = call_expr.callee,
                .field_index = 1,
            }
        );

        call_expr.callee = analyzer.create(
            sir::CastExpr{
                .ast_node = nullptr,
                .type = &closure_type->func_type,
                .value = func_ptr,
            }
        );

        std::span<sir::Expr> args = analyzer.allocate_array<sir::Expr>(call_expr.args.size() + 1);
        args[0] = data_ptr;

        for (unsigned i = 0; i < call_expr.args.size(); i++) {
            args[i + 1] = call_expr.args[i];
        }

        call_expr.args = args;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_dot_expr_callee(sir::DotExpr &dot_expr, sir::CallExpr &out_call_expr, bool &is_method) {
    Result partial_result;

    partial_result = analyze(dot_expr.lhs);
    if (partial_result != Result::SUCCESS) {
        return partial_result;
    }

    if (dot_expr.rhs.is_error_token()) {
        return Result::ERROR;
    }

    sir::Expr lhs = dot_expr.lhs;
    sir::Expr lhs_type = analyzer.get_resolved_type(dot_expr.lhs);

    while (auto pointer_type = lhs_type.match<sir::PointerType>()) {
        if (pointer_type->base_type.match_symbol<sir::ProtoDef>()) {
            break;
        }

        lhs = analyzer.create(
            sir::UnaryExpr{
                .ast_node = nullptr,
                .type = pointer_type->base_type,
                .op = sir::UnaryOp::DEREF,
                .value = lhs,
            }
        );

        lhs_type = pointer_type->base_type;
    }

    if (auto concrete_struct = lhs_type.match_concrete<sir::StructDef>()) {
        sir::StructDef &struct_def = *concrete_struct->def;
        sir::Symbol method = struct_def.block.symbol_table->look_up_local(dot_expr.rhs.value);

        if (method) {
            create_method_call(out_call_expr, lhs, dot_expr.rhs, method);
            is_method = true;
            return Result::SUCCESS;
        }

        // FIXME: This doesn't work for generic structs.

        sir::StructField *field = struct_def.find_field(dot_expr.rhs.value);

        if (field) {
            analyzer.add_symbol_use(dot_expr.rhs.ast_node, field);

            out_call_expr.callee = analyzer.create(
                sir::FieldExpr{
                    .ast_node = dot_expr.ast_node,
                    .type = field->type,
                    .base = lhs,
                    .field_index = field->index,
                }
            );

            return Result::SUCCESS;
        }

        analyzer.report_generator.report_err_no_method(dot_expr.rhs, struct_def);
        return Result::ERROR;
    } else if (auto union_def = lhs_type.match_symbol<sir::UnionDef>()) {
        sir::Symbol method = union_def->block.symbol_table->look_up_local(dot_expr.rhs.value);

        if (method) {
            create_method_call(out_call_expr, lhs, dot_expr.rhs, method);
            is_method = true;
            return Result::SUCCESS;
        }

        analyzer.report_generator.report_err_no_method(dot_expr.rhs, *union_def);
        return Result::ERROR;
    } else if (auto proto_def = lhs_type.match_proto_ptr()) {
        sir::Symbol method = proto_def->block.symbol_table->look_up_local(dot_expr.rhs.value);

        if (method) {
            create_method_call(out_call_expr, lhs, dot_expr.rhs, method);
            is_method = true;
            return Result::SUCCESS;
        }

        analyzer.report_generator.report_err_no_method(dot_expr.rhs, *proto_def);
        return Result::ERROR;
    } else if (auto generic_param = lhs_type.match_symbol<sir::GenericParam>()) {
        std::string_view name = dot_expr.rhs.value;
        auto [method, concrete_proto] = resolve_generic_method_call(*generic_param, name);

        if (method) {
            // if (!method) {
            //     analyzer.report_generator.report_err_no_method(dot_expr.rhs, *concrete_proto->def);
            //     return Result::SUCCESS;
            // }

            analyzer.add_symbol_use(dot_expr.rhs.ast_node, &method);

            sir::FuncType *func_type = analyzer.create(method->type);

            if (concrete_proto.is_specialization()) {
                sir::Specializer specializer{analyzer.mod->trivial_arena, concrete_proto};
                func_type = specializer.specialize_func_type(*func_type);
            }

            // TODO: generics
            if (auto pseudo_type = func_type->return_type.match<sir::PseudoType>()) {
                if (pseudo_type->kind == sir::PseudoTypeKind::SELF_TYPE) {
                    func_type->return_type = lhs_type;
                }
            }

            out_call_expr.callee = analyzer.create(
                sir::PlaceholderExpr{
                    .ast_node = nullptr,
                    .type = func_type,
                    .kind = sir::PlaceholderExpr::GenericMethod{
                        .param = generic_param,
                        .proto_def = concrete_proto.def,
                        .decl = method,
                        .is_copy = concrete_proto.def == analyzer.std_copy_def,
                    },
                }
            );

            std::span<sir::Expr> args = analyzer.allocate_array<sir::Expr>(out_call_expr.args.size() + 1);
            args[0] = lhs;

            for (unsigned i = 0; i < out_call_expr.args.size(); i++) {
                args[i + 1] = out_call_expr.args[i];
            }

            out_call_expr.args = args;
            is_method = true;

            return Result::SUCCESS;
        }
    }

    partial_result = analyze_dot_expr_rhs(dot_expr, out_call_expr.callee);
    is_method = false;

    return partial_result;
}

std::pair<sir::FuncDecl *, sir::Concrete<sir::ProtoDef>> ExprAnalyzer::resolve_generic_method_call(
    sir::GenericParam &generic_param,
    std::string_view name
) {
    std::vector<sir::Expr> components;
    components.assign(generic_param.constraint.components.begin(), generic_param.constraint.components.end());

    if (auto narrowing = analyzer.scope_stack.top().type_narrowing) {
        if (narrowing->generic_param == &generic_param) {
            components.push_back(narrowing->constraint);
        }
    }

    for (sir::Expr component : components) {
        auto concrete_proto = component.match_concrete<sir::ProtoDef>();
        sir::Symbol method = concrete_proto->def->block.symbol_table->look_up_local(name);

        if (auto func_decl = method.match<sir::FuncDecl>()) {
            return {func_decl, *concrete_proto};
        } else if (method.is<sir::FuncDef>()) {
            // TODO
            ASSERT_UNREACHABLE;
        }
    }

    return {nullptr, {}};
}

Result ExprAnalyzer::analyze_union_case_literal(sir::CallExpr &call_expr, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    sir::UnionCase &union_case = call_expr.callee.as_symbol<sir::UnionCase>();

    for (unsigned i = 0; i < call_expr.args.size(); i++) {
        sir::Expr expected_type = union_case.fields[i].type;

        partial_result = analyze_value(call_expr.args[i], expected_type);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    out_expr = analyzer.create(
        sir::UnionCaseLiteral{
            .ast_node = nullptr,
            .type = call_expr.callee,
            .args = call_expr.args,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_static_array_type(sir::StaticArrayType &static_array_type) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = analyze_type(static_array_type.base_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    partial_result = analyze_value(static_array_type.length);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    ConstEvaluator::Output length_evaluated = ConstEvaluator{analyzer}.evaluate(static_array_type.length);
    if (length_evaluated.result != Result::SUCCESS) {
        return Result::ERROR;
    }

    // TODO: Maybe only allow unsigned types as array lengths?
    // This requires better literal coercion though, so int literals can be coerced to
    // unsigned integers for array lengths.

    if (auto int_literal = length_evaluated.expr.match<sir::IntLiteral>()) {
        if (int_literal->value < 0) {
            analyzer.report_generator.report_err_negative_array_length(static_array_type.length, int_literal->value);
            return Result::ERROR;
        }
    } else {
        analyzer.report_generator.report_err_unexpected_array_length_type(static_array_type.length);
        return Result::ERROR;
    }

    static_array_type.length = length_evaluated.expr;
    return result;
}

Result ExprAnalyzer::analyze_func_type(sir::FuncType &func_type) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::Param &param : func_type.params) {
        partial_result = analyze_type(param.type);

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    partial_result = analyze_type(func_type.return_type);

    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    return result;
}

Result ExprAnalyzer::analyze_optional_type(sir::OptionalType &optional_type, sir::Expr &out_expr) {
    Result partial_result;

    partial_result = analyze_type(optional_type.base_type);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    std::span<sir::Expr> generic_args = analyzer.create_array({optional_type.base_type});
    specialize(*analyzer.std_optional_def, generic_args, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_result_type(sir::ResultType &result_type, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = analyze_type(result_type.value_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    partial_result = analyze_type(result_type.error_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::StructDef &struct_def = *analyzer.std_result_def;
    std::span<sir::Expr> generic_args = analyzer.create_array({result_type.value_type, result_type.error_type});
    specialize(struct_def, generic_args, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_array_type(sir::ArrayType &array_type, sir::Expr &out_expr) {
    Result partial_result;

    partial_result = analyze_type(array_type.base_type);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    std::span<sir::Expr> generic_args = analyzer.create_array<sir::Expr>({array_type.base_type});
    specialize(*analyzer.std_array_def, generic_args, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_map_type(sir::MapType &map_type, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = analyze_type(map_type.key_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    partial_result = analyze_type(map_type.value_type);
    if (partial_result != Result::SUCCESS) {
        result = Result::ERROR;
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    std::span<sir::Expr> generic_args = analyzer.create_array({map_type.key_type, map_type.value_type});
    specialize(*analyzer.std_map_def, generic_args, out_expr);

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_closure_type(sir::ClosureType &closure_type) {
    closure_type.underlying_struct = analyzer.std_closure_def;
    return analyze_func_type(closure_type.func_type);
}

Result ExprAnalyzer::analyze_reference_type(sir::ReferenceType &reference_type) {
    return analyze_type(reference_type.base_type);
}

Result ExprAnalyzer::analyze_range_expr(sir::RangeExpr &range_expr) {
    Result lhs_result;
    Result rhs_result;

    lhs_result = analyze_uncoerced(range_expr.lhs);
    rhs_result = analyze_uncoerced(range_expr.rhs);

    if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::Expr lhs_type = analyzer.get_resolved_type(range_expr.lhs);
    sir::Expr rhs_type = analyzer.get_resolved_type(range_expr.rhs);
    bool can_lhs_be_coerced = can_be_coerced(range_expr.lhs);
    bool can_rhs_be_coerced = can_be_coerced(range_expr.rhs);

    if (can_lhs_be_coerced && !can_rhs_be_coerced) {
        rhs_result = ExprFinalizer(analyzer).finalize(range_expr.rhs);

        if (rhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        lhs_result = ExprFinalizer(analyzer).finalize_by_coercion(range_expr.lhs, rhs_type);
    } else if (can_rhs_be_coerced && !can_lhs_be_coerced) {
        lhs_result = ExprFinalizer(analyzer).finalize(range_expr.lhs);

        if (lhs_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        rhs_result = ExprFinalizer(analyzer).finalize_by_coercion(range_expr.rhs, lhs_type);
    } else {
        lhs_result = ExprFinalizer(analyzer).finalize(range_expr.lhs);
        rhs_result = ExprFinalizer(analyzer).finalize(range_expr.rhs);
    }

    if (lhs_result != Result::SUCCESS || rhs_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_try_expr(sir::TryExpr &try_expr) {
    Result partial_result = analyze(try_expr.value);

    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::Expr value_type = try_expr.value.get_type();
    auto value_result_def = value_type.match_specialization(*analyzer.std_result_def);

    if (!value_result_def) {
        analyzer.report_generator.report_err_cannot_use_in_try_expr(try_expr.value);
        return Result::ERROR;
    }

    sir::FuncDef *func_def = analyzer.get_decl().match<sir::FuncDef>();

    if (!func_def) {
        analyzer.report_generator.report_err_try_expr_not_allowed(try_expr);
        return Result::ERROR;
    }

    sir::Expr return_type = func_def->type.return_type;
    auto return_result_def = return_type.match_specialization(*analyzer.std_result_def);

    if (!return_result_def) {
        analyzer.report_generator.report_err_try_expr_return_type_not_result(try_expr, *func_def);
        return Result::ERROR;
    }

    sir::Expr unwrap_error = value_result_def->generic_args[1];
    sir::Expr return_error = return_result_def->generic_args[1];

    if (unwrap_error != return_error) {
        analyzer.report_generator
            .report_err_try_expr_return_error_mismatch(try_expr, unwrap_error, return_error, *func_def);
        return Result::ERROR;
    }

    try_expr.type = value_result_def->generic_args[0];
    try_expr.return_type = return_type;

    return Result::SUCCESS;
}

void ExprAnalyzer::analyze_tuple_expr(sir::TupleExpr &tuple_expr) {
    ASSERT(!tuple_expr.exprs.empty());

    for (sir::Expr &expr : tuple_expr.exprs) {
        analyze_uncoerced(expr);
    }

    if (tuple_expr.exprs[0].is_type()) {
        return;
    }

    std::span<sir::Expr> types = analyzer.allocate_array<sir::Expr>(tuple_expr.exprs.size());

    for (unsigned i = 0; i < tuple_expr.exprs.size(); i++) {
        types[i] = analyzer.get_resolved_type(tuple_expr.exprs[i]);
    }

    tuple_expr.type = analyzer.create(
        sir::TupleExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .exprs = types,
        }
    );
}

Result ExprAnalyzer::analyze_ident_expr(sir::IdentExpr &ident_expr, sir::Expr &out_expr) {
    // TODO: Add custom error message for using `self` outside of a method.

    if (ident_expr.is_completion_token()) {
        return analyze_completion_token();
    }

    SymbolLookupResult lookup_result = analyzer.symbol_ctx.look_up(ident_expr);
    sir::Symbol symbol = lookup_result.symbol;

    if (lookup_result.kind == SymbolLookupResult::Kind::ERROR) {
        out_expr = sir::create_error_value(analyzer.get_mod(), ident_expr.ast_node);
        return Result::ERROR;
    }

    if (lookup_result.kind == SymbolLookupResult::Kind::CAPTURED) {
        ClosureContext &closure_ctx = *lookup_result.closure_ctx;

        if (symbol.is_one_of<sir::Local, sir::Param>()) {
            std::optional<unsigned> captured_var_index;

            for (unsigned i = 0; i < closure_ctx.captured_vars.size(); i++) {
                if (closure_ctx.captured_vars[i] == symbol) {
                    captured_var_index = i;
                    break;
                }
            }

            if (!captured_var_index) {
                captured_var_index = closure_ctx.captured_vars.size();
                closure_ctx.captured_vars.push_back(symbol);
            }

            sir::FuncDef &func_def = analyzer.get_decl().as<sir::FuncDef>();
            sir::Symbol data_ptr_param = &func_def.type.params[0];

            out_expr = analyzer.create(
                sir::FieldExpr{
                    .ast_node = nullptr,
                    .type = symbol.get_type(),
                    .base = analyzer.create(
                        sir::UnaryExpr{
                            .ast_node = nullptr,
                            .type = closure_ctx.data_type,
                            .op = sir::UnaryOp::DEREF,
                            .value = analyzer.create(
                                sir::CastExpr{
                                    .ast_node = nullptr,
                                    .type = analyzer.create(
                                        sir::PointerType{
                                            .ast_node = nullptr,
                                            .base_type = closure_ctx.data_type,
                                        }
                                    ),
                                    .value = analyzer.create(
                                        sir::SymbolExpr{
                                            .ast_node = nullptr,
                                            .type = data_ptr_param.get_type(),
                                            .symbol = data_ptr_param,
                                        }
                                    )
                                }
                            ),
                        }
                    ),
                    .field_index = *captured_var_index,
                }
            );

            return Result::SUCCESS;
        }
    }

    if (!symbol.is<sir::OverloadSet>()) {
        analyzer.add_symbol_use(ident_expr.ast_node, symbol);
    }

    if (auto generic_arg = symbol.match<sir::GenericArg>()) {
        if (generic_arg->value.is<sir::StringLiteral>()) {
            out_expr = sir::Cloner{analyzer.get_mod()}.clone_expr(generic_arg->value);
        } else {
            out_expr = generic_arg->value;
        }

        return Result::SUCCESS;
    }

    if (flags & ANALYZE_SYMBOL_INTERFACES) {
        Result partial_result = analyzer.ensure_interface_analyzed(symbol, ident_expr.ast_node);

        if (partial_result != Result::SUCCESS) {
            return partial_result;
        }
    }

    out_expr = analyzer.create(
        sir::SymbolExpr{
            .ast_node = ident_expr.ast_node,
            .type = symbol.get_type(),
            .symbol = symbol,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr) {
    Result result;

    result = analyze(star_expr.value);
    if (result != Result::SUCCESS) {
        return result;
    }

    if (star_expr.value.is_type()) {
        out_expr = analyzer.create(
            sir::PointerType{
                .ast_node = star_expr.ast_node,
                .base_type = star_expr.value,
            }
        );
    } else {
        sir::Expr value_type = analyzer.get_resolved_type(star_expr.value);

        if (auto struct_def = value_type.match_symbol<sir::StructDef>()) {
            std::string_view impl_name = sir::MagicMethods::look_up(sir::UnaryOp::DEREF);
            sir::Symbol symbol = struct_def->block.symbol_table->look_up_local(impl_name);

            if (!symbol) {
                analyzer.report_generator.report_err_operator_overload_not_found(star_expr);
                return Result::ERROR;
            }

            std::span<sir::Expr> call_args = analyzer.create_array<sir::Expr>({star_expr.value});
            return analyze_operator_overload_call(symbol, call_args, out_expr);
        }

        if (auto pointer_type = value_type.match<sir::PointerType>()) {
            out_expr = analyzer.create(
                sir::UnaryExpr{
                    .ast_node = star_expr.ast_node,
                    .type = pointer_type->base_type,
                    .op = sir::UnaryOp::DEREF,
                    .value = star_expr.value,
                }
            );
        } else if (auto reference_type = value_type.match<sir::ReferenceType>()) {
            out_expr = analyzer.create(
                sir::UnaryExpr{
                    .ast_node = star_expr.ast_node,
                    .type = reference_type->base_type,
                    .op = sir::UnaryOp::DEREF,
                    .value = star_expr.value,
                }
            );
        } else {
            analyzer.report_generator.report_err_cannot_deref(star_expr.value);
            return Result::ERROR;
        }
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    partial_result = analyze(bracket_expr.lhs);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    for (sir::Expr &expr : bracket_expr.rhs) {
        partial_result = analyze(expr);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    if (result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (auto func_def = bracket_expr.lhs.match_symbol<sir::FuncDef>()) {
        if (func_def->is_generic()) {
            if (bracket_expr.rhs.size() != func_def->generic_params.size()) {
                analyzer.report_generator.report_err_unexpected_generic_arg_count(bracket_expr, *func_def);
                return Result::ERROR;
            }

            for (unsigned i = 0; i < func_def->generic_params.size(); i++) {
                ASTNode *ast_node = bracket_expr.rhs[i].get_ast_node();
                RESULT_MERGE(result, check_type_constraint(ast_node, func_def->generic_params, bracket_expr.rhs, i));
            }

            RESULT_RETURN_ON_ERROR(result);
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
    } else if (auto proto_def = bracket_expr.lhs.match_symbol<sir::ProtoDef>()) {
        if (proto_def->is_generic()) {
            if (bracket_expr.rhs.size() != proto_def->generic_params.size()) {
                analyzer.report_generator.report_err_unexpected_generic_arg_count(bracket_expr, *proto_def);
                return Result::ERROR;
            }

            return specialize(*proto_def, bracket_expr.rhs, out_expr);
        }
    }

    sir::Expr lhs_type = analyzer.get_resolved_type(bracket_expr.lhs);

    if (auto pointer_type = lhs_type.match<sir::PointerType>()) {
        return analyze_index_expr(bracket_expr, pointer_type->base_type, out_expr);
    } else if (auto static_array_type = lhs_type.match<sir::StaticArrayType>()) {
        return analyze_index_expr(bracket_expr, static_array_type->base_type, out_expr);
    } else if (auto concrete_struct = lhs_type.match_concrete<sir::StructDef>()) {
        sir::StructDef &struct_def = *concrete_struct->def;

        ExprProperties properties = ExprPropertyAnalyzer().analyze(bracket_expr.lhs);
        bool is_mutable = properties.mutability == Mutability::MUTABLE;

        std::string_view symbol_name = is_mutable ? sir::MagicMethods::OP_MUT_INDEX : sir::MagicMethods::OP_INDEX;
        sir::Symbol symbol = struct_def.block.symbol_table->look_up(symbol_name);

        // If the value is mutable, also try the operator overload for immutable indexing.
        if (!symbol && is_mutable) {
            symbol = struct_def.block.symbol_table->look_up(sir::MagicMethods::OP_INDEX);
        }

        if (!symbol) {
            analyzer.report_generator.report_err_operator_overload_not_found(bracket_expr, is_mutable);
            return Result::ERROR;
        }

        std::span<sir::Expr> call_args = analyzer.allocate_array<sir::Expr>(bracket_expr.rhs.size() + 1);
        call_args[0] = bracket_expr.lhs;

        for (unsigned i = 0; i < bracket_expr.rhs.size(); i++) {
            call_args[i + 1] = bracket_expr.rhs[i];
        }

        result = analyze_operator_overload_call(symbol, call_args, out_expr, concrete_struct->generic_args);
        return result;
    } else {
        analyzer.report_generator.report_err_expected_generic_or_indexable(bracket_expr.lhs);
        return Result::ERROR;
    }
}

Result ExprAnalyzer::analyze_dot_expr(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    if (!dot_expr.lhs) {
        return Result::SUCCESS;
    }

    Result result = analyze(dot_expr.lhs);
    if (result != Result::SUCCESS) {
        return result;
    }

    if (analyzer.mode == Mode::COMPLETION && dot_expr.rhs.is_completion_token()) {
        analyzer.completion_context = CompleteAfterDot{
            .lhs = dot_expr.lhs,
        };

        return Result::SUCCESS;
    }

    result = analyze_dot_expr_rhs(dot_expr, out_expr);
    return result;
}

Result ExprAnalyzer::analyze_meta_access(sir::MetaAccess &meta_access) {
    return analyze(meta_access.expr);
}

Result ExprAnalyzer::analyze_meta_field_expr(sir::MetaFieldExpr &meta_field_expr, bool in_call /* = false */) {
    sir::Expr &base = meta_field_expr.base.as<sir::MetaAccess>().expr;

    Result result = analyze(base);
    RESULT_RETURN_ON_ERROR(result);

    if (meta_field_expr.field.value == "size") {
        meta_field_expr.type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::USIZE);
    } else if (meta_field_expr.field.value == "name") {
        meta_field_expr.type = analyzer.create(
            sir::PointerType{
                .ast_node = nullptr,
                .base_type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::U8),
            }
        );
    } else if (Utils::is_one_of(meta_field_expr.field.value, {"is_enum"})) {
        meta_field_expr.type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL);
    } else if (meta_field_expr.field.value == "variants") {
        sir::Expr string_type = analyzer.create(
            sir::PointerType{
                .ast_node = nullptr,
                .base_type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::U8),
            }
        );

        sir::Expr tuple_type = analyzer.create(
            sir::TupleExpr{
                .ast_node = nullptr,
                .type = nullptr,
                .exprs = analyzer.create_array({string_type, base}),
            }
        );

        sir::Expr array_length = analyzer.create(
            sir::MetaFieldExpr{
                .ast_node = nullptr,
                .type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::USIZE),
                .base = meta_field_expr.base,
                .field{
                    .ast_node = nullptr,
                    .value = "num_variants",
                },
            }
        );

        meta_field_expr.type = analyzer.create(
            sir::StaticArrayType{
                .ast_node = nullptr,
                .base_type = tuple_type,
                .length = array_length,
            }
        );
    } else if (meta_field_expr.field.value == "num_variants") {
        meta_field_expr.type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::USIZE);
    } else if (!in_call) {
        analyzer.report_generator.report_err_invalid_meta_field(meta_field_expr);
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_meta_call_expr(sir::MetaCallExpr &meta_call_expr, sir::Expr &out_expr) {
    if (flags & DONT_EVAL_META_EXPRS) {
        return MetaExprEvaluator(analyzer).evaluate(meta_call_expr, out_expr);
    }

    Result result = Result::SUCCESS;

    sir::MetaFieldExpr &meta_field_expr = meta_call_expr.callee.as<sir::MetaFieldExpr>();
    RESULT_RETURN_ON_ERROR(analyze_meta_field_expr(meta_field_expr, true));

    for (sir::Expr &arg : meta_call_expr.args) {
        RESULT_MERGE(result, analyze(arg));
    }

    RESULT_RETURN_ON_ERROR(result);

    if (meta_field_expr.field.value == "impl_of") {
        sir::Expr expr = meta_field_expr.base.as<sir::MetaAccess>().expr;

        out_expr = analyzer.create(
            sir::TypeGuardExpr{
                .ast_node = meta_call_expr.ast_node,
                .type = sir::create_primitive_type(*analyzer.mod, sir::Primitive::BOOL),
                .generic_param = &expr.as_symbol<sir::GenericParam>(),
                .constraint = meta_call_expr.args[0],
            }
        );

        type_narrowing = sir::TypeNarrowing{
            .generic_param = &expr.as_symbol<sir::GenericParam>(),
            .constraint = meta_call_expr.args[0],
        };

        meta_call_expr.type = sir::create_primitive_type(analyzer.get_mod(), sir::Primitive::BOOL);
    } else {
        analyzer.report_generator.report_err_invalid_meta_method(meta_call_expr);
        return Result::ERROR;
    }

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_completion_token() {
    if (analyzer.is_in_stmt_block()) {
        analyzer.completion_context = CompleteInBlock{
            .block = &analyzer.get_block(),
        };
    } else if (analyzer.is_in_decl()) {
        analyzer.completion_context = CompleteInDeclBlock{
            .decl_block = &analyzer.get_decl_block(),
        };
    } else {
        analyzer.completion_context = CompleteInDeclBlock{
            .decl_block = &analyzer.get_mod().block,
        };
    }

    // Return an error so as not to continue analysing expressions that contain
    // a completion token and cannot be valid anyway.
    return Result::ERROR;
}

Result ExprAnalyzer::analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr) {
    if (dot_expr.rhs.is_error_token()) {
        return Result::ERROR;
    }

    sir::DeclBlock *decl_block = dot_expr.lhs.get_decl_block();

    if (decl_block) {
        SymbolLookupResult lookup_result = analyzer.symbol_ctx.look_up_rhs_local(dot_expr);

        if (lookup_result.symbol) {
            analyzer.add_symbol_use(dot_expr.rhs.ast_node, lookup_result.symbol);

            if (auto specialize_expr = dot_expr.lhs.match<sir::SpecializeExpr>()) {
                sir::Specializer specializer{
                    analyzer.mod->trivial_arena,
                    specialize_expr->symbol.get_generic_params(),
                    specialize_expr->args,
                };

                out_expr = analyzer.create(
                    sir::SpecializeExpr{
                        .ast_node = dot_expr.ast_node,
                        .type = specializer.specialize_expr(lookup_result.symbol.get_type()),
                        .symbol = lookup_result.symbol,
                        .args = specialize_expr->args,
                    }
                );
            } else {
                out_expr = analyzer.create(
                    sir::SymbolExpr{
                        .ast_node = dot_expr.ast_node,
                        .type = lookup_result.symbol.get_type(),
                        .symbol = lookup_result.symbol,
                    }
                );
            }

            return Result::SUCCESS;
        }

        if (auto mod = dot_expr.lhs.match_symbol<sir::Module>()) {
            ModulePath sub_mod_path = mod->path;
            sub_mod_path.append(dot_expr.rhs.value);
            sir::Module *sub_mod = analyzer.sir_unit.mods_by_path[sub_mod_path];

            if (sub_mod) {
                analyzer.add_symbol_use(dot_expr.rhs.ast_node, sub_mod);

                out_expr = analyzer.create(
                    sir::SymbolExpr{
                        .ast_node = dot_expr.ast_node,
                        .type = nullptr,
                        .symbol = sub_mod,
                    }
                );

                return Result::SUCCESS;
            }
        }

        out_expr = sir::create_error_value(analyzer.get_mod(), dot_expr.ast_node);
        analyzer.report_generator.report_err_symbol_not_found(dot_expr.rhs);
        return Result::ERROR;
    }

    sir::Expr lhs = dot_expr.lhs;
    sir::Expr lhs_type = analyzer.get_resolved_type(dot_expr.lhs);

    while (auto pointer_type = lhs_type.match<sir::PointerType>()) {
        lhs = analyzer.create(
            sir::UnaryExpr{
                .ast_node = nullptr,
                .type = pointer_type->base_type,
                .op = sir::UnaryOp::DEREF,
                .value = lhs,
            }
        );

        lhs_type = pointer_type->base_type;
    }

    if (!lhs_type) {
        return Result::ERROR;
    } else if (auto concrete_struct = lhs_type.match_concrete<sir::StructDef>()) {
        sir::StructDef *struct_def = concrete_struct->def;
        sir::StructField *field = struct_def->find_field(dot_expr.rhs.value);

        if (!field) {
            analyzer.report_generator.report_err_no_field(dot_expr.rhs, *struct_def);
            return Result::ERROR;
        }

        sir::Expr field_type = field->type;

        if (!concrete_struct->generic_args.empty()) {
            sir::Specializer specializer{
                analyzer.mod->trivial_arena,
                struct_def->generic_params,
                concrete_struct->generic_args,
            };

            field_type = specializer.specialize_expr(field_type);
        }

        out_expr = analyzer.create(
            sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field_type,
                .base = lhs,
                .field_index = field->index,
            }
        );

        analyzer.add_symbol_use(dot_expr.rhs.ast_node, field);
        return Result::SUCCESS;
    } else if (auto union_case = lhs_type.match_symbol<sir::UnionCase>()) {
        std::optional<unsigned> field_index = union_case->find_field(dot_expr.rhs.value);

        if (!field_index) {
            analyzer.report_generator.report_err_no_field(dot_expr.rhs, *union_case);
            return Result::ERROR;
        }

        sir::UnionCaseField &field = union_case->fields[*field_index];

        out_expr = analyzer.create(
            sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = field.type,
                .base = lhs,
                .field_index = *field_index,
            }
        );

        analyzer.add_symbol_use(dot_expr.rhs.ast_node, &field);
        return Result::SUCCESS;
    } else if (auto tuple_expr = lhs_type.match<sir::TupleExpr>()) {
        std::optional<std::uint64_t> field_parsed = Utils::parse_u64(dot_expr.rhs.value);

        if (!field_parsed) {
            analyzer.report_generator.report_err_no_field(dot_expr.rhs, *tuple_expr);
            return Result::ERROR;
        }

        unsigned field_index = static_cast<unsigned>(*field_parsed);

        if (field_index >= tuple_expr->exprs.size()) {
            analyzer.report_generator.report_err_no_field(dot_expr.rhs, *tuple_expr);
            return Result::ERROR;
        }

        out_expr = analyzer.create(
            sir::FieldExpr{
                .ast_node = dot_expr.ast_node,
                .type = tuple_expr->exprs[field_index],
                .base = lhs,
                .field_index = field_index,
            }
        );

        return Result::SUCCESS;
    }

    analyzer.report_generator.report_err_no_members(dot_expr);
    return Result::ERROR;
}

Result ExprAnalyzer::analyze_index_expr(sir::BracketExpr &bracket_expr, sir::Expr base_type, sir::Expr &out_expr) {
    if (bracket_expr.rhs.size() == 0) {
        analyzer.report_generator.report_err_expected_index(bracket_expr);
        return Result::ERROR;
    } else if (bracket_expr.rhs.size() > 1) {
        analyzer.report_generator.report_err_too_many_indices(bracket_expr);
        return Result::ERROR;
    }

    out_expr = analyzer.create(
        sir::IndexExpr{
            .ast_node = bracket_expr.ast_node,
            .type = base_type,
            .base = bracket_expr.lhs,
            .index = create_isize_cast(bracket_expr.rhs[0]),
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::analyze_operator_overload_call(
    sir::Symbol symbol,
    std::span<sir::Expr> args,
    sir::Expr &inout_expr,
    std::span<sir::Expr> generic_args /* = {} */
) {
    sir::FuncDef *impl;

    if (auto func_def = symbol.match<sir::FuncDef>()) {
        impl = func_def;
    } else if (auto overload_set = symbol.match<sir::OverloadSet>()) {
        impl = OverloadResolver(analyzer).resolve(*overload_set, args);

        if (!impl) {
            analyzer.report_generator.report_err_no_matching_overload(inout_expr, *overload_set);
            return Result::ERROR;
        }
    } else {
        ASSERT_UNREACHABLE;
    }

    sir::Concrete<sir::FuncDef> concrete_func{
        .def = impl,
        .generic_args = generic_args,
    };

    return analyze_operator_overload_call(concrete_func, args, inout_expr);
}

Result ExprAnalyzer::analyze_operator_overload_call(
    sir::Concrete<sir::FuncDef> concrete_func,
    std::span<sir::Expr> args,
    sir::Expr &inout_expr
) {
    Result partial_result;

    sir::FuncDef &func_def = *concrete_func.def;
    ASSERT(!(func_def.type.params[0].attrs && func_def.type.params[0].attrs->byval));

    // FIXME
    // analyzer.add_symbol_use(inout_expr.get_ast_node(), impl);

    sir::CallExpr *call_expr = sir::create_call(analyzer.get_mod(), concrete_func, args);
    sir::FuncType &func_type = call_expr->callee.get_type().as<sir::FuncType>();

    partial_result = finalize_call_expr_args(*call_expr, func_type, &func_def);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    inout_expr = call_expr;
    return Result::SUCCESS;
}

Result ExprAnalyzer::finalize_call_expr_args(
    sir::CallExpr &call_expr,
    sir::FuncType &func_type,
    sir::FuncDef *func_def
) {
    Result result = Result::SUCCESS;
    Result partial_result;

    if (call_expr.args.size() != func_type.params.size()) {
        analyzer.report_generator.report_err_unexpected_arg_count(call_expr, func_type.params.size(), func_def);
        return Result::ERROR;
    }

    for (unsigned i = 0; i < call_expr.args.size(); i++) {
        sir::Expr &arg = call_expr.args[i];
        sir::Expr expected_type = func_type.params[i].type;

        // If we're calling a method on a generic type, the type of the `self` parameter
        // is the type that is substituted for `T` and not a pointer to a `proto`.
        // FIXME: `mut self`, `@byval self`
        if (i == 0 && call_expr.callee.is<sir::PlaceholderExpr>()) {
            auto &generic_method =
                std::get<sir::PlaceholderExpr::GenericMethod>(call_expr.callee.as<sir::PlaceholderExpr>().kind);

            expected_type = analyzer.create(
                sir::ReferenceType{
                    .ast_node = nullptr,
                    .mut = false,
                    .base_type = analyzer.create(
                        sir::SymbolExpr{
                            .ast_node = nullptr,
                            .type = nullptr,
                            .symbol = generic_method.param,
                        }
                    ),
                }
            );
        }

        partial_result = ExprFinalizer(analyzer).finalize_by_coercion(arg, expected_type);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

Result ExprAnalyzer::specialize(sir::FuncDef &func_def, std::span<sir::Expr> generic_args, sir::Expr &inout_expr) {
    ASSERT(func_def.generic_params.size() == generic_args.size());

    sir::Specializer specializer{analyzer.mod->trivial_arena, func_def.generic_params, generic_args};

    inout_expr = analyzer.create(
        sir::SpecializeExpr{
            .ast_node = inout_expr.get_ast_node(),
            .type = specializer.specialize_func_type(func_def.type),
            .symbol = &func_def,
            .args = generic_args,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::specialize(sir::StructDef &struct_def, std::span<sir::Expr> generic_args, sir::Expr &inout_expr) {
    ASSERT(struct_def.generic_params.size() == generic_args.size());

    sir::Specializer specializer{analyzer.mod->trivial_arena, struct_def.generic_params, generic_args};

    inout_expr = analyzer.create(
        sir::SpecializeExpr{
            .ast_node = inout_expr.get_ast_node(),
            .type = nullptr,
            .symbol = &struct_def,
            .args = generic_args,
        }
    );

    return Result::SUCCESS;
}

Result ExprAnalyzer::specialize(sir::ProtoDef &proto_def, std::span<sir::Expr> generic_args, sir::Expr &inout_expr) {
    ASSERT(proto_def.generic_params.size() == generic_args.size());

    sir::Specializer specializer{analyzer.mod->trivial_arena, proto_def.generic_params, generic_args};

    inout_expr = analyzer.create(
        sir::SpecializeExpr{
            .ast_node = inout_expr.get_ast_node(),
            .type = nullptr,
            .symbol = &proto_def,
            .args = generic_args,
        }
    );

    return Result::SUCCESS;
}

void ExprAnalyzer::create_method_call(
    sir::CallExpr &call_expr,
    sir::Expr lhs,
    const sir::Ident &rhs,
    sir::Symbol method
) {
    analyzer.add_symbol_use(rhs.ast_node, method);

    if (auto specialize_expr = lhs.get_type().match<sir::SpecializeExpr>()) {
        sir::Specializer specializer{
            analyzer.get_mod().trivial_arena,
            specialize_expr->symbol.get_generic_params(),
            specialize_expr->args,
        };

        call_expr.callee = analyzer.create(
            sir::SpecializeExpr{
                .ast_node = rhs.ast_node,
                .type = specializer.specialize_expr(method.get_type()),
                .symbol = method,
                .args = specialize_expr->args,
            }
        );
    } else {
        call_expr.callee = analyzer.create(
            sir::SymbolExpr{
                .ast_node = rhs.ast_node,
                .type = method.get_type(),
                .symbol = method,
            }
        );
    }

    std::span<sir::Expr> args = analyzer.allocate_array<sir::Expr>(call_expr.args.size() + 1);
    args[0] = lhs;

    for (unsigned i = 0; i < call_expr.args.size(); i++) {
        args[i + 1] = call_expr.args[i];
    }

    call_expr.args = args;
}

sir::Expr ExprAnalyzer::create_isize_cast(sir::Expr value) {
    // TODO: Add isize primitive type
    sir::Primitive target_type = analyzer.target->get_descr().is_wasm() ? sir::Primitive::I32 : sir::Primitive::I64;

    return analyzer.create(
        sir::CastExpr{
            .ast_node = value.get_ast_node(),
            .type = analyzer.create(
                sir::PrimitiveType{
                    .ast_node = nullptr,
                    .primitive = target_type,
                }
            ),
            .value = value,
        }
    );
}

ExprAnalyzer::BinaryOpType ExprAnalyzer::get_binary_op_type(sir::BinaryOp op) {
    switch (op) {
        case sir::BinaryOp::ADD:
        case sir::BinaryOp::SUB:
        case sir::BinaryOp::MUL:
        case sir::BinaryOp::DIV:
        case sir::BinaryOp::MOD:
        case sir::BinaryOp::BIT_AND:
        case sir::BinaryOp::BIT_OR:
        case sir::BinaryOp::BIT_XOR:
        case sir::BinaryOp::SHL:
        case sir::BinaryOp::SHR: return BinaryOpType::ARITHMETIC;
        case sir::BinaryOp::EQ:
        case sir::BinaryOp::NE: return BinaryOpType::EQUALITY_COMP;
        case sir::BinaryOp::GT:
        case sir::BinaryOp::LT:
        case sir::BinaryOp::GE:
        case sir::BinaryOp::LE: return BinaryOpType::ORDER_COMP;
        case sir::BinaryOp::AND:
        case sir::BinaryOp::OR: return BinaryOpType::LOGICAL;
    }
}

bool ExprAnalyzer::can_be_coerced(sir::Expr value) {
    if (auto dot_expr = value.match<sir::DotExpr>()) {
        return !dot_expr->lhs;
    }

    return analyzer.get_resolved_type(value).is<sir::PseudoType>();
}

Result ExprAnalyzer::check_type_constraint(
    ASTNode *ast_node,
    std::span<sir::GenericParam *> params,
    std::span<sir::Expr> args,
    unsigned index
) {
    sir::GenericParam &param = *params[index];
    sir::Expr arg = args[index];

    if (param.kind != sir::GenericParamKind::TYPE) {
        return Result::SUCCESS;
    }

    utils::Arena arena;
    sir::Specializer specializer{arena, params, args};

    if (satisfies_type_constraint(param.constraint, arg, specializer)) {
        return Result::SUCCESS;
    } else {
        analyzer.report_generator.report_err_constraint_not_satisfied(ast_node, arg, param);
        return Result::ERROR;
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
