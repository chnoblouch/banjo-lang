#ifndef BANJO_LSP_HANDLERS_SHUTDOWN_HANDLER_H
#define BANJO_LSP_HANDLERS_SHUTDOWN_HANDLER_H

#include "connection.hpp"

namespace banjo::lsp {

class ShutdownHandler : public RequestHandler {

public:
    json::Value handle(const json::Object &params, Connection &connection);
};

} // namespace banjo::lsp

#endif
