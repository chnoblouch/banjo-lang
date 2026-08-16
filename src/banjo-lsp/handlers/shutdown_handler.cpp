#include "shutdown_handler.hpp"

namespace banjo::lsp {

json::Value ShutdownHandler::handle(const json::Object &, Connection &) {
    return {
        {"result", json::Value{nullptr}},
    };
}

} // namespace banjo::lsp
