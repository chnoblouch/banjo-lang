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
            PathContext ctx{
                .mod = nullptr,
                .symbol_table = nullptr,
            };

            resolve_use_item(use_decl->root_item, ctx);
        }
    }
}

void UseResolver::resolve_use_item(sir::UseItem &use_item, PathContext &inout_ctx) {
    if (auto use_ident = use_item.match<sir::UseIdent>()) resolve_use_ident(*use_ident, inout_ctx);
    else if (auto use_rebind = use_item.match<sir::UseRebind>()) resolve_use_rebind(*use_rebind, inout_ctx);
    else if (auto use_dot_expr = use_item.match<sir::UseDotExpr>()) resolve_use_dot_expr(*use_dot_expr, inout_ctx);
    else if (auto use_list = use_item.match<sir::UseList>()) resolve_use_list(*use_list, inout_ctx);
    else ASSERT_UNREACHABLE;
}

void UseResolver::resolve_use_ident(sir::UseIdent &use_ident, PathContext &inout_ctx) {
    use_ident.symbol = resolve_symbol(use_ident.value, inout_ctx);
}

void UseResolver::resolve_use_rebind(sir::UseRebind &use_rebind, PathContext &inout_ctx) {
    use_rebind.symbol = resolve_symbol(use_rebind.target_ident.value, inout_ctx);
}

void UseResolver::resolve_use_dot_expr(sir::UseDotExpr &use_dot_expr, PathContext &inout_ctx) {
    resolve_use_item(use_dot_expr.lhs, inout_ctx);
    resolve_use_item(use_dot_expr.rhs, inout_ctx);
}

void UseResolver::resolve_use_list(sir::UseList &use_list, PathContext &inout_ctx) {
    for (sir::UseItem &use_item : use_list.items) {
        PathContext ctx_copy = inout_ctx;
        resolve_use_item(use_item, ctx_copy);
    }
}

sir::Symbol UseResolver::resolve_symbol(const std::string &name, PathContext &inout_ctx) {
    if (!inout_ctx.mod) {
        sir::Module *mod = analyzer.sir_unit.mods_by_path[{name}];
        inout_ctx.mod = mod;
        inout_ctx.symbol_table = mod->block.symbol_table;
        return mod;
    }

    sir::Symbol symbol = inout_ctx.symbol_table->look_up(name);
    if (symbol) {
        sir::SymbolTable *new_symbol_table = symbol.get_symbol_table();
        if (new_symbol_table) {
            inout_ctx.symbol_table = new_symbol_table;
        }

        return symbol;
    }

    ModulePath sub_mod_path = inout_ctx.mod->path;
    sub_mod_path.append(name);
    sir::Module *sub_mod = analyzer.sir_unit.mods_by_path[sub_mod_path];

    if (sub_mod) {
        inout_ctx.mod = sub_mod;
        inout_ctx.symbol_table = sub_mod->block.symbol_table;
        return sub_mod;
    }

    ASSERT_UNREACHABLE;
}

} // namespace sema

} // namespace lang

} // namespace banjo
