#include "references_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/utils/json.hpp"
#include "protocol_structs.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

ReferencesHandler::ReferencesHandler(Workspace &workspace) : workspace(workspace) {}

ReferencesHandler::~ReferencesHandler() {}

JSONValue ReferencesHandler::handle(const JSONObject &params, Connection & /*connection*/) {
    const File *file = find_file(params);
    if (!file) {
        return JSONArray{};
    }

    const SymbolRef *symbol_def = find_symbol(*file, params);
    if (!symbol_def) {
        return JSONArray{};
    }

    JSONArray array;

    for (const auto &[path, mod_index] : workspace.get_index().mods) {
        for (const SymbolRef &symbol_ref : mod_index.symbol_refs) {
            if (symbol_ref.symbol != symbol_def->symbol) {
                continue;
            }

            if (symbol_ref.range == symbol_def->range) {
                continue;
            }

            File *file = workspace.find_file(*path);
            if (!file) {
                continue;
            }

            array.add(JSONObject{
                {"uri", URI::encode_from_path(file->fs_path)},
                {"range", ProtocolStructs::range_to_lsp(file->content, symbol_ref.range)}
            });
        }
    }

    return array;
}

const File *ReferencesHandler::find_file(const JSONObject &params) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);
    return workspace.find_file(fs_path);
}

const SymbolRef *ReferencesHandler::find_symbol(const File &file, const JSONObject &params) {
    ModuleIndex *index = workspace.find_index(file.sir_module);
    if (!index) {
        return nullptr;
    }

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_int("line");
    int column = lsp_position.get_int("character");
    lang::TextPosition position = ASTNavigation::pos_from_lsp(file.content, line, column);

    for (const SymbolRef &symbol_ref : index->symbol_refs) {
        if (position >= symbol_ref.range.start && position <= symbol_ref.range.end) {
            return &symbol_ref;
        }
    }

    return nullptr;
}

} // namespace lsp

} // namespace banjo
