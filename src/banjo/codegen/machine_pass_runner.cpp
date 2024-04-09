#include "machine_pass_runner.hpp"

#include "codegen/prolog_epilog_pass.hpp"
#include "codegen/reg_alloc_pass.hpp"
#include "codegen/stack_frame_pass.hpp"
#include "config/config.hpp"
#include "emit/debug_emitter.hpp"

#include <fstream>

namespace codegen {

MachinePassRunner::MachinePassRunner(target::Target *target) : target(target) {}

void MachinePassRunner::create_and_run(mcode::Module &module) {
    std::vector<MachinePass *> passes;

    std::vector<MachinePass *> pre_passes = target->create_pre_passes();
    passes.insert(passes.end(), pre_passes.begin(), pre_passes.end());

    passes.push_back(new RegAllocPass(target->get_reg_analyzer()));
    passes.push_back(new StackFramePass(target->get_reg_analyzer()));
    passes.push_back(new PrologEpilogPass());

    std::vector<MachinePass *> post_passes = target->create_post_passes();
    passes.insert(passes.end(), post_passes.begin(), post_passes.end());

    run_all(passes, module);
}

void MachinePassRunner::run_all(std::vector<MachinePass *> passes, mcode::Module &module) {
    if (lang::Config::instance().is_debug()) {
        emit(module, "lir_input");
    }

    for (unsigned i = 0; i < passes.size(); i++) {
        passes[i]->run(module);
        delete passes[i];

        if (lang::Config::instance().is_debug()) {
            emit(module, "lir_pass" + std::to_string(i));
        }
    }
}

void MachinePassRunner::emit(mcode::Module &module, std::string file_name) {
    std::ofstream stream("logs/" + file_name + ".cryoasm");
    DebugEmitter(module, stream, target->get_descr()).generate();
}

} // namespace codegen
