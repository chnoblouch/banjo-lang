#ifndef BANJO_LSP_HANDLERS_INITIALIZE_HANDLER_H
#define BANJO_LSP_HANDLERS_INITIALIZE_HANDLER_H

#include "connection.hpp"

namespace banjo {

namespace lsp {

class InitializeHandler : public RequestHandler {

public:
    InitializeHandler();
    ~InitializeHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    void init_config(const JSONValue &workspace_folders);
};

} // namespace lsp

} // namespace banjo

#endif
