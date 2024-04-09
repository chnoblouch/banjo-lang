#include "target.hpp"

#include "target/aarch64/aarch64_target.hpp"
#include "target/x86_64/x86_64_target.hpp"

namespace target {

Target::Target(TargetDescription descr, CodeModel code_model) : descr(descr), code_model(code_model) {}

ir::CallingConv Target::get_default_calling_conv() {
    // TODO: move into virtual function

    if (descr.get_architecture() == target::Architecture::X86_64) {
        switch (descr.get_operating_system()) {
            case target::OperatingSystem::WINDOWS: return ir::CallingConv::X86_64_MS_ABI;
            case target::OperatingSystem::LINUX: return ir::CallingConv::X86_64_SYS_V_ABI;
            case target::OperatingSystem::MACOS: return ir::CallingConv::X86_64_SYS_V_ABI;
            case target::OperatingSystem::ANDROID: return ir::CallingConv::X86_64_SYS_V_ABI;
            default: return ir::CallingConv::NONE;
        }
    } else if (descr.get_architecture() == target::Architecture::AARCH64) {
        return ir::CallingConv::AARCH64_AAPCS;
    } else {
        return ir::CallingConv::NONE;
    }
}

Target *Target::create(TargetDescription descr, CodeModel code_model) {
    Architecture arch = descr.get_architecture();

    switch (arch) {
        case Architecture::INVALID: return nullptr;
        case Architecture::X86_64: return new X8664Target(descr, code_model);
        case Architecture::AARCH64: return new AArch64Target(descr, code_model);
    }
}

} // namespace target
