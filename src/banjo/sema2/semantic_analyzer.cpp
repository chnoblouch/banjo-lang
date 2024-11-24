#include "semantic_analyzer.hpp"

#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sema2/extra_analysis.hpp"
#include "banjo/sema2/meta_expansion.hpp"
#include "banjo/sema2/resource_analyzer.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sema2/symbol_collector.hpp"
#include "banjo/sema2/type_alias_resolver.hpp"
#include "banjo/sema2/use_resolver.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"

#include <vector>

namespace banjo {

namespace lang {

namespace sema {

SemanticAnalyzer::SemanticAnalyzer(
    sir::Unit &sir_unit,
    target::Target *target,
    ReportManager &report_manager,
    Mode mode /* = Mode::COMPILATION */
)
  : sir_unit(sir_unit),
    target(target),
    report_manager(report_manager),
    report_generator(*this),
    mode(mode) {}

void SemanticAnalyzer::analyze() {
    analyze(sir_unit.mods);
}

void SemanticAnalyzer::analyze(const std::vector<sir::Module *> &mods) {
    SymbolCollector(*this).collect(mods);
    populate_preamble_symbols();
    MetaExpansion(*this).run(mods);
    UseResolver(*this).resolve(mods);
    TypeAliasResolver(*this).analyze(mods);
    DeclInterfaceAnalyzer(*this).analyze(mods);
    DeclBodyAnalyzer(*this).analyze(mods);
    ResourceAnalyzer(*this).analyze(mods);
    run_postponed_analyses();
}

void SemanticAnalyzer::analyze(sir::Module &mod) {
    enter_mod(&mod);

    SymbolCollector(*this).collect_in_block(mod.block);
    populate_preamble_symbols();
    MetaExpansion(*this).run_on_decl_block(mod.block);
    UseResolver(*this).resolve_in_block(mod.block);
    TypeAliasResolver(*this).analyze_decl_block(mod.block);
    DeclInterfaceAnalyzer(*this).analyze_decl_block(mod.block);
    DeclBodyAnalyzer(*this).analyze_decl_block(mod.block);
    ResourceAnalyzer(*this).analyze_decl_block(mod.block);
    run_postponed_analyses();

    exit_mod();
}

void SemanticAnalyzer::enter_mod(sir::Module *mod) {
    cur_sir_mod = mod;

    scopes.push(Scope{
        .decl = mod,
        .block = nullptr,
    });
}

sir::SymbolTable &SemanticAnalyzer::get_symbol_table() {
    Scope &scope = get_scope();
    return scope.block ? *scope.block->symbol_table : *scope.decl.get_symbol_table();
}

void SemanticAnalyzer::populate_preamble_symbols() {
    preamble_symbols = {
        {"print", find_std_symbol({"internal", "preamble"}, "print")},
        {"println", find_std_symbol({"internal", "preamble"}, "println")},
        {"assert", find_std_symbol({"internal", "preamble"}, "assert")},
        {"Optional", find_std_symbol({"std", "optional"}, "Optional")},
        {"Result", find_std_symbol({"std", "result"}, "Result")},
        {"Array", find_std_symbol({"std", "array"}, "Array")},
        {"String", find_std_symbol({"std", "string"}, "String")},
        {"Map", find_std_symbol({"std", "map"}, "Map")},
        {"Set", find_std_symbol({"std", "set"}, "Set")},
        {"Closure", find_std_symbol({"std", "closure"}, "Closure")},
    };
}

void SemanticAnalyzer::run_postponed_analyses() {
    while (!decls_awaiting_body_analysis.empty()) {
        auto clone = decls_awaiting_body_analysis;
        decls_awaiting_body_analysis.clear();

        for (auto &[decl, scope] : clone) {
            scopes.push(scope);
            DeclBodyAnalyzer(*this).analyze_decl(decl);
            ResourceAnalyzer(*this).analyze_decl(decl);
            scopes.pop();
        }
    }
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

sir::Symbol SemanticAnalyzer::find_std_map() {
    return find_std_symbol({"std", "map"}, "Map");
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

sir::Specialization<sir::StructDef> *SemanticAnalyzer::as_std_map_specialization(sir::Expr &type) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::StructDef &optional_def = find_std_map().as<sir::StructDef>();

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

void SemanticAnalyzer::add_symbol_def(sir::Symbol sir_symbol) {
    if (mode != Mode::INDEXING || !get_scope().generic_args.empty()) {
        return;
    }

    ASTNode *ast_node = sir_symbol.get_ident().ast_node;
    if (!ast_node) {
        return;
    }

    ExtraAnalysis::SymbolDef def{
        .symbol = sir_symbol,
        .ident_range = ast_node->get_range(),
    };

    extra_analysis.mods[cur_sir_mod].symbol_defs.push_back(def);
}

void SemanticAnalyzer::add_symbol_use(ASTNode *ast_node, sir::Symbol sir_symbol) {
    if (mode != Mode::INDEXING || !get_scope().generic_args.empty() || !ast_node) {
        return;
    }

    ExtraAnalysis::SymbolUse use{
        .range = ast_node->get_range(),
        .symbol = sir_symbol,
    };

    extra_analysis.mods[cur_sir_mod].symbol_uses.push_back(use);
}

} // namespace sema

} // namespace lang

} // namespace banjo
