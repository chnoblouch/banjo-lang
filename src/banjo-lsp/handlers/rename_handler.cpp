#include "rename_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/utils/json.hpp"
#include "protocol_structs.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

RenameHandler::RenameHandler(Workspace &workspace) : workspace(workspace) {}

RenameHandler::~RenameHandler() {}

JSONValue RenameHandler::handle(const JSONObject &params, Connection & /*connection*/) {
    const lang::SourceFile *file = find_file(params);
    if (!file) {
        return JSONObject{{"changes", JSONObject{}}};
    }

    const SymbolRef *symbol_def = find_symbol(*file, params);
    if (!symbol_def) {
        return JSONObject{{"changes", JSONObject{}}};
    }

    const std::string &new_name = params.get_string("newName");
    JSONObject changes;

    for (const auto &[mod, mod_index] : workspace.get_index().mods) {
        lang::SourceFile *use_file = workspace.find_file(mod->path);
        if (!use_file) {
            continue;
        }

        JSONArray edits;

        for (const SymbolRef &symbol_ref : mod_index.symbol_refs) {
            if (symbol_ref.symbol != symbol_def->symbol) {
                continue;
            }

            edits.add(JSONObject{
                {"range", ProtocolStructs::range_to_lsp(use_file->buffer, symbol_ref.range)},
                {"newText", new_name},
            });
        }

        if (edits.length() > 0) {
            std::string uri = URI::encode_from_path(use_file->fs_path);
            changes.add(uri, edits);
        }
    }

    return JSONObject{{"changes", changes}};
}

const lang::SourceFile *RenameHandler::find_file(const JSONObject &params) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);
    return workspace.find_file(fs_path);
}

const SymbolRef *RenameHandler::find_symbol(const lang::SourceFile &file, const JSONObject &params) {
    ModuleIndex *index = workspace.find_index(file.sir_mod);
    if (!index) {
        return nullptr;
    }

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_int("line");
    int column = lsp_position.get_int("character");
    lang::TextPosition position = ASTNavigation::pos_from_lsp(file.buffer, line, column);

    for (const SymbolRef &symbol_ref : index->symbol_refs) {
        if (position >= symbol_ref.range.start && position <= symbol_ref.range.end) {
            return &symbol_ref;
        }
    }

    return nullptr;
}

} // namespace lsp

} // namespace banjo
