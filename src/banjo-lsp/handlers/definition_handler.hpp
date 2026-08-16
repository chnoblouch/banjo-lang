#ifndef BANJO_LSP_HANDLERS_DEFINITION_HANDLER_H
#define BANJO_LSP_HANDLERS_DEFINITION_HANDLER_H

#include "connection.hpp"
#include "workspace.hpp"

namespace banjo::lsp {

class DefinitionHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    DefinitionHandler(Workspace &workspace);
    ~DefinitionHandler();

    json::Value handle(const json::Object &params, Connection &connection);
};

} // namespace banjo::lsp

#endif
