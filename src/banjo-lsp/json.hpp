#ifndef LSP_JSON_H
#define LSP_JSON_H

#include "banjo/utils/box.hpp"

#include <initializer_list>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace banjo {

namespace lsp {

class JSONValue;
typedef std::string JSONString;
typedef double JSONNumber;
typedef bool JSONBool;
class JSONObject;
class JSONArray;

enum JSONNull { JSON_NULL };

class JSONValue {

private:
    std::variant<JSONString, JSONNumber, JSONBool, Box<JSONObject>, Box<JSONArray>, JSONNull> value;

public:
    JSONValue(JSONString value) : value(value) {}
    JSONValue(const char *value) : value(std::string(value)) {}
    JSONValue(JSONNumber value) : value(value) {}
    JSONValue(int value) : value((double)value) {}
    JSONValue(JSONBool value) : value(value) {}
    JSONValue(JSONObject value);
    JSONValue(JSONArray value);
    JSONValue(JSONNull value) : value(value) {}

    const JSONString &as_string() const { return std::get<0>(value); }
    JSONNumber as_number() const { return std::get<1>(value); }
    JSONBool as_bool() const { return std::get<2>(value); }
    const JSONObject &as_object() const { return *std::get<3>(value); }
    const JSONArray &as_array() const { return *std::get<4>(value); }

    bool is_string() const { return value.index() == 0; }
    bool is_number() const { return value.index() == 1; }
    bool is_bool() const { return value.index() == 2; }
    bool is_object() const { return value.index() == 3; }
    bool is_array() const { return value.index() == 4; }
    bool is_null() const { return value.index() == 5; }
};

class JSONObject {

    friend class JSONSerializer;

private:
    std::unordered_map<std::string, JSONValue> values;

public:
    JSONObject();
    JSONObject(std::initializer_list<std::pair<std::string, JSONValue>> values);

    const JSONValue &get(std::string key) const;
    const JSONString &get_string(std::string key) const;
    JSONNumber get_number(std::string key) const;
    JSONBool get_bool(std::string key) const;
    const JSONObject &get_object(std::string key) const;
    const JSONArray &get_array(std::string key) const;

    bool contains(std::string key) const;
    void add(std::string key, JSONValue value);
};

class JSONArray {

    friend class JSONSerializer;

private:
    std::vector<JSONValue> values;

public:
    JSONArray();
    JSONArray(std::initializer_list<JSONValue> values);

    const JSONValue &get(int index) const;
    const JSONString &get_string(int index) const;
    JSONNumber get_number(int index) const;
    JSONBool get_bool(int index) const;
    const JSONObject &get_object(int index) const;
    const JSONArray &get_array(int index) const;

    int length() const;
    void add(JSONValue value);
};

} // namespace lsp

} // namespace banjo

#endif
