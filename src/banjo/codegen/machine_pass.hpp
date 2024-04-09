#ifndef CODEGEN_MACHINE_PASS_H
#define CODEGEN_MACHINE_PASS_H

#include "mcode/module.hpp"

namespace codegen {

class MachinePass {

public:
    virtual ~MachinePass() = default;
    virtual void run(mcode::Module &module_) = 0;
};

} // namespace codegen

#endif
