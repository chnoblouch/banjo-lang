#ifndef LSP_SEMANTIC_TOKENS_HANDLER_H
#define LSP_SEMANTIC_TOKENS_HANDLER_H

#include "connection.hpp"
#include "source/text_range.hpp"
#include "source_manager.hpp"

namespace banjo {

namespace lsp {

struct SemanticToken {
    lang::TextRange range;
    int type;
    int modifiers;
};

struct LSPSemanticToken {
    int delta_line;
    int delta_start_column;
    int length;
    int type;
    int modifiers;
};

class SemanticTokensHandler : public RequestHandler {

private:
    SourceManager &source_manager;

public:
    SemanticTokensHandler(SourceManager &source_manager);
    ~SemanticTokensHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
    void publish_diagnostics(const std::string &uri, Connection &connection);
    void build_tokens(std::vector<SemanticToken> &tokens, lang::ASTNode *node);
    std::vector<LSPSemanticToken> tokens_to_lsp(const std::string &source, const std::vector<SemanticToken> &tokens);
    JSONArray serialize(const std::vector<LSPSemanticToken> &lsp_tokens);
};

} // namespace lsp

} // namespace banjo

#endif
