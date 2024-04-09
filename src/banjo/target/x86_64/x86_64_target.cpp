#include "x86_64_target.hpp"

#include "config/config.hpp"
#include "emit/elf/elf_emitter.hpp"
#include "emit/nasm_emitter.hpp"
#include "emit/pe/pe_emitter.hpp"
#include "target/target_description.hpp"
#include "target/x86_64/x86_64_ir_lowerer.hpp"
#include "target/x86_64/x86_64_peephole_opt_pass.hpp"

namespace target {

X8664Target::X8664Target(TargetDescription descr, CodeModel code_model) : Target(descr, code_model) {}

codegen::IRLowerer *X8664Target::create_ir_lowerer() {
    return new X8664IRLowerer(this);
}

std::vector<codegen::MachinePass *> X8664Target::create_pre_passes() {
    return {};
}

std::vector<codegen::MachinePass *> X8664Target::create_post_passes() {
    return {new X8664PeepholeOptPass()};
}

std::string X8664Target::get_output_file_ext() {
    if (lang::Config::instance().is_force_asm()) {
        return "asm";
    }

    if (descr.get_operating_system() == OperatingSystem::WINDOWS) {
        return descr.get_environment() == Environment::MSVC ? "obj" : "o";
    } else if (descr.get_operating_system() == OperatingSystem::LINUX) {
        return "o";
    } else {
        return "asm";
    }
}

codegen::Emitter *X8664Target::create_emitter(mcode::Module &module, std::ostream &stream) {
    if (lang::Config::instance().is_force_asm()) {
        return new codegen::NASMEmitter(module, stream, descr);
    }

    switch (descr.get_operating_system()) {
        case OperatingSystem::WINDOWS: return new codegen::PEEmitter(module, stream);
        case OperatingSystem::LINUX: return new codegen::ELFEmitter(module, stream);
        default: return new codegen::NASMEmitter(module, stream, descr);
    }
}

} // namespace target
