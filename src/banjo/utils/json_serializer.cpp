#include "json_serializer.hpp"

#include "banjo/utils/json.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::json {

Serializer::Serializer(std::ostream &stream) : stream{stream} {}

void Serializer::serialize(const Value &value) {
    if (value.is_null()) {
        serialize_null();
    } else if (value.is_bool()) {
        serialize_bool(value.as_bool());
    } else if (value.is_int()) {
        serialize_int(value.as_int());
    } else if (value.is_float()) {
        serialize_float(value.as_float());
    } else if (value.is_string()) {
        serialize_string(value.as_string());
    } else if (value.is_array()) {
        serialize_array(value.as_array());
    } else if (value.is_object()) {
        serialize_object(value.as_object());
    } else {
        ASSERT_UNREACHABLE;
    }
}

void Serializer::serialize_null() {
    stream << "null";
}

void Serializer::serialize_bool(bool value) {
    stream << (value ? "true" : "false");
}

void Serializer::serialize_int(long long value) {
    stream << value;
}

void Serializer::serialize_float(double value) {
    stream << value;
}

void Serializer::serialize_string(const std::string &value) {
    stream << '\"';

    for (char c : value) {
        if (c == '\"') {
            stream << "\\\"";
        } else if (c == '\\') {
            stream << "\\\\";
        } else if (c == '\b') {
            stream << "\\b";
        } else if (c == '\f') {
            stream << "\\f";
        } else if (c == '\n') {
            stream << "\\n";
        } else if (c == '\r') {
            stream << "\\r";
        } else if (c == '\t') {
            stream << "\\t";
        } else {
            stream << c;
        }
    }

    stream << '\"';
}

void Serializer::serialize_array(const Array &array) {
    stream << "[";

    if (array.length() == 0) {
        stream << "]";
        return;
    } else {
        stream << "\n";
    }

    indent += 1;

    for (unsigned i = 0; i < array.length(); i++) {
        emit_indent();
        serialize(array.get(i));

        if (i != array.length() - 1) {
            stream << ",";
        }

        stream << "\n";
    }

    indent -= 1;

    emit_indent();
    stream << "]";
}

void Serializer::serialize_object(const Object &object) {
    stream << "{";

    if (object.length() == 0) {
        stream << "}";
        return;
    } else {
        stream << "\n";
    }

    indent += 1;
    unsigned index = 0;

    for (const auto &[key, value] : object) {
        emit_indent();
        serialize(key);
        stream << ": ";
        serialize(value);

        if (index != object.length() - 1) {
            stream << ",";
        }

        stream << "\n";
        index += 1;
    }

    indent -= 1;

    emit_indent();
    stream << "}";
}

void Serializer::emit_indent() {
    stream << std::string(2 * indent, ' ');
}

} // namespace banjo::json
