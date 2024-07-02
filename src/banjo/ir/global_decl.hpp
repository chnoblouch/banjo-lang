#ifndef IR_GLOBAL_DECL_H
#define IR_GLOBAL_DECL_H

#include "ir/type.hpp"
#include <string>

namespace banjo {

namespace ir {

class GlobalDecl {

private:
    std::string name;
    Type type;
    bool external = false;

public:
    GlobalDecl(std::string name, Type type) : name(name), type(type) {}

    const std::string &get_name() const { return name; }
    Type get_type() const { return type; }
    bool is_external() const { return external; }

    void set_external(bool external) { this->external = external; }
};

} // namespace ir

} // namespace banjo

#endif
