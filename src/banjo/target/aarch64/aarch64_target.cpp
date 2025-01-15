#include "aarch64_target.hpp"

#include "banjo/emit/aarch64_asm_emitter.hpp"
#include "banjo/target/aarch64/aarch64_instr_merge_pass.hpp"
#include "banjo/target/aarch64/aarch64_ssa_lowerer.hpp"
#include "banjo/target/aarch64/aarch64_stack_offset_fixup_pass.hpp"

namespace banjo {

namespace target {

AArch64Target::AArch64Target(TargetDescription descr, CodeModel code_model) : Target(descr, code_model) {}

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
    return "s";
}

codegen::Emitter *AArch64Target::create_emitter(mcode::Module &module, std::ostream &stream) {
    return new codegen::AArch64AsmEmitter(module, stream, descr);
}

} // namespace target

} // namespace banjo
