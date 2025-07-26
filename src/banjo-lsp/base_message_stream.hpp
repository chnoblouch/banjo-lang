#ifndef BANJO_LSP_MESSAGE_STREAM_H
#define BANJO_LSP_MESSAGE_STREAM_H

#include "message.hpp"

#include <functional>
#include <string>

namespace banjo {

namespace lsp {

struct BaseMessage {
    unsigned long long content_length;
    std::string content_type;
    std::string content;
};

struct HeaderField {
    std::string name;
    std::string value;
};

class BaseMessageStream {

public:
    typedef std::function<void(BaseMessage &)> MessageHandler;

public:
    void start_reading(MessageHandler message_handler);
    void write_message(BaseMessage &message);

private:
    void read_header(BaseMessage &message);
    std::string read_line();
    HeaderField parse_header_field(std::string line);
    void process_header_field(const HeaderField &field, BaseMessage &message);
    void read_content(BaseMessage &message);
};

} // namespace lsp

} // namespace banjo

#endif
