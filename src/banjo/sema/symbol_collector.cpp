#include "symbol_collector.hpp"

#include "banjo/sema/completion_context.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"

#include <memory>

namespace banjo {

namespace lang {

namespace sema {

SymbolCollector::SymbolCollector(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void SymbolCollector::collect(const std::vector<sir::Module *> &mods) {
    for (sir::Module *mod : mods) {
        collect_in_mod(*mod);
    }
}

void SymbolCollector::collect_in_mod(sir::Module &mod) {
    mod.sema_index = create_decl_state();

    DeclState &state = analyzer.decl_states[*mod.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = &mod,
        .decl = &mod,
        .decl_block = &mod.block,
    });

    analyzer.enter_decl_scope(*state.scope);
    collect_in_block(mod.block);
    analyzer.exit_decl_scope();
}

void SymbolCollector::collect_in_block(sir::DeclBlock &decl_block) {
    for (unsigned i = 0; i < decl_block.decls.size(); i++) {
        collect_decl(decl_block.decls[i], i);
    }
}

void SymbolCollector::collect_func_specialization(sir::FuncDef &generic_def, sir::FuncDef &specialization) {
    DeclScope &generic_def_scope = *analyzer.decl_states[*generic_def.sema_index].scope;

    specialization.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*specialization.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = generic_def_scope.mod,
        .decl = &specialization,
        .decl_block = generic_def_scope.decl_block,
        .generic_args = generic_def_scope.generic_args,
        .closure_ctx = generic_def_scope.closure_ctx,
    });

    std::span<sir::Expr> args = specialization.parent_specialization->args;

    for (unsigned i = 0; i < args.size(); i++) {
        std::string_view param_name = generic_def.generic_params[i].ident.value;
        state.scope->generic_args.insert({param_name, args[i]});
    }
}

void SymbolCollector::collect_struct_specialization(sir::StructDef &generic_def, sir::StructDef &specialization) {
    DeclScope &generic_def_scope = *analyzer.decl_states[*generic_def.sema_index].scope;

    specialization.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*specialization.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = generic_def_scope.mod,
        .decl = &specialization,
        .decl_block = &specialization.block,
        .generic_args = generic_def_scope.generic_args,
        .closure_ctx = generic_def_scope.closure_ctx,
    });

    std::span<sir::Expr> args = specialization.parent_specialization->args;

    for (unsigned i = 0; i < args.size(); i++) {
        std::string_view param_name = generic_def.generic_params[i].ident.value;
        state.scope->generic_args.insert({param_name, args[i]});
    }

    analyzer.enter_decl_scope(*state.scope);
    collect_in_block(specialization.block);
    analyzer.exit_decl_scope();
}

ClosureContext &SymbolCollector::collect_closure_func(sir::FuncDef &func_def, sir::TupleExpr *data_type) {
    func_def.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*func_def.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &func_def,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.closure_ctxs.create(
            ClosureContext{
                .captured_vars = {},
                .data_type = data_type,
                .parent_block = &analyzer.get_block(),
            }
        ),
    });

    func_def.parent = analyzer.get_decl_scope().decl.get_parent();

    return *state.scope->closure_ctx;
}

void SymbolCollector::collect_decl(sir::Decl &decl, unsigned index) {
    SIR_VISIT_DECL(
        decl,
        SIR_VISIT_IMPOSSIBLE,                   // empty
        collect_func_def(*inner),               // func_def
        collect_func_decl(*inner),              // func_decl
        collect_native_func_decl(*inner),       // native_func_decl
        collect_const_def(*inner),              // const_def
        collect_struct_def(*inner),             // struct_def
        SIR_VISIT_IMPOSSIBLE,                   // struct_field
        collect_var_decl(*inner),               // var_decl
        collect_native_var_decl(*inner),        // native_var_decl
        collect_enum_def(*inner),               // enum_def
        collect_enum_variant(*inner),           // enum_variant
        collect_union_def(*inner),              // union_def
        collect_union_case(*inner),             // union_case
        collect_proto_def(*inner),              // proto_def
        collect_type_alias(*inner),             // type_alias
        collect_use_decl(*inner),               // use_decl
        collect_in_meta_if_stmt(*inner, index), // meta_if_stmt
        SIR_VISIT_IGNORE,                       // expanded_meta_stmt
        SIR_VISIT_IGNORE                        // error
    );
}

