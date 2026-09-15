#ifndef BANJO_PASSESS_LEGALIZER_H
#define BANJO_PASSESS_LEGALIZER_H

#include "banjo/passes/pass.hpp"

namespace banjo::passes {

class Legalizer : public Pass {

public:
    Legalizer(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void run(ssa::Function &func, ssa::BasicBlockIter block);

    void legalize_load(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr);
    void legalize_store(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr);
    void legalize_loadarg(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr);
    void legalize_cjmp(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr);
};

} // namespace banjo::passes

#endif
