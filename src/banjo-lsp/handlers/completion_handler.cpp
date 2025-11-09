#include "completion_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/reports/report_texts.hpp"
#include "banjo/sema/completion_context.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/json.hpp"
#include "uri.hpp"

#include <set>
#include <string_view>
#include <variant>

namespace banjo {

namespace lsp {

using namespace lang;

CompletionHandler::CompletionHandler(Workspace &workspace) : workspace(workspace) {}

CompletionHandler::~CompletionHandler() {}

JSONValue CompletionHandler::handle(const JSONObject &params, Connection & /*connection*/) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);

    File *file = workspace.find_file(fs_path);
    if (!file) {
        return JSONObject{{"data", JSONArray{}}};
    }

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_int("line");
    int column = lsp_position.get_int("character");
    TextPosition position = ASTNavigation::pos_from_lsp(file->content, line, column);

    sir::Module sir_mod;
    CompletionInfo completion_info = workspace.run_completion(file, position, sir_mod);
    JSONArray items;

    if (auto in_decl_block = std::get_if<sema::CompleteInDeclBlock>(&completion_info.context)) {
        build_in_block(items, *in_decl_block->decl_block->symbol_table, completion_info);
    } else if (auto in_block = std::get_if<sema::CompleteInBlock>(&completion_info.context)) {
        build_in_block(items, *in_block->block->symbol_table, completion_info);
    } else if (auto after_dot = std::get_if<sema::CompleteAfterDot>(&completion_info.context)) {
        build_after_dot(items, after_dot->lhs);
    } else if (auto after_implicit_dot = std::get_if<sema::CompleteAfterImplicitDot>(&completion_info.context)) {
        build_after_implicit_dot(items, after_implicit_dot->type);
    } else if (std::holds_alternative<sema::CompleteInUse>(completion_info.context)) {
        build_in_use(items);
    } else if (auto after_use_dot = std::get_if<sema::CompleteAfterUseDot>(&completion_info.context)) {
        build_after_use_dot(items, after_use_dot->lhs);
    } else if (auto in_struct_literal = std::get_if<sema::CompleteInStructLiteral>(&completion_info.context)) {
        build_in_struct_literal(items, *in_struct_literal->struct_literal);
    }

    workspace.undo_infection(completion_info.infection);
    return items;
}

void CompletionHandler::build_in_block(
    JSONArray &items,
    lang::sir::SymbolTable &symbol_table,
    const CompletionInfo &completion_info
) {
    CompletionConfig config{
        .allow_values = true,
        .include_parent_scopes = true,
        .include_uses = true,
        .append_func_parameters = true,
    };

    build_items(config, items, symbol_table);

    for (const lang::sir::Symbol &symbol : completion_info.preamble_symbols) {
        CompletionConfig config{
            .allow_values = true,
            .include_parent_scopes = false,
            .include_uses = false,
            .append_func_parameters = true,
        };

        build_item(config, items, symbol.get_name(), symbol);
    }
}

void CompletionHandler::build_after_dot(JSONArray &items, lang::sir::Expr &lhs) {
    sir::Expr type = lhs.get_type();

    if (type) {
        while (auto pointer_type = type.match<sir::PointerType>()) {
            type = pointer_type->base_type;
        }

        if (auto struct_def = type.match_symbol<sir::StructDef>()) {
            build_value_members(items, *struct_def);
        }
    } else if (auto symbol_expr = lhs.match<sir::SymbolExpr>()) {
        CompletionConfig config{
            .allow_values = true,
            .include_parent_scopes = false,
            .include_uses = false,
            .append_func_parameters = true,
        };

        build_symbol_members(config, items, symbol_expr->symbol);
    }
}

void CompletionHandler::build_after_implicit_dot(JSONArray &items, lang::sir::Expr &type) {
    if (auto symbol_expr = type.match<sir::SymbolExpr>()) {
        CompletionConfig config{
            .allow_values = true,
            .include_parent_scopes = false,
            .include_uses = false,
            .append_func_parameters = true,
        };

        build_symbol_members(config, items, symbol_expr->symbol);
    }
}

