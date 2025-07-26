#ifndef BANJO_PASSES_CONTROL_FLOW_OPT_PASS_H
#define BANJO_PASSES_CONTROL_FLOW_OPT_PASS_H

#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/passes/pass.hpp"

namespace banjo {

namespace passes {

class ControlFlowOptPass : public Pass {

public:
    ControlFlowOptPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    void optimize_blocks(ssa::Function &func);
};

} // namespace passes

} // namespace banjo

#endif
