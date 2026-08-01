#ifndef BANJO_LSP_HANDLERS_SEMANTIC_TOKENS_HANDLER_H
#define BANJO_LSP_HANDLERS_SEMANTIC_TOKENS_HANDLER_H

#include "banjo/source/text_range.hpp"
#include "connection.hpp"
#include "index.hpp"
#include "workspace.hpp"

namespace banjo::lsp {

struct SemanticToken {
    TextRange range;
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
    Workspace &workspace;

public:
    SemanticTokensHandler(Workspace &workspace);
    ~SemanticTokensHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
    std::vector<LSPSemanticToken> tokens_to_lsp(const std::string &source, const std::vector<SemanticToken> &tokens);
    JSONArray serialize(const std::vector<LSPSemanticToken> &lsp_tokens);

private:
    void add_symbol_token(std::vector<SemanticToken> &tokens, TextRange range, const sir::Symbol &symbol);
};

} // namespace banjo::lsp

#endif
