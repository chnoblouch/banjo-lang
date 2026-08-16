#include "connection.hpp"

#include "banjo/utils/json_parser.hpp"
#include "banjo/utils/json_serializer.hpp"

#include <sstream>

namespace banjo::lsp {

void Connection::start() {
    stream.start_reading([this](BaseMessage &message) { handle_message(message); });
}

void Connection::handle_message(BaseMessage &message) {
    json::Object object = json::Parser{message.content}.parse()->as_object();
    std::string method = object.get("method").as_string();

    json::Object params;
    if (object.contains("params")) {
        params = object.get("params").as_object();
    }

    if (object.contains("id")) {
        json::Value id = object.get("id");

        if (request_handlers.count(method)) {
            RequestHandler *request_handler = request_handlers[method];
            json::Value response = request_handler->handle(params, *this);
            send_response(id, response);
        }

        json::Object response{
            {"method", "window/logMessage"},
            {"params", json::Object{{"type", 3.0}, {"message", "request: " + method}}}
        };
        BaseMessage response_message = json_object_to_message(response);
        stream.write_message(response_message);
    } else {
        if (notification_handlers.count(method)) {
            NotificationHandler notification_handler = notification_handlers[method];
            notification_handler(params);
        }

        json::Object response{
            {"method", "window/logMessage"},
            {"params", json::Object{{"type", 3.0}, {"message", "notification: " + method}}}
        };
        BaseMessage response_message = json_object_to_message(response);
        stream.write_message(response_message);
    }
}

void Connection::on_request(std::string method, RequestHandler *request_handler) {
    request_handlers.emplace(std::move(method), request_handler);
}

void Connection::on_notification(std::string method, NotificationHandler notification_handler) {
    notification_handlers.emplace(std::move(method), std::move(notification_handler));
}

void Connection::send_response(json::Value id, const json::Value &result) {
    json::Object response{{"id", id}, {"result", result}};
    BaseMessage response_message = json_object_to_message(response);
    stream.write_message(response_message);
}

void Connection::send_notification(std::string method, const json::Object &params) {
    json::Object notification{{"method", method}, {"params", params}};
    BaseMessage message = json_object_to_message(notification);
    stream.write_message(message);
}

BaseMessage Connection::json_object_to_message(const json::Object &object) {
    std::stringstream content_stream;
    json::Serializer(content_stream).serialize(object);
    std::string content = content_stream.str();

    return {
        .content_length = content.size(),
        .content_type = "application/vscode-jsonrpc; charset=utf-8",
        .content = content
    };
}

} // namespace banjo::lsp
