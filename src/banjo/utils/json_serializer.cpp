#include "json_serializer.hpp"

namespace banjo {

JSONSerializer::JSONSerializer(std::ostream &stream) : stream(stream) {}

void JSONSerializer::serialize(const JSONObject &object) {
    serialize(object, 0);
}

void JSONSerializer::serialize(const JSONArray &array) {
    serialize(array, 0);
}

void JSONSerializer::serialize(const JSONObject &object, int indent) {
    stream << "{\n";

    for (auto it = object.values.begin(); it != object.values.end(); it++) {
        stream << std::string(2 * (indent + 1), ' ');
        stream << "\"" << it->first << "\": ";
        serialize(it->second, indent);

        if (std::next(it) != object.values.end()) {
            stream << ", ";
        }
        stream << "\n";
    }

    stream << std::string(2 * indent, ' ') << "}";
}

void JSONSerializer::serialize(const JSONArray &array, int indent) {
    stream << "[\n";

    for (auto it = array.values.begin(); it != array.values.end(); it++) {
        stream << std::string(2 * (indent + 1), ' ');
        serialize(*it, indent);

        if (std::next(it) != array.values.end()) {
            stream << ",";
        }
        stream << "\n";
    }

    stream << std::string(2 * indent, ' ') + "]";
}

void JSONSerializer::serialize(const JSONValue &value, int indent) {
    if (value.is_string()) {
        std::string escaped_string;
        for (char c : value.as_string()) {
            if (c == '\"') escaped_string += "\\\"";
            else if (c == '\\') escaped_string += "\\\\";
            else if (c == '/') escaped_string += "\\/";
            else if (c == '\b') escaped_string += "\\b";
            else if (c == '\f') escaped_string += "\\f";
            else if (c == '\n') escaped_string += "\\n";
            else if (c == '\r') escaped_string += "\\r";
            else if (c == '\t') escaped_string += "\\t";
            else escaped_string += c;
        }

        stream << "\"" << escaped_string << "\"";
    } else if (value.is_number()) stream << value.as_number();
    else if (value.is_bool()) stream << (value.as_bool() ? "true" : "false");
    else if (value.is_object()) serialize(value.as_object(), indent + 1);
    else if (value.is_array()) serialize(value.as_array(), indent + 1);
    else if (value.is_null()) stream << "null";
}

} // namespace banjo
