#include "wasm_target.hpp"

#include "banjo/codegen/prolog_epilog_pass.hpp"
#include "banjo/codegen/stack_frame_pass.hpp"
#include "banjo/emit/wasm/wasm_emitter.hpp"
#include "banjo/target/standard_data_layout.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/target/wasm/wasm_ssa_lowerer.hpp"

namespace banjo::target {

WasmTarget::WasmTarget(TargetDescription descr, CodeModel code_model)
  : Target(descr, code_model),
    data_layout{TargetDataLayout::Params{
        .register_size = 8,
        .usize_type = ssa::Primitive::U32,
        .max_regs_per_arg = 1,
        .supports_structs_in_regs = false,
    }} {}

codegen::SSALowerer *WasmTarget::create_ssa_lowerer() {
    return new WasmSSALowerer(this);
}

std::vector<std::unique_ptr<codegen::MachinePass>> WasmTarget::create_passes() {
    std::vector<std::unique_ptr<codegen::MachinePass>> passes;
    passes.emplace_back(std::make_unique<codegen::StackFramePass>());
    passes.emplace_back(std::make_unique<codegen::PrologEpilogPass>());
    return passes;
}

std::string WasmTarget::get_output_file_ext() {
    return "o";
}

codegen::Emitter *WasmTarget::create_emitter(mcode::Module &module, std::ostream &stream) {
    return new codegen::WasmEmitter(module, stream);
}

} // namespace banjo::target
