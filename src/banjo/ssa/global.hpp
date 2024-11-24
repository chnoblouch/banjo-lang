#ifndef IR_GLOBAL_H
#define IR_GLOBAL_H

#include "banjo/ssa/global_decl.hpp"
#include "banjo/ssa/operand.hpp"

#include <optional>

namespace banjo {

namespace ssa {

class Global : public GlobalDecl {

public:
    std::optional<Value> initial_value;

public:
    Global(std::string name, Type type, std::optional<Value> initial_value)
      : GlobalDecl(name, type),
        initial_value(initial_value) {}
};

} // namespace ssa

} // namespace banjo

#endif
