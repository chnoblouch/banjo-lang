#include "shutdown_handler.hpp"

namespace banjo {

namespace lsp {

JSONValue ShutdownHandler::handle(const JSONObject &, Connection &) {
    return {
        {"result", JSONValue(JSON_NULL)},
    };
}

} // namespace lsp

} // namespace banjo
