#ifndef BANJO_LSP_MESSAGE_H
#define BANJO_LSP_MESSAGE_H

#include "banjo/utils/json.hpp"

#include <string>

namespace banjo {

namespace lsp {

struct Request {
    std::string id;
    std::string method;
    JSONObject params;
};

struct Notification {
    std::string method;
    JSONValue params;
};

} // namespace lsp

} // namespace banjo

#endif
