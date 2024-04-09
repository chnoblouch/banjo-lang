#ifndef IR_VALIDATOR_H
#define IR_VALIDATOR_H

#include "ir/module.hpp"
#include <ostream>

namespace ir {

class Validator {

private:
    std::ostream &stream;

public:
    Validator(std::ostream &stream);
    bool validate(ir::Module &mod);
    bool validate(ir::Module &mod, ir::Function &func);

private:
    bool validate_memberptr(ir::Module &mod, ir::Instruction &instr);
};

} // namespace ir

#endif
