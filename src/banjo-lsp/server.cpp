#include "server.hpp"

#include "connection.hpp"
#include "diagnostics.hpp"
#include "uri.hpp"
#include "workspace.hpp"

#include "handlers/completion_handler.hpp"
#include "handlers/completion_item_resolve_handler.hpp"
#include "handlers/definition_handler.hpp"
#include "handlers/formatting_handler.hpp"
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
    CompletionHandler completion_handler{workspace};
    CompletionItemResolveHandler completion_item_resolve_handler{workspace};
    DefinitionHandler definition_handler{workspace};
    ReferencesHandler references_handler{workspace};
    RenameHandler rename_handler{workspace};
    FormattingHandler formatting_handler{workspace};
    SemanticTokensHandler semantic_tokens_handler{workspace};
    ShutdownHandler shutdown_handler;

    connection.on_request("initialize", &initialize_handler);
    connection.on_request("textDocument/completion", &completion_handler);
    connection.on_request("completionItem/resolve", &completion_item_resolve_handler);
    connection.on_request("textDocument/definition", &definition_handler);
    connection.on_request("textDocument/references", &references_handler);
    connection.on_request("textDocument/rename", &rename_handler);
    connection.on_request("textDocument/formatting", &formatting_handler);
    connection.on_request("textDocument/semanticTokens/full", &semantic_tokens_handler);
    connection.on_request("shutdown", &shutdown_handler);

    connection.on_notification("initialized", [&](json::Object &) {
        std::vector<SourceFile *> mods = workspace.initialize();
        publish_diagnostics(connection, workspace, mods);
    });

    connection.on_notification("exit", [](json::Object &) { std::exit(0); });

    /*
    connection.on_request("textDocument/hover", [&source_manager](json::Object& params) {
        const json::Object& document = params.get_object("textDocument");
        std::filesystem::path path = URI::decode_to_path(document.get_string("uri"));
        const SourceFile& file = source_manager.get_file(path);

        const json::Object& lsp_position = params.get_object("position");
        int line = lsp_position.get_number("line");
        int column = lsp_position.get_number("character");
        TextPosition position = ASTNavigation::pos_from_lsp(file.source, line, column);

        ASTNode* hovered_node = ASTNavigation::get_node_at(file.module_node, position);
        return json::Object { {"contents", "HOVER"} };
    });
    */

    /*
    connection.on_notification("textDocument/didOpen", [&source_manager](json::Object &params) {
        const json::Object &document = params.get_object("textDocument");
        std::filesystem::path path = URI::decode_to_path(document.get_string("uri"));
        std::string text = document.get_string("text");
    });
    */

    connection.on_notification("textDocument/didChange", [&](json::Object &params) {
        const json::Object &document = params.get_object("textDocument");
        std::filesystem::path fs_path = URI::decode_to_path(document.get_string("uri"));

        const json::Array &changes = params.get_array("contentChanges");
        const json::Object &last_change = changes.get_object(changes.length() - 1);
        std::string new_content = last_change.get_string("text");

        std::vector<SourceFile *> files = workspace.update(fs_path, new_content);
        publish_diagnostics(connection, workspace, files);
    });

    connection.start();
}

} // namespace lsp

} // namespace banjo