void CompletionHandler::build_in_use(JSONArray &items) {
    for (lang::ASTModule *mod : workspace.get_mod_list()) {
        if (mod->file.mod_path.get_size() == 1) {
            items.add(create_simple_item(mod->file.mod_path[0], LSPCompletionItemKind::MODULE));
        }
    }
}

void CompletionHandler::build_after_use_dot(JSONArray &items, lang::sir::UseItem &lhs) {
    CompletionConfig config{
        .allow_values = false,
        .include_parent_scopes = false,
        .include_uses = false,
        .append_func_parameters = false,
    };

    if (auto use_ident = lhs.match<sir::UseIdent>()) {
        build_symbol_members(config, items, use_ident->symbol);
    } else if (auto use_dot_expr = lhs.match<sir::UseDotExpr>()) {
        build_symbol_members(config, items, use_dot_expr->rhs.as<sir::UseIdent>().symbol);
    }
}

void CompletionHandler::build_in_struct_literal(JSONArray &items, lang::sir::StructLiteral &struct_literal) {
    if (auto struct_def = struct_literal.type.match_symbol<sir::StructDef>()) {
        std::set<sir::StructField *> set_fields;

        for (lang::sir::StructLiteralEntry &entry : struct_literal.entries) {
            if (entry.field) {
                set_fields.insert(entry.field);
            }
        }

        for (sir::StructField *field : struct_def->fields) {
            if (set_fields.contains(field)) {
                continue;
            }

            std::string_view name = field->ident.value;

            items.add(
                JSONObject{
                    {"label", name},
                    {"kind", LSPCompletionItemKind::FIELD},
                    {"insertText", std::string{name} + ": "},
                    {"insertTextFormat", 2},
                }
            );
        }
    }
}

void CompletionHandler::build_value_members(JSONArray &items, lang::sir::StructDef &struct_def) {
    CompletionConfig config{
        .allow_values = false,
        .include_parent_scopes = false,
        .include_uses = false,
        .append_func_parameters = true,
    };

    for (lang::sir::StructField *field : struct_def.fields) {
        items.add(create_simple_item(field->ident.value, LSPCompletionItemKind::FIELD));
    }

    for (const auto &[name, symbol] : struct_def.block.symbol_table->symbols) {
        bool is_value_member = false;

        if (auto func_def = symbol.match<sir::FuncDef>()) {
            is_value_member = func_def->is_method();
        } else if (symbol.is<sir::OverloadSet>()) {
            // TODO: Only include the overloads that are actually methods.
            is_value_member = true;
        }

        if (is_value_member) {
            build_item(config, items, name, symbol);
        }
    }
}

void CompletionHandler::build_items(
    const CompletionConfig &config,
    JSONArray &items,
    lang::sir::SymbolTable &symbol_table
) {
    for (const auto &[name, symbol] : symbol_table.symbols) {
        build_item(config, items, name, symbol);
    }

    if (config.include_parent_scopes && symbol_table.parent) {
        build_items(config, items, *symbol_table.parent);
    }
}

void CompletionHandler::build_symbol_members(
    const CompletionConfig &config,
    JSONArray &items,
    lang::sir::Symbol &symbol
) {
    if (auto mod = symbol.match<sir::Module>()) {
        for (const lang::ModulePath &path : workspace.list_sub_mods(mod)) {
            items.add(create_simple_item(path[path.get_size() - 1], LSPCompletionItemKind::MODULE));
        }
    }

    if (sir::SymbolTable *symbol_table = symbol.get_symbol_table()) {
        build_items(config, items, *symbol_table);
    }
}

