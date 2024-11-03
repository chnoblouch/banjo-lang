#include "expr_finalizer.hpp"

#include "banjo/sema2/generics_specializer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_create.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

ExprFinalizer::ExprFinalizer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

Result ExprFinalizer::finalize_by_coercion(sir::Expr &expr, sir::Expr expected_type) {
    if (auto undefined_literal = expr.match<sir::UndefinedLiteral>()) {
        undefined_literal->type = expected_type;
        return Result::SUCCESS;
    }

    if (expr.get_type() != expected_type) {
        if (expected_type.is_symbol<sir::UnionDef>()) {
            return coerce_to_union(expr, expected_type);
        } else if (auto proto_def = expected_type.match_proto_ptr()) {
            return coerce_to_proto_ptr(expr, *proto_def, expected_type);
        } else if (auto specialization = analyzer.as_std_optional_specialization(expected_type)) {
            return coerce_to_std_optional(expr, *specialization);
        } else if (auto specialization = analyzer.as_std_result_specialization(expected_type)) {
            return coerce_to_std_result(expr, *specialization);
        }
    }

    if (auto array_literal = expr.match<sir::ArrayLiteral>()) {
        return finalize_coercion(*array_literal, expected_type, expr);
    } else if (auto struct_literal = expr.match<sir::StructLiteral>()) {
        return finalize_coercion(*struct_literal, expected_type);
    } else if (auto map_literal = expr.match<sir::MapLiteral>()) {
        return finalize_coercion(*map_literal, expected_type, expr);
    } else if (auto tuple_literal = expr.match<sir::TupleExpr>()) {
        return finalize_coercion(*tuple_literal, expected_type);
    }

    if (expr.get_type() == expected_type) {
        return Result::SUCCESS;
    }

    if (auto int_literal = expr.match<sir::IntLiteral>()) {
        return finalize_coercion(*int_literal, expected_type);
    } else if (auto fp_literal = expr.match<sir::FPLiteral>()) {
        return finalize_coercion(*fp_literal, expected_type);
    } else if (auto null_literal = expr.match<sir::NullLiteral>()) {
        return finalize_coercion(*null_literal, expected_type);
    } else if (auto none_literal = expr.match<sir::NoneLiteral>()) {
        analyzer.report_generator.report_err_cannot_coerce(*none_literal, expected_type);
        return Result::ERROR;
    } else if (auto string_literal = expr.match<sir::StringLiteral>()) {
        return finalize_coercion(*string_literal, expected_type, expr);
    } else if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        return finalize_coercion(*unary_expr, expected_type);
    } else {
        analyzer.report_generator.report_err_type_mismatch(expr, expected_type, expr.get_type());
        return Result::ERROR;
    }
}

Result ExprFinalizer::coerce_to_union(sir::Expr &inout_expr, sir::Expr union_type) {
    Result partial_result;

    partial_result = finalize(inout_expr);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    // FIXME: Handle the case where coercion is not possible.

    inout_expr = analyzer.create_expr(sir::CoercionExpr{
        .ast_node = nullptr,
        .type = union_type,
        .value = inout_expr,
    });

    return Result::SUCCESS;
}

Result ExprFinalizer::coerce_to_proto_ptr(sir::Expr &inout_expr, sir::ProtoDef &proto_def, sir::Expr proto_ptr_type) {
    Result partial_result;

    partial_result = finalize(inout_expr);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    sir::Expr type = inout_expr.get_type();
    if (type == proto_ptr_type) {
        return Result::SUCCESS;
    }
    
    bool is_valid = false;

    if (auto pointer_type = type.match<sir::PointerType>()) {
        if (auto struct_def = pointer_type->base_type.match_symbol<sir::StructDef>()) {
            is_valid = struct_def->has_impl_for(proto_def);
        }
    }

    if (!is_valid) {
        analyzer.report_generator.report_err_cannot_coerce(inout_expr, proto_ptr_type);
        return Result::ERROR;
    }

    inout_expr = analyzer.create_expr(sir::CoercionExpr{
        .ast_node = nullptr,
        .type = proto_ptr_type,
        .value = inout_expr,
    });

    return Result::SUCCESS;
}

