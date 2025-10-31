#include "aarch64_target.hpp"

#include "banjo/config/config.hpp"
#include "banjo/emit/aarch64_asm_emitter.hpp"
#include "banjo/emit/elf/elf_emitter.hpp"
#include "banjo/emit/macho/macho_emitter.hpp"
#include "banjo/target/aarch64/aarch64_ssa_lowerer.hpp"
#include "banjo/target/aarch64/aarch64_stack_offset_fixup_pass.hpp"

namespace banjo {

namespace target {

AArch64Target::AArch64Target(TargetDescription descr, CodeModel code_model)
  : Target(descr, code_model),
    data_layout{TargetDataLayout::Params{
        .register_size = 8,
        .usize_type = ssa::Primitive::U64,
        .max_regs_per_arg = 2,
        .supports_structs_in_regs = true,
    }} {}

codegen::SSALowerer *AArch64Target::create_ssa_lowerer() {
    return new AArch64SSALowerer(this);
}

std::vector<codegen::MachinePass *> AArch64Target::create_pre_passes() {
    return {};
}

std::vector<codegen::MachinePass *> AArch64Target::create_post_passes() {
    return {new AArch64StackOffsetFixupPass()};
}

std::string AArch64Target::get_output_file_ext() {
    if (lang::Config::instance().force_asm) {
        return "s";
    }

    if (descr.get_operating_system() == OperatingSystem::LINUX) {
        return "o";
    } else if (descr.get_operating_system() == OperatingSystem::MACOS) {
        return "o";
    } else {
        return "s";
    }
}

codegen::Emitter *AArch64Target::create_emitter(mcode::Module &module, std::ostream &stream) {
    if (lang::Config::instance().force_asm) {
        return new codegen::AArch64AsmEmitter(module, stream, descr);
    }

    switch (descr.get_operating_system()) {
        case OperatingSystem::LINUX: return new codegen::ELFEmitter(module, stream, descr);
        case OperatingSystem::MACOS: return new codegen::MachOEmitter(module, stream, descr);
        default: return new codegen::AArch64AsmEmitter(module, stream, descr);
    }
}

} // namespace target

} // namespace banjo
