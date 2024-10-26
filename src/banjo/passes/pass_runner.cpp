#include "pass_runner.hpp"

#include "banjo/passes/addr_table_pass.hpp"
#include "banjo/passes/branch_elimination.hpp"
#include "banjo/passes/control_flow_opt_pass.hpp"
#include "banjo/passes/cse_pass.hpp"
#include "banjo/passes/dead_func_elimination_pass.hpp"
#include "banjo/passes/heap_to_stack_pass.hpp"
#include "banjo/passes/inlining_pass.hpp"
#include "banjo/passes/licm_pass.hpp"
#include "banjo/passes/loop_inversion_pass.hpp"
#include "banjo/passes/peephole_optimizer.hpp"
#include "banjo/passes/sroa_pass.hpp"
#include "banjo/passes/stack_slot_merge_pass.hpp"
#include "banjo/passes/stack_to_reg_pass.hpp"


#include "banjo/ir/validator.hpp"
#include "banjo/ir/writer.hpp"
#include "banjo/utils/timing.hpp"

#include <iostream>

namespace banjo {

namespace passes {

void PassRunner::run(ir::Module &mod, target::Target *target) {
    std::vector<Pass *> passes = create_opt_passes(target);

    if (is_generate_addr_table) {
        passes.push_back(new AddrTablePass(target));
    }

    for (unsigned i = 0; i < passes.size(); i++) {
        run_pass(passes[i], i, mod);
    }
}

std::vector<Pass *> PassRunner::create_opt_passes(target::Target *target) {
    std::vector<Pass *> passes = {new DeadFuncEliminationPass(target)};

    if (opt_level >= 1) {
        passes.push_back(new ControlFlowOptPass(target));
        passes.push_back(new SROAPass(target));
        passes.push_back(new StackToRegPass(target));
    }

    if (opt_level >= 2) {
        passes.push_back(new LoopInversionPass(target));
    }

    if (opt_level >= 1) {
        passes.push_back(new PeepholeOptimizer(target));
        passes.push_back(new BranchElimination(target));
        passes.push_back(new InliningPass(target));
        passes.push_back(new DeadFuncEliminationPass(target));
        passes.push_back(new ControlFlowOptPass(target));
        passes.push_back(new SROAPass(target));
        passes.push_back(new StackToRegPass(target));
    }

    if (opt_level >= 2) {
        // passes.push_back(new CSEPass(target));
        passes.push_back(new LICMPass(target));
    }

    if (opt_level >= 1) {
        passes.push_back(new PeepholeOptimizer(target));
        passes.push_back(new HeapToStackPass(target));
        // passes.push_back(new StackSlotMergePass(target));
        passes.push_back(new ControlFlowOptPass(target));
    }

    return passes;
}

void PassRunner::run_pass(Pass *pass, unsigned index, ir::Module &mod) {
    PROFILE_SCOPE(std::to_string(index) + "_" + pass->get_name());
    pass->run(mod);

    if (is_debug) {
        std::string number = std::to_string(index);
        std::string prefix = std::string(2 - number.size(), '0') + number;
        std::string file_name = "ssa.pass" + prefix + "_" + pass->get_name() + ".cryoir";
        std::string path = "logs/" + file_name;
        std::ofstream stream(path);
        ir::Writer(stream).write(mod);

        std::cout << "validating pass " << index << std::endl;
        if (!ir::Validator(std::cout).validate(mod)) {
            std::exit(1);
        }
    }

    delete pass;
}

} // namespace passes

} // namespace banjo
