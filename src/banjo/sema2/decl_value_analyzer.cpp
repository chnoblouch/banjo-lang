#include "decl_value_analyzer.hpp"

#include "banjo/sema2/const_evaluator.hpp"
#include "banjo/sema2/expr_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclValueAnalyzer::DeclValueAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result DeclValueAnalyzer::analyze_const_def(sir::ConstDef &const_def) {
    if (!const_def.type) {
        return Result::ERROR;
    }

    return ExprAnalyzer(analyzer, ExprConstraints::expect_type(const_def.type)).analyze(const_def.value);
}

Result DeclValueAnalyzer::analyze_enum_def(sir::EnumDef &enum_def) {
    LargeInt next_value = 0;

    for (sir::EnumVariant *variant : enum_def.variants) {
        if (variant->value) {
            ExprAnalyzer(analyzer).analyze(variant->value);
            next_value = ConstEvaluator(analyzer).evaluate_to_int(variant->value) + 1;
        } else {
            variant->value = analyzer.create_expr(sir::IntLiteral{
                .ast_node = nullptr,
                .type = nullptr,
                .value = next_value,
            });

            ExprAnalyzer(analyzer).analyze(variant->value);
            next_value += 1;
        }
    }

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
