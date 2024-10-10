#ifndef LSP_JSON_PARSER_H
#define LSP_JSON_PARSER_H

#include "banjo/utils/json.hpp"

#include <sstream>

namespace banjo {

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

} // namespace banjo

#endif
