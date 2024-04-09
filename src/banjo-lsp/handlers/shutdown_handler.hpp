#ifndef LSP_SHUTDOWN_HANDLER_H
#define LSP_SHUTDOWN_HANDLER_H

#include "connection.hpp"

namespace lsp {

class ShutdownHandler : public RequestHandler {

public:
    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace lsp

#endif
