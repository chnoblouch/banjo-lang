#include "connection.hpp"

#include "json_parser.hpp"
#include "json_serializer.hpp"

namespace lsp {

void Connection::start() {
    stream.start_reading([this](BaseMessage &message) { handle_message(message); });
}

void Connection::handle_message(BaseMessage &message) {
    JSONObject object = JSONParser(message.content).parse_object();

    std::string method = object.get("method").as_string();

    JSONObject params;
    if (object.contains("params")) {
        params = object.get("params").as_object();
    }

    if (object.contains("id")) {
        JSONValue id = object.get("id");

        if (request_handlers.count(method)) {
            RequestHandler *request_handler = request_handlers[method];
            JSONValue response = request_handler->handle(params, *this);
            send_response(id, response);
        }

        JSONObject response{
            {"method", "window/logMessage"},
            {"params", JSONObject{{"type", 3.0}, {"message", "request: " + method}}}
        };
        BaseMessage response_message = json_object_to_message(response);
        stream.write_message(response_message);
    } else {
        if (notification_handlers.count(method)) {
            NotificationHandler notification_handler = notification_handlers[method];
            notification_handler(params);
        }

        JSONObject response{
            {"method", "window/logMessage"},
            {"params", JSONObject{{"type", 3.0}, {"message", "notification: " + method}}}
        };
        BaseMessage response_message = json_object_to_message(response);
        stream.write_message(response_message);
    }
}

void Connection::on_request(std::string method, RequestHandler *request_handler) {
    request_handlers.insert({method, request_handler});
}

void Connection::on_notification(std::string method, NotificationHandler notification_handler) {
    notification_handlers.insert({method, notification_handler});
}

void Connection::send_response(JSONValue id, const JSONValue &result) {
    JSONObject response{{"id", id}, {"result", result}};
    BaseMessage response_message = json_object_to_message(response);
    stream.write_message(response_message);
}

void Connection::send_notification(std::string method, const JSONObject &params) {
    JSONObject notification{{"method", method}, {"params", params}};
    BaseMessage message = json_object_to_message(notification);
    stream.write_message(message);
}

BaseMessage Connection::json_object_to_message(const JSONObject &object) {
    std::stringstream content_stream;
    JSONSerializer(content_stream).serialize(object);
    std::string content = content_stream.str();

    return {
        .content_length = content.size(),
        .content_type = "application/vscode-jsonrpc; charset=utf-8",
        .content = content
    };
}

} // namespace lsp
