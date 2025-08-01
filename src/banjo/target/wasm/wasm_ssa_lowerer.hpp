#ifndef BANJO_TARGET_WASM_SSA_LOWERER_H
#define BANJO_TARGET_WASM_SSA_LOWERER_H

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"

#include <unordered_map>

namespace banjo::target {

class WasmSSALowerer final : public codegen::SSALowerer {

private:
    std::unordered_map<ssa::VirtualRegister, unsigned> vregs2locals;

public:
    WasmSSALowerer(target::Target *target);

    void init_module(ssa::Module &mod) override;
    void analyze_func(ssa::Function &func) override;
    mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) override;

private:
    void lower_loadarg(ssa::Instruction &instr) override;
    void lower_add(ssa::Instruction &instr) override;
    void lower_sub(ssa::Instruction &instr) override;
    void lower_mul(ssa::Instruction &instr) override;
    void lower_sdiv(ssa::Instruction &instr) override;
    void lower_srem(ssa::Instruction &instr) override;
    void lower_udiv(ssa::Instruction &instr) override;
    void lower_urem(ssa::Instruction &instr) override;
    void lower_ret(ssa::Instruction &instr) override;
    void lower_call(ssa::Instruction &instr) override;

    void lower_2_operand_numeric(ssa::Instruction &instr, mcode::Opcode m_opcode);
    void push_operand(ssa::Operand &operand);

    WasmFuncType lower_func_type(ssa::FunctionType type);
    WasmType lower_type(ssa::Type type);
};

} // namespace banjo::target

#endif
