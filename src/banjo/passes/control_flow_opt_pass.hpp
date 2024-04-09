#ifndef PASSES_CONTROL_FLOW_OPT_PASS_H
#define PASSES_CONTROL_FLOW_OPT_PASS_H

#include "ir/control_flow_graph.hpp"
#include "passes/pass.hpp"

namespace passes {

class ControlFlowOptPass : public Pass {

public:
    ControlFlowOptPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
    void optimize_blocks(ir::Function &func);
};

} // namespace passes

#endif
