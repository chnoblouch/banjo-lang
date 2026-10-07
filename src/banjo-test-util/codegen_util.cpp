#include "codegen_util.hpp"

#include "banjo/codegen/machine_pass_runner.hpp"
#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/mcode/printer.hpp"
#include "banjo/ssa/ssa_parser.hpp"
#include "banjo/target/target.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/macros.hpp"

#include <iostream>
#include <memory>
#include <string_view>

namespace banjo::test {

void CodegenUtil::run(target::Architecture arch, std::string_view source) {
    target::TargetDescription target_descr(arch, target::OperatingSystem::LINUX, target::Environment::GNU);
    target::Target *target = target::Target::create(target_descr, target::CodeModel::LARGE);
    codegen::SSALowerer *ssa_lowerer = target->create_ssa_lowerer();

    utils::TokenStream tokens = utils::GenericLexer{source}.tokenize();
    ssa::Module ssa_mod = ssa::Parser{tokens, target->get_default_calling_conv()}.parse();

    mcode::Module mcode_mod = ssa_lowerer->lower_module(ssa_mod);
    codegen::MachinePassRunner{target}.create_and_run(mcode_mod);

    std::string buffer;

    std::unique_ptr<mcode::Printer> printer = target->create_printer();
    printer->set_buffer(buffer);
    printer->set_flags(mcode::Printer::NO_ATTRIBUTES);
    printer->print(mcode_mod);

    std::cout << buffer;

    delete ssa_lowerer;
    delete target;
}

} // namespace banjo::test
