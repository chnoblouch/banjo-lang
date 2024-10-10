#include "use_resolver.hpp"

#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

UseResolver::UseResolver(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void UseResolver::resolve() {
    for (sir::Module *mod : analyzer.sir_unit.mods) {
        analyzer.enter_mod(mod);
        resolve_in_block(mod->block);
        analyzer.exit_mod();
    }
}

void UseResolver::resolve_in_block(sir::DeclBlock &decl_block) {
    for (sir::Decl &decl : decl_block.decls) {
        if (auto use_decl = decl.match<sir::UseDecl>()) {
            sir::Symbol symbol = nullptr;
            resolve_use_item(use_decl->root_item, symbol);
        }
    }
}

void UseResolver::resolve_in_meta_block(sir::MetaBlock &meta_block) {
    for (sir::Node &node : meta_block.nodes) {
        if (auto use_decl = node.as<sir::Decl>().match<sir::UseDecl>()) {
            sir::Symbol symbol = nullptr;
            resolve_use_item(use_decl->root_item, symbol);
        }
    }
}

void UseResolver::resolve_use_item(sir::UseItem &use_item, sir::Symbol &symbol) {
    if (auto use_ident = use_item.match<sir::UseIdent>()) resolve_use_ident(*use_ident, symbol);
    else if (auto use_rebind = use_item.match<sir::UseRebind>()) resolve_use_rebind(*use_rebind, symbol);
    else if (auto use_dot_expr = use_item.match<sir::UseDotExpr>()) resolve_use_dot_expr(*use_dot_expr, symbol);
    else if (auto use_list = use_item.match<sir::UseList>()) resolve_use_list(*use_list, symbol);
    else ASSERT_UNREACHABLE;
}

void UseResolver::resolve_use_ident(sir::UseIdent &use_ident, sir::Symbol &symbol) {
    use_ident.symbol = resolve_symbol(use_ident.ident, symbol);
    analyzer.add_symbol_use(use_ident.ident.ast_node, symbol);
}

void UseResolver::resolve_use_rebind(sir::UseRebind &use_rebind, sir::Symbol &symbol) {
    use_rebind.symbol = resolve_symbol(use_rebind.target_ident, symbol);
}

void UseResolver::resolve_use_dot_expr(sir::UseDotExpr &use_dot_expr, sir::Symbol &symbol) {
    resolve_use_item(use_dot_expr.lhs, symbol);
    resolve_use_item(use_dot_expr.rhs, symbol);
}

void UseResolver::resolve_use_list(sir::UseList &use_list, sir::Symbol &symbol) {
    for (sir::UseItem &use_item : use_list.items) {
        sir::Symbol symbol_copy = symbol;
        resolve_use_item(use_item, symbol_copy);
    }
}

sir::Symbol UseResolver::resolve_symbol(sir::Ident &ident, sir::Symbol &symbol) {
    if (!symbol) {
        return resolve_module(ident, symbol);
    }

    sir::Symbol new_symbol = symbol.get_symbol_table()->look_up(ident.value);
    if (new_symbol) {
        symbol = new_symbol;
        return new_symbol;
    }

    if (auto mod = symbol.match<sir::Module>()) {
        ModulePath sub_mod_path = mod->path;
        sub_mod_path.append(ident.value);
        sir::Module *sub_mod = analyzer.sir_unit.mods_by_path[sub_mod_path];

        if (sub_mod) {
            symbol = sub_mod;
            return sub_mod;
        }
    }

    analyzer.report_generator.report_err_symbol_not_found_in(ident, symbol.get_name());
    return nullptr;
}

sir::Symbol UseResolver::resolve_module(sir::Ident &ident, sir::Symbol &symbol) {
    auto iter = analyzer.sir_unit.mods_by_path.find({ident.value});
    if (iter == analyzer.sir_unit.mods_by_path.end()) {
        analyzer.report_generator.report_err_module_not_found(ident);
        return nullptr;
    }

    sir::Module *mod = iter->second;
    symbol = mod;
    return mod;
}

} // namespace sema

} // namespace lang

} // namespace banjo
