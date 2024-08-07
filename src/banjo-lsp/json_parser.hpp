#ifndef LSP_JSON_PARSER_H
#define LSP_JSON_PARSER_H

#include "json.hpp"
#include <sstream>

namespace banjo {

namespace lsp {

class JSONParser {

private:
    std::stringstream stream;

public:
    JSONParser(std::string string);
    JSONObject parse_object();
    JSONArray parse_array();

private:
    std::string parse_key();
    JSONValue parse_value();

    JSONNumber parse_number();

    void skip_whitespace();
};

} // namespace lsp

} // namespace banjo

#endif
