#include "definition_handler.hpp"

#include "ast/ast_module.hpp"
#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "ast_navigation.hpp"
#include "protocol_structs.hpp"
#include "source/text_range.hpp"
#include "symbol/symbol_ref.hpp"
#include "symbol_ref_info.hpp"
#include "uri.hpp"

namespace lsp {

DefinitionHandler::DefinitionHandler(SourceManager &source_manager) : source_manager(source_manager) {}

DefinitionHandler::~DefinitionHandler() {}

JSONValue DefinitionHandler::handle(const JSONObject &params, Connection &connection) {
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

    lang::Identifier *identifier = node->as<lang::Identifier>();

    if (identifier->get_symbol() && identifier->get_symbol()->get_kind() == lang::SymbolKind::MODULE) {
        lang::ASTModule *mod = identifier->get_symbol()->get_module();
        return build_def(mod, mod->get_range(), {0, 0});
    }

    std::optional<SymbolRefInfo> info = SymbolRefInfo::look_up(identifier);
    if (info) {
        lang::ASTModule *mod = info->find_definition_mod();
        lang::TextRange range = info->symbol->get_node()->get_range();
        lang::TextRange selection_range = info->definition_ident->get_range();
        return build_def(mod, range, selection_range);
    }

    return JSONArray{};
}

JSONObject DefinitionHandler::build_def(
    lang::ASTModule *module_,
    lang::TextRange range,
    lang::TextRange selection_range
) {
    const SourceFile &target_file = source_manager.get_file(module_->get_path());
    const std::string &source = target_file.source;

    return {JSONObject{
        {"targetUri", URI::encode_from_path(target_file.file_path)},
        {"targetRange", ProtocolStructs::range_to_lsp(source, range)},
        {"targetSelectionRange", ProtocolStructs::range_to_lsp(source, selection_range)}
    }};
}

} // namespace lsp
