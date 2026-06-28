#include "pipeline.hpp"

#include "banjo/passes/addr_table_pass.hpp"
#include "banjo/passes/branch_elimination.hpp"
#include "banjo/passes/canonicalization_pass.hpp"
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

#include "banjo/ssa/validator.hpp"
#include "banjo/ssa/writer.hpp"
#include "banjo/utils/timing.hpp"

#include <fstream>
#include <iostream>

namespace banjo::passes {

Pipeline::Pipeline(Config config) : config{config} {}

void Pipeline::run(ssa::Module &mod) {
    std::vector<Pass *> passes = create_opt_passes();

    if (config.generate_addr_table) {
        passes.push_back(new AddrTablePass(config.target));
    }

    for (unsigned i = 0; i < passes.size(); i++) {
        run_pass(passes[i], i, mod);
    }
}

std::vector<Pass *> Pipeline::create_opt_passes() {
    target::Target *target = config.target;

    std::vector<Pass *> passes = {new DeadFuncEliminationPass(target)};

    if (config.opt_level >= 1) {
        passes.push_back(new ControlFlowOptPass(target));
        passes.push_back(new CanonicalizationPass(target));
        passes.push_back(new SROAPass(target));
        passes.push_back(new StackToRegPass(target));
    }

    if (config.opt_level >= 2) {
        passes.push_back(new LoopInversionPass(target));
    }

    if (config.opt_level >= 1) {
        passes.push_back(new PeepholeOptimizer(target));
        passes.push_back(new BranchElimination(target));
        passes.push_back(new InliningPass(target));
        passes.push_back(new DeadFuncEliminationPass(target));
        passes.push_back(new ControlFlowOptPass(target));
        passes.push_back(new CanonicalizationPass(target));
        passes.push_back(new SROAPass(target));
        passes.push_back(new StackToRegPass(target));
    }

    if (config.opt_level >= 2) {
        // passes.push_back(new CSEPass(target));
        passes.push_back(new LICMPass(target));
    }

    if (config.opt_level >= 1) {
        passes.push_back(new PeepholeOptimizer(target));
        passes.push_back(new ControlFlowOptPass(target));
        passes.push_back(new HeapToStackPass(target));
        passes.push_back(new SROAPass(target));
        passes.push_back(new StackToRegPass(target));
        // passes.push_back(new StackSlotMergePass(target));
    }

    return passes;
}

void Pipeline::run_pass(Pass *pass, unsigned index, ssa::Module &mod) {
    PROFILE_SCOPE(std::to_string(index) + "_" + pass->get_name());

    std::string number = std::to_string(index);
    std::string prefix = std::string(2 - number.size(), '0') + number;
    std::string name = "ssa.pass" + prefix + "_" + pass->get_name();

    if (config.debug) {
        std::ofstream stream{"dumps/logs." + name + ".txt"};
        pass->enable_logging(stream);
        pass->run(mod);
    } else {
        pass->run(mod);
    }

    if (config.debug) {
        std::ofstream stream{"dumps/" + name + ".bnjssa"};
        ssa::Writer(stream).write(mod);

        std::cout << "validating pass " << index << "\n";
        if (!ssa::Validator(std::cout).validate(mod)) {
            std::exit(1);
        }
    }

    delete pass;
}

} // namespace banjo::passes
