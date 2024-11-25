#ifndef LSP_JSON_H
#define LSP_JSON_H

#include "banjo/utils/box.hpp"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace banjo {

class JSONValue;
typedef std::string JSONString;
typedef long long JSONInt;
typedef double JSONFloat;
typedef bool JSONBool;
class JSONObject;
class JSONArray;
typedef std::nullptr_t JSONNull;

class JSONValue {

private:
    std::variant<JSONString, JSONInt, JSONFloat, JSONBool, Box<JSONObject>, Box<JSONArray>, JSONNull> value;

public:
    JSONValue(JSONString value) : value(value) {}
    JSONValue(const char *value) : value(std::string(value)) {}
    JSONValue(std::string_view value) : value(std::string(value)) {}
    JSONValue(long long value) : value(value) {}
    JSONValue(int value) : value(static_cast<JSONInt>(value)) {}
    JSONValue(unsigned value) : value(static_cast<JSONInt>(value)) {}
    JSONValue(double value) : value(value) {}
    JSONValue(JSONBool value) : value(value) {}
    JSONValue(JSONObject value);
    JSONValue(JSONArray value);
    JSONValue(std::nullptr_t value) : value(value) {}

    const JSONString &as_string() const { return std::get<JSONString>(value); }
    JSONInt as_int() const { return std::get<JSONInt>(value); }
    JSONFloat as_float() const { return std::get<JSONFloat>(value); }
    JSONBool as_bool() const { return std::get<JSONBool>(value); }
    const JSONObject &as_object() const { return *std::get<Box<JSONObject>>(value); }
    const JSONArray &as_array() const { return *std::get<Box<JSONArray>>(value); }

    bool is_string() const { return std::holds_alternative<JSONString>(value); }
    bool is_int() const { return std::holds_alternative<JSONInt>(value); }
    bool is_float() const { return std::holds_alternative<JSONFloat>(value); }
    bool is_bool() const { return std::holds_alternative<JSONBool>(value); }
    bool is_object() const { return std::holds_alternative<Box<JSONObject>>(value); }
    bool is_array() const { return std::holds_alternative<Box<JSONArray>>(value); }
    bool is_null() const { return std::holds_alternative<JSONNull>(value); }
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
    JSONInt get_int(std::string key) const;
    JSONFloat get_float(std::string key) const;
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
    JSONInt get_int(int index) const;
    JSONFloat get_float(int index) const;
    JSONBool get_bool(int index) const;
    const JSONObject &get_object(int index) const;
    const JSONArray &get_array(int index) const;

    int length() const;
    void add(JSONValue value);
};

} // namespace banjo

#endif