Result ExprFinalizer::coerce_to_std_optional(
    sir::Expr &inout_expr,
    sir::Specialization<sir::StructDef> &specialization
) {
    Result partial_result;

    if (inout_expr.is<sir::NoneLiteral>()) {
        create_std_optional_none(specialization, inout_expr);
        return Result::SUCCESS;
    }

    partial_result = finalize_by_coercion(inout_expr, specialization.args[0]);
    if (partial_result != Result::SUCCESS) {
        return partial_result;
    }

    create_std_optional_some(specialization, inout_expr);
    return Result::SUCCESS;
}

Result ExprFinalizer::coerce_to_std_result(sir::Expr &inout_expr, sir::Specialization<sir::StructDef> &specialization) {
    Result partial_result;

    partial_result = finalize(inout_expr);
    if (partial_result != Result::SUCCESS) {
        return partial_result;
    }

    sir::Expr type = inout_expr.get_type();

    if (type == specialization.args[0]) {
        create_std_result_success(specialization, inout_expr);
        return Result::SUCCESS;
    } else if (type == specialization.args[1]) {
        create_std_result_failure(specialization, inout_expr);
        return Result::SUCCESS;
    }

    // FIXME: error handling
    ASSERT_UNREACHABLE;
}

Result ExprFinalizer::finalize_coercion(sir::IntLiteral &int_literal, sir::Expr type) {
    if (!(type.is_int_type() || type.is_addr_like_type())) {
        analyzer.report_generator.report_err_cannot_coerce(int_literal, type);
        return Result::ERROR;
    }

    int_literal.type = type;
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_coercion(sir::FPLiteral &fp_literal, sir::Expr type) {
    if (!type.is_fp_type()) {
        analyzer.report_generator.report_err_cannot_coerce(fp_literal, type);
        return Result::ERROR;
    }

    fp_literal.type = type;
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_coercion(sir::NullLiteral &null_literal, sir::Expr type) {
    if (!type.is_addr_like_type()) {
        analyzer.report_generator.report_err_cannot_coerce(null_literal, type);
        return Result::ERROR;
    }

    null_literal.type = type;
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_coercion(sir::ArrayLiteral &array_literal, sir::Expr type, sir::Expr &out_expr) {
    if (auto specialization = analyzer.as_std_array_specialization(type)) {
        sir::Expr element_type = specialization->args[0];

        array_literal.type = type;
        finalize_array_literal_elements(array_literal, element_type);
        create_std_array(array_literal, element_type, out_expr);
        return Result::SUCCESS;
    } else if (auto static_array_type = type.match<sir::StaticArrayType>()) {
        array_literal.type = type;
        finalize_array_literal_elements(array_literal, static_array_type->base_type);

        unsigned length = array_literal.values.size();
        unsigned expected_length = static_array_type->length.as<sir::IntLiteral>().value.to_u64();

        if (length != expected_length) {
            analyzer.report_generator.report_err_unexpected_array_length(array_literal, expected_length);
            return Result::ERROR;
        }

        return Result::SUCCESS;
    } else {
        analyzer.report_generator.report_err_cannot_coerce(array_literal, type);
        return Result::ERROR;
    }
}

Result ExprFinalizer::finalize_coercion(sir::StringLiteral &string_literal, sir::Expr type, sir::Expr &out_expr) {
    if (type.is_u8_ptr()) {
        string_literal.type = type;
        return Result::SUCCESS;
    } else if (type.is_symbol(analyzer.find_std_string())) {
        create_std_string(string_literal, out_expr);
        return Result::SUCCESS;
    } else {
        analyzer.report_generator.report_err_cannot_coerce(string_literal, type);
        return Result::ERROR;
    }
}

Result ExprFinalizer::finalize_coercion(sir::StructLiteral &struct_literal, sir::Expr type) {
    if (!struct_literal.type) {
        if (type.is_symbol<sir::StructDef>()) {
            struct_literal.type = type;
        } else {
            analyzer.report_generator.report_err_cannot_coerce(struct_literal, type);
            return Result::ERROR;
        }
    }

    finalize_struct_literal_fields(struct_literal);
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_coercion(sir::TupleExpr &tuple_literal, sir::Expr type) {
    Result partial_result;
    Result result;

    if (auto tuple_type = type.match<sir::TupleExpr>()) {
        if (tuple_literal.exprs.size() == tuple_type->exprs.size()) {
            for (unsigned i = 0; i < tuple_literal.exprs.size(); i++) {
                sir::Expr &value = tuple_literal.exprs[i];

                partial_result = finalize_by_coercion(value, tuple_type->exprs[i]);
                if (partial_result != Result::SUCCESS) {
                    result = Result::ERROR;
                }

                tuple_literal.type.as<sir::TupleExpr>().exprs[i] = value.get_type();
            }

            if (tuple_literal.type != type) {
                analyzer.report_generator.report_err_type_mismatch(&tuple_literal, type, tuple_literal.type);
                return Result::ERROR;
            }

            tuple_literal.type = type;
            return result;
        }
    }

    analyzer.report_generator.report_err_type_mismatch(&tuple_literal, type, tuple_literal.type);
    return Result::ERROR;
}

Result ExprFinalizer::finalize_coercion(sir::MapLiteral &map_literal, sir::Expr type, sir::Expr &out_expr) {
    if (auto specialization = analyzer.as_std_map_specialization(type)) {
        sir::Expr key_type = specialization->args[0];
        sir::Expr value_type = specialization->args[1];

        map_literal.type = type;
        finalize_map_literal_elements(map_literal, key_type, value_type);
        create_std_map(map_literal, out_expr);
        return Result::SUCCESS;
    } else {
        analyzer.report_generator.report_err_cannot_coerce(map_literal, type);
        return Result::ERROR;
    }
}

Result ExprFinalizer::finalize_coercion(sir::UnaryExpr &unary_expr, sir::Expr type) {
    finalize_by_coercion(unary_expr.value, type);

    if (unary_expr.type.is<sir::PseudoType>()) {
        unary_expr.type = unary_expr.value.get_type();
    }

    return Result::SUCCESS;
}

Result ExprFinalizer::finalize(sir::Expr &expr) {
    if (auto int_literal = expr.match<sir::IntLiteral>()) return finalize_default(*int_literal);
    else if (auto fp_literal = expr.match<sir::FPLiteral>()) return finalize_default(*fp_literal);
    else if (auto null_literal = expr.match<sir::NullLiteral>()) return finalize_default(*null_literal);
    else if (auto none_literal = expr.match<sir::NoneLiteral>()) return finalize_default(*none_literal);
    else if (auto undefined_literal = expr.match<sir::UndefinedLiteral>()) return finalize_default(*undefined_literal);
    else if (auto array_literal = expr.match<sir::ArrayLiteral>()) return finalize_default(*array_literal, expr);
    else if (auto string_literal = expr.match<sir::StringLiteral>()) return finalize_default(*string_literal, expr);
    else if (auto struct_literal = expr.match<sir::StructLiteral>()) return finalize_default(*struct_literal);
    else if (auto tuple_literal = expr.match<sir::TupleExpr>()) return finalize_default(*tuple_literal);
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) return finalize_default(*unary_expr);
    else if (auto map_literal = expr.match<sir::MapLiteral>()) return finalize_default(*map_literal, expr);
    else return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::IntLiteral &int_literal) {
    int_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::I32,
    });

    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::FPLiteral &fp_literal) {
    fp_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::F32,
    });

    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::NullLiteral &null_literal) {
    null_literal.type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::ADDR,
    });

    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::NoneLiteral &none_literal) {
    analyzer.report_generator.report_err_cannot_infer_type(none_literal);
    return Result::ERROR;
}

