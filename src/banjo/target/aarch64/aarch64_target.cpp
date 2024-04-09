#include "aarch64_target.hpp"

#include "emit/clang_asm_emitter.hpp"
#include "target/aarch64/aarch64_instr_merge_pass.hpp"
#include "target/aarch64/aarch64_ir_lowerer.hpp"

namespace target {

AArch64Target::AArch64Target(TargetDescription descr, CodeModel code_model) : Target(descr, code_model) {}

codegen::IRLowerer *AArch64Target::create_ir_lowerer() {
    return new AArch64IRLowerer(this);
}

std::vector<codegen::MachinePass *> AArch64Target::create_pre_passes() {
    return {};
}

std::vector<codegen::MachinePass *> AArch64Target::create_post_passes() {
    return {};
}

std::string AArch64Target::get_output_file_ext() {
    return "s";
}

codegen::Emitter *AArch64Target::create_emitter(mcode::Module &module, std::ostream &stream) {
    return new codegen::ClangAsmEmitter(module, stream, descr);
}

} // namespace target
