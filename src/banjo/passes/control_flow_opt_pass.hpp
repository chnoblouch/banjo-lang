#ifndef PASSES_CONTROL_FLOW_OPT_PASS_H
#define PASSES_CONTROL_FLOW_OPT_PASS_H

#include "banjo/ir/control_flow_graph.hpp"
#include "banjo/passes/pass.hpp"

namespace banjo {

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

} // namespace banjo

#endif
