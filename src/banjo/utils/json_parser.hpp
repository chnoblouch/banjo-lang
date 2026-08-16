#ifndef BANJO_UTILS_JSON_PARSER_H
#define BANJO_UTILS_JSON_PARSER_H

#include "banjo/utils/json.hpp"

#include <string_view>

namespace banjo::json {

class Parser {

private:
    std::string_view buffer;
    unsigned position;

public:
    Parser(std::string_view buffer);
    std::optional<Value> parse();

private:
    std::string parse_key();
    std::optional<Value> parse_value();

    std::optional<Value> parse_null();
    std::optional<Value> parse_false();
    std::optional<Value> parse_true();
    std::optional<Value> parse_number();
    std::optional<Value> parse_string();
    std::optional<Value> parse_array();
    std::optional<Value> parse_object();

    std::optional<std::string> parse_string_raw();
    void skip_whitespace();

    std::optional<char> get();
    std::optional<char> consume();
    std::optional<std::string_view> consume_n(unsigned count);
};

} // namespace banjo::json

#endif
