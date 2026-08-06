#include "machine_pass_runner.hpp"

#include "banjo/config/config.hpp"
#include "banjo/emit/debug_emitter.hpp"
#include "banjo/target/target_description.hpp"

#include <fstream>
#include <memory>

namespace banjo::codegen {

MachinePassRunner::MachinePassRunner(target::Target *target) : target{target} {}

void MachinePassRunner::create_and_run(mcode::Module &mod) {
    std::vector<std::unique_ptr<MachinePass>> passes = target->create_passes();

    if (Config::instance().debug) {
        emit(mod, "mcode.input");
    }

    for (unsigned i = 0; i < passes.size(); i++) {
        passes[i]->run(mod);

        if (Config::instance().debug) {
            emit(mod, "mcode.pass" + std::to_string(i));
        }
    }
}

void MachinePassRunner::emit(mcode::Module &mod, const std::string &file_name) {
    std::ofstream stream("dumps/" + file_name + ".bnjasm");
    DebugEmitter(mod, stream, target->get_descr()).generate();
}

} // namespace banjo::codegen
