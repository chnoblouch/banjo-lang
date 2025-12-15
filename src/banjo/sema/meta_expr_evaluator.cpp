#include "meta_expr_evaluator.hpp"

#include "banjo/sema/decl_body_analyzer.hpp"
#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sema/resource_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <string_view>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

MetaExprEvaluator::MetaExprEvaluator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

Result MetaExprEvaluator::evaluate(sir::MetaFieldExpr &meta_field_expr, sir::Expr &out_expr) {
    Result partial_result;

    ast_node = meta_field_expr.ast_node;

    sir::Expr base_expr = meta_field_expr.base.as<sir::MetaAccess>().expr;
    std::string_view field_name = meta_field_expr.field.value;

    partial_result = ExprAnalyzer(analyzer).analyze(base_expr);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    base_expr = unwrap_expr(base_expr);

    if (field_name == "size") {
        out_expr = create_int_literal(analyzer.compute_size(base_expr));
    } else if (field_name == "name") {
        out_expr = compute_name(base_expr);
    } else if (field_name == "is_pointer") {
        out_expr = create_bool_literal(base_expr.is<sir::PointerType>());
    } else if (field_name == "is_struct") {
        out_expr = create_bool_literal(base_expr.is_symbol<sir::StructDef>());
    } else if (field_name == "is_enum") {
        out_expr = create_bool_literal(base_expr.is_symbol<sir::EnumDef>());
    } else if (field_name == "is_static_array") {
        out_expr = create_bool_literal(base_expr.is<sir::StaticArrayType>());
    } else if (field_name == "is_tuple") {
        out_expr = create_bool_literal(base_expr.is<sir::TupleExpr>());
    } else if (field_name == "fields") {
        out_expr = compute_fields(base_expr);
    } else if (field_name == "is_resource") {
        out_expr = compute_is_resource(base_expr);
    } else if (field_name == "variants") {
        out_expr = compute_variants(base_expr);
    } else if (field_name == "array_base") {
        out_expr = compute_array_base(base_expr);
    } else if (field_name == "array_length") {
        out_expr = compute_array_length(base_expr);
    } else if (field_name == "tuple_num_fields") {
        out_expr = compute_tuple_num_fields(base_expr);
    } else {
        analyzer.report_generator.report_err_invalid_meta_field(meta_field_expr);
        return Result::ERROR;
    }

    ExprAnalyzer(analyzer).analyze_uncoerced(out_expr);
    return Result::SUCCESS;
}

Result MetaExprEvaluator::evaluate(sir::MetaCallExpr &meta_call_expr, sir::Expr &out_expr) {
    ast_node = meta_call_expr.ast_node;

    sir::MetaFieldExpr field_expr = meta_call_expr.callee.as<sir::MetaFieldExpr>();
    sir::Expr base_expr = field_expr.base.as<sir::MetaAccess>().expr;
    std::string_view callee_name = field_expr.field.value;

    ExprAnalyzer(analyzer).analyze(base_expr);
    base_expr = unwrap_expr(base_expr);

    for (sir::Expr &arg : meta_call_expr.args) {
        ExprAnalyzer(analyzer).analyze_uncoerced(arg);
        arg = unwrap_expr(arg);
    }

    if (callee_name == "has_method") {
        out_expr = compute_has_method(base_expr, meta_call_expr.args);
    } else if (callee_name == "field") {
        out_expr = compute_field(base_expr, meta_call_expr.args);
    } else if (callee_name == "tuple_field") {
        out_expr = compute_tuple_field(base_expr, meta_call_expr.args);
    } else {
        analyzer.report_generator.report_err_invalid_meta_method(meta_call_expr);
        return Result::ERROR;
    }

    ExprAnalyzer(analyzer).analyze(out_expr);
    return Result::SUCCESS;
}

sir::Expr MetaExprEvaluator::compute_name(sir::Expr &type) {
    if (auto symbol_expr = type.match<sir::SymbolExpr>()) {
        return create_string_literal(symbol_expr->symbol.get_name());
    } else {
        return create_string_literal("");
    }
}

sir::Expr MetaExprEvaluator::compute_fields(sir::Expr &type) {
    sir::StructDef *struct_def = type.match_symbol<sir::StructDef>();
    if (!struct_def) {
        return create_array_literal({});
    }

    std::span<sir::Expr> fields = analyzer.allocate_array<sir::Expr>(struct_def->fields.size());

    for (unsigned i = 0; i < struct_def->fields.size(); i++) {
        sir::StructField *field = struct_def->fields[i];
        fields[i] = create_string_literal(field->ident.value);
    }

    return create_array_literal(fields);
}

sir::Expr MetaExprEvaluator::compute_is_resource(sir::Expr &type) {
    bool is_resource = ResourceAnalyzer(analyzer).create_resource(type).has_value();
    return create_bool_literal(is_resource);
}