Result ExprFinalizer::finalize_default(sir::UndefinedLiteral &undefined_literal) {
    analyzer.report_generator.report_err_cannot_infer_type(undefined_literal);
    return Result::ERROR;
}

Result ExprFinalizer::finalize_default(sir::ArrayLiteral &array_literal, sir::Expr &out_expr) {
    if (array_literal.values.empty()) {
        analyzer.report_generator.report_err_cannot_infer_type(array_literal);
        return Result::ERROR;
    }

    finalize_array_literal_elements(array_literal, nullptr);
    create_std_array(array_literal, array_literal.values[0].get_type(), out_expr);
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::StringLiteral &string_literal, sir::Expr &out_expr) {
    create_std_string(string_literal, out_expr);
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::StructLiteral &struct_literal) {
    if (!struct_literal.type) {
        analyzer.report_generator.report_err_cannot_infer_type(struct_literal);
        return Result::ERROR;
    }

    finalize_struct_literal_fields(struct_literal);
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::TupleExpr &tuple_literal) {
    if (!tuple_literal.type) {
        return Result::SUCCESS;
    }

    sir::TupleExpr &tuple_type = tuple_literal.type.as<sir::TupleExpr>();

    for (unsigned i = 0; i < tuple_literal.exprs.size(); i++) {
        sir::Expr &value = tuple_literal.exprs[i];
        finalize(value);
        tuple_type.exprs[i] = value.get_type();
    }

    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::MapLiteral &map_literal, sir::Expr &out_expr) {
    finalize_map_literal_elements(map_literal, nullptr, nullptr);
    create_std_map(map_literal, out_expr);
    return Result::SUCCESS;
}

