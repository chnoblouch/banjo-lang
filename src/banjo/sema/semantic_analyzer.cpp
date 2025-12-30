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
#include "banjo/sir/sir_visitor.hpp"
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
            .decl = nullptr,
            .block = nullptr,
            .symbol_table = nullptr,
            .closure_ctx = nullptr,
        }
    );

    // HACK

    sir::Module &mod = *sir_unit.mods[0];

    meta_field_types = {
        {"size", sir::create_pseudo_type(mod, sir::PseudoTypeKind::INT_LITERAL)},
        {"name", sir::create_pseudo_type(mod, sir::PseudoTypeKind::STRING_LITERAL)},
        {"is_pointer", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
        {"is_struct", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
        {"is_static_array", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
        {"is_tuple", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
        {"is_enum", sir::create_primitive_type(mod, sir::Primitive::BOOL)},
        {"array_base", sir::create_primitive_type(mod, sir::Primitive::VOID)},
        {"array_length", sir::create_pseudo_type(mod, sir::PseudoTypeKind::INT_LITERAL)},
        {"tuple_num_fields", sir::create_pseudo_type(mod, sir::PseudoTypeKind::INT_LITERAL)},
    };
}

void SemanticAnalyzer::analyze() {
    PROFILE_SCOPE("semantic analyzer");
    analyze(sir_unit.mods);
}

void SemanticAnalyzer::analyze(const std::vector<sir::Module *> &mods) {
    stage = sir::SemaStage::NAME;
    SymbolCollector(*this).collect(mods);
    populate_preamble_symbols();
    MetaExpansion(*this).run(mods);
    UseResolver(*this).resolve(mods);

    stage = sir::SemaStage::INTERFACE;
    TypeAliasResolver(*this).analyze(mods);
    DeclInterfaceAnalyzer(*this).analyze(mods);

    stage = sir::SemaStage::BODY;
    DeclBodyAnalyzer(*this).analyze(mods);

    stage = sir::SemaStage::RESOURCES;
    ResourceAnalyzer(*this).analyze(mods);
}

void SemanticAnalyzer::analyze(sir::Module &mod) {
    stage = sir::SemaStage::NAME;
    SymbolCollector(*this).collect_in_mod(mod);
    populate_preamble_symbols();
    MetaExpansion(*this).run_on_decl_block(mod.block);
    UseResolver(*this).resolve_in_block(mod.block);

    stage = sir::SemaStage::INTERFACE;
    TypeAliasResolver(*this).analyze_decl_block(mod.block);
    DeclInterfaceAnalyzer(*this).analyze_decl_block(mod.block);

    stage = sir::SemaStage::BODY;
    DeclBodyAnalyzer(*this).analyze_decl_block(mod.block);

    stage = sir::SemaStage::RESOURCES;
    ResourceAnalyzer(*this).analyze_decl_block(mod.block);
}

void SemanticAnalyzer::enter_decl(sir::Symbol decl) {
    mod = &decl.find_mod();

    sir::DeclBlock *decl_block = decl.get_decl_block();

    if (!decl_block) {
        decl_block = decl.get_parent().get_decl_block();
    }

    Scope scope{
        .decl = decl,
        .decl_block = decl_block,
        .block = nullptr,
        .symbol_table = decl_block->symbol_table,
        .closure_ctx = nullptr,
    };

    scope_stack.push(scope);
}

void SemanticAnalyzer::exit_decl() {
    scope_stack.pop();
    sir::Symbol decl = scope_stack.top().decl;
    mod = decl ? &decl.find_mod() : nullptr;
}

void SemanticAnalyzer::enter_block(sir::Block &block) {
    Scope scope{
        .decl = scope_stack.top().decl,
        .decl_block = scope_stack.top().decl_block,
        .block = &block,
        .symbol_table = block.symbol_table,
        .closure_ctx = scope_stack.top().closure_ctx,
    };

    scope_stack.push(scope);
}

void SemanticAnalyzer::enter_closure_ctx(ClosureContext &closure_ctx) {
    Scope scope{
        .decl = scope_stack.top().decl,
        .decl_block = scope_stack.top().decl_block,
        .block = scope_stack.top().block,
        .symbol_table = scope_stack.top().symbol_table,
        .closure_ctx = &closure_ctx,
    };

    scope_stack.push(scope);
}

bool SemanticAnalyzer::is_in_specialization() {
    sir::Symbol decl = get_decl();

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

    std_result_def = &find_std_symbol({"std", "result"}, "Result").as<sir::StructDef>();

    preamble_symbols = {
        {"print", find_std_symbol({"internal", "preamble"}, "print")},
        {"println", find_std_symbol({"internal", "preamble"}, "println")},
        {"assert", find_std_symbol({"internal", "preamble"}, "assert")},
        {"Optional", find_std_symbol({"std", "optional"}, "Optional")},
        {"Result", std_result_def},
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
        for (sir::Specialization<sir::StructDef> &specialization : std_result_def->specializations) {
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

Result SemanticAnalyzer::ensure_interface_analyzed(sir::Symbol symbol, ASTNode *ident_ast_node) {
    if (auto type_alias = symbol.match<sir::TypeAlias>()) {
        if (type_alias->stage >= sir::SemaStage::BODY) {
            return Result::SUCCESS;
        }

        Result result = TypeAliasResolver{*this}.analyze_type_alias(*type_alias);

        if (result == Result::DEF_CYCLE) {
            report_generator.report_err_cyclical_definition(ident_ast_node);
        }

        if (result != Result::SUCCESS) {
            return Result::ERROR;
        }
        return Result::SUCCESS;
    } else {
        DeclInterfaceAnalyzer{*this}.visit_symbol(symbol);
        return Result::SUCCESS;
    }
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

} // namespace sema

} // namespace lang

} // namespace banjo
