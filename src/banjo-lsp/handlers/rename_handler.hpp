#ifndef LSP_RENAME_HANDLER_H
#define LSP_RENAME_HANDLER_H

#include "banjo/ast/ast_node.hpp"
#include "connection.hpp"
#include "workspace.hpp"
#include "banjo/symbol/symbol.hpp"

namespace banjo {

namespace lsp {

struct SymbolDefinition {
    lang::Symbol *symbol;
    lang::ASTNode *identifier;
};

class RenameHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    RenameHandler(Workspace &workspace);
    ~RenameHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace lsp

} // namespace banjo

#endif
