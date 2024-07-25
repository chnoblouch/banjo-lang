#include "decl_body_analyzer.hpp"

#include "banjo/sema2/stmt_analyzer.hpp"
#include "expr_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclBodyAnalyzer::DeclBodyAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void DeclBodyAnalyzer::analyze() {
    for (sir::Module *mod : analyzer.sir_unit.mods) {
        analyzer.enter_mod(mod);
        analyze_decl_block(mod->block);
        analyzer.exit_mod();
    }
}

void DeclBodyAnalyzer::analyze_decl_block(sir::DeclBlock &decl_block) {
    for (sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) analyze_func_def(*func_def);
        else if (auto struct_def = decl.match<sir::StructDef>()) analyze_struct_def(*struct_def);
        else if (auto enum_def = decl.match<sir::EnumDef>()) analyze_enum_def(*enum_def);
    }
}

void DeclBodyAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    if (func_def.is_generic()) {
        return;
    }

    analyzer.push_scope().func_def = &func_def;
    StmtAnalyzer(analyzer).analyze_block(func_def.block);
    analyzer.pop_scope();
}

void DeclBodyAnalyzer::analyze_struct_def(sir::StructDef &struct_def) {
    analyzer.push_scope().struct_def = &struct_def;
    analyze_decl_block(struct_def.block);
    analyzer.pop_scope();
}

void DeclBodyAnalyzer::analyze_enum_def(sir::EnumDef &enum_def) {
    LargeInt next_value = 0;

    for (sir::EnumVariant *variant : enum_def.variants) {
        if (variant->value) {
            ExprAnalyzer(analyzer).analyze(variant->value);
            next_value = variant->value.as<sir::IntLiteral>().value + 1;
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

    analyzer.push_scope().enum_def = &enum_def;
    analyze_decl_block(enum_def.block);
    analyzer.pop_scope();
}

} // namespace sema

} // namespace lang

} // namespace banjo
