#include "machine_pass_runner.hpp"

#include "banjo/codegen/prolog_epilog_pass.hpp"
#include "banjo/codegen/reg_alloc_pass.hpp"
#include "banjo/codegen/stack_frame_pass.hpp"
#include "banjo/config/config.hpp"
#include "banjo/emit/debug_emitter.hpp"

#include <fstream>

namespace banjo {

namespace codegen {

MachinePassRunner::MachinePassRunner(target::Target *target) : target(target) {}

void MachinePassRunner::create_and_run(mcode::Module &module) {
    if (target->get_descr().get_architecture() == target::Architecture::WASM) {
        run_all({}, module);
        return;
    }

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
    if (lang::Config::instance().debug) {
        emit(module, "mcode.input");
    }

    for (unsigned i = 0; i < passes.size(); i++) {
        passes[i]->run(module);
        delete passes[i];

        if (lang::Config::instance().debug) {
            emit(module, "mcode.pass" + std::to_string(i));
        }
    }
}

void MachinePassRunner::emit(mcode::Module &module, std::string file_name) {
    std::ofstream stream("logs/" + file_name + ".cryoasm");
    DebugEmitter(module, stream, target->get_descr()).generate();
}

} // namespace codegen

} // namespace banjo
