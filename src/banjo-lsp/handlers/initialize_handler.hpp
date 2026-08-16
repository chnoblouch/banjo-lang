#ifndef BANJO_LSP_HANDLERS_INITIALIZE_HANDLER_H
#define BANJO_LSP_HANDLERS_INITIALIZE_HANDLER_H

#include "connection.hpp"

namespace banjo::lsp {

class InitializeHandler : public RequestHandler {

public:
    InitializeHandler();
    ~InitializeHandler();

    json::Value handle(const json::Object &params, Connection &connection);

private:
    void init_config(const json::Value &workspace_folders);
};

} // namespace banjo::lsp

#endif
