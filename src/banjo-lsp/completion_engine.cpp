#include "completion_engine.hpp"

#include "banjo/sema/completion_context.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"
#include "workspace.hpp"

#include <utility>

using namespace banjo::lang;

namespace banjo::lsp {

CompletionEngine::CompletionEngine(Workspace &workspace) : workspace(workspace) {}

void CompletionEngine::complete(Request request) {
    state.file = &request.file;
    state.items.clear();
    state.symbols.clear();

    if (auto inner = std::get_if<sema::CompleteInDeclBlock>(&request.context)) {
        sir::SymbolTable &symbol_table = *inner->decl_block->symbol_table;
        complete_in_block(request, symbol_table);
    } else if (auto inner = std::get_if<sema::CompleteInBlock>(&request.context)) {
        sir::SymbolTable &symbol_table = *inner->block->symbol_table;
        complete_in_block(request, symbol_table);
    } else if (auto inner = std::get_if<sema::CompleteAfterDot>(&request.context)) {
        complete_after_dot(inner->lhs);
    } else if (auto inner = std::get_if<sema::CompleteAfterImplicitDot>(&request.context)) {
        complete_after_implicit_dot(inner->type);
    } else if ([[maybe_unused]] auto inner = std::get_if<sema::CompleteInUse>(&request.context)) {
        complete_in_use();
    } else if (auto inner = std::get_if<sema::CompleteAfterUseDot>(&request.context)) {
        complete_after_use_dot(inner->lhs);
    } else if (auto inner = std::get_if<sema::CompleteInStructLiteral>(&request.context)) {
        complete_in_struct_literal(*inner->struct_literal);
    }
}

void CompletionEngine::complete_in_block(Request request, sir::SymbolTable &symbol_table) {
    Options options{
        .allow_values = true,
        .include_parent_scopes = true,
        .include_uses = true,
        .create_func_call_template = true,
        .file_to_use = nullptr,
    };

    collect_items(symbol_table, options);

    for (sir::Symbol &symbol : request.preamble_symbols) {
        Options options{
            .allow_values = true,
            .include_parent_scopes = false,
            .include_uses = false,
            .create_func_call_template = true,
            .file_to_use = nullptr,
        };

        try_collect_item(symbol.get_name(), symbol, options);
    }

    for (const std::unique_ptr<SourceFile> &mod : workspace.get_mod_list()) {
        for (auto &[name, symbol] : mod->sir_mod->block.symbol_table->symbols) {
            if (symbol.is_one_of<sir::UseIdent, sir::UseRebind>()) {
                continue;
            }

            if (state.symbols.contains(symbol)) {
                continue;
            }

            Options options{
                .allow_values = true,
                .include_parent_scopes = false,
                .include_uses = false,
                .create_func_call_template = true,
                .file_to_use = mod.get(),
            };

            try_collect_item(symbol.get_name(), symbol, options);
        }
    }
}

void CompletionEngine::complete_after_dot(sir::Expr lhs) {
    sir::Expr type = lhs.get_type();

    if (type) {
        while (auto pointer_type = type.match<sir::PointerType>()) {
            type = pointer_type->base_type;
        }

        if (auto struct_def = type.match_symbol<sir::StructDef>()) {
            collect_value_members(*struct_def);
        }
    } else if (auto symbol_expr = lhs.match<sir::SymbolExpr>()) {
        Options options{
            .allow_values = true,
            .include_parent_scopes = false,
            .include_uses = false,
            .create_func_call_template = true,
            .file_to_use = nullptr,
        };

        collect_symbol_members(symbol_expr->symbol, options);
    }
}

void CompletionEngine::complete_after_implicit_dot(lang::sir::Expr type) {
    if (auto symbol_expr = type.match<sir::SymbolExpr>()) {
        Options options{
            .allow_values = true,
            .include_parent_scopes = false,
            .include_uses = false,
            .create_func_call_template = true,
            .file_to_use = nullptr,
        };

        collect_symbol_members(symbol_expr->symbol, options);
    }
}

void CompletionEngine::complete_in_use() {
    for (const std::unique_ptr<SourceFile> &file : workspace.get_mod_list()) {
        if (file->mod_path.get_size() == 1) {
            Item item{
                .kind = Item::Kind::SIMPLE,
                .name{file->mod_path[0]},
                .symbol = file->sir_mod,
                .file_to_use = nullptr,
            };

            add_item(item);
        }
    }
}

void CompletionEngine::complete_after_use_dot(sir::UseItem &lhs) {
    Options options{
        .allow_values = false,
        .include_parent_scopes = false,
        .include_uses = false,
        .create_func_call_template = false,
        .file_to_use = nullptr,
    };

    if (auto use_ident = lhs.match<sir::UseIdent>()) {
        collect_symbol_members(use_ident->symbol, options);
    } else if (auto use_dot_expr = lhs.match<sir::UseDotExpr>()) {
        collect_symbol_members(use_dot_expr->rhs.as<sir::UseIdent>().symbol, options);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void CompletionEngine::complete_in_struct_literal(sir::StructLiteral &struct_literal) {
    if (auto struct_def = struct_literal.type.match_symbol<sir::StructDef>()) {
        std::set<sir::StructField *> set_fields;

        for (sir::StructLiteralEntry &entry : struct_literal.entries) {
            if (entry.field) {
                set_fields.insert(entry.field);
            }
        }

        for (sir::StructField *field : struct_def->fields) {
            if (set_fields.contains(field)) {
                continue;
            }

            Item item{
                .kind = Item::Kind::STRUCT_FIELD_TEMPLATE,
                .name{field->ident.value},
                .symbol = field,
                .file_to_use = nullptr,
            };

            add_item(item);
        }
    }
}

void CompletionEngine::collect_symbol_members(sir::Symbol &symbol, Options &options) {
    if (auto mod = symbol.match<sir::Module>()) {
        for (ModulePath &path : workspace.find_file(mod->path)->sub_mod_paths) {
            Item item{
                .kind = Item::Kind::SIMPLE,
                .name{path[path.get_size() - 1]},
                .symbol = symbol,
                .file_to_use = options.file_to_use,
            };

            add_item(item);
        }
    }

    if (sir::SymbolTable *symbol_table = symbol.get_symbol_table()) {
        collect_items(*symbol_table, options);
    }
}

void CompletionEngine::collect_value_members(sir::StructDef &struct_def) {
    Options options{
        .allow_values = false,
        .include_parent_scopes = false,
        .include_uses = false,
        .create_func_call_template = true,
        .file_to_use = nullptr,
    };

    for (sir::StructField *field : struct_def.fields) {
        Item item{
            .kind = Item::Kind::SIMPLE,
            .name{field->ident.value},
            .symbol = field,
            .file_to_use = options.file_to_use,
        };

        add_item(item);
    }

    for (auto &[name, symbol] : struct_def.block.symbol_table->symbols) {
        bool is_value_member = false;

        if (auto func_def = symbol.match<sir::FuncDef>()) {
            is_value_member = func_def->is_method();
        } else if (symbol.is<sir::OverloadSet>()) {
            // TODO: Only include the overloads that are actually methods.
            is_value_member = true;
        }

        if (is_value_member) {
            try_collect_item(std::string{name}, symbol, options);
        }
    }
}

void CompletionEngine::collect_items(sir::SymbolTable &symbol_table, Options &options) {
    for (auto &[name, symbol] : symbol_table.symbols) {
        try_collect_item(std::string{name}, symbol, options);
    }

    if (options.include_parent_scopes && symbol_table.parent) {
        collect_items(*symbol_table.parent, options);
    }
}

void CompletionEngine::try_collect_item(std::string name, sir::Symbol symbol, Options &options) {
    if (!options.include_uses && symbol.is_one_of<sir::UseIdent, sir::UseRebind>()) {
        return;
    }

    if (auto overload_set = symbol.match<sir::OverloadSet>()) {
        for (sir::FuncDef *func : overload_set->func_defs) {
            try_collect_item(name, func, options);
        }

        return;
    } else if (auto use_ident = symbol.match<sir::UseIdent>()) {
        symbol = use_ident->symbol;
    } else if (auto use_rebind = symbol.match<sir::UseRebind>()) {
        symbol = use_rebind->symbol;
    }

    Item item{
        .kind = Item::Kind::SIMPLE,
        .name = name,
        .symbol = symbol,
        .file_to_use = options.file_to_use,
    };

    if (options.create_func_call_template) {
        if (symbol.is_one_of<sir::FuncDef, sir::FuncDecl, sir::NativeFuncDecl>()) {
            item.kind = Item::Kind::FUNC_CALL_TEMPLATE;
        }
    }

    add_item(item);

    if (options.allow_values) {
        if (auto struct_def = symbol.match<sir::StructDef>()) {
            if (!struct_def->is_generic()) {
                Item item{
                    .kind = Item::Kind::STRUCT_LITERAL_TEMPLATE,
                    .name = name,
                    .symbol = symbol,
                    .file_to_use = options.file_to_use,
                };

                add_item(item);
            }
        }
    }
}

void CompletionEngine::add_item(Item item) {
    if (item.symbol.is<sir::GuardedSymbol>()) {
        return;
    }

    state.symbols.insert(item.symbol);
    state.items.push_back(std::move(item));
}

} // namespace banjo::lsp
