#ifndef BANJO_LSP_HANDLERS_DEFINITION_HANDLER_H
#define BANJO_LSP_HANDLERS_DEFINITION_HANDLER_H

#include "connection.hpp"
#include "workspace.hpp"

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
