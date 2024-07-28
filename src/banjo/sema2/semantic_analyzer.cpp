#include "semantic_analyzer.hpp"

#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sema2/symbol_collector.hpp"
#include "banjo/sema2/use_resolver.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

SemanticAnalyzer::SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target) : sir_unit(sir_unit), target(target) {}

void SemanticAnalyzer::analyze() {
    SymbolCollector(*this).collect();
    UseResolver(*this).resolve();
    DeclInterfaceAnalyzer(*this).analyze();
    DeclBodyAnalyzer(*this).analyze();
}

void SemanticAnalyzer::enter_mod(sir::Module *mod) {
    cur_sir_mod = mod;

    scopes.push(Scope{
        .func_def = nullptr,
        .block = nullptr,
        .symbol_table = mod->block.symbol_table,
    });
}

void SemanticAnalyzer::enter_struct_def(sir::StructDef *struct_def) {
    Scope &scope = push_scope();
    scope.struct_def = struct_def;
    scope.symbol_table = struct_def->block.symbol_table;
}

void SemanticAnalyzer::enter_enum_def(sir::EnumDef *enum_def) {
    Scope &scope = push_scope();
    scope.enum_def = enum_def;
    scope.symbol_table = enum_def->block.symbol_table;
}

} // namespace sema

} // namespace lang

} // namespace banjo