Result ExprFinalizer::finalize_default(sir::UnaryExpr &unary_expr) {
    finalize(unary_expr.value);

    if (unary_expr.type.is<sir::PseudoType>()) {
        unary_expr.type = unary_expr.value.get_type();
    }

    return Result::SUCCESS;
}

void ExprFinalizer::create_std_string(sir::StringLiteral &string_literal, sir::Expr &out_expr) {
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

void ExprFinalizer::create_std_array(
    sir::ArrayLiteral &array_literal,
    const sir::Expr &element_type,
    sir::Expr &out_expr
) {
    array_literal.type = analyzer.create_expr(sir::StaticArrayType{
        .ast_node = nullptr,
        .base_type = element_type,
        .length = analyzer.create_expr(sir::IntLiteral{
            .ast_node = nullptr,
            .type = nullptr,
            .value = static_cast<unsigned>(array_literal.values.size()),
        }),
    });

    sir::Expr array_pointer = sir::create_unary_ref(*analyzer.cur_sir_mod, &array_literal);

    sir::Expr data_pointer = analyzer.create_expr(sir::CastExpr{
        .ast_node = nullptr,
        .type = analyzer.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = element_type,
        }),
        .value = array_pointer,
    });

    sir::Expr length = analyzer.create_expr(sir::IntLiteral{
        .ast_node = nullptr,
        .type = sir::create_primitive_type(*analyzer.cur_sir_mod, sir::Primitive::USIZE),
        .value = static_cast<unsigned>(array_literal.values.size()),
    });

    sir::StructDef &array_type = analyzer.find_std_array().as<sir::StructDef>();
    sir::StructDef *specialization = GenericsSpecializer(analyzer).specialize(array_type, {element_type});
    sir::FuncDef &func_def = specialization->block.symbol_table->look_up("from").as<sir::FuncDef>();

    sir::Expr callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &func_def.type,
        .symbol = &func_def,
    });

    out_expr = analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = func_def.type.return_type,
        .callee = callee,
        .args = {data_pointer, length},
    });
}

void ExprFinalizer::create_std_optional_some(
    sir::Specialization<sir::StructDef> &specialization,
    sir::Expr &inout_expr
) {
    sir::FuncDef &func_def = specialization.def->block.symbol_table->look_up("new_some").as<sir::FuncDef>();
    inout_expr = sir::create_call(*analyzer.cur_sir_mod, func_def, {inout_expr});
}

void ExprFinalizer::create_std_optional_none(sir::Specialization<sir::StructDef> &specialization, sir::Expr &out_expr) {
    sir::FuncDef &func_def = specialization.def->block.symbol_table->look_up("new_none").as<sir::FuncDef>();
    out_expr = sir::create_call(*analyzer.cur_sir_mod, func_def, {});
}

void ExprFinalizer::create_std_result_success(
    sir::Specialization<sir::StructDef> &specialization,
    sir::Expr &inout_expr
) {
    sir::FuncDef &func_def = specialization.def->block.symbol_table->look_up("new_success").as<sir::FuncDef>();

    sir::Expr callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &func_def.type,
        .symbol = &func_def,
    });

    inout_expr = analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = func_def.type.return_type,
        .callee = callee,
        .args = {inout_expr},
    });
}

void ExprFinalizer::create_std_result_failure(
    sir::Specialization<sir::StructDef> &specialization,
    sir::Expr &inout_expr
) {
    sir::FuncDef &func_def = specialization.def->block.symbol_table->look_up("new_failure").as<sir::FuncDef>();

    sir::Expr callee = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = &func_def.type,
        .symbol = &func_def,
    });

    inout_expr = analyzer.create_expr(sir::CallExpr{
        .ast_node = nullptr,
        .type = func_def.type.return_type,
        .callee = callee,
        .args = {inout_expr},
    });
}

