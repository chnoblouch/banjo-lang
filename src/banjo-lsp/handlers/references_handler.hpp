#ifndef LSP_REFERENCES_HANDLER_H
#define LSP_REFERENCES_HANDLER_H

#include "connection.hpp"
#include "workspace.hpp"

namespace banjo {

namespace lsp {

class ReferencesHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    ReferencesHandler(Workspace &workspace);
    ~ReferencesHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace lsp

} // namespace banjo

#endif
