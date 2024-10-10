#include "definition_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "protocol_structs.hpp"
#include "symbol_ref_info.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

using namespace lang;

DefinitionHandler::DefinitionHandler(Workspace &workspace) : workspace(workspace) {}

DefinitionHandler::~DefinitionHandler() {}

JSONValue DefinitionHandler::handle(const JSONObject &params, Connection &connection) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);

    File *file = workspace.find_file(fs_path);
    if (!file) {
        return JSONObject{{"data", JSONArray{}}};
    }

    ModuleIndex *index = workspace.find_index(file->sir_module);
    if (!index) {
        return JSONObject{{"data", JSONArray{}}};
    }

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_number("line");
    int column = lsp_position.get_number("character");
    lang::TextPosition position = ASTNavigation::pos_from_lsp(file->content, line, column);

    for (const SymbolRef &symbol_ref : index->symbol_refs) {
        if (position >= symbol_ref.range.start && position <= symbol_ref.range.end) {
            File *target_file = workspace.find_file(symbol_ref.def_mod->path);

            return {JSONObject{
                {"targetUri", URI::encode_from_path(target_file->fs_path)},
                {"targetRange", ProtocolStructs::range_to_lsp(target_file->content, symbol_ref.def_range)},
                {"targetSelectionRange", ProtocolStructs::range_to_lsp(target_file->content, symbol_ref.def_range)}
            }};
        }
    }

    return JSONArray{};
}

} // namespace lsp

} // namespace banjo
