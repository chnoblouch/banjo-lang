#include "semantic_analyzer.hpp"

#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sema2/meta_expansion.hpp"
#include "banjo/sema2/preamble.hpp"
#include "banjo/sema2/symbol_collector.hpp"
#include "banjo/sema2/use_resolver.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/sema2/type_alias_resolver.hpp"

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
    MetaExpansion(*this).run();
    UseResolver(*this).resolve();
    TypeAliasResolver(*this).analyze();
    DeclInterfaceAnalyzer(*this).analyze();
    DeclBodyAnalyzer(*this).analyze();

    while (!decls_awaiting_body_analysis.empty()) {
        auto clone = decls_awaiting_body_analysis;
        decls_awaiting_body_analysis.clear();

        for (auto &[decl, scope] : clone) {
            scopes.push(scope);
            DeclBodyAnalyzer(*this).analyze_decl(decl);
            scopes.pop();
        }
    }
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

void SemanticAnalyzer::enter_union_def(sir::UnionDef *union_def) {
    Scope &scope = push_scope();
    scope.union_def = union_def;
    scope.decl_block = &union_def->block;
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

sir::Symbol SemanticAnalyzer::find_std_result() {
    return find_std_symbol({"std", "result"}, "Result");
}

sir::Symbol SemanticAnalyzer::find_std_array() {
    return find_std_symbol({"std", "array"}, "Array");
}

sir::Symbol SemanticAnalyzer::find_std_string() {
    return find_std_symbol({"std", "string"}, "String");
}

sir::Symbol SemanticAnalyzer::find_std_closure() {
    return find_std_symbol({"std", "closure"}, "Closure");
}

sir::Specialization<sir::StructDef> *SemanticAnalyzer::as_std_array_specialization(sir::Expr &type) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::StructDef &array_def = find_std_array().as<sir::StructDef>();

        for (sir::Specialization<sir::StructDef> &specialization : array_def.specializations) {
            if (specialization.def == struct_def) {
                return &specialization;
            }
        }
    }

    return nullptr;
}

sir::Specialization<sir::StructDef> *SemanticAnalyzer::as_std_optional_specialization(sir::Expr &type) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::StructDef &optional_def = find_std_optional().as<sir::StructDef>();

        for (sir::Specialization<sir::StructDef> &specialization : optional_def.specializations) {
            if (specialization.def == struct_def) {
                return &specialization;
            }
        }
    }

    return nullptr;
}

sir::Specialization<sir::StructDef> *SemanticAnalyzer::as_std_result_specialization(sir::Expr &type) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::StructDef &optional_def = find_std_result().as<sir::StructDef>();

        for (sir::Specialization<sir::StructDef> &specialization : optional_def.specializations) {
            if (specialization.def == struct_def) {
                return &specialization;
            }
        }
    }

    return nullptr;
}

unsigned SemanticAnalyzer::compute_size(sir::Expr type) {
    SSAGeneratorContext dummy_ssa_gen_ctx(target);
    ssa::Type ssa_type = TypeSSAGenerator(dummy_ssa_gen_ctx).generate(type);
    return target->get_data_layout().get_size(ssa_type);
}

} // namespace sema

} // namespace lang

} // namespace banjo
