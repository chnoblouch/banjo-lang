#ifndef BANJO_LSP_MESSAGE_H
#define BANJO_LSP_MESSAGE_H

#include "banjo/utils/json.hpp"

#include <string>

namespace banjo::lsp {

struct Request {
    std::string id;
    std::string method;
    json::Object params;
};

struct Notification {
    std::string method;
    json::Value params;
};

} // namespace banjo::lsp

#endif
