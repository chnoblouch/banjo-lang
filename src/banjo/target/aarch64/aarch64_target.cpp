#include "aarch64_target.hpp"

#include "banjo/codegen/prolog_epilog_pass.hpp"
#include "banjo/codegen/reg_alloc_pass.hpp"
#include "banjo/codegen/stack_frame_pass.hpp"
#include "banjo/config/config.hpp"
#include "banjo/emit/aarch64_asm_emitter.hpp"
#include "banjo/emit/elf/elf_emitter.hpp"
#include "banjo/emit/macho/macho_emitter.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_reg_analyzer.hpp"
#include "banjo/target/aarch64/aarch64_ssa_lowerer.hpp"
#include "banjo/target/aarch64/aarch64_stack_addr_fixup_pass.hpp"

namespace banjo::target {

AArch64Target::AArch64Target(TargetDescription descr, CodeModel code_model)
  : Target(descr, code_model),
    data_layout{TargetDataLayout::Params{
        .register_size = 8,
        .usize_type = ssa::Primitive::U64,
        .max_regs_per_arg = 2,
        .supports_structs_in_regs = true,
    }},
    reg_analyzer{descr.is_darwin() ? AArch64Register::R18 : std::optional<mcode::PhysicalReg>{}} {}

codegen::SSALowerer *AArch64Target::create_ssa_lowerer() {
    return new AArch64SSALowerer(this);
}

std::vector<std::unique_ptr<codegen::MachinePass>> AArch64Target::create_passes() {
    std::vector<std::unique_ptr<codegen::MachinePass>> passes;
    passes.emplace_back(std::make_unique<codegen::RegAllocPass>(reg_analyzer));
    passes.emplace_back(std::make_unique<codegen::StackFramePass>());
    passes.emplace_back(std::make_unique<codegen::PrologEpilogPass>());
    passes.emplace_back(std::make_unique<AArch64StackAddrFixupPass>());
    return passes;
}

std::string AArch64Target::get_output_file_ext() {
    if (Config::instance().force_asm) {
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
    if (Config::instance().force_asm) {
        return new codegen::AArch64AsmEmitter(module, stream, descr);
    }

    switch (descr.get_operating_system()) {
        case OperatingSystem::LINUX: return new codegen::ELFEmitter(module, stream, descr);
        case OperatingSystem::MACOS: return new codegen::MachOEmitter(module, stream, descr);
        default: return new codegen::AArch64AsmEmitter(module, stream, descr);
    }
}

} // namespace banjo::target
