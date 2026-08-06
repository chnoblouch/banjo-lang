#ifndef BANJO_CODEGEN_MACHINE_PASS_RUNNER_H
#define BANJO_CODEGEN_MACHINE_PASS_RUNNER_H

#include "banjo/mcode/module.hpp"
#include "banjo/target/target.hpp"

#include <string>

namespace banjo::codegen {

class MachinePassRunner {

private:
    target::Target *target;

public:
    MachinePassRunner(target::Target *target);
    void create_and_run(mcode::Module &mod);

private:
    void emit(mcode::Module &mod, const std::string &file_name);
};

} // namespace banjo::codegen

#endif
