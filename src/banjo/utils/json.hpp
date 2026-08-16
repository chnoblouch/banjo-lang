#ifndef BANJO_UTILS_JSON_H
#define BANJO_UTILS_JSON_H

#include "banjo/utils/box.hpp"

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace banjo::json {

class Value;
typedef std::string String;
typedef long long Int;
typedef double Float;
typedef bool Bool;
class Object;
class Array;
typedef std::nullptr_t Null;

class Value {

private:
    std::variant<String, Int, Float, Bool, Box<Object>, Box<Array>, Null> value;

public:
    Value(String value) : value(value) {}
    Value(const char *value) : value(std::string(value)) {}
    Value(std::string_view value) : value(std::string(value)) {}
    Value(long long value) : value(value) {}
    Value(int value) : value(static_cast<Int>(value)) {}
    Value(unsigned value) : value(static_cast<Int>(value)) {}
    Value(double value) : value(value) {}
    Value(Bool value) : value(value) {}
    Value(Object value);
    Value(Array value);
    Value(std::nullptr_t value) : value(value) {}

    const String &as_string() const { return std::get<String>(value); }
    Int as_int() const { return std::get<Int>(value); }
    Float as_float() const { return std::get<Float>(value); }
    Bool as_bool() const { return std::get<Bool>(value); }
    const Object &as_object() const { return *std::get<Box<Object>>(value); }
    const Array &as_array() const { return *std::get<Box<Array>>(value); }

    bool is_string() const { return std::holds_alternative<String>(value); }
    bool is_int() const { return std::holds_alternative<Int>(value); }
    bool is_float() const { return std::holds_alternative<Float>(value); }
    bool is_bool() const { return std::holds_alternative<Bool>(value); }
    bool is_object() const { return std::holds_alternative<Box<Object>>(value); }
    bool is_array() const { return std::holds_alternative<Box<Array>>(value); }
    bool is_null() const { return std::holds_alternative<Null>(value); }
};

class Object {

private:
    std::unordered_map<std::string, Value> values;

public:
    Object();
    Object(std::initializer_list<std::pair<std::string, Value>> values);

    const Value &get(const std::string &key) const;
    const String &get_string(const std::string &key) const;
    Int get_int(const std::string &key) const;
    Float get_float(const std::string &key) const;
    Bool get_bool(const std::string &key) const;
    const Object &get_object(const std::string &key) const;
    const Array &get_array(const std::string &key) const;

    const Value *try_get(const std::string &key) const;
    const String *try_get_string(const std::string &key) const;
    std::optional<Int> try_get_int(const std::string &key) const;
    std::optional<Float> try_get_float(const std::string &key) const;
    std::optional<Bool> try_get_bool(const std::string &key) const;
    const Object *try_get_object(const std::string &key) const;
    const Array *try_get_array(const std::string &key) const;

    std::vector<std::string> get_string_array(const std::string &key) const;
    std::string get_string_or(const std::string &key, const std::string &default_value) const;

    unsigned length() const { return values.size(); }
    bool contains(const std::string &key) const { return values.count(key); }
    void add(std::string key, Value value) { values.emplace(std::move(key), std::move(value)); }

    std::unordered_map<std::string, Value>::const_iterator begin() const { return values.begin(); }
    std::unordered_map<std::string, Value>::const_iterator end() const { return values.end(); }
};

class Array {

private:
    std::vector<Value> values;

public:
    Array();
    Array(std::initializer_list<Value> values);

    template <typename T>
    Array(std::vector<T> values) {
        this->values.reserve(values.size());

        for (T value : values) {
            add(std::move(value));
        }
    }

    const Value &get(unsigned index) const;
    const String &get_string(unsigned index) const;
    Int get_int(unsigned index) const;
    Float get_float(unsigned index) const;
    Bool get_bool(unsigned index) const;
    const Object &get_object(unsigned index) const;
    const Array &get_array(unsigned index) const;

    unsigned length() const { return values.size(); }
    void add(Value value) { values.push_back(std::move(value)); }

    std::vector<Value>::const_iterator begin() const { return values.begin(); }
    std::vector<Value>::const_iterator end() const { return values.end(); }
};

} // namespace banjo::json

#endif
