#ifndef BANJO_SSA_VALIDATOR_H
#define BANJO_SSA_VALIDATOR_H

#include "banjo/ssa/module.hpp"
#include <ostream>

namespace banjo {

namespace ssa {

class Validator {

private:
    std::ostream &stream;

public:
    Validator(std::ostream &stream);
    bool validate(ssa::Module &mod);
    bool validate(ssa::Module &mod, ssa::Function &func);

private:
    bool validate_memberptr(ssa::Instruction &instr);
};

} // namespace ssa

} // namespace banjo

#endif
