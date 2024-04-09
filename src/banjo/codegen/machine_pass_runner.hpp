#ifndef CODEGEN_MACHINE_PASS_RUNNER_H
#define CODEGEN_MACHINE_PASS_RUNNER_H

#include "codegen/machine_pass.hpp"
#include "mcode/module.hpp"
#include "target/target.hpp"

#include <string>
#include <vector>

namespace codegen {

class MachinePassRunner {

private:
    target::Target *target;

public:
    MachinePassRunner(target::Target *target);
    void create_and_run(mcode::Module &module);

private:
    void run_all(std::vector<MachinePass *> passes, mcode::Module &module);
    void emit(mcode::Module &module, std::string file_name);
};

} // namespace codegen

#endif