void ExprFinalizer::create_std_map(sir::MapLiteral &map_literal, sir::Expr &out_expr) {
    sir::Expr key_type = map_literal.entries[0].key.get_type();
    sir::Expr value_type = map_literal.entries[0].value.get_type();

    sir::StructDef &map_type = analyzer.find_std_map().as<sir::StructDef>();
    sir::StructDef *specialization = GenericsSpecializer(analyzer).specialize(map_type, {key_type, value_type});
    sir::FuncDef &func_def = specialization->block.symbol_table->look_up("from").as<sir::FuncDef>();

    sir::Expr entry_type = analyzer.create_expr(sir::TupleExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .exprs = {key_type, value_type},
    });

    std::vector<sir::Expr> entries(map_literal.entries.size());

    for (unsigned i = 0; i < map_literal.entries.size(); i++) {
        sir::MapLiteralEntry &entry = map_literal.entries[i];

        entries[i] = analyzer.create_expr(sir::TupleExpr{
            .ast_node = nullptr,
            .type = entry_type,
            .exprs = {entry.key, entry.value},
        });
    }

    sir::Expr array_type = analyzer.create_expr(sir::StaticArrayType{
        .ast_node = nullptr,
        .base_type = entry_type,
        .length = analyzer.create_expr(sir::IntLiteral{
            .ast_node = nullptr,
            .type = nullptr,
            .value = static_cast<unsigned>(map_literal.entries.size()),
        }),
    });

    sir::Expr array = analyzer.create_expr(sir::ArrayLiteral{
        .ast_node = nullptr,
        .type = array_type,
        .values = entries,
    });

    sir::Expr array_pointer = sir::create_unary_ref(*analyzer.cur_sir_mod, array);

    sir::Expr data_pointer = analyzer.create_expr(sir::CastExpr{
        .ast_node = nullptr,
        .type = analyzer.create_expr(sir::PointerType{
            .ast_node = nullptr,
            .base_type = entry_type,
        }),
        .value = array_pointer,
    });

    sir::Expr length = analyzer.create_expr(sir::IntLiteral{
        .ast_node = nullptr,
        .type = sir::create_primitive_type(*analyzer.cur_sir_mod, sir::Primitive::USIZE),
        .value = static_cast<unsigned>(map_literal.entries.size()),
    });

    out_expr = sir::create_call(*analyzer.cur_sir_mod, func_def, {data_pointer, length});
}

Result ExprFinalizer::finalize_array_literal_elements(sir::ArrayLiteral &array_literal, sir::Expr element_type) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::Expr &value : array_literal.values) {
        if (element_type) {
            partial_result = finalize_by_coercion(value, element_type);
        } else {
            partial_result = finalize(value);
            element_type = value.get_type();
        }

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

Result ExprFinalizer::finalize_struct_literal_fields(sir::StructLiteral &struct_literal) {
    Result result = Result::SUCCESS;
    Result partial_result;

    sir::StructDef &struct_def = struct_literal.type.as<sir::SymbolExpr>().symbol.as<sir::StructDef>();

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        for (sir::StructField *field : struct_def.fields) {
            if (field->ident.value == entry.ident.value) {
                entry.field = field;
            }
        }

        if (!entry.field) {
            analyzer.report_generator.report_err_no_field(entry.ident, struct_def);
            result = Result::ERROR;
            continue;
        }

        partial_result = finalize_by_coercion(entry.value, entry.field->type);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }

        analyzer.add_symbol_use(entry.ident.ast_node, entry.field);
    }

    return result;
}

Result ExprFinalizer::finalize_map_literal_elements(
    sir::MapLiteral &map_literal,
    sir::Expr key_type,
    sir::Expr value_type
) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::MapLiteralEntry &entry : map_literal.entries) {
        if (key_type) {
            partial_result = finalize_by_coercion(entry.key, key_type);
        } else {
            partial_result = finalize(entry.key);
        }

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }

        if (value_type) {
            partial_result = finalize_by_coercion(entry.value, value_type);
        } else {
            partial_result = finalize(entry.value);
        }

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

} // namespace sema

} // namespace lang

} // namespace banjo
