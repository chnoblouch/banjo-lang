#include "json.hpp"

namespace banjo {

JSONValue::JSONValue(JSONObject value) : value(value) {}

JSONValue::JSONValue(JSONArray value) : value(value) {}

JSONObject::JSONObject() {}

JSONObject::JSONObject(std::initializer_list<std::pair<std::string, JSONValue>> values) {
    for (const auto &value : values) {
        add(value.first, value.second);
    }
}

const JSONValue &JSONObject::get(std::string key) const {
    return values.at(key);
}

const JSONString &JSONObject::get_string(std::string key) const {
    return get(key).as_string();
}

JSONInt JSONObject::get_int(std::string key) const {
    return get(key).as_int();
}

JSONFloat JSONObject::get_float(std::string key) const {
    return get(key).as_float();
}

JSONBool JSONObject::get_bool(std::string key) const {
    return get(key).as_bool();
}

const JSONObject &JSONObject::get_object(std::string key) const {
    return get(key).as_object();
}

const JSONArray &JSONObject::get_array(std::string key) const {
    return get(key).as_array();
}

const JSONValue *JSONObject::try_get(const std::string &key) const {
    auto iter = values.find(key);
    return iter == values.end() ? nullptr : &iter->second;
}

const JSONString *JSONObject::try_get_string(const std::string &key) const {
    const JSONValue *value = try_get(key);
    return value ? &value->as_string() : nullptr;
}

std::optional<JSONInt> JSONObject::try_get_int(const std::string &key) const {
    const JSONValue *value = try_get(key);
    return value ? value->as_int() : std::optional<JSONInt>{};
}

std::optional<JSONFloat> JSONObject::try_get_float(const std::string &key) const {
    const JSONValue *value = try_get(key);
    return value ? value->as_float() : std::optional<JSONFloat>{};
}

std::optional<JSONBool> JSONObject::try_get_bool(const std::string &key) const {
    const JSONValue *value = try_get(key);
    return value ? value->as_bool() : std::optional<JSONBool>{};
}

const JSONObject *JSONObject::try_get_object(const std::string &key) const {
    const JSONValue *value = try_get(key);
    return value ? &value->as_object() : nullptr;
}

const JSONArray *JSONObject::try_get_array(const std::string &key) const {
    const JSONValue *value = try_get(key);
    return value ? &value->as_array() : nullptr;
}

std::vector<std::string> JSONObject::get_string_array(const std::string &key) const {
    const JSONArray &json_array = get_array(key);

    std::vector<std::string> array(json_array.length());

    for (unsigned i = 0; i < json_array.length(); i++) {
        array[i] = json_array.get_string(i);
    }

    return array;
}

std::string JSONObject::get_string_or(const std::string &key, const std::string &default_value) const {
    const std::string *string = try_get_string(key);
    return string ? *string : default_value;
}

bool JSONObject::contains(std::string key) const {
    return values.count(key);
}

void JSONObject::add(std::string key, JSONValue value) {
    values.insert({key, value});
}

JSONArray::JSONArray() {}

JSONArray::JSONArray(std::initializer_list<JSONValue> values) {
    for (const auto &value : values) {
        add(value);
    }
}

const JSONValue &JSONArray::get(int index) const {
    return values.at(index);
}

const JSONString &JSONArray::get_string(int index) const {
    return get(index).as_string();
}

JSONInt JSONArray::get_int(int index) const {
    return get(index).as_int();
}

JSONFloat JSONArray::get_float(int index) const {
    return get(index).as_float();
}

JSONBool JSONArray::get_bool(int index) const {
    return get(index).as_bool();
}

const JSONObject &JSONArray::get_object(int index) const {
    return get(index).as_object();
}

const JSONArray &JSONArray::get_array(int index) const {
    return get(index).as_array();
}

int JSONArray::length() const {
    return values.size();
}

void JSONArray::add(JSONValue value) {
    values.push_back(value);
}

} // namespace banjo
