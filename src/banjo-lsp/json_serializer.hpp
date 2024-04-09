#ifndef LSP_JSON_SERIALIZER_H
#define LSP_JSON_SERIALIZER_H

#include "json.hpp"
#include <ostream>

namespace lsp {

class JSONSerializer {

private:
    std::ostream &stream;

public:
    JSONSerializer(std::ostream &stream);
    void serialize(const JSONObject &object);
    void serialize(const JSONArray &array);

private:
    void serialize(const JSONObject &object, int indent);
    void serialize(const JSONArray &array, int indent);
    void serialize(const JSONValue &value, int indent);
};

} // namespace lsp

#endif
