#ifndef MCODE_GLOBAL_H
#define MCODE_GLOBAL_H

#include "mcode/operand.hpp"
#include <vector>

namespace mcode {

class Global {

private:
    std::string name;
    Value value;

public:
    Global() {}
    Global(std::string name, Value value) : name(name), value(value) {}

    std::string get_name() const { return name; }
    Value get_value() const { return value; }

    friend bool operator==(const Global &lhs, const Global &rhs) {
        return lhs.name == rhs.name && lhs.value == rhs.value;
    }

    friend bool operator!=(const Global &lhs, const Global &rhs) { return !(lhs == rhs); }
};

} // namespace mcode

#endif
