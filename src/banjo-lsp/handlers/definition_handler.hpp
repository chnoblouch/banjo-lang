#ifndef LSP_DEFINITION_HANDLER_H
#define LSP_DEFINITION_HANDLER_H

#include "banjo/ast/ast_module.hpp"
#include "connection.hpp"
#include "workspace.hpp"
#include "banjo/symbol/enumeration.hpp"

namespace banjo {

namespace lsp {

class DefinitionHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    DefinitionHandler(Workspace &workspace);
    ~DefinitionHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace lsp

} // namespace banjo

#endif
