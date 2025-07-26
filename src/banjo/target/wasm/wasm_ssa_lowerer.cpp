#include "wasm_ssa_lowerer.hpp"

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/target/x86_64/ms_abi_calling_conv.hpp"

namespace banjo::target {

WasmSSALowerer::WasmSSALowerer(target::Target *target) : codegen::SSALowerer(target) {}

void WasmSSALowerer::lower_block_instrs(ssa::BasicBlock &block) {
    mcode::BasicBlock &m_block = *basic_block_context.basic_block;

    if (func->name == "add") {
        m_block.append({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(1)}});
        m_block.append({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(0)}});
        m_block.append({WasmOpcode::I32_ADD});
        m_block.append({WasmOpcode::END});
    } else if (func->name == "fadd") {
        m_block.append({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(1)}});
        m_block.append({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(0)}});
        m_block.append({WasmOpcode::F32_ADD});
        m_block.append({WasmOpcode::END});
    }
}

mcode::CallingConvention *WasmSSALowerer::get_calling_convention(ssa::CallingConv /*calling_conv*/) {
    return (mcode::CallingConvention *)&MSABICallingConv::INSTANCE;
}

} // namespace banjo::target