sir::Expr MetaExprEvaluator::compute_variants(sir::Expr &type) {
    sir::EnumDef *enum_def = type.match_symbol<sir::EnumDef>();
    if (!enum_def) {
        return create_array_literal({});
    }

    DeclBodyAnalyzer(analyzer).visit_enum_def(*enum_def);

    std::span<sir::Expr> variants = analyzer.allocate_array<sir::Expr>(enum_def->variants.size());

    for (unsigned i = 0; i < enum_def->variants.size(); i++) {
        sir::EnumVariant *variant = enum_def->variants[i];

        variants[i] = analyzer.create(
            sir::TupleExpr{
                .ast_node = nullptr,
                .type = nullptr,
                .exprs = analyzer.create_array<sir::Expr>({
                    create_string_literal(variant->ident.value),
                    variant->value,
                }),
            }
        );
    }

    return create_array_literal(variants);
}

sir::Expr MetaExprEvaluator::compute_array_base(sir::Expr &base) {
    sir::StaticArrayType *array_type = base.match<sir::StaticArrayType>();
    if (!array_type) {
        return analyzer.create(
            sir::PrimitiveType{
                .ast_node = nullptr,
                .primitive = sir::Primitive::VOID,
            }
        );
    }

    return array_type->base_type;
}

sir::Expr MetaExprEvaluator::compute_array_length(sir::Expr &base) {
    sir::StaticArrayType *array_type = base.match<sir::StaticArrayType>();
    if (!array_type) {
        return create_int_literal(0);
    }

    return create_int_literal(array_type->length.as<sir::IntLiteral>().value);
}

sir::Expr MetaExprEvaluator::compute_tuple_num_fields(sir::Expr &type) {
    sir::TupleExpr *tuple_expr = type.match<sir::TupleExpr>();
    if (!tuple_expr) {
        return create_int_literal(0);
    }

    return create_int_literal(tuple_expr->exprs.size());
}

sir::Expr MetaExprEvaluator::compute_has_method(sir::Expr &type, std::span<sir::Expr> args) {
    // TODO: Proper error reporting

    if (args.size() != 1 || !args[0].is<sir::StringLiteral>()) {
        return create_bool_literal(false);
    }

    std::string_view name = args[0].as<sir::StringLiteral>().value;

    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::Symbol symbol = struct_def->block.symbol_table->look_up_local(name);

        if (auto func_def = symbol.match<sir::FuncDef>()) {
            return create_bool_literal(func_def->is_method());
        }
    }

    return create_bool_literal(false);
}

sir::Expr MetaExprEvaluator::compute_field(sir::Expr &base, std::span<sir::Expr> args) {
    // TODO: Proper error reporting

    if (args.size() != 1 || !args[0].is<sir::StringLiteral>()) {
        return create_bool_literal(false);
    }

    std::string_view name = args[0].as<sir::StringLiteral>().value;

    return analyzer.create(
        sir::DotExpr{
            .ast_node = nullptr,
            .lhs = base,
            .rhs{
                .ast_node = nullptr,
                .value = analyzer.create_string(name),
            },
        }
    );
}

sir::Expr MetaExprEvaluator::compute_tuple_field(sir::Expr &base, std::span<sir::Expr> args) {
    // TODO: Proper error reporting

    if (args.size() != 1 || !args[0].is<sir::IntLiteral>()) {
        return create_bool_literal(false);
    }

    std::string index = args[0].as<sir::IntLiteral>().value.to_string();

    return analyzer.create(
        sir::DotExpr{
            .ast_node = nullptr,
            .lhs = base,
            .rhs{
                .ast_node = nullptr,
                .value = analyzer.create_string(index),
            },
        }
    );
}

sir::Expr MetaExprEvaluator::create_int_literal(LargeInt value) {
    return analyzer.create(
        sir::IntLiteral{
            .ast_node = ast_node,
            .type = nullptr,
            .value = value,
        }
    );
}

sir::Expr MetaExprEvaluator::create_bool_literal(bool value) {
    return analyzer.create(
        sir::BoolLiteral{
            .ast_node = ast_node,
            .type = nullptr,
            .value = value,
        }
    );
}

sir::Expr MetaExprEvaluator::create_array_literal(std::span<sir::Expr> values) {
    return analyzer.create(
        sir::ArrayLiteral{
            .ast_node = ast_node,
            .type = nullptr,
            .values = values,
        }
    );
}

sir::Expr MetaExprEvaluator::create_string_literal(std::string_view value) {
    return analyzer.create(
        sir::StringLiteral{
            .ast_node = ast_node,
            .type = nullptr,
            .value = analyzer.create_string(value),
        }
    );
}

sir::Expr MetaExprEvaluator::unwrap_expr(sir::Expr expr) {
    if (auto reference_type = expr.match<sir::ReferenceType>()) {
        return reference_type->base_type;
    } else {
        return expr;
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
