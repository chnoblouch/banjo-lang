#include "x86_64_target.hpp"

#include "banjo/config/config.hpp"
#include "banjo/emit/elf/elf_emitter.hpp"
#include "banjo/emit/nasm_emitter.hpp"
#include "banjo/emit/pe/pe_emitter.hpp"
#include "banjo/target/standard_data_layout.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/target/x86_64/x86_64_peephole_opt_pass.hpp"
#include "banjo/target/x86_64/x86_64_ssa_lowerer.hpp"

namespace banjo {

namespace target {

X8664Target::X8664Target(TargetDescription descr, CodeModel code_model)
  : Target(descr, code_model),
    data_layout{TargetDataLayout::Params{
        .register_size = 8,
        .usize_type = ssa::Primitive::U64,
        .max_regs_per_arg = descr.is_windows() ? 1u : 2u,
        .supports_structs_in_regs = true,
    }} {}

codegen::SSALowerer *X8664Target::create_ssa_lowerer() {
    return new X8664SSALowerer(this);
}

std::vector<codegen::MachinePass *> X8664Target::create_pre_passes() {
    return {};
}

std::vector<codegen::MachinePass *> X8664Target::create_post_passes() {
    return {new X8664PeepholeOptPass()};
}

std::string X8664Target::get_output_file_ext() {
    if (lang::Config::instance().force_asm) {
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
    if (lang::Config::instance().force_asm) {
        return new codegen::NASMEmitter(module, stream, descr);
    }

    switch (descr.get_operating_system()) {
        case OperatingSystem::WINDOWS: return new codegen::PEEmitter(module, stream);
        case OperatingSystem::LINUX: return new codegen::ELFEmitter(module, stream, descr);
        default: return new codegen::NASMEmitter(module, stream, descr);
    }
}

} // namespace target

} // namespace banjo
