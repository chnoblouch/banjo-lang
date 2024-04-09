#include "shutdown_handler.hpp"

namespace lsp {

JSONValue ShutdownHandler::handle(const JSONObject &, Connection &) {
    return {
        {"result", JSONValue(JSON_NULL)},
    };
}

} // namespace lsp
