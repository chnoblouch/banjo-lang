#ifndef BANJO_TARGET_WASM_SSA_LOWERER_H
#define BANJO_TARGET_WASM_SSA_LOWERER_H

#include "banjo/codegen/ssa_lowerer.hpp"

namespace banjo::target {

class WasmSSALowerer final : public codegen::SSALowerer {

public:
    WasmSSALowerer(target::Target *target);

    void lower_block_instrs(ssa::BasicBlock &block) override;

    mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) override;
};

} // namespace banjo::target

#endif
