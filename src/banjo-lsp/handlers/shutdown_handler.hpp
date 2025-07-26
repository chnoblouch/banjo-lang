#ifndef BANJO_LSP_HANDLERS_SHUTDOWN_HANDLER_H
#define BANJO_LSP_HANDLERS_SHUTDOWN_HANDLER_H

#include "connection.hpp"

namespace banjo {

namespace lsp {

class ShutdownHandler : public RequestHandler {

public:
    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace lsp

} // namespace banjo

#endif
