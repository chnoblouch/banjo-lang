#include "generics_specializer.hpp"

#include "banjo/sema/decl_interface_analyzer.hpp"
#include "banjo/sema/meta_expansion.hpp"
#include "banjo/sema/symbol_collector.hpp"
#include "banjo/sema/type_alias_resolver.hpp"
#include "banjo/sema/use_resolver.hpp"
#include "banjo/sir/sir_cloner.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

GenericsSpecializer::GenericsSpecializer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

sir::FuncDef *GenericsSpecializer::specialize(sir::FuncDef &generic_func_def, const std::vector<sir::Expr> &args) {
    ASSERT(args.size() == generic_func_def.generic_params.size());

    if (sir::FuncDef *existing_specialization = find_existing_specialization(generic_func_def, args)) {
        return existing_specialization;
    } else {
        return create_specialized_clone(generic_func_def, args);
    }
}

sir::StructDef *GenericsSpecializer::specialize(
    sir::StructDef &generic_struct_def,
    const std::vector<sir::Expr> &args
) {
    ASSERT(args.size() == generic_struct_def.generic_params.size());

    if (sir::StructDef *existing_specialization = find_existing_specialization(generic_struct_def, args)) {
        return existing_specialization;
    } else {
        return create_specialized_clone(generic_struct_def, args);
    }
}

sir::FuncDef *GenericsSpecializer::create_specialized_clone(
    sir::FuncDef &generic_func_def,
    const std::vector<sir::Expr> &args
) {
    sir::Module &def_mod = generic_func_def.find_mod();
    sir::Cloner cloner(def_mod);

    sir::FuncDef *clone = def_mod.create_decl(sir::FuncDef{
        .ast_node = generic_func_def.ast_node,
        .ident = generic_func_def.ident,
        .type = *cloner.clone_func_type(generic_func_def.type),
        .block = cloner.clone_block(generic_func_def.block),
        .generic_params = {},
        .specializations = {},
        .parent_specialization = nullptr,
    });

    generic_func_def.specializations.push_back(sir::Specialization<sir::FuncDef>{
        .args = cloner.clone_expr_list(args),
        .def = clone,
    });

    clone->parent_specialization = &generic_func_def.specializations.back();

    sir::Module *prev_mod = analyzer.cur_sir_mod;

    analyzer.cur_sir_mod = &def_mod;
    analyzer.push_scope().decl = clone;
    analyzer.get_scope().closure_ctx = nullptr;
    analyzer.get_scope().generic_args.clear();

    for (unsigned i = 0; i < args.size(); i++) {
        std::string_view param_name = generic_func_def.generic_params[i].ident.value;
        analyzer.get_scope().generic_args.insert({param_name, args[i]});
    }

    DeclInterfaceAnalyzer(analyzer).process_func_def(*clone);
    analyzer.decls_awaiting_body_analysis.push_back({clone, analyzer.get_scope()});

    analyzer.pop_scope();
    analyzer.cur_sir_mod = prev_mod;

    return clone;
}

sir::StructDef *GenericsSpecializer::create_specialized_clone(
    sir::StructDef &generic_struct_def,
    const std::vector<sir::Expr> &args
) {
    sir::Module &def_mod = generic_struct_def.find_mod();
    sir::Cloner cloner(def_mod);

    sir::StructDef *clone = def_mod.create_decl(sir::StructDef{
        .ast_node = generic_struct_def.ast_node,
        .ident = generic_struct_def.ident,
        .block = cloner.clone_decl_block(generic_struct_def.block),
        .generic_params = {},
        .specializations = {},
        .parent_specialization = nullptr,
    });

    generic_struct_def.specializations.push_back(sir::Specialization<sir::StructDef>{
        .args = cloner.clone_expr_list(args),
        .def = clone,
    });

    clone->parent_specialization = &generic_struct_def.specializations.back();

    sir::Module *prev_mod = analyzer.cur_sir_mod;

    analyzer.cur_sir_mod = &def_mod;
    analyzer.push_empty_scope();
    analyzer.push_scope().decl = clone;

    for (unsigned i = 0; i < args.size(); i++) {
        std::string_view param_name = generic_struct_def.generic_params[i].ident.value;
        analyzer.get_scope().generic_args.insert({param_name, args[i]});
    }

    SymbolCollector(analyzer).collect_in_block(clone->block);
    MetaExpansion(analyzer).run_on_decl_block(clone->block);
    UseResolver(analyzer).resolve_in_block(clone->block);
    TypeAliasResolver(analyzer).analyze_decl_block(clone->block);
    DeclInterfaceAnalyzer(analyzer).process_struct_def(*clone);
    analyzer.decls_awaiting_body_analysis.push_back({clone, analyzer.get_scope()});

    analyzer.pop_scope();
    analyzer.pop_scope();
    analyzer.cur_sir_mod = prev_mod;

    return clone;
}

} // namespace sema

} // namespace lang

} // namespace banjo
