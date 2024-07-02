#ifndef IR_GLOBAL_H
#define IR_GLOBAL_H

#include "banjo/ir/global_decl.hpp"
#include "banjo/ir/operand.hpp"

namespace banjo {

namespace ir {

class Global : public GlobalDecl {

private:
    Value initial_value;

public:
    Global(std::string name, Type type, Value initial_value) : GlobalDecl(name, type), initial_value(initial_value) {}
    Value &get_initial_value() { return initial_value; }
};

} // namespace ir

} // namespace banjo

#endif
