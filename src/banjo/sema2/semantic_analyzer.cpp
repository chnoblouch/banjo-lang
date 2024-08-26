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

SemanticAnalyzer::SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target, ReportManager &report_manager)
  : sir_unit(sir_unit),
    target(target),
    report_manager(report_manager),
    report_generator(*this) {}

void SemanticAnalyzer::analyze() {
    Preamble(*this).insert();
    SymbolCollector(*this).collect();
    UseResolver(*this).resolve();
    DeclInterfaceAnalyzer(*this).analyze();
    DeclValueAnalyzer(*this).analyze();
    MetaExpansion(*this).run();
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

sir::Symbol SemanticAnalyzer::find_std_symbol(const ModulePath &mod_path, const std::string &name) {
    return sir_unit.mods_by_path[mod_path]->block.symbol_table->look_up(name);
}

sir::Symbol SemanticAnalyzer::find_std_optional() {
    return find_std_symbol({"std", "optional"}, "Optional");
}

sir::Symbol SemanticAnalyzer::find_std_array() {
    return find_std_symbol({"std", "array"}, "Array");
}

sir::Symbol SemanticAnalyzer::find_std_string() {
    return find_std_symbol({"std", "string"}, "String");
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

} // namespace sema

} // namespace lang

} // namespace banjo
