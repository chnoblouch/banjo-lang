#include "generics_specializer.hpp"

#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sir/sir_cloner.hpp"

#include <cassert>

namespace banjo {

namespace lang {

namespace sema {

GenericsSpecializer::GenericsSpecializer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

sir::FuncDef *GenericsSpecializer::specialize(sir::FuncDef &generic_func_def, const std::vector<sir::Expr> &args) {
    assert(args.size() == generic_func_def.generic_params.size());

    analyzer.push_scope();

    for (unsigned i = 0; i < args.size(); i++) {
        std::string_view param_name = generic_func_def.generic_params[i].ident.value;
        analyzer.get_scope().generic_args.insert({param_name, args[i]});
    }

    sir::SIRCloner cloner(*analyzer.cur_sir_mod);

    sir::FuncDef *specialization = analyzer.cur_sir_mod->create_decl(sir::FuncDef{
        .ast_node = generic_func_def.ast_node,
        .ident = generic_func_def.ident,
        .type = *cloner.clone_func_type(generic_func_def.type),
        .block = cloner.clone_block(generic_func_def.block),
        .generic_params = {},
        .specializations = {},
    });

    DeclInterfaceAnalyzer(analyzer).analyze_func_def(*specialization);
    DeclBodyAnalyzer(analyzer).analyze_func_def(*specialization);
    generic_func_def.specializations.push_back(specialization);

    analyzer.pop_scope();

    return specialization;
}

} // namespace sema

} // namespace lang

} // namespace banjo
