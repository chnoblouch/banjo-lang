#include "x86_64_target.hpp"

#include "banjo/codegen/prolog_epilog_pass.hpp"
#include "banjo/codegen/reg_alloc_pass.hpp"
#include "banjo/codegen/stack_frame_pass.hpp"
#include "banjo/config/config.hpp"
#include "banjo/emit/elf/elf_emitter.hpp"
#include "banjo/emit/nasm_emitter.hpp"
#include "banjo/emit/pe/pe_emitter.hpp"
#include "banjo/target/standard_data_layout.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/target/x86_64/x86_64_peephole_opt_pass.hpp"
#include "banjo/target/x86_64/x86_64_ssa_lowerer.hpp"

namespace banjo::target {

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

std::vector<std::unique_ptr<codegen::MachinePass>> X8664Target::create_passes() {
    std::vector<std::unique_ptr<codegen::MachinePass>> passes;
    passes.emplace_back(std::make_unique<codegen::RegAllocPass>(reg_analyzer));
    passes.emplace_back(std::make_unique<codegen::StackFramePass>());
    passes.emplace_back(std::make_unique<codegen::PrologEpilogPass>());
    passes.emplace_back(std::make_unique<X8664PeepholeOptPass>());
    return passes;
}

std::string X8664Target::get_output_file_ext() {
    if (Config::instance().force_asm) {
        return descr.get_environment() == Environment::MSVC ? "asm" : "s";
    }

    return descr.get_environment() == Environment::MSVC ? "obj" : "o";
}

codegen::Emitter *X8664Target::create_emitter(mcode::Module &module, std::ostream &stream) {
    if (Config::instance().force_asm) {
        return new codegen::NASMEmitter(module, stream, descr);
    }

    switch (descr.get_operating_system()) {
        case OperatingSystem::WINDOWS: return new codegen::PEEmitter(module, stream);
        case OperatingSystem::LINUX: return new codegen::ELFEmitter(module, stream, descr);
        default: return new codegen::NASMEmitter(module, stream, descr);
    }
}

} // namespace banjo
