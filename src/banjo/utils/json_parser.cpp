#include "json_parser.hpp"

namespace banjo {

JSONParser::JSONParser(std::string string) : stream(string) {}

JSONObject JSONParser::parse_object() {
    JSONObject object;

    stream.get(); // Consume '{'
    skip_whitespace();

    while (stream.peek() != '}') {
        std::string key = parse_key();

        skip_whitespace();
        stream.get(); // Consume ':'
        skip_whitespace();

        JSONValue value = parse_value();
        object.add(key, value);

        skip_whitespace();
        if (stream.peek() == ',') {
            stream.get();
        }
        skip_whitespace();
    }

    stream.get(); // Consume '}'

    return object;
}

JSONArray JSONParser::parse_array() {
    JSONArray array;

    stream.get(); // Consume '['
    skip_whitespace();

    while (stream.peek() != ']') {
        JSONValue value = parse_value();
        array.add(value);

        skip_whitespace();
        if (stream.peek() == ',') {
            stream.get();
        }
        skip_whitespace();
    }

    stream.get(); // Consume ']'

    return array;
}

std::string JSONParser::parse_key() {
    std::string key;
    stream.get(); // Consume '"'

    char c;
    while ((c = stream.get()) != '\"') {
        key += c;
    }

    return key;
}

JSONValue JSONParser::parse_value() {
    char c = stream.peek();

    if (c == '\"') {
        stream.get(); // Consume '""
        std::string string;
        while ((c = stream.get()) != '\"') {
            if (c != '\\') {
                string += c;
            } else {
                c = stream.get();
                if (c == '\\') string += '\\';
                else if (c == 'n') string += '\n';
                else if (c == 'r') string += '\r';
                else if (c == 't') string += '\t';
                else if (c == '\"') string += '\"';
            }
        }
        return JSONValue(string);
    } else if ((c >= '0' && c <= '9') || c == '-') {
        std::string string;
        bool is_int = true;

        while ((c >= '0' && c <= '9') || c == '-' || c == '.') {
            if (c == '.') {
                is_int = false;
            }

            string += c;
            stream.get();
            c = stream.peek();
        }

        return is_int ? JSONValue(std::stoll(string)) : JSONValue(std::stod(string));
    } else if (c == 'f') {
        for (int i = 0; i < 5; i++) {
            stream.get(); // Consume 'false'
        }
        return JSONValue(false);
    } else if (c == 't') {
        for (int i = 0; i < 4; i++) {
            stream.get(); // Consume 'true'
        }
        return JSONValue(true);
    } else if (c == 'n') {
        for (int i = 0; i < 4; i++) {
            stream.get(); // Consume 'null'
        }
        return JSONValue(nullptr);
    } else if (c == '{') {
        return JSONValue(parse_object());
    } else if (c == '[') {
        return JSONValue(parse_array());
    }

    return JSONValue(nullptr);
}

void JSONParser::skip_whitespace() {
    char c = stream.peek();
    while (c == ' ' || c == '\r' || c == '\n' || c == '\r') {
        stream.get();
        c = stream.peek();
    }
}

} // namespace banjo
