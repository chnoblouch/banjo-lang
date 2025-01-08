#ifndef X86_64_PEEPHOLE_OPT_PASS_H
#define X86_64_PEEPHOLE_OPT_PASS_H

#include "banjo/codegen/machine_pass.hpp"

namespace banjo {

namespace target {

class X8664PeepholeOptPass : public codegen::MachinePass {

public:
    void run(mcode::Module &module_);
    void run(mcode::Function *func);
};

} // namespace target

} // namespace banjo

#endif
