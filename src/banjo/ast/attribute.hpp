#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <optional>
#include <string>

namespace lang {

class Attribute {

private:
    std::string name;
    std::optional<std::string> value;

public:
    Attribute(std::string name) : name(name), value{} {}
    Attribute(std::string name, std::string value) : name(name), value{value} {}

    std::string get_name() const { return name; }
    bool has_value() const { return value.has_value(); }
    std::string get_value() const { return value.value(); }
};

} // namespace lang

#endif