void SymbolCollector::collect_func_def(sir::FuncDef &func_def) {
    if (guarded_scope) {
        analyzer.add_symbol_def(&func_def);
        return;
    }

    func_def.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*func_def.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &func_def,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    func_def.parent = analyzer.get_decl_scope().decl;

    analyzer.enter_decl_scope(*state.scope);
    sir::Symbol &cur_entry = get_symbol_table().symbols[func_def.ident.value];

    if (!cur_entry) {
        cur_entry = &func_def;
    } else if (auto cur_func_def = cur_entry.match<sir::FuncDef>()) {
        cur_entry = analyzer.create(
            sir::OverloadSet{
                .func_defs = {cur_func_def, &func_def},
            }
        );
    } else if (auto cur_overload_set = cur_entry.match<sir::OverloadSet>()) {
        cur_overload_set->func_defs.push_back(&func_def);
    }

    // if (analyzer.symbol_ctx.is_guarded()) {
    //     sir::GuardedSymbol::Variant variant{
    //         .truth_table = analyzer.symbol_ctx.get_meta_condition(),
    //         .symbol = cur_entry,
    //     };

    //     // TODO
    //     cur_entry = analyzer.create(
    //         sir::GuardedSymbol{
    //             .variants{variant},
    //         }
    //     );
    // }

    analyzer.add_symbol_def(&func_def);

    if (auto proto_def = func_def.parent.match<sir::ProtoDef>()) {
        if (func_def.is_method()) {
            proto_def->func_decls.push_back(
                sir::ProtoFuncDecl{
                    .decl = &func_def,
                }
            );
        }
    }

    analyzer.exit_decl_scope();
}

void SymbolCollector::collect_func_decl(sir::FuncDecl &func_decl) {
    func_decl.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*func_decl.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &func_decl,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    func_decl.parent = analyzer.get_decl_scope().decl;

    add_symbol(func_decl.ident.value, &func_decl);
    analyzer.add_symbol_def(&func_decl);

    if (auto proto_def = func_decl.parent.match<sir::ProtoDef>()) {
        proto_def->func_decls.push_back(
            sir::ProtoFuncDecl{
                .decl = &func_decl,
            }
        );
    }
}

void SymbolCollector::collect_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    native_func_decl.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*native_func_decl.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &native_func_decl,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    add_symbol(native_func_decl.ident.value, &native_func_decl);
    analyzer.add_symbol_def(&native_func_decl);
}

void SymbolCollector::collect_const_def(sir::ConstDef &const_def) {
    const_def.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*const_def.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &const_def,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    add_symbol(const_def.ident.value, &const_def);
    analyzer.add_symbol_def(&const_def);
}

void SymbolCollector::collect_struct_def(sir::StructDef &struct_def) {
    struct_def.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*struct_def.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &struct_def,
        .decl_block = &struct_def.block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    struct_def.parent = analyzer.get_decl_scope().decl;

    if (!struct_def.is_generic()) {
        analyzer.enter_decl_scope(*state.scope);
        collect_in_block(struct_def.block);
        analyzer.exit_decl_scope();
    }

    add_symbol(struct_def.ident.value, &struct_def);
    analyzer.add_symbol_def(&struct_def);
}

void SymbolCollector::collect_var_decl(sir::VarDecl &var_decl) {
    var_decl.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*var_decl.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &var_decl,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    var_decl.parent = analyzer.get_decl_scope().decl;

    if (!state.scope->decl.is<sir::StructDef>()) {
        add_symbol(var_decl.ident.value, &var_decl);
        analyzer.add_symbol_def(&var_decl);
    }
}

void SymbolCollector::collect_native_var_decl(sir::NativeVarDecl &native_var_decl) {
    native_var_decl.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*native_var_decl.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &native_var_decl,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    add_symbol(native_var_decl.ident.value, &native_var_decl);
    analyzer.add_symbol_def(&native_var_decl);
}

void SymbolCollector::collect_enum_def(sir::EnumDef &enum_def) {
    enum_def.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*enum_def.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &enum_def,
        .decl_block = &enum_def.block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    analyzer.enter_decl_scope(*state.scope);
    collect_in_block(enum_def.block);
    analyzer.exit_decl_scope();

    add_symbol(enum_def.ident.value, &enum_def);
    analyzer.add_symbol_def(&enum_def);
}

void SymbolCollector::collect_enum_variant(sir::EnumVariant &enum_variant) {
    enum_variant.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*enum_variant.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &enum_variant,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    enum_variant.parent = analyzer.get_decl_scope().decl;

    add_symbol(enum_variant.ident.value, &enum_variant);
    analyzer.add_symbol_def(&enum_variant);
}

void SymbolCollector::collect_union_def(sir::UnionDef &union_def) {
    union_def.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*union_def.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &union_def,
        .decl_block = &union_def.block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    analyzer.enter_decl_scope(*state.scope);
    collect_in_block(union_def.block);
    analyzer.exit_decl_scope();

    add_symbol(union_def.ident.value, &union_def);
    analyzer.add_symbol_def(&union_def);
}

void SymbolCollector::collect_union_case(sir::UnionCase &union_case) {
    union_case.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*union_case.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &union_case,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    union_case.parent = analyzer.get_decl_scope().decl;

    add_symbol(union_case.ident.value, &union_case);
    analyzer.add_symbol_def(&union_case);
}

