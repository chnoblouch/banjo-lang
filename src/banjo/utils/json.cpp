#include "json.hpp"

namespace banjo::json {

Value::Value(Object value) : value(value) {}

Value::Value(Array value) : value(value) {}

Object::Object() {}

Object::Object(std::initializer_list<std::pair<std::string, Value>> values) {
    for (const auto &value : values) {
        add(value.first, value.second);
    }
}

const Value &Object::get(const std::string &key) const {
    return values.at(key);
}

const String &Object::get_string(const std::string &key) const {
    return get(key).as_string();
}

Int Object::get_int(const std::string &key) const {
    return get(key).as_int();
}

Float Object::get_float(const std::string &key) const {
    return get(key).as_float();
}

Bool Object::get_bool(const std::string &key) const {
    return get(key).as_bool();
}

const Object &Object::get_object(const std::string &key) const {
    return get(key).as_object();
}

const Array &Object::get_array(const std::string &key) const {
    return get(key).as_array();
}

const Value *Object::try_get(const std::string &key) const {
    auto iter = values.find(key);
    return iter == values.end() ? nullptr : &iter->second;
}

const String *Object::try_get_string(const std::string &key) const {
    const Value *value = try_get(key);
    return value ? &value->as_string() : nullptr;
}

std::optional<Int> Object::try_get_int(const std::string &key) const {
    const Value *value = try_get(key);
    return value ? value->as_int() : std::optional<Int>{};
}

std::optional<Float> Object::try_get_float(const std::string &key) const {
    const Value *value = try_get(key);
    return value ? value->as_float() : std::optional<Float>{};
}

std::optional<Bool> Object::try_get_bool(const std::string &key) const {
    const Value *value = try_get(key);
    return value ? value->as_bool() : std::optional<Bool>{};
}

const Object *Object::try_get_object(const std::string &key) const {
    const Value *value = try_get(key);
    return value ? &value->as_object() : nullptr;
}

const Array *Object::try_get_array(const std::string &key) const {
    const Value *value = try_get(key);
    return value ? &value->as_array() : nullptr;
}

std::vector<std::string> Object::get_string_array(const std::string &key) const {
    const Array &json_array = get_array(key);

    std::vector<std::string> array(json_array.length());

    for (unsigned i = 0; i < json_array.length(); i++) {
        array[i] = json_array.get_string(i);
    }

    return array;
}

std::string Object::get_string_or(const std::string &key, const std::string &default_value) const {
    const std::string *string = try_get_string(key);
    return string ? *string : default_value;
}

Array::Array() {}

Array::Array(std::initializer_list<Value> values) {
    for (const auto &value : values) {
        add(value);
    }
}

const Value &Array::get(unsigned index) const {
    return values.at(index);
}

const String &Array::get_string(unsigned index) const {
    return get(index).as_string();
}

Int Array::get_int(unsigned index) const {
    return get(index).as_int();
}

Float Array::get_float(unsigned index) const {
    return get(index).as_float();
}

Bool Array::get_bool(unsigned index) const {
    return get(index).as_bool();
}

const Object &Array::get_object(unsigned index) const {
    return get(index).as_object();
}

const Array &Array::get_array(unsigned index) const {
    return get(index).as_array();
}

} // namespace banjo::json
