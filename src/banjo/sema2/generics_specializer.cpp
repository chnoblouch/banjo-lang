#include "generics_specializer.hpp"

#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sema2/decl_value_analyzer.hpp"
#include "banjo/sema2/meta_expansion.hpp"
#include "banjo/sema2/symbol_collector.hpp"
#include "banjo/sir/sir_cloner.hpp"

#include <cassert>

namespace banjo {

namespace lang {

namespace sema {

GenericsSpecializer::GenericsSpecializer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

sir::FuncDef *GenericsSpecializer::specialize(sir::FuncDef &generic_func_def, const std::vector<sir::Expr> &args) {
    assert(args.size() == generic_func_def.generic_params.size());

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
    assert(args.size() == generic_struct_def.generic_params.size());

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
    sir::Cloner cloner(*analyzer.cur_sir_mod);

    sir::FuncDef *clone = analyzer.cur_sir_mod->create_decl(sir::FuncDef{
        .ast_node = generic_func_def.ast_node,
        .ident = generic_func_def.ident,
        .type = *cloner.clone_func_type(generic_func_def.type),
        .block = cloner.clone_block(generic_func_def.block),
        .generic_params = {},
        .specializations = {},
    });

    generic_func_def.specializations.push_back(sir::Specialization<sir::FuncDef>{
        .args = args,
        .def = clone,
    });

    analyzer.push_scope().func_def = clone;

    for (unsigned i = 0; i < args.size(); i++) {
        std::string_view param_name = generic_func_def.generic_params[i].ident.value;
        analyzer.get_scope().generic_args.insert({param_name, args[i]});
    }

    DeclInterfaceAnalyzer(analyzer).process_func_def(*clone);
    DeclBodyAnalyzer(analyzer).process_func_def(*clone);

    analyzer.pop_scope();
    return clone;
}

sir::StructDef *GenericsSpecializer::create_specialized_clone(
    sir::StructDef &generic_struct_def,
    const std::vector<sir::Expr> &args
) {
    sir::Cloner cloner(*analyzer.cur_sir_mod);

    sir::StructDef *clone = analyzer.cur_sir_mod->create_decl(sir::StructDef{
        .ast_node = generic_struct_def.ast_node,
        .ident = generic_struct_def.ident,
        .block = cloner.clone_decl_block(generic_struct_def.block),
        .generic_params = {},
        .specializations = {},
    });

    generic_struct_def.specializations.push_back(sir::Specialization<sir::StructDef>{
        .args = args,
        .def = clone,
    });

    analyzer.enter_struct_def(clone);

    for (unsigned i = 0; i < args.size(); i++) {
        std::string_view param_name = generic_struct_def.generic_params[i].ident.value;
        analyzer.get_scope().generic_args.insert({param_name, args[i]});
    }

    SymbolCollector(analyzer).collect_in_block(clone->block);
    DeclInterfaceAnalyzer(analyzer).process_struct_def(*clone);
    DeclValueAnalyzer(analyzer).process_struct_def(*clone);
    MetaExpansion(analyzer).run_on_decl_block(clone->block);
    DeclBodyAnalyzer(analyzer).process_struct_def(*clone);

    analyzer.exit_struct_def();
    return clone;
}

} // namespace sema

} // namespace lang

} // namespace banjo
