#include "symbol_collector.hpp"

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

SymbolCollector::SymbolCollector(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void SymbolCollector::collect(const std::vector<sir::Module *> &mods) {
    for (sir::Module *mod : mods) {
        analyzer.enter_mod(mod);
        collect_in_block(mod->block);
        analyzer.exit_mod();
    }
}

void SymbolCollector::collect_in_block(sir::DeclBlock &decl_block) {
    for (sir::Decl &decl : decl_block.decls) {
        collect_decl(decl);
    }

    for (sir::Decl &decl : decl_block.decls) {
        if (decl.is<sir::MetaIfStmt>()) {
            analyzer.incomplete_decl_blocks.insert({&decl_block, analyzer.get_scope()});
            break;
        }
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
        SIR_VISIT_IMPOSSIBLE,             // empty
        collect_func_def(*inner),         // func_def
        collect_func_decl(*inner),        // func_decl
        collect_native_func_decl(*inner), // native_func_decl
        collect_const_def(*inner),        // const_def
        collect_struct_def(*inner),       // struct_def
        SIR_VISIT_IMPOSSIBLE,             // struct_field
        collect_var_decl(*inner),         // var_decl
        collect_native_var_decl(*inner),  // native_var_decl
        collect_enum_def(*inner),         // enum_def
        collect_enum_variant(*inner),     // enum_variant
        collect_union_def(*inner),        // union_def
        collect_union_case(*inner),       // union_case
        collect_proto_def(*inner),        // proto_def
        collect_type_alias(*inner),       // type_alias
        collect_use_decl(*inner),         // use_decl
        SIR_VISIT_IGNORE,                 // meta_if_stmt
        SIR_VISIT_IGNORE,                 // expanded_meta_stmt
        SIR_VISIT_IGNORE,                 // error
        SIR_VISIT_IGNORE                  // completion_token
    );
}

void SymbolCollector::collect_func_def(sir::FuncDef &func_def) {
    func_def.parent = analyzer.get_scope().decl;

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

    analyzer.add_symbol_def(&func_def);

    if (auto proto_def = analyzer.get_scope().decl.match<sir::ProtoDef>()) {
        proto_def->func_decls.push_back(sir::ProtoFuncDecl{
            .decl = &func_def,
        });
    }
}

void SymbolCollector::collect_func_decl(sir::FuncDecl &func_decl) {
    func_decl.parent = analyzer.get_scope().decl;
    add_symbol(func_decl.ident.value, &func_decl);
    analyzer.add_symbol_def(&func_decl);

    if (auto proto_def = analyzer.get_scope().decl.match<sir::ProtoDef>()) {
        proto_def->func_decls.push_back(sir::ProtoFuncDecl{
            .decl = &func_decl,
        });
    }
}

void SymbolCollector::collect_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    add_symbol(native_func_decl.ident.value, &native_func_decl);
    analyzer.add_symbol_def(&native_func_decl);
}

void SymbolCollector::collect_const_def(sir::ConstDef &const_def) {
    add_symbol(const_def.ident.value, &const_def);
    analyzer.add_symbol_def(&const_def);
}

void SymbolCollector::collect_struct_def(sir::StructDef &struct_def) {
    struct_def.parent = analyzer.get_scope().decl;
    add_symbol(struct_def.ident.value, &struct_def);
    analyzer.add_symbol_def(&struct_def);

    if (!struct_def.is_generic()) {
        collect_in_decl(&struct_def, struct_def.block);
    }
}

void SymbolCollector::collect_var_decl(sir::VarDecl &var_decl) {
    if (!analyzer.get_scope().decl.is<sir::StructDef>()) {
        add_symbol(var_decl.ident.value, &var_decl);
        analyzer.add_symbol_def(&var_decl);
    }
}

void SymbolCollector::collect_native_var_decl(sir::NativeVarDecl &native_var_decl) {
    add_symbol(native_var_decl.ident.value, &native_var_decl);
    analyzer.add_symbol_def(&native_var_decl);
}

