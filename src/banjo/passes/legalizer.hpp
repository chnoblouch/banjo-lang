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
};

} // namespace banjo::passes

#endif
