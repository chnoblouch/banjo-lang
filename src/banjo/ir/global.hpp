#ifndef IR_GLOBAL_H
#define IR_GLOBAL_H

#include "banjo/ir/global_decl.hpp"
#include "banjo/ir/operand.hpp"

#include <optional>

namespace banjo {

namespace ir {

class Global : public GlobalDecl {

public:
    std::optional<Value> initial_value;

public:
    Global(std::string name, Type type, std::optional<Value> initial_value)
      : GlobalDecl(name, type),
        initial_value(initial_value) {}
};

} // namespace ir

} // namespace banjo

#endif
