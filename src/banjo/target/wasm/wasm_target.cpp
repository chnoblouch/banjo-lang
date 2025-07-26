#include "wasm_target.hpp"

#include "banjo/emit/wasm/wasm_emitter.hpp"
#include "banjo/target/x86_64/x86_64_ssa_lowerer.hpp"

namespace banjo::target {

WasmTarget::WasmTarget(TargetDescription descr, CodeModel code_model) : Target(descr, code_model) {}

codegen::SSALowerer *WasmTarget::create_ssa_lowerer() {
    return new X8664SSALowerer(this);
}

std::vector<codegen::MachinePass *> WasmTarget::create_pre_passes() {
    return {};
}

std::vector<codegen::MachinePass *> WasmTarget::create_post_passes() {
    return {};
}

std::string WasmTarget::get_output_file_ext() {
    return "o";
}

codegen::Emitter *WasmTarget::create_emitter(mcode::Module &module, std::ostream &stream) {
    return new codegen::WasmEmitter(module, stream);
}

} // namespace banjo::target
