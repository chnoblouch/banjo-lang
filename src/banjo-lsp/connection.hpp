#ifndef BANJO_LSP_CONNECTION_H
#define BANJO_LSP_CONNECTION_H

#include "banjo/utils/json.hpp"
#include "base_message_stream.hpp"

#include <functional>
#include <unordered_map>

namespace banjo::lsp {

class Connection;

class RequestHandler {
public:
    virtual ~RequestHandler() = default;
    virtual json::Value handle(const json::Object &params, Connection &connection) = 0;
};

class Connection {

private:
    typedef std::function<void(json::Object &params)> NotificationHandler;

private:
    BaseMessageStream stream;
    std::unordered_map<std::string, RequestHandler *> request_handlers;
    std::unordered_map<std::string, NotificationHandler> notification_handlers;

public:
    void on_request(std::string method, RequestHandler *request_handler);
    void on_notification(std::string method, NotificationHandler notification_handler);
    void start();
    void send_notification(std::string method, const json::Object &params);

private:
    void handle_message(BaseMessage &message);
    void send_response(json::Value id, const json::Value &result);
    BaseMessage json_object_to_message(const json::Object &object);
};

} // namespace banjo::lsp

#endif
