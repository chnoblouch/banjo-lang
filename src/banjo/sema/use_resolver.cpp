#include "use_resolver.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

UseResolver::UseResolver(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void UseResolver::resolve(const std::vector<sir::Module *> &mods) {
    for (sir::Module *mod : mods) {
        analyzer.enter_decl(mod);
        resolve_in_block(mod->block);
        analyzer.exit_decl();
    }
}

void UseResolver::resolve_in_block(sir::DeclBlock &decl_block) {
    for (sir::Decl &decl : decl_block.decls) {
        if (auto use_decl = decl.match<sir::UseDecl>()) {
            resolve_use_decl(*use_decl);
        }
    }
}

Result UseResolver::resolve_use_decl_remotely(sir::UseDecl &use_decl, sir::Symbol symbol_to_analyze) {
    this->symbol_to_analyze = symbol_to_analyze;
    Result result = resolve_use_decl(use_decl);
    this->symbol_to_analyze = nullptr;

    return result;
}

Result UseResolver::resolve_use_decl(sir::UseDecl &use_decl) {
    sir::Symbol symbol = nullptr;
    return resolve_use_item(use_decl.root_item, symbol, true);
}

Result UseResolver::resolve_use_item(sir::UseItem &use_item, sir::Symbol &symbol, bool is_leaf) {
    if (auto use_ident = use_item.match<sir::UseIdent>()) {
        return resolve_use_ident(*use_ident, symbol, is_leaf);
    } else if (auto use_rebind = use_item.match<sir::UseRebind>()) {
        return resolve_use_rebind(*use_rebind, symbol);
    } else if (auto use_dot_expr = use_item.match<sir::UseDotExpr>()) {
        return resolve_use_dot_expr(*use_dot_expr, symbol, is_leaf);
    } else if (auto use_list = use_item.match<sir::UseList>()) {
        return resolve_use_list(*use_list, symbol);
    } else {
        return Result::SUCCESS;
    }
}

Result UseResolver::resolve_use_ident(sir::UseIdent &use_ident, sir::Symbol &symbol, bool is_leaf) {
    if (use_ident.stage >= sir::SemaStage::BODY) {
        symbol = use_ident.symbol;
        return Result::SUCCESS;
    }

    if (is_leaf && symbol_to_analyze && symbol_to_analyze != sir::Symbol{&use_ident}) {
        return Result::SUCCESS;
    }

    if (std::ranges::count(item_stack, sir::UseItem{&use_ident})) {
        analyzer.report_generator.report_err_cyclical_definition(use_ident.ident.ast_node);
        return Result::DEF_CYCLE;
    }

    item_stack.push_back(&use_ident);

    use_ident.symbol = resolve_symbol(use_ident.ident, symbol);
    analyzer.add_symbol_use(use_ident.ident.ast_node, symbol);

    item_stack.pop_back();

    use_ident.stage = sir::SemaStage::BODY;
    return use_ident.symbol ? Result::SUCCESS : Result::ERROR;
}

Result UseResolver::resolve_use_rebind(sir::UseRebind &use_rebind, sir::Symbol &symbol) {
    if (use_rebind.stage >= sir::SemaStage::BODY) {
        symbol = use_rebind.symbol;
        return Result::SUCCESS;
    }

    if (symbol_to_analyze && symbol_to_analyze != sir::Symbol{&use_rebind}) {
        return Result::SUCCESS;
    }

    if (std::ranges::count(item_stack, sir::UseItem{&use_rebind})) {
        analyzer.report_generator.report_err_cyclical_definition(use_rebind.ast_node);
        return Result::DEF_CYCLE;
    }

    item_stack.push_back(&use_rebind);

    use_rebind.symbol = resolve_symbol(use_rebind.target_ident, symbol);
    analyzer.add_symbol_use(use_rebind.target_ident.ast_node, symbol);
    analyzer.add_symbol_use(use_rebind.local_ident.ast_node, symbol);

    item_stack.pop_back();

    use_rebind.stage = sir::SemaStage::BODY;
    return use_rebind.symbol ? Result::SUCCESS : Result::ERROR;
}

Result UseResolver::resolve_use_dot_expr(sir::UseDotExpr &use_dot_expr, sir::Symbol &symbol, bool is_leaf) {
    Result partial_result;

    partial_result = resolve_use_item(use_dot_expr.lhs, symbol, false);
    if (partial_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    partial_result = resolve_use_item(use_dot_expr.rhs, symbol, is_leaf);
    if (partial_result != Result::SUCCESS) {
        return partial_result;
    }

    return Result::SUCCESS;
}

Result UseResolver::resolve_use_list(sir::UseList &use_list, sir::Symbol &symbol) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::UseItem &use_item : use_list.items) {
        sir::Symbol symbol_copy = symbol;
        partial_result = resolve_use_item(use_item, symbol_copy, true);

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

sir::Symbol UseResolver::resolve_symbol(sir::Ident &ident, sir::Symbol &symbol) {
    if (!symbol) {
        return resolve_module(ident, symbol);
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

    sir::SymbolTable &symbol_table = *symbol.get_symbol_table();
    sir::Symbol new_symbol = symbol_table.look_up_local(ident.value);

    if (!new_symbol) {
        new_symbol = analyzer.symbol_ctx.try_resolve_meta_if(symbol_table, ident.value);
    }

    if (new_symbol) {
        if (auto use_ident = new_symbol.match<sir::UseIdent>()) {
            resolve_use_decl(use_ident->parent);
            new_symbol = use_ident->symbol;
        } else if (auto use_rebind = new_symbol.match<sir::UseRebind>()) {
            resolve_use_decl(use_rebind->parent);
            new_symbol = use_rebind->symbol;
        }

        symbol = new_symbol;
        return new_symbol;
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
