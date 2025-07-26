#ifndef BANJO_CODEGEN_PROLOG_EPILOG_PASS_H
#define BANJO_CODEGEN_PROLOG_EPILOG_PASS_H

#include "banjo/codegen/machine_pass.hpp"

namespace banjo {

namespace codegen {

class PrologEpilogPass : public MachinePass {

public:
    void run(mcode::Module &module_);
    void run(mcode::Function *func);

private:
    void insert_prolog(mcode::Function *func, std::vector<long> &modified_volatile_regs);
    void insert_epilog(mcode::Function *func, std::vector<long> &modified_volatile_regs);
};

} // namespace codegen

} // namespace banjo

#endif
