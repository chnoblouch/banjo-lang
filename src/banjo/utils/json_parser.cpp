#include "json_parser.hpp"

#include "banjo/utils/json.hpp"

#include <optional>
#include <string>

namespace banjo::json {

Parser::Parser(std::string_view buffer) : buffer{buffer}, position{0} {}

std::optional<Value> Parser::parse() {
    skip_whitespace();
    std::optional<Value> value = parse_value();
    skip_whitespace();
    return position == buffer.size() ? value : std::optional<Value>{};
}

std::optional<Value> Parser::parse_value() {
    std::optional<char> c = get();

    if (c == 'n') {
        return parse_null();
    } else if (c == 'f') {
        return parse_false();
    } else if (c == 't') {
        return parse_true();
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        return parse_number();
    } else if (c == '\"') {
        return parse_string();
    } else if (c == '[') {
        return parse_array();
    } else if (c == '{') {
        return parse_object();
    } else {
        return {};
    }
}

std::optional<Value> Parser::parse_null() {
    if (consume_n(4) == "null") {
        return Value{nullptr};
    } else {
        return {};
    }
}

std::optional<Value> Parser::parse_false() {
    if (consume_n(5) == "false") {
        return Value{false};
    } else {
        return {};
    }
}

std::optional<Value> Parser::parse_true() {
    if (consume_n(4) == "true") {
        return Value{true};
    } else {
        return {};
    }
}

std::optional<Value> Parser::parse_number() {
    unsigned start = position;
    bool is_fp = false;
    std::optional<char> c = get();

    if (c == '-') {
        position += 1;
        c = get();
    }

    while (c && c >= '0' && c <= '9') {
        position += 1;
        c = get();

        if (c == '.') {
            if (is_fp) {
                return {};
            } else {
                is_fp = true;
                position += 1;
                c = get();
            }
        }
    }

    if (buffer[position - 1] == '-') {
        return {};
    }

    std::string slice{buffer.substr(start, position - start)};

    if (is_fp) {
        return Value{std::stod(slice)};
    } else {
        return Value{std::stoll(slice)};
    }
}

std::optional<Value> Parser::parse_string() {
    if (std::optional<std::string> string = parse_string_raw()) {
        return Value{*string};
    } else {
        return {};
    }
}

std::optional<Value> Parser::parse_array() {
    position += 1;
    Array array;

    skip_whitespace();

    if (get() == ']') {
        position += 1;
        return array;
    }

    while (true) {
        if (std::optional<Value> value = parse_value()) {
            array.add(*value);
        } else {
            return {};
        }

        skip_whitespace();

        if (get() == ']') {
            position += 1;
            return array;
        } else if (get() == ',') {
            position += 1;
            skip_whitespace();
        } else {
            return {};
        }
    }

    return array;
}

std::optional<Value> Parser::parse_object() {
    position += 1;
    Object object;

    skip_whitespace();

    if (get() == '}') {
        position += 1;
        return object;
    }

    while (true) {
        std::optional<std::string> key = parse_string_raw();
        if (!key) {
            return {};
        }

        skip_whitespace();

        if (get() == ':') {
            position += 1;
        } else {
            return {};
        }

        skip_whitespace();

        if (std::optional<Value> value = parse_value()) {
            object.add(*key, *value);
        } else {
            return {};
        }

        skip_whitespace();

        if (get() == '}') {
            position += 1;
            return object;
        } else if (get() == ',') {
            position += 1;
            skip_whitespace();
        } else {
            return {};
        }
    }

    return object;
}

std::optional<std::string> Parser::parse_string_raw() {
    position += 1;
    std::string value;

    while (position < buffer.size()) {
        std::optional<char> c = consume();

        if (!c) {
            return {};
        } else if (c == '\\') {
            std::optional<char> c = consume();

            if (c == '\"') {
                value += '\"';
            } else if (c == '\\') {
                value += '\\';
            } else if (c == 'b') {
                value += '\b';
            } else if (c == 'f') {
                value += '\f';
            } else if (c == 'n') {
                value += '\n';
            } else if (c == 'r') {
                value += '\r';
            } else if (c == 't') {
                value += '\t';
            } else {
                return {};
            }
        } else if (c == '\"') {
            return value;
        } else {
            value += *c;
        }
    }

    return {};
}

void Parser::skip_whitespace() {
    while (position < buffer.size()) {
        char c = buffer[position];

        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            break;
        }

        position += 1;
    }
}

std::optional<char> Parser::get() {
    return position < buffer.size() ? buffer[position] : std::optional<char>{};
}

std::optional<char> Parser::consume() {
    return position < buffer.size() ? buffer[position++] : std::optional<char>{};
}

std::optional<std::string_view> Parser::consume_n(unsigned count) {
    if (position + count > buffer.size()) {
        return {};
    }

    std::string_view slice = buffer.substr(position, count);
    position += count;
    return slice;
}

} // namespace banjo::json
