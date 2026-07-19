#ifndef BANJO_CODEGEN_PROLOG_EPILOG_PASS_H
#define BANJO_CODEGEN_PROLOG_EPILOG_PASS_H

#include "banjo/codegen/machine_pass.hpp"

namespace banjo::codegen {

class PrologEpilogPass : public MachinePass {

public:
    void run(mcode::Module &mod) override;
    void run(mcode::Function &func);

private:
    void insert_prolog(mcode::Function &func);
    void insert_epilog(mcode::Function &func);
};

} // namespace banjo::codegen

#endif
