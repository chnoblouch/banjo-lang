#ifndef LSP_CONNECTION_H
#define LSP_CONNECTION_H

#include "base_message_stream.hpp"
#include "message.hpp"

#include <functional>
#include <unordered_map>

namespace lsp {

class Connection;

class RequestHandler {
public:
    virtual ~RequestHandler() = default;
    virtual JSONValue handle(const JSONObject &params, Connection &connection) = 0;
};

class Connection {

private:
    typedef std::function<void(JSONObject &params)> NotificationHandler;

private:
    BaseMessageStream stream;
    std::unordered_map<std::string, RequestHandler *> request_handlers;
    std::unordered_map<std::string, NotificationHandler> notification_handlers;

public:
    void on_request(std::string method, RequestHandler *request_handler);
    void on_notification(std::string method, NotificationHandler notification_handler);
    void start();
    void send_notification(std::string method, const JSONObject &params);

private:
    void handle_message(BaseMessage &message);
    void send_response(JSONValue id, const JSONValue &result);
    BaseMessage json_object_to_message(const JSONObject &object);
};

} // namespace lsp

#endif
