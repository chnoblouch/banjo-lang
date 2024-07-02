#ifndef CODEGEN_MACHINE_PASS_H
#define CODEGEN_MACHINE_PASS_H

#include "banjo/mcode/module.hpp"

namespace banjo {

namespace codegen {

class MachinePass {

public:
    virtual ~MachinePass() = default;
    virtual void run(mcode::Module &module_) = 0;
};

} // namespace codegen

} // namespace banjo

#endif
