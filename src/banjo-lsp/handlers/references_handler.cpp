#include "references_handler.hpp"

#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "ast_navigation.hpp"
#include "json.hpp"
#include "protocol_structs.hpp"
#include "symbol/symbol.hpp"
#include "symbol/symbol_ref.hpp"
#include "symbol_ref_info.hpp"
#include "uri.hpp"

namespace lsp {

ReferencesHandler::ReferencesHandler(SourceManager &source_manager) : source_manager(source_manager) {}

ReferencesHandler::~ReferencesHandler() {}

JSONValue ReferencesHandler::handle(const JSONObject &params, Connection &connection) {
    const JSONObject &document = params.get_object("textDocument");
    std::filesystem::path path = URI::decode_to_path(document.get_string("uri"));
    if (!source_manager.has_file(path)) {
        return JSONArray{};
    }

    const SourceFile &file = source_manager.get_file(path);

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_number("line");
    int column = lsp_position.get_number("character");
    lang::TextPosition position = ASTNavigation::pos_from_lsp(file.source, line, column);

    lang::ASTNode *node = ASTNavigation::get_node_at(file.module_node, position);
    if (node->get_type() != lang::AST_IDENTIFIER) {
        return JSONArray{};
    }

    lang::Identifier *ident = node->as<lang::Identifier>();
    std::optional<SymbolRefInfo> info = SymbolRefInfo::look_up(ident);
    if (!info) {
        return JSONArray{};
    }

    JSONArray array;
    for (const lang::SymbolUse &use : info->symbol->get_uses()) {
        const SourceFile &use_file = source_manager.get_file(use.mod->get_path());

        array.add({JSONObject{
            {"uri", URI::encode_from_path(use_file.file_path)},
            {"range", ProtocolStructs::range_to_lsp(use_file.source, use.node->get_range())},
        }});
    }

    return array;
}

} // namespace lsp