void SymbolCollector::collect_enum_def(sir::EnumDef &enum_def) {
    add_symbol(enum_def.ident.value, &enum_def);
    analyzer.add_symbol_def(&enum_def);
    collect_in_decl(&enum_def, enum_def.block);
}

void SymbolCollector::collect_enum_variant(sir::EnumVariant &enum_variant) {
    add_symbol(enum_variant.ident.value, &enum_variant);
    analyzer.add_symbol_def(&enum_variant);
}

void SymbolCollector::collect_union_def(sir::UnionDef &union_def) {
    add_symbol(union_def.ident.value, &union_def);
    analyzer.add_symbol_def(&union_def);
    collect_in_decl(&union_def, union_def.block);
}

void SymbolCollector::collect_union_case(sir::UnionCase &union_case) {
    add_symbol(union_case.ident.value, &union_case);
    analyzer.add_symbol_def(&union_case);
}

void SymbolCollector::collect_proto_def(sir::ProtoDef &proto_def) {
    add_symbol(proto_def.ident.value, &proto_def);
    analyzer.add_symbol_def(&proto_def);
    collect_in_decl(&proto_def, proto_def.block);
}

void SymbolCollector::collect_type_alias(sir::TypeAlias &type_alias) {
    add_symbol(type_alias.ident.value, &type_alias);
    analyzer.add_symbol_def(&type_alias);
}

void SymbolCollector::collect_use_decl(sir::UseDecl &use_decl) {
    collect_use_item(use_decl.root_item);
}

void SymbolCollector::collect_use_item(sir::UseItem &use_item) {
    if (auto use_ident = use_item.match<sir::UseIdent>()) collect_use_ident(*use_ident);
    else if (auto use_rebind = use_item.match<sir::UseRebind>()) collect_use_rebind(*use_rebind);
    else if (auto use_dot_expr = use_item.match<sir::UseDotExpr>()) collect_use_dot_expr(*use_dot_expr);
    else if (auto use_list = use_item.match<sir::UseList>()) collect_use_list(*use_list);
}

void SymbolCollector::collect_use_ident(sir::UseIdent &use_ident) {
    add_symbol(use_ident.ident.value, &use_ident);
}

void SymbolCollector::collect_use_rebind(sir::UseRebind &use_rebind) {
    add_symbol(use_rebind.local_ident.value, &use_rebind);
}

void SymbolCollector::collect_use_dot_expr(sir::UseDotExpr &use_dot_expr) {
    if (analyzer.mode == Mode::COMPLETION) {
        if (use_dot_expr.rhs.is<sir::CompletionToken>()) {
            analyzer.completion_context = CompleteAfterUseDot{
                .lhs = use_dot_expr.lhs,
            };
        }
    }

    if (auto use_ident = use_dot_expr.rhs.match<sir::UseIdent>()) collect_use_ident(*use_ident);
    else if (auto use_rebind = use_dot_expr.rhs.match<sir::UseRebind>()) collect_use_rebind(*use_rebind);
    else if (auto use_list = use_dot_expr.rhs.match<sir::UseList>()) collect_use_list(*use_list);
}

void SymbolCollector::collect_use_list(sir::UseList &use_list) {
    for (sir::UseItem &use_item : use_list.items) {
        collect_use_item(use_item);
    }
}

void SymbolCollector::add_symbol(std::string_view name, sir::Symbol symbol) {
    get_symbol_table().symbols.insert({name, symbol});
}

sir::SymbolTable &SymbolCollector::get_symbol_table() {
    return *analyzer.get_scope().decl.get_symbol_table();
}

void SymbolCollector::collect_in_decl(sir::Symbol decl, sir::DeclBlock &block) {
    analyzer.push_scope().decl = decl;
    collect_in_block(block);
    analyzer.pop_scope();
}

} // namespace sema

} // namespace lang

} // namespace banjo
