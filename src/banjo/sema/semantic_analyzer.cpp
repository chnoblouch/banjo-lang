#include "semantic_analyzer.hpp"

#include "banjo/config/config.hpp"
#include "banjo/sema/decl_body_analyzer.hpp"
#include "banjo/sema/decl_interface_analyzer.hpp"
#include "banjo/sema/extra_analysis.hpp"
#include "banjo/sema/meta_expansion.hpp"
#include "banjo/sema/resource_analyzer.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/symbol_collector.hpp"
#include "banjo/sema/type_alias_resolver.hpp"
#include "banjo/sema/use_resolver.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_create.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/utils/timing.hpp"

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
  : symbol_ctx(*this),
    sir_unit(sir_unit),
    target(target),
    report_manager(report_manager),
    report_generator(*this),
    mode(mode) {

    scope_stack.push(
        Scope{
            .decl_scope = nullptr,
            .block = nullptr,
        }
    );

    // HACK

    sir::Module &mod = *sir_unit.mods[0];

    meta_field_types = {
        {"size", sir::create_pseudo_type(mod, sir::PseudoTypeKind::INT_LITERAL)},
        {"name", sir::create_pseudo_type(mod, sir::PseudoTypeKind::STRING_LITERAL)},
        {"is_pointer", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
        {"is_struct", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
        {"is_enum", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
    };
}

void SemanticAnalyzer::analyze() {
    PROFILE_SCOPE("semantic analyzer");
    analyze(sir_unit.mods);
}

void SemanticAnalyzer::analyze(const std::vector<sir::Module *> &mods) {
    stage = DeclStage::NAME;
    SymbolCollector(*this).collect(mods);
    populate_preamble_symbols();
    MetaExpansion(*this).run(mods);
    UseResolver(*this).resolve(mods);

    stage = DeclStage::INTERFACE;
    TypeAliasResolver(*this).analyze(mods);
    DeclInterfaceAnalyzer(*this).analyze(mods);

    stage = DeclStage::BODY;
    DeclBodyAnalyzer(*this).analyze(mods);

    stage = DeclStage::RESOURCES;
    ResourceAnalyzer(*this).analyze(mods);
}

void SemanticAnalyzer::analyze(sir::Module &mod) {
    stage = DeclStage::NAME;
    SymbolCollector(*this).collect_in_mod(mod);
    populate_preamble_symbols();
    MetaExpansion(*this).run_on_decl_block(mod.block);
    UseResolver(*this).resolve_in_block(mod.block);

    stage = DeclStage::INTERFACE;
    TypeAliasResolver(*this).analyze_decl_block(mod.block);
    DeclInterfaceAnalyzer(*this).analyze_decl_block(mod.block);

    stage = DeclStage::BODY;
    DeclBodyAnalyzer(*this).analyze_decl_block(mod.block);

    stage = DeclStage::RESOURCES;
    ResourceAnalyzer(*this).analyze_decl_block(mod.block);
}

void SemanticAnalyzer::enter_decl_scope(DeclScope &decl_scope) {
    mod = decl_scope.mod;

    Scope scope{
        .decl_scope = &decl_scope,
        .block = nullptr,
        .symbol_table = decl_scope.decl_block->symbol_table,
    };

    scope_stack.push(scope);
}

void SemanticAnalyzer::exit_decl_scope() {
    scope_stack.pop();
    DeclScope *decl_scope = scope_stack.top().decl_scope;
    mod = decl_scope ? decl_scope->mod : nullptr;
}

void SemanticAnalyzer::enter_block(sir::Block &block) {
    Scope scope{
        .decl_scope = scope_stack.top().decl_scope,
        .block = &block,
        .symbol_table = block.symbol_table,
    };

    scope_stack.push(scope);
}

void SemanticAnalyzer::enter_symbol_table(sir::SymbolTable &symbol_table) {
    Scope scope{
        .decl_scope = scope_stack.top().decl_scope,
        .block = scope_stack.top().block,
        .symbol_table = &symbol_table,
    };

    scope_stack.push(scope);
}

bool SemanticAnalyzer::is_in_specialization() {
    sir::Symbol decl = get_decl_scope().decl;

    while (decl && !decl.is<sir::Module>()) {
        if (auto func_def = decl.match<sir::FuncDef>()) {
            if (func_def->parent_specialization) {
                return true;
            }
        } else if (auto struct_def = decl.match<sir::StructDef>()) {
            if (struct_def->parent_specialization) {
                return true;
            }
        }

        decl = decl.get_parent();
    }

    return false;
}

void SemanticAnalyzer::populate_preamble_symbols() {
    if (!Config::instance().is_stdlib_enabled()) {
        return;
    }

    preamble_symbols = {
        {"print", find_std_symbol({"internal", "preamble"}, "print")},
        {"println", find_std_symbol({"internal", "preamble"}, "println")},
        {"assert", find_std_symbol({"internal", "preamble"}, "assert")},
        {"Optional", find_std_symbol({"std", "optional"}, "Optional")},
        {"Result", find_std_symbol({"std", "result"}, "Result")},
        {"Array", find_std_symbol({"std", "array"}, "Array")},
        {"Slice", find_std_symbol({"std", "slice"}, "Slice")},
        {"String", find_std_symbol({"std", "string"}, "String")},
        {"StringSlice", find_std_symbol({"std", "string_slice"}, "StringSlice")},
        {"Map", find_std_symbol({"std", "map"}, "Map")},
        {"Set", find_std_symbol({"std", "set"}, "Set")},
        {"Closure", find_std_symbol({"std", "closure"}, "Closure")},
    };
}

sir::Symbol SemanticAnalyzer::find_std_symbol(const ModulePath &mod_path, const std::string &name) {
    sir::Module &mod = *sir_unit.mods_by_path.at(mod_path);
    return mod.block.symbol_table->look_up_local(name);
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

sir::Symbol SemanticAnalyzer::find_std_string_slice() {
    return find_std_symbol({"std", "string_slice"}, "StringSlice");
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
    ssa::Module dummy_ssa_mod;
    SSAGeneratorContext dummy_ssa_gen_ctx(target);
    dummy_ssa_gen_ctx.ssa_mod = &dummy_ssa_mod;

    ssa::Type ssa_type = TypeSSAGenerator(dummy_ssa_gen_ctx).generate(type);
    return target->get_data_layout().get_size(ssa_type);
}

void SemanticAnalyzer::add_symbol_def(sir::Symbol sir_symbol) {
    if (mode != Mode::INDEXING || is_in_specialization()) {
        return;
    }

    ASTNode *ast_node = sir_symbol.get_ident().ast_node;
    if (!ast_node) {
        return;
    }

    ExtraAnalysis::SymbolDef def{
        .symbol = sir_symbol,
        .ident_range = ast_node->range,
    };

    extra_analysis.mods[&get_mod()].symbol_defs.push_back(def);
}

void SemanticAnalyzer::add_symbol_use(ASTNode *ast_node, sir::Symbol sir_symbol) {
    if (mode != Mode::INDEXING || is_in_specialization() || !ast_node) {
        return;
    }

    ExtraAnalysis::SymbolUse use{
        .range = ast_node->range,
        .symbol = sir_symbol,
    };

    extra_analysis.mods[&get_mod()].symbol_uses.push_back(use);
}

std::strong_ordering operator<=>(const DeclStage &lhs, const DeclStage &rhs) {
    return static_cast<unsigned>(lhs) <=> static_cast<unsigned>(rhs);
}

} // namespace sema

} // namespace lang

} // namespace banjo
