#include "completion_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/reports/report_texts.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/json.hpp"
#include "banjo/utils/macros.hpp"

#include "completion_engine.hpp"
#include "uri.hpp"
#include "workspace.hpp"

#include <string_view>

using namespace banjo::lang;

namespace banjo::lsp {

CompletionHandler::CompletionHandler(Workspace &workspace) : workspace(workspace) {}

JSONValue CompletionHandler::handle(const JSONObject &params, Connection & /*connection*/) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);

    lang::SourceFile *file = workspace.find_file(fs_path);
    if (!file) {
        return JSONObject{{"data", JSONArray{}}};
    }

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_int("line");
    int column = lsp_position.get_int("character");
    TextPosition position = ASTNavigation::pos_from_lsp(file->buffer, line, column);

    sir::Module sir_mod;
    CompletionInfo completion_info = workspace.run_completion(file, position, sir_mod);
    JSONArray items_serialized;

    workspace.completion_engine.complete(
        CompletionEngine::Request{
            .file = *file,
            .context = completion_info.context,
            .preamble_symbols = completion_info.preamble_symbols,
        }
    );

    std::vector<CompletionEngine::Item> &items = workspace.completion_engine.state.items;

    for (unsigned i = 0; i < items.size(); i++) {
        items_serialized.add(serialize_item(i, items[i]));
    }

    workspace.undo_infection(completion_info.infection);
    return items_serialized;
}

JSONObject CompletionHandler::serialize_item(unsigned index, CompletionEngine::Item item) {
    if (item.kind == CompletionEngine::Item::Kind::SIMPLE) {
        if (item.symbol.is<sir::Module>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::MODULE);
        } else if (item.symbol.is_one_of<sir::FuncDef, sir::FuncDecl, sir::NativeFuncDecl>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::FUNCTION);
        } else if (item.symbol.is<sir::ConstDef>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::CONSTANT);
        } else if (item.symbol
                       .is_one_of<sir::StructDef, sir::UnionDef, sir::UnionCase, sir::ProtoDef, sir::TypeAlias>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::STRUCT);
        } else if (item.symbol.is_one_of<sir::VarDecl, sir::NativeVarDecl, sir::Local, sir::Param>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::VARIABLE);
        } else if (item.symbol.is<sir::StructField>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::FIELD);
        } else if (item.symbol.is<sir::EnumDef>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::ENUM);
        } else if (item.symbol.is<sir::EnumVariant>()) {
            return serialize_simple_item(index, item, LSPCompletionItemKind::ENUM_MEMBER);
        } else if (item.symbol.is_one_of<sir::GenericArg, sir::GenericParam>()) {
            // TODO: Generic arguments shouldn't appear here
            return serialize_simple_item(index, item, LSPCompletionItemKind::TYPE_PARAMETER);
        } else {
            ASSERT_UNREACHABLE;
        }
    } else if (item.kind == CompletionEngine::Item::Kind::FUNC_CALL_TEMPLATE) {
        if (auto func_def = item.symbol.match<sir::FuncDef>()) {
            return serialize_func_call_template(index, item, func_def->type);
        } else if (auto func_decl = item.symbol.match<sir::FuncDecl>()) {
            return serialize_func_call_template(index, item, func_decl->type);
        } else if (auto native_func_decl = item.symbol.match<sir::NativeFuncDecl>()) {
            return serialize_func_call_template(index, item, native_func_decl->type);
        } else {
            ASSERT_UNREACHABLE;
        }
    } else if (item.kind == CompletionEngine::Item::Kind::STRUCT_LITERAL_TEMPLATE) {
        return serialize_struct_literal_template(index, item);
    } else if (item.kind == CompletionEngine::Item::Kind::STRUCT_FIELD_TEMPLATE) {
        return serialize_struct_field_template(index, item);
    } else {
        ASSERT_UNREACHABLE;
    }
}

JSONObject CompletionHandler::serialize_simple_item(
    unsigned index,
    CompletionEngine::Item item,
    LSPCompletionItemKind kind
) {
    JSONObject result{
        {"label", item.name},
        {"kind", kind},
        {"insertText", item.name},
        {"insertTextFormat", 2},
        {"data", index},
    };

    if (item.file_to_use) {
        std::string path{item.file_to_use->mod_path.to_string()};
        result.add("labelDetails", JSONObject{{"description", path}});
    }

    return result;
}

JSONObject CompletionHandler::serialize_func_call_template(
    unsigned index,
    CompletionEngine::Item &item,
    const sir::FuncType &type
) {
    bool is_method = type.is_func_method();

    std::string detail = "(";

    for (unsigned i = 0; i < type.params.size(); i++) {
        // if (!(is_method && i == 0)) {
        //     detail += ReportText::to_string(type.params[i].type);
        // } else {
        //     detail += "self";
        // }

        std::string_view param_name = type.params[i].name.value;
        if (param_name.empty()) {
            param_name = "_";
        }

        detail += param_name;

        if (i != type.params.size() - 1) {
            detail += ", ";
        }
    }

    detail += ")";

    sir::Expr return_type = type.return_type;

    if (return_type && !return_type.is_primitive_type(sir::Primitive::VOID)) {
        detail += " -> " + ReportText::to_string(return_type);
    }

    std::string insert_text = item.name + "(";

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

    JSONObject serialized_details;

    if (item.file_to_use) {
        std::string path{item.file_to_use->mod_path.to_string()};
        serialized_details = JSONObject{{"detail", detail}, {"description", path}};
    } else {
        serialized_details = JSONObject{{"detail", detail}};
    }

    return JSONObject{
        {"label", item.name},
        {"labelDetails", serialized_details},
        {"kind", is_method ? LSPCompletionItemKind::METHOD : LSPCompletionItemKind::FUNCTION},
        {"insertText", insert_text},
        {"insertTextFormat", 2},
        {"data", index},
    };
}

JSONObject CompletionHandler::serialize_struct_literal_template(unsigned index, CompletionEngine::Item &item) {
    sir::StructDef &struct_def = item.symbol.as<sir::StructDef>();

    std::string insert_text = item.name + " {\n";
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

    JSONObject serialized_details;

    if (item.file_to_use) {
        std::string path{item.file_to_use->mod_path.to_string()};
        serialized_details = JSONObject{{"detail", detail}, {"description", path}};
    } else {
        serialized_details = JSONObject{{"detail", detail}};
    }

    return JSONObject{
        {"label", struct_def.ident.value},
        {"labelDetails", serialized_details},
        {"kind", LSPCompletionItemKind::STRUCT},
        {"insertText", insert_text},
        {"insertTextFormat", 2},
        {"data", index},
    };
}

JSONObject CompletionHandler::serialize_struct_field_template(unsigned index, CompletionEngine::Item &item) {
    JSONObject result{
        {"label", item.name},
        {"kind", LSPCompletionItemKind::FIELD},
        {"insertText", item.name + ": "},
        {"insertTextFormat", 2},
        {"data", index},
    };

    if (item.file_to_use) {
        std::string path{item.file_to_use->mod_path.to_string()};
        result.add("labelDetails", JSONObject{{"description", path}});
    }

    return result;
}

} // namespace banjo::lsp
