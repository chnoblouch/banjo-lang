#ifndef IR_VALIDATOR_H
#define IR_VALIDATOR_H

#include "banjo/ir/module.hpp"
#include <ostream>

namespace banjo {

namespace ir {

class Validator {

private:
    std::ostream &stream;

public:
    Validator(std::ostream &stream);
    bool validate(ir::Module &mod);
    bool validate(ir::Module &mod, ir::Function &func);

private:
    bool validate_memberptr(ir::Instruction &instr);
};

} // namespace ir

} // namespace banjo

#endif
