#ifndef LSP_RENAME_HANDLER_H
#define LSP_RENAME_HANDLER_H

#include "ast/ast_node.hpp"
#include "connection.hpp"
#include "source_manager.hpp"
#include "symbol/symbol.hpp"

namespace lsp {

struct SymbolDefinition {
    lang::Symbol *symbol;
    lang::ASTNode *identifier;
};

class RenameHandler : public RequestHandler {

private:
    SourceManager &source_manager;

public:
    RenameHandler(SourceManager &source_manager);
    ~RenameHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace lsp

#endif
