#ifndef BANJO_UTILS_JSON_SERIALIZER_H
#define BANJO_UTILS_JSON_SERIALIZER_H

#include "banjo/utils/json.hpp"

#include <ostream>

namespace banjo::json {

class Serializer {

private:
    std::ostream &stream;
    unsigned indent = 0;

public:
    Serializer(std::ostream &stream);
    void serialize(const Value &value);

private:
    void serialize_null();
    void serialize_bool(bool value);
    void serialize_int(long long value);
    void serialize_float(double value);
    void serialize_string(const std::string &value);
    void serialize_array(const Array &value);
    void serialize_object(const Object &object);

    void emit_indent();
};

} // namespace banjo::json

#endif
