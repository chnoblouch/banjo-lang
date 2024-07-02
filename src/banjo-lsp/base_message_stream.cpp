#include "base_message_stream.hpp"

#include "banjo/utils/platform.hpp"

#include <cstdio>
#include <iostream>

#ifdef OS_WINDOWS
#    include <fcntl.h>
#    include <io.h>
#endif

namespace banjo {

namespace lsp {

void BaseMessageStream::start_reading(MessageHandler handler) {
#ifdef OS_WINDOWS
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    while (true) {
        BaseMessage message;
        read_header(message);
        read_content(message);
        handler(message);
    }
}

void BaseMessageStream::read_header(BaseMessage &message) {
    std::string line = read_line();

    while (line.length() > 0) {
        HeaderField header_field = parse_header_field(line);
        process_header_field(header_field, message);
        line = read_line();
    }
}

std::string BaseMessageStream::read_line() {
    std::string line;

    while (line.size() < 2 || line[line.size() - 2] != '\r' || line[line.size() - 1] != '\n') {
        line += (char)std::cin.get();
    }

    line = line.substr(0, line.length() - 2);
    return line;
}

HeaderField BaseMessageStream::parse_header_field(std::string line) {
    std::string name;
    std::string value;

    for (int i = 0; i < line.length(); i++) {
        if (i <= line.length() - 2 && line[i] == ':' && line[i + 1] == ' ') {
            value = line.substr(i + 2);
            break;
        }

        name += line[i];
    }

    return HeaderField{name, value};
}

void BaseMessageStream::process_header_field(const HeaderField &field, BaseMessage &message) {
    if (field.name == "Content-Length") {
        message.content_length = std::stoi(field.value);
    } else if (field.name == "Content-Type") {
        message.content_type = field.value;
    }
}

void BaseMessageStream::read_content(BaseMessage &message) {
    message.content.resize(message.content_length);
    std::cin.read(message.content.data(), message.content_length);
}

void BaseMessageStream::write_message(BaseMessage &message) {
    std::cout << "Content-Type: application/vscode-jsonrpc; charset=utf-8\r\n";
    std::cout << "Content-Length: " << message.content_length << "\r\n\r\n";
    std::cout << message.content;
    std::cout.flush();
}

} // namespace lsp

} // namespace banjo