void CompletionHandler::build_item(
    const CompletionConfig &config,
    JSONArray &items,
    std::string_view name,
    const lang::sir::Symbol &symbol
) {
    if (symbol.is<sir::Module>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::MODULE));
    } else if (auto func_def = symbol.match<sir::FuncDef>()) {
        if (config.append_func_parameters) {
            items.add(build_item(name, func_def->type, func_def->is_method()));
        } else {
            items.add(create_simple_item(name, LSPCompletionItemKind::FUNCTION));
        }
    } else if (auto native_func_decl = symbol.match<sir::NativeFuncDecl>()) {
        if (config.append_func_parameters) {
            items.add(build_item(name, native_func_decl->type, false));
        } else {
            items.add(create_simple_item(name, LSPCompletionItemKind::FUNCTION));
        }
    } else if (symbol.is<sir::ConstDef>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::CONSTANT));
    } else if (auto struct_def = symbol.match<sir::StructDef>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::STRUCT));

        if (config.allow_values && !struct_def->is_generic()) {
            items.add(build_struct_literal_item(*struct_def));
        }
    } else if (symbol.is_one_of<sir::UnionDef, sir::UnionCase, sir::TypeAlias>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::STRUCT));
    } else if (symbol.is_one_of<sir::VarDecl, sir::NativeVarDecl, sir::Local, sir::Param>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::VARIABLE));
    } else if (symbol.is<sir::EnumDef>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::ENUM));
    } else if (symbol.is<sir::EnumVariant>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::ENUM_MEMBER));
    } else if (auto overload_set = symbol.match<sir::OverloadSet>()) {
        build_item(items, name, *overload_set);
    } else if (config.include_uses) {
        if (auto use_ident = symbol.match<sir::UseIdent>()) {
            build_item(config, items, name, use_ident->symbol);
        } else if (auto use_rebind = symbol.match<sir::UseRebind>()) {
            build_item(config, items, name, use_rebind->symbol);
        }
    }
}

JSONObject CompletionHandler::build_item(std::string_view name, const lang::sir::FuncType &type, bool is_method) {
    std::string detail = "(";

    for (unsigned i = 0; i < type.params.size(); i++) {
        // if (!(is_method && i == 0)) {
        //     detail += ReportText::to_string(type.params[i].type);
        // } else {
        //     detail += "self";
        // }

        detail += type.params[i].name.value;

        if (i != type.params.size() - 1) {
            detail += ", ";
        }
    }

    detail += ")";

    sir::Expr return_type = type.return_type;

    if (return_type && !return_type.is_primitive_type(sir::Primitive::VOID)) {
        detail += " -> " + ReportText::to_string(return_type);
    }

    std::string insert_text = std::string(name) + "(";

    for (unsigned i = is_method ? 1 : 0; i < type.params.size(); i++) {
        std::string_view param_name = type.params[i].name.value;
        if (param_name.empty()) {
            param_name = "_";
        }

        insert_text += "${" + std::to_string(i + 1) + ":" + std::string{param_name} + "}";
        if (i != type.params.size() - 1) {
            insert_text += ", ";
        }
    }

    insert_text += ")";

    return JSONObject{
        {"label", name},
        {"labelDetails", JSONObject{{"detail", detail}}},
        {"kind", is_method ? LSPCompletionItemKind::METHOD : LSPCompletionItemKind::FUNCTION},
        {"insertText", insert_text},
        {"insertTextFormat", 2},
    };
}

void CompletionHandler::build_item(JSONArray &items, std::string_view name, const sir::OverloadSet &overload_set) {
    for (sir::FuncDef *func_def : overload_set.func_defs) {
        items.add(build_item(name, func_def->type, func_def->is_method()));
    }
}

JSONObject CompletionHandler::build_struct_literal_item(const lang::sir::StructDef &struct_def) {
    std::string insert_text = std::string(struct_def.ident.value) + " {\n";
    std::string detail = " { ";

    for (unsigned i = 0; i < struct_def.fields.size(); i++) {
        std::string field_name{struct_def.fields[i]->ident.value};
        insert_text += "    " + field_name + ": ${" + std::to_string(i + 1) + ":},\n";

        detail += field_name;

        if (i != struct_def.fields.size() - 1) {
            detail += ", ";
        }
    }

    insert_text += "}";
    detail += " }";

    return JSONObject{
        {"label", struct_def.ident.value},
        {"labelDetails", JSONObject{{"detail", detail}}},
        {"kind", LSPCompletionItemKind::STRUCT},
        {"insertText", insert_text},
        {"insertTextFormat", 2},
    };
}

JSONObject CompletionHandler::create_simple_item(std::string_view name, LSPCompletionItemKind kind) {
    return JSONObject{
        {"label", name},
        {"kind", kind},
        {"insertText", name},
        {"insertTextFormat", 2},
    };
}

} // namespace lsp

} // namespace banjo
