#include "semantic_tokens_handler.hpp"

#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "ast_navigation.hpp"
#include "protocol_structs.hpp"
#include "symbol/symbol_ref.hpp"
#include "uri.hpp"

namespace lsp {

SemanticTokensHandler::SemanticTokensHandler(SourceManager &source_manager) : source_manager(source_manager) {}

SemanticTokensHandler::~SemanticTokensHandler() {}

JSONValue SemanticTokensHandler::handle(const JSONObject &params, Connection &connection) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path path = URI::decode_to_path(uri);
    if (!source_manager.has_file(path)) {
        return JSONObject{{"data", JSONArray{}}};
    }

    const SourceFile &file = source_manager.get_file(path);

    std::vector<SemanticToken> tokens;
    build_tokens(tokens, file.module_node);
    std::vector<LSPSemanticToken> lsp_tokens = tokens_to_lsp(file.source, tokens);
    JSONArray data = serialize(lsp_tokens);

    publish_diagnostics(uri, connection);

    return JSONObject{{"data", data}};
}

void SemanticTokensHandler::publish_diagnostics(const std::string &uri, Connection &connection) {
    std::filesystem::path path = URI::decode_to_path(uri);
    JSONArray diagnostics;

    for (const lang::Report &report : source_manager.get_reports()) {
        const lang::SourceLocation &location = *report.get_message().location;

        std::filesystem::path report_file_path = *source_manager.get_module_manager().find_source_file(location.path);
        if (std::filesystem::absolute(report_file_path) != std::filesystem::absolute(path)) {
            continue;
        }

        const std::string report_source = source_manager.get_file(report_file_path).source;
        LSPTextPosition start = ASTNavigation::pos_to_lsp(report_source, location.range.start);
        LSPTextPosition end = ASTNavigation::pos_to_lsp(report_source, location.range.end);

        std::string message = report.get_message().text;

        for (const lang::ReportMessage &note : report.get_notes()) {
            message += "\n" + note.text;
        }

        // TODO: add protocol enum
        int severity;
        switch (report.get_type()) {
            case lang::Report::Type::ERROR: severity = 1; break;
            case lang::Report::Type::WARNING: severity = 2; break;
        }

        diagnostics.add(JSONObject{
            {"range",
             JSONObject{
                 {"start", JSONObject{{"line", start.line}, {"character", start.column}}},
                 {"end", JSONObject{{"line", end.line}, {"character", end.column}}},
             }},
            {"severity", severity},
            {"message", message},
        });
    }

    JSONObject notification{{"uri", uri}, {"diagnostics", diagnostics}};
    connection.send_notification("textDocument/publishDiagnostics", notification);
}

void SemanticTokensHandler::build_tokens(std::vector<SemanticToken> &tokens, lang::ASTNode *node) {
    lang::TextRange range = node->get_range();

    if (node->get_type() == lang::AST_IDENTIFIER) {
        lang::Identifier *identifier = node->as<lang::Identifier>();

        if (identifier->get_symbol()) {
            std::optional<lang::SymbolRef> symbol = identifier->get_symbol()->resolve();

            if (symbol) {
                switch (symbol->get_kind()) {
                    case lang::SymbolKind::MODULE:
                        tokens.push_back({range, SemanticTokenType::NAMESPACE, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::FUNCTION:
                    case lang::SymbolKind::GENERIC_FUNC:
                        tokens.push_back({range, SemanticTokenType::FUNCTION, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::LOCAL:
                        tokens.push_back({range, SemanticTokenType::VARIABLE, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::PARAM:
                        tokens.push_back({range, SemanticTokenType::PARAMETER, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::GLOBAL:
                        tokens.push_back({range, SemanticTokenType::VARIABLE, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::CONST:
                        tokens.push_back({range, SemanticTokenType::VARIABLE, SemanticTokenModifiers::READONLY});
                        break;
                    case lang::SymbolKind::STRUCT:
                    case lang::SymbolKind::UNION:
                    case lang::SymbolKind::UNION_CASE:
                    case lang::SymbolKind::PROTO:
                    case lang::SymbolKind::GENERIC_STRUCT:
                        tokens.push_back({range, SemanticTokenType::STRUCT, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::ENUM:
                        tokens.push_back({range, SemanticTokenType::ENUM, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::ENUM_VARIANT:
                        tokens.push_back({range, SemanticTokenType::ENUM_MEMBER, SemanticTokenModifiers::NONE});
                        break;
                    case lang::SymbolKind::FIELD:
                    case lang::SymbolKind::UNION_CASE_FIELD:
                        tokens.push_back({range, SemanticTokenType::PROPERTY, SemanticTokenModifiers::NONE});
                        break;
                    default: break;
                }
            }
        }
    } else if (node->get_type() == lang::AST_SELF) {
        tokens.push_back({range, SemanticTokenType::KEYWORD, SemanticTokenModifiers::NONE});
    }

    for (lang::ASTNode *child : node->get_children()) {
        build_tokens(tokens, child);
    }
}

std::vector<LSPSemanticToken> SemanticTokensHandler::tokens_to_lsp(
    const std::string &source,
    const std::vector<SemanticToken> &tokens
) {
    std::vector<LSPSemanticToken> lsp_tokens;
    int last_line = 0;
    int last_start_column = 0;

    for (const SemanticToken &token : tokens) {
        LSPTextPosition start = ASTNavigation::pos_to_lsp(source, token.range.start);
        LSPTextPosition end = ASTNavigation::pos_to_lsp(source, token.range.end);
        end.column++;

        lsp_tokens.push_back({
            .delta_line = start.line - last_line,
            .delta_start_column = start.line == last_line == 0 ? start.column : start.column - last_start_column,
            .length = static_cast<int>(token.range.end - token.range.start),
            .type = token.type,
            .modifiers = token.modifiers,
        });

        last_line = start.line;
        last_start_column = start.column;
    }

    return lsp_tokens;
}

JSONArray SemanticTokensHandler::serialize(const std::vector<LSPSemanticToken> &lsp_tokens) {
    JSONArray data;

    for (const LSPSemanticToken &lsp_token : lsp_tokens) {
        data.add(lsp_token.delta_line);
        data.add(lsp_token.delta_start_column);
        data.add(lsp_token.length);
        data.add(lsp_token.type);
        data.add(lsp_token.modifiers);
    }

    return data;
}

} // namespace lsp
