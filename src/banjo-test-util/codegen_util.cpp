#include "codegen_util.hpp"

#include "banjo/codegen/machine_pass_runner.hpp"
#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/target/target.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/utils/macros.hpp"

#include "ssa_parser.hpp"

#include <iostream>

namespace banjo::test {

void CodegenUtil::lower(target::Architecture arch) {
    target::TargetDescription target_descr(arch, target::OperatingSystem::LINUX, target::Environment::GNU);
    target::Target *target = target::Target::create(target_descr, target::CodeModel::LARGE);
    codegen::SSALowerer *ssa_lowerer = target->create_ssa_lowerer();

    ssa::Module ssa_mod = SSAParser{target->get_default_calling_conv()}.parse();
    mcode::Module mcode_mod = ssa_lowerer->lower_module(ssa_mod);
    codegen::MachinePassRunner{target}.create_and_run(mcode_mod);

    std::string buffer;
    target->create_printer()->set_buffer(buffer).print(mcode_mod);
    std::cout << buffer;

    delete ssa_lowerer;
    delete target;
}

} // namespace banjo::test
