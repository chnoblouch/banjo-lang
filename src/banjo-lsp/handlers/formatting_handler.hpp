#ifndef BANJO_LSP_HANDLERS_FORMATTING_HANDLER_H
#define BANJO_LSP_HANDLERS_FORMATTING_HANDLER_H

#include "connection.hpp"
#include "workspace.hpp"

namespace banjo::lsp {

class FormattingHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    FormattingHandler(Workspace &workspace);
    ~FormattingHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace banjo::lsp

#endif
