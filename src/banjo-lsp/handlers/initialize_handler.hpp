#ifndef LSP_INITIALIZE_HANDLER_H
#define LSP_INITIALIZE_HANDLER_H

#include "connection.hpp"

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

#endif
