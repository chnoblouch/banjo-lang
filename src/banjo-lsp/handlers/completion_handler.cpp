#include "completion_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/reports/report_texts.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/json.hpp"
#include "uri.hpp"

#include <string_view>
#include <variant>

namespace banjo {

namespace lsp {

using namespace lang;

CompletionHandler::CompletionHandler(Workspace &workspace) : workspace(workspace) {}

CompletionHandler::~CompletionHandler() {}

JSONValue CompletionHandler::handle(const JSONObject &params, Connection &connection) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);

    File *file = workspace.find_file(fs_path);
    if (!file) {
        return JSONObject{{"data", JSONArray{}}};
    }

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_number("line");
    int column = lsp_position.get_number("character");
    TextPosition position = ASTNavigation::pos_from_lsp(file->content, line, column);

    CompletionInfo completion_info = workspace.run_completion(file, position);
    JSONArray items;

    if (auto in_decl_block = std::get_if<sema::CompleteInDeclBlock>(&completion_info.context)) {
        build_items(completion_info.context, items, *in_decl_block->decl_block->symbol_table);
    } else if (auto in_block = std::get_if<sema::CompleteInBlock>(&completion_info.context)) {
        build_items(completion_info.context, items, *in_block->block->symbol_table);
    } else if (auto after_dot = std::get_if<sema::CompleteAfterDot>(&completion_info.context)) {
        build_after_dot(completion_info.context, items, after_dot->lhs);
    } else if (auto after_use_dot = std::get_if<sema::CompleteAfterUseDot>(&completion_info.context)) {
        build_after_use_dot(completion_info.context, items, after_use_dot->lhs);
    }

    return items;
}

void CompletionHandler::build_after_dot(
    lang::sema::CompletionContext &context,
    JSONArray &items,
    lang::sir::Expr &lhs
) {
    sir::Expr type = lhs.get_type();

    if (type) {
        while (auto pointer_type = type.match<sir::PointerType>()) {
            type = pointer_type->base_type;
        }

        if (auto struct_def = type.match_symbol<sir::StructDef>()) {
            build_value_members(context, items, *struct_def->block.symbol_table);
        }
    } else if (auto symbol_expr = lhs.match<sir::SymbolExpr>()) {
        build_symbol_members(context, items, symbol_expr->symbol);
    }
}

void CompletionHandler::build_after_use_dot(
    lang::sema::CompletionContext &context,
    JSONArray &items,
    lang::sir::UseItem &lhs
) {
    if (auto use_ident = lhs.match<sir::UseIdent>()) {
        build_symbol_members(context, items, use_ident->symbol);
    } else if (auto use_dot_expr = lhs.match<sir::UseDotExpr>()) {
        build_symbol_members(context, items, use_dot_expr->rhs.as<sir::UseIdent>().symbol);
    }
}

void CompletionHandler::build_symbol_members(
    lang::sema::CompletionContext &context,
    JSONArray &items,
    lang::sir::Symbol &symbol
) {
    if (auto mod = symbol.match<sir::Module>()) {
        for (const lang::ModulePath &path : workspace.list_sub_mods(mod)) {
            items.add(create_simple_item(path.get_path().back(), LSPCompletionItemKind::MODULE));
        }
    }

    if (sir::SymbolTable *symbol_table = symbol.get_symbol_table()) {
        build_items(context, items, *symbol_table);
    }
}

void CompletionHandler::build_value_members(
    lang::sema::CompletionContext &context,
    JSONArray &items,
    const lang::sir::SymbolTable &symbol_table
) {
    for (const auto &[name, symbol] : symbol_table.symbols) {
        bool is_value_member = false;

        if (symbol.is<sir::StructField>()) {
            is_value_member = true;
        } else if (auto func_def = symbol.match<sir::FuncDef>()) {
            is_value_member = func_def->is_method();
        }

        if (is_value_member) {
            build_item(context, items, name, symbol);
        }
    }
}

void CompletionHandler::build_items(
    lang::sema::CompletionContext &context,
    JSONArray &items,
    const lang::sir::SymbolTable &symbol_table
) {
    for (const auto &[name, symbol] : symbol_table.symbols) {
        build_item(context, items, name, symbol);
    }

    if (symbol_table.parent) {
        build_items(context, items, *symbol_table.parent);
    }
}

void CompletionHandler::build_item(
    lang::sema::CompletionContext &context,
    JSONArray &items,
    std::string_view name,
    const lang::sir::Symbol &symbol
) {
    if (symbol.is<sir::Module>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::MODULE));
    } else if (auto func_def = symbol.match<sir::FuncDef>()) {
        items.add(build_item(name, func_def->type, func_def->is_method()));
    } else if (auto native_func_decl = symbol.match<sir::NativeFuncDecl>()) {
        items.add(build_item(name, native_func_decl->type, false));
    } else if (symbol.is<sir::ConstDef>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::CONSTANT));
    } else if (symbol.is_one_of<sir::StructDef, sir::UnionDef, sir::UnionCase, sir::TypeAlias>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::STRUCT));
    } else if (symbol.is_one_of<sir::VarDecl, sir::NativeVarDecl, sir::Local, sir::Param>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::VARIABLE));
    } else if (symbol.is<sir::EnumDef>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::ENUM));
    } else if (symbol.is<sir::EnumVariant>()) {
        items.add(create_simple_item(name, LSPCompletionItemKind::ENUM_MEMBER));
    } else if (auto overload_set = symbol.match<sir::OverloadSet>()) {
        build_item(items, name, *overload_set);
    } else if (std::holds_alternative<lang::sema::CompleteInBlock>(context)) {
        if (auto use_ident = symbol.match<sir::UseIdent>()) {
            build_item(context, items, name, use_ident->symbol);
        } else if (auto use_rebind = symbol.match<sir::UseRebind>()) {
            build_item(context, items, name, use_rebind->symbol);
        }
    }
}

JSONObject CompletionHandler::build_item(std::string_view name, const lang::sir::FuncType &type, bool is_method) {
    std::string detail = "(";

    for (int i = 0; i < type.params.size(); i++) {
        if (!(is_method && i == 0)) {
            detail += ReportText::to_string(type.params[i].type);
        } else {
            detail += "self";
        }

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
