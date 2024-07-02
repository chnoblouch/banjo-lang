#include "server.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast_navigation.hpp"
#include "config/config.hpp"
#include "connection.hpp"
#include "source_manager.hpp"
#include "uri.hpp"

#include "handlers/completion_handler.hpp"
#include "handlers/definition_handler.hpp"
#include "handlers/initialize_handler.hpp"
#include "handlers/references_handler.hpp"
#include "handlers/rename_handler.hpp"
#include "handlers/semantic_tokens_handler.hpp"
#include "handlers/shutdown_handler.hpp"

namespace banjo {

namespace lsp {

void Server::start() {
    Connection connection;
    SourceManager source_manager;

    InitializeHandler initialize_handler;
    CompletionHandler completion_handler(source_manager);
    DefinitionHandler definition_handler(source_manager);
    ReferencesHandler references_handler(source_manager);
    RenameHandler rename_handler(source_manager);
    SemanticTokensHandler semantic_tokens_handler(source_manager);
    ShutdownHandler shutdown_handler;

    connection.on_request("initialize", &initialize_handler);
    connection.on_request("textDocument/completion", &completion_handler);
    connection.on_request("textDocument/definition", &definition_handler);
    connection.on_request("textDocument/references", &references_handler);
    connection.on_request("textDocument/rename", &rename_handler);
    connection.on_request("textDocument/semanticTokens/full", &semantic_tokens_handler);
    connection.on_request("shutdown", &shutdown_handler);

    connection.on_notification("initialized", [&source_manager](JSONObject &) { source_manager.parse_full_source(); });
    connection.on_notification("exit", [](JSONObject &) { std::exit(0); });

    /*
    connection.on_request("textDocument/hover", [&source_manager](JSONObject& params) {
        const JSONObject& document = params.get_object("textDocument");
        std::filesystem::path path = URI::decode_to_path(document.get_string("uri"));
        const SourceFile& file = source_manager.get_file(path);

        const JSONObject& lsp_position = params.get_object("position");
        int line = lsp_position.get_number("line");
        int column = lsp_position.get_number("character");
        lang::TextPosition position = ASTNavigation::pos_from_lsp(file.source, line, column);

        lang::ASTNode* hovered_node = ASTNavigation::get_node_at(file.module_node, position);
        return JSONObject { {"contents", "HOVER"} };
    });
    */

    /*
    connection.on_notification("textDocument/didOpen", [&source_manager](JSONObject &params) {
        const JSONObject &document = params.get_object("textDocument");
        std::filesystem::path path = URI::decode_to_path(document.get_string("uri"));
        std::string text = document.get_string("text");
    });
    */

    connection.on_notification("textDocument/didChange", [&source_manager](JSONObject &params) {
        const JSONObject &document = params.get_object("textDocument");
        std::filesystem::path path = URI::decode_to_path(document.get_string("uri"));
        SourceFile &file = source_manager.get_file(path);

        const JSONArray &changes = params.get_array("contentChanges");
        const JSONObject &last_change = changes.get_object(changes.length() - 1);
        file.source = last_change.get_string("text");

        source_manager.on_file_changed(file);
    });

    connection.start();
}

} // namespace lsp

} // namespace banjo
