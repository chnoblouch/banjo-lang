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
    const File *file = find_file(params);
    if (!file) {
        return JSONObject{{"changes", JSONObject{}}};
    }

    const SymbolRef *symbol_def = find_symbol(*file, params);
    if (!symbol_def) {
        return JSONObject{{"changes", JSONObject{}}};
    }

    std::unordered_map<const lang::sir::Module *, std::vector<const SymbolRef *>> uses_by_mod;
    uses_by_mod[file->sir_module].push_back(symbol_def);

    for (const SymbolKey &key : symbol_def->uses) {
        const SymbolRef &symbol_ref = workspace.get_index_symbol(key);
        uses_by_mod[key.mod].push_back(&symbol_ref);
    }

    const std::string &new_name = params.get_string("newName");
    JSONObject changes;

    for (const auto &[mod, uses] : uses_by_mod) {
        File &file = *workspace.find_file(*mod);
        std::string uri = URI::encode_from_path(file.fs_path);

        JSONArray edits;

        for (const SymbolRef *use : uses) {
            edits.add(JSONObject{
                {"range", ProtocolStructs::range_to_lsp(file.content, use->range)},
                {"newText", new_name},
            });
        }

        changes.add(uri, edits);
    }

    return JSONObject{{"changes", changes}};
}

const File *RenameHandler::find_file(const JSONObject &params) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);
    return workspace.find_file(fs_path);
}

const SymbolRef *RenameHandler::find_symbol(const File &file, const JSONObject &params) {
    ModuleIndex *index = workspace.find_index(file.sir_module);
    if (!index) {
        return nullptr;
    }

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_number("line");
    int column = lsp_position.get_number("character");
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
