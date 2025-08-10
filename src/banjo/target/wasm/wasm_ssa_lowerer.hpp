#ifndef BANJO_TARGET_WASM_SSA_LOWERER_H
#define BANJO_TARGET_WASM_SSA_LOWERER_H

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"

#include <unordered_map>

namespace banjo::target {

class WasmSSALowerer final : public codegen::SSALowerer {

private:
    struct AddrComponents {
        ssa::Operand &base;
        unsigned const_offset;
    };

    std::unordered_map<ssa::VirtualRegister, unsigned> vregs2locals;
    std::unordered_map<ssa::BasicBlockIter, unsigned> block_indices;
    unsigned stack_pointer_local;
    unsigned block_index_local;
    unsigned return_value_local;
    unsigned block_depth;

public:
    WasmSSALowerer(target::Target *target);

    void init_module(ssa::Module &mod) override;
    void analyze_func(ssa::Function &func) override;
    BlockMap generate_blocks(ssa::Function &func) override;
    mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) override;

private:
    void lower_load(ssa::Instruction &instr) override;
    void lower_store(ssa::Instruction &instr) override;
    void lower_loadarg(ssa::Instruction &instr) override;
    void lower_add(ssa::Instruction &instr) override;
    void lower_sub(ssa::Instruction &instr) override;
    void lower_mul(ssa::Instruction &instr) override;
    void lower_sdiv(ssa::Instruction &instr) override;
    void lower_srem(ssa::Instruction &instr) override;
    void lower_udiv(ssa::Instruction &instr) override;
    void lower_urem(ssa::Instruction &instr) override;
    void lower_jmp(ssa::Instruction &instr) override;
    void lower_cjmp(ssa::Instruction &instr) override;
    void lower_fcjmp(ssa::Instruction &instr) override;
    void lower_call(ssa::Instruction &instr) override;
    void lower_ret(ssa::Instruction &instr) override;
    void lower_offsetptr(ssa::Instruction &instr) override;
    void lower_memberptr(ssa::Instruction &instr) override;

    void lower_2_operand_numeric(ssa::Instruction &instr, mcode::Opcode m_opcode);
    void push_operand(ssa::Operand &operand);
    AddrComponents collect_addr(ssa::Operand &addr);

    WasmFuncType lower_func_type(ssa::FunctionType type);
    WasmType lower_type(ssa::Type type);
    ssa::Primitive type_as_primitive(ssa::Type type);
    bool is_64_bit_int(ssa::Type type);
};

} // namespace banjo::target

#endif
