#include "semantic_analyzer.hpp"

#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sema2/decl_value_analyzer.hpp"
#include "banjo/sema2/meta_expansion.hpp"
#include "banjo/sema2/preamble.hpp"
#include "banjo/sema2/symbol_collector.hpp"
#include "banjo/sema2/use_resolver.hpp"
#include "banjo/sir/sir.hpp"

#include <utility>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

SemanticAnalyzer::SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target) : sir_unit(sir_unit), target(target) {}

void SemanticAnalyzer::analyze() {
    Preamble(*this).insert();
    SymbolCollector(*this).collect();
    UseResolver(*this).resolve();
    DeclInterfaceAnalyzer(*this).analyze();
    DeclValueAnalyzer(*this).analyze();
    MetaExpansion(*this).run();
    run_postponed_analyses();
    DeclBodyAnalyzer(*this).analyze();
}

void SemanticAnalyzer::enter_mod(sir::Module *mod) {
    cur_sir_mod = mod;

    scopes.push(Scope{
        .func_def = nullptr,
        .block = nullptr,
        .decl_block = &mod->block,
    });
}

void SemanticAnalyzer::enter_struct_def(sir::StructDef *struct_def) {
    Scope &scope = push_scope();
    scope.struct_def = struct_def;
    scope.decl_block = &struct_def->block;
}

void SemanticAnalyzer::enter_enum_def(sir::EnumDef *enum_def) {
    Scope &scope = push_scope();
    scope.enum_def = enum_def;
    scope.decl_block = &enum_def->block;
}

sir::SymbolTable &SemanticAnalyzer::get_symbol_table() {
    Scope &scope = get_scope();
    return scope.block ? *scope.block->symbol_table : *scope.decl_block->symbol_table;
}

void SemanticAnalyzer::check_for_completeness(sir::DeclBlock &block) {
    block.symbol_table->complete = true;

    for (sir::Decl &decl : block.decls) {
        if (decl.is<sir::MetaIfStmt>()) {
            block.symbol_table->complete = false;
            break;
        }
    }
}

void SemanticAnalyzer::postpone_analysis(std::function<void()> function) {
    postponed_analyses.push_back({
        .callback = std::move(function),
        .context = scopes.top(),
    });
}

void SemanticAnalyzer::run_postponed_analyses() {
    while (!postponed_analyses.empty()) {
        std::vector<PostponedAnalysis> postponed_analyses_copy = std::move(postponed_analyses);
        postponed_analyses.clear();

        for (const PostponedAnalysis &postponed_analysis : postponed_analyses_copy) {
            scopes.push(postponed_analysis.context);
            postponed_analysis.callback();
            scopes.pop();
        }
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
