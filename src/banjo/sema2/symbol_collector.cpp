#include "symbol_collector.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

SymbolCollector::SymbolCollector(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void SymbolCollector::collect() {
    for (sir::Module *mod : analyzer.sir_unit.mods) {
        analyzer.enter_mod(mod);
        collect_in_block(mod->block);
        analyzer.exit_mod();
    }
}

void SymbolCollector::collect_in_block(sir::DeclBlock &decl_block) {
    analyzer.check_for_completeness(decl_block);

    for (sir::Decl &decl : decl_block.decls) {
        collect_decl(decl);
    }
}

void SymbolCollector::collect_in_meta_block(sir::MetaBlock &meta_block) {
    for (sir::Node &node : meta_block.nodes) {
        collect_decl(node.as<sir::Decl>());
    }
}

void SymbolCollector::collect_decl(sir::Decl &decl) {
    SIR_VISIT_DECL(
        decl,
        SIR_VISIT_IMPOSSIBLE,
        collect_func_def(*inner),
        collect_native_func_decl(*inner),
        collect_const_def(*inner),
        collect_struct_def(*inner),
        SIR_VISIT_IMPOSSIBLE,
        collect_var_decl(*inner),
        collect_native_var_decl(*inner),
        collect_enum_def(*inner),
        collect_enum_variant(*inner),
        collect_union_def(*inner),
        collect_union_case(*inner),
        collect_type_alias(*inner),
        collect_use_decl(*inner),
        SIR_VISIT_IGNORE,
        SIR_VISIT_IGNORE
    );
}

void SymbolCollector::collect_func_def(sir::FuncDef &func_def) {
    sir::Symbol &cur_entry = get_symbol_table().symbols[func_def.ident.value];

    if (!cur_entry) {
        cur_entry = &func_def;
    } else if (auto cur_func_def = cur_entry.match<sir::FuncDef>()) {
        cur_entry = analyzer.cur_sir_mod->create_overload_set({
            .func_defs = {cur_func_def, &func_def},
        });
    } else if (auto cur_overload_set = cur_entry.match<sir::OverloadSet>()) {
        cur_overload_set->func_defs.push_back(&func_def);
    }
}

void SymbolCollector::collect_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    get_symbol_table().symbols.insert({native_func_decl.ident.value, &native_func_decl});
}

void SymbolCollector::collect_const_def(sir::ConstDef &const_def) {
    get_symbol_table().symbols.insert({const_def.ident.value, &const_def});
}

void SymbolCollector::collect_struct_def(sir::StructDef &struct_def) {
    get_symbol_table().symbols.insert({struct_def.ident.value, &struct_def});

    if (!struct_def.is_generic()) {
        analyzer.enter_struct_def(&struct_def);
        collect_in_block(struct_def.block);
        analyzer.exit_struct_def();
    }
}

void SymbolCollector::collect_var_decl(sir::VarDecl &var_decl) {
    if (!analyzer.get_scope().struct_def) {
        get_symbol_table().symbols.insert({var_decl.ident.value, &var_decl});
    }
}

void SymbolCollector::collect_native_var_decl(sir::NativeVarDecl &native_var_decl) {
    get_symbol_table().symbols.insert({native_var_decl.ident.value, &native_var_decl});
}

void SymbolCollector::collect_enum_def(sir::EnumDef &enum_def) {
    get_symbol_table().symbols.insert({enum_def.ident.value, &enum_def});

    analyzer.enter_enum_def(&enum_def);
    collect_in_block(enum_def.block);
    analyzer.exit_enum_def();
}

void SymbolCollector::collect_enum_variant(sir::EnumVariant &enum_variant) {
    get_symbol_table().symbols.insert({enum_variant.ident.value, &enum_variant});
}

void SymbolCollector::collect_union_def(sir::UnionDef &union_def) {
    get_symbol_table().symbols.insert({union_def.ident.value, &union_def});

    analyzer.enter_union_def(&union_def);
    collect_in_block(union_def.block);
    analyzer.exit_union_def();
}

void SymbolCollector::collect_union_case(sir::UnionCase &union_case) {
    get_symbol_table().symbols.insert({union_case.ident.value, &union_case});
}

void SymbolCollector::collect_type_alias(sir::TypeAlias &type_alias) {
    get_symbol_table().symbols.insert({type_alias.ident.value, &type_alias});
}

void SymbolCollector::collect_use_decl(sir::UseDecl &use_decl) {
    collect_use_item(use_decl.root_item);
}

void SymbolCollector::collect_use_item(sir::UseItem &use_item) {
    if (auto use_ident = use_item.match<sir::UseIdent>()) collect_use_ident(*use_ident);
    else if (auto use_rebind = use_item.match<sir::UseRebind>()) collect_use_rebind(*use_rebind);
    else if (auto use_dot_expr = use_item.match<sir::UseDotExpr>()) collect_use_dot_expr(*use_dot_expr);
    else if (auto use_list = use_item.match<sir::UseList>()) collect_use_list(*use_list);
    else ASSERT_UNREACHABLE;
}

void SymbolCollector::collect_use_ident(sir::UseIdent &use_ident) {
    get_symbol_table().symbols.insert({use_ident.ident.value, &use_ident});
}

void SymbolCollector::collect_use_rebind(sir::UseRebind &use_rebind) {
    get_symbol_table().symbols.insert({use_rebind.local_ident.value, &use_rebind});
}

void SymbolCollector::collect_use_dot_expr(sir::UseDotExpr &use_dot_expr) {
    if (auto use_ident = use_dot_expr.rhs.match<sir::UseIdent>()) collect_use_ident(*use_ident);
    else if (auto use_rebind = use_dot_expr.rhs.match<sir::UseRebind>()) collect_use_rebind(*use_rebind);
    else if (auto use_list = use_dot_expr.rhs.match<sir::UseList>()) collect_use_list(*use_list);
    else ASSERT_UNREACHABLE;
}

void SymbolCollector::collect_use_list(sir::UseList &use_list) {
    for (sir::UseItem &use_item : use_list.items) {
        collect_use_item(use_item);
    }
}

sir::SymbolTable &SymbolCollector::get_symbol_table() {
    return *analyzer.get_scope().decl_block->symbol_table;
}

} // namespace sema

} // namespace lang

} // namespace banjo
