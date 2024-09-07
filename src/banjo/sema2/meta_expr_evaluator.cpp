#include "meta_expr_evaluator.hpp"

#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

MetaExprEvaluator::MetaExprEvaluator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

Result MetaExprEvaluator::evaluate(sir::MetaFieldExpr &meta_field_expr, sir::Expr &out_expr) {
    sir::Expr base_expr = meta_field_expr.base.as<sir::MetaAccess>().expr;
    const std::string &field_name = meta_field_expr.field.value;

    ExprAnalyzer(analyzer).analyze(base_expr);

    if (field_name == "size") {
        out_expr = analyzer.create_expr(sir::IntLiteral{
            .ast_node = nullptr,
            .type = nullptr,
            .value = analyzer.compute_size(base_expr),
        });
    } else {
        ASSERT_UNREACHABLE;
    }

    ExprAnalyzer(analyzer).analyze(out_expr);
    return Result::SUCCESS;
}

Result MetaExprEvaluator::evaluate(sir::MetaCallExpr &meta_call_expr, sir::Expr &out_expr) {
    sir::MetaFieldExpr field_expr = meta_call_expr.callee.as<sir::MetaFieldExpr>();
    sir::Expr base_expr = field_expr.base.as<sir::MetaAccess>().expr;
    const std::string &callee_name = field_expr.field.value;

    ExprAnalyzer(analyzer).analyze(base_expr);

    if (callee_name == "has_method") {
        out_expr = has_method(base_expr, meta_call_expr.args[0].as<sir::StringLiteral>().value);
    } else {
        ASSERT_UNREACHABLE;
    }

    ExprAnalyzer(analyzer).analyze(out_expr);
    return Result::SUCCESS;
}

sir::Expr MetaExprEvaluator::has_method(sir::Expr &type, const std::string &name) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::Symbol symbol = struct_def->block.symbol_table->look_up(name);

        if (auto func_def = symbol.match<sir::FuncDef>()) {
            return create_bool_literal(func_def->is_method());
        }
    }

    return create_bool_literal(false);
}

sir::Expr MetaExprEvaluator::create_bool_literal(bool value) {
    return analyzer.create_expr(sir::BoolLiteral{
        .ast_node = nullptr,
        .type = nullptr,
        .value = value,
    });
}

} // namespace sema

} // namespace lang

} // namespace banjo
