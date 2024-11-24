#ifndef IR_GLOBAL_DECL_H
#define IR_GLOBAL_DECL_H

#include "banjo/ssa/type.hpp"
#include <string>

namespace banjo {

namespace ssa {

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

    void set_type(Type type) { this->type = type; }
    void set_external(bool external) { this->external = external; }
};

} // namespace ssa

} // namespace banjo

#endif
