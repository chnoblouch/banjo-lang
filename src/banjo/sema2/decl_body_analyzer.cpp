#include "decl_body_analyzer.hpp"

#include "banjo/sema2/const_evaluator.hpp"
#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/sema2/stmt_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclBodyAnalyzer::DeclBodyAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result DeclBodyAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    analyzer.push_scope().decl = &func_def;
    StmtAnalyzer(analyzer).analyze_block(func_def.block);
    analyzer.pop_scope();

    return Result::SUCCESS;
}

Result DeclBodyAnalyzer::analyze_const_def(sir::ConstDef &const_def) {
    if (!const_def.type) {
        return Result::ERROR;
    }

    return ExprAnalyzer(analyzer).analyze(const_def.value, ExprConstraints::expect_type(const_def.type));
}

Result DeclBodyAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
    Result partial_result;

    if (!var_decl.type) {
        return Result::ERROR;
    }

    if (var_decl.value) {
        partial_result = ExprAnalyzer(analyzer).analyze(var_decl.value, ExprConstraints::expect_type(var_decl.type));

        if (partial_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        var_decl.value = ConstEvaluator(analyzer, false).evaluate(var_decl.value);
    }

    return Result::SUCCESS;
}

Result DeclBodyAnalyzer::analyze_enum_def(sir::EnumDef &enum_def) {
    LargeInt next_value = 0;

    for (sir::EnumVariant *variant : enum_def.variants) {
        if (variant->value) {
            variant->value = ConstEvaluator(analyzer, false).evaluate(variant->value);
            next_value = variant->value.as<sir::IntLiteral>().value + 1;
            ExprAnalyzer(analyzer).analyze(variant->value);
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
