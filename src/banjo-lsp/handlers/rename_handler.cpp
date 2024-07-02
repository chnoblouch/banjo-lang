#include "rename_handler.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/ast_utils.hpp"
#include "banjo/ast/expr.hpp"
#include "ast_navigation.hpp"
#include "json.hpp"
#include "protocol_structs.hpp"
#include "source_manager.hpp"
#include "banjo/symbol/local_variable.hpp"
#include "banjo/symbol/symbol.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "symbol_ref_info.hpp"
#include "uri.hpp"

#include <unordered_map>

namespace banjo {

namespace lsp {

RenameHandler::RenameHandler(SourceManager &source_manager) : source_manager(source_manager) {}

RenameHandler::~RenameHandler() {}

JSONValue RenameHandler::handle(const JSONObject &params, Connection &connection) {
    const JSONObject &document = params.get_object("textDocument");
    std::filesystem::path path = URI::decode_to_path(document.get_string("uri"));
    if (!source_manager.has_file(path)) {
        return JSONArray{};
    }

    const SourceFile &file = source_manager.get_file(path);

    const std::string &new_name = params.get_string("newName");

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

    std::unordered_map<lang::ASTModule *, std::vector<lang::Identifier *>> references;
    references[info->find_definition_mod()].push_back(info->definition_ident);

    for (const lang::SymbolUse &use : info->symbol->get_uses()) {
        references[use.mod].push_back(use.node);
    }

    JSONObject changes;

    for (const auto &[mod, references] : references) {
        SourceFile &reference_file = source_manager.get_file(mod->get_file_path());

        JSONArray edits;
        for (lang::Identifier *reference : references) {
            edits.add(JSONObject{
                {"range", ProtocolStructs::range_to_lsp(reference_file.source, reference->get_range())},
                {"newText", new_name},
            });
        }

        std::string uri = URI::encode_from_path(reference_file.file_path);
        changes.add(uri, edits);
    }

    return JSONObject{
        {"changes", changes},
    };
}

} // namespace lsp

} // namespace banjo
