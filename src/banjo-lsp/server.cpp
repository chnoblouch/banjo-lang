#include "server.hpp"

#include "ast_navigation.hpp"
#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/config/config.hpp"
#include "connection.hpp"
#include "uri.hpp"
#include "workspace.hpp"

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
    Workspace workspace;

    InitializeHandler initialize_handler;
    CompletionHandler completion_handler(workspace);
    DefinitionHandler definition_handler(workspace);
    ReferencesHandler references_handler(workspace);
    RenameHandler rename_handler(workspace);
    SemanticTokensHandler semantic_tokens_handler(workspace);
    ShutdownHandler shutdown_handler;

    connection.on_request("initialize", &initialize_handler);
    connection.on_request("textDocument/completion", &completion_handler);
    connection.on_request("textDocument/definition", &definition_handler);
    connection.on_request("textDocument/references", &references_handler);
    connection.on_request("textDocument/rename", &rename_handler);
    connection.on_request("textDocument/semanticTokens/full", &semantic_tokens_handler);
    connection.on_request("shutdown", &shutdown_handler);

    connection.on_notification("initialized", [&](JSONObject &) { workspace.initialize(); });
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

    connection.on_notification("textDocument/didChange", [&](JSONObject &params) {
        const JSONObject &document = params.get_object("textDocument");
        std::filesystem::path fs_path = URI::decode_to_path(document.get_string("uri"));

        const JSONArray &changes = params.get_array("contentChanges");
        const JSONObject &last_change = changes.get_object(changes.length() - 1);
        std::string new_content = last_change.get_string("text");

        workspace.update(fs_path, new_content);
    });

    connection.start();
}

} // namespace lsp

} // namespace banjo