void SymbolCollector::collect_proto_def(sir::ProtoDef &proto_def) {
    proto_def.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*proto_def.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &proto_def,
        .decl_block = &proto_def.block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    analyzer.enter_decl_scope(*state.scope);
    collect_in_block(proto_def.block);
    analyzer.exit_decl_scope();

    add_symbol(proto_def.ident.value, &proto_def);
    analyzer.add_symbol_def(&proto_def);
}

void SymbolCollector::collect_type_alias(sir::TypeAlias &type_alias) {
    type_alias.sema_index = create_decl_state();
    DeclState &state = analyzer.decl_states[*type_alias.sema_index];

    state.scope = std::make_unique<DeclScope>(DeclScope{
        .mod = analyzer.get_decl_scope().mod,
        .decl = &type_alias,
        .decl_block = analyzer.get_decl_scope().decl_block,
        .generic_args = analyzer.get_decl_scope().generic_args,
        .closure_ctx = analyzer.get_decl_scope().closure_ctx,
    });

    add_symbol(type_alias.ident.value, &type_alias);
    analyzer.add_symbol_def(&type_alias);
}

void SymbolCollector::collect_use_decl(sir::UseDecl &use_decl) {
    if (auto use_ident = use_decl.root_item.match<sir::UseIdent>()) {
        if (analyzer.mode == Mode::COMPLETION && use_ident->is_completion_token()) {
            analyzer.completion_context = CompleteInUse{};
            return;
        }
    }

    collect_use_item(use_decl.root_item);
}

void SymbolCollector::collect_in_meta_if_stmt(sir::MetaIfStmt &meta_if_stmt, unsigned index) {
    std::optional<unsigned> prev_guarded_scope = guarded_scope;
    guarded_scope = analyzer.guarded_scopes.size();

    analyzer.guarded_scopes.push_back(
        GuardedScope{
            .guard_stmt_index = index,
            .scope = analyzer.get_decl_scope(),
        }
    );

    for (sir::MetaIfCondBranch &cond_branch : meta_if_stmt.cond_branches) {
        collect_in_block(*std::get<sir::DeclBlock *>(cond_branch.block));
    }

    if (meta_if_stmt.else_branch) {
        collect_in_block(*std::get<sir::DeclBlock *>(meta_if_stmt.else_branch->block));
    }

    guarded_scope = prev_guarded_scope;
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
    if (auto use_ident = use_dot_expr.rhs.match<sir::UseIdent>()) {
        if (analyzer.mode == Mode::COMPLETION && use_ident->is_completion_token()) {
            analyzer.completion_context = CompleteAfterUseDot{
                .lhs = use_dot_expr.lhs,
            };

            return;
        }

        collect_use_ident(*use_ident);
    } else if (auto use_rebind = use_dot_expr.rhs.match<sir::UseRebind>()) {
        collect_use_rebind(*use_rebind);
    } else if (auto use_list = use_dot_expr.rhs.match<sir::UseList>()) {
        if (analyzer.mode == Mode::COMPLETION) {
            for (sir::UseItem &use_item : use_list->items) {
                if (auto use_ident = use_item.match<sir::UseIdent>()) {
                    if (use_ident->is_completion_token()) {
                        analyzer.completion_context = CompleteAfterUseDot{.lhs = use_dot_expr.lhs};
                    }
                }
            }
        }

        collect_use_list(*use_list);
    }
}

void SymbolCollector::collect_use_list(sir::UseList &use_list) {
    for (sir::UseItem &use_item : use_list.items) {
        collect_use_item(use_item);
    }
}

unsigned SymbolCollector::create_decl_state() {
    analyzer.decl_states.push_back(
        DeclState{
            .stage = DeclStage::NAME,
            .scope = nullptr,
        }
    );

    return static_cast<unsigned>(analyzer.decl_states.size() - 1);
}

void SymbolCollector::add_symbol(std::string_view name, sir::Symbol symbol) {
    sir::SymbolTable &symbol_table = get_symbol_table();

    if (guarded_scope) {
        symbol_table.guarded_scopes.insert({name, *guarded_scope});
    } else {
        symbol_table.insert_decl(name, symbol);
    }

    // if (analyzer.symbol_ctx.is_guarded()) {
    //     sir::Symbol &cur_entry = symbol_table.symbols[name];

    //     sir::GuardedSymbol::Variant variant{
    //         .truth_table = analyzer.symbol_ctx.get_meta_condition(),
    //         .symbol = symbol,
    //     };

    //     if (!cur_entry) {
    //         cur_entry = analyzer.create(
    //             sir::GuardedSymbol{
    //                 .variants{variant},
    //             }
    //         );
    //     } else if (auto guarded_symbol = cur_entry.match<sir::GuardedSymbol>()) {
    //         guarded_symbol->variants.push_back(variant);
    //     } else {
    //         // TODO
    //         ASSERT_UNREACHABLE;
    //     }
    // } else {
    //     symbol_table.insert_decl(name, symbol);
    // }
}

sir::SymbolTable &SymbolCollector::get_symbol_table() {
    return analyzer.get_symbol_table();
}

} // namespace sema

} // namespace lang

} // namespace banjo
