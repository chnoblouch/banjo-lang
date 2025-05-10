#include "meta_expr_evaluator.hpp"

#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sema/resource_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <utility>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

MetaExprEvaluator::MetaExprEvaluator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

Result MetaExprEvaluator::evaluate(sir::MetaFieldExpr &meta_field_expr, sir::Expr &out_expr) {
    Result partial_result;

    sir::Expr base_expr = meta_field_expr.base.as<sir::MetaAccess>().expr;
    const std::string &field_name = meta_field_expr.field.value;

    partial_result = ExprAnalyzer(analyzer).analyze(base_expr);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    base_expr = unwrap_expr(base_expr);

    if (field_name == "size") {
        out_expr = analyzer.create_expr(
            sir::IntLiteral{
                .ast_node = nullptr,
                .type = nullptr,
                .value = analyzer.compute_size(base_expr),
            }
        );
    } else if (field_name == "name") {
        out_expr = compute_name(base_expr);
    } else if (field_name == "is_pointer") {
        out_expr = create_bool_literal(base_expr.is<sir::PointerType>());
    } else if (field_name == "is_struct") {
        out_expr = create_bool_literal(base_expr.is_symbol<sir::StructDef>());
    } else if (field_name == "is_enum") {
        out_expr = create_bool_literal(base_expr.is_symbol<sir::EnumDef>());
    } else if (field_name == "fields") {
        out_expr = compute_fields(base_expr);
    } else if (field_name == "is_resource") {
        out_expr = compute_is_resource(base_expr);
    } else if (field_name == "variants") {
        out_expr = compute_variants(base_expr);
    } else {
        analyzer.report_generator.report_err_invalid_meta_field(meta_field_expr);
        return Result::ERROR;
    }

    ExprAnalyzer(analyzer).analyze_uncoerced(out_expr);
    return Result::SUCCESS;
}

Result MetaExprEvaluator::evaluate(sir::MetaCallExpr &meta_call_expr, sir::Expr &out_expr) {
    sir::MetaFieldExpr field_expr = meta_call_expr.callee.as<sir::MetaFieldExpr>();
    sir::Expr base_expr = field_expr.base.as<sir::MetaAccess>().expr;
    const std::string &callee_name = field_expr.field.value;

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

    std::vector<sir::Expr> fields(struct_def->fields.size());

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

    std::vector<sir::Expr> variants(enum_def->variants.size());

    for (unsigned i = 0; i < enum_def->variants.size(); i++) {
        sir::EnumVariant *variant = enum_def->variants[i];

        variants[i] = analyzer.create_expr(
            sir::TupleExpr{
                .ast_node = nullptr,
                .type = nullptr,
                .exprs{
                    create_string_literal(variant->ident.value),
                    variant->value,
                },
            }
        );
    }

    return create_array_literal(variants);
}

sir::Expr MetaExprEvaluator::compute_has_method(sir::Expr &type, const std::vector<sir::Expr> &args) {
    // TODO: Proper error reporting

    if (args.size() != 1 || !args[0].is<sir::StringLiteral>()) {
        return create_bool_literal(false);
    }

    const std::string &name = args[0].as<sir::StringLiteral>().value;

    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::Symbol symbol = struct_def->block.symbol_table->look_up_local(name);

        if (auto func_def = symbol.match<sir::FuncDef>()) {
            return create_bool_literal(func_def->is_method());
        }
    }

    return create_bool_literal(false);
}

sir::Expr MetaExprEvaluator::compute_field(sir::Expr &base, const std::vector<sir::Expr> &args) {
    // TODO: Proper error reporting

    if (args.size() != 1 || !args[0].is<sir::StringLiteral>()) {
        return create_bool_literal(false);
    }

    std::string name = args[0].as<sir::StringLiteral>().value;

    return analyzer.create_expr(
        sir::DotExpr{
            .ast_node = nullptr,
            .lhs = base,
            .rhs{
                .ast_node = nullptr,
                .value = name,
            },
        }
    );
}

sir::Expr MetaExprEvaluator::create_bool_literal(bool value) {
    return analyzer.create_expr(
        sir::BoolLiteral{
            .ast_node = nullptr,
            .type = nullptr,
            .value = value,
        }
    );
}

sir::Expr MetaExprEvaluator::create_array_literal(std::vector<sir::Expr> values) {
    return analyzer.create_expr(
        sir::ArrayLiteral{
            .ast_node = nullptr,
            .type = nullptr,
            .values = std::move(values),
        }
    );
}

sir::Expr MetaExprEvaluator::create_string_literal(std::string value) {
    return analyzer.create_expr(
        sir::StringLiteral{
            .ast_node = nullptr,
            .type = nullptr,
            .value = std::move(value),
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
