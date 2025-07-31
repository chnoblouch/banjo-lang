#include "wasm_ssa_lowerer.hpp"

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/target/x86_64/ms_abi_calling_conv.hpp"

namespace banjo::target {

WasmSSALowerer::WasmSSALowerer(target::Target *target) : codegen::SSALowerer(target) {}

void WasmSSALowerer::analyze_func(ssa::Function &func) {
    WasmFunctionData func_data;
    func_data.params.resize(func.type.params.size());

    for (unsigned i = 0; i < func.type.params.size(); i++) {
        func_data.params[i] = lower_type(func.type.params[i]);
    }

    if (!func.type.return_type.is_primitive(ssa::Primitive::VOID)) {
        func_data.result_type = lower_type(func.type.return_type);
    }

    for (ssa::BasicBlock &block : func) {
        for (ssa::Instruction &instr : block) {
            if (!instr.get_dest()) {
                continue;
            }

            unsigned local_index = func.type.params.size() + func_data.locals.size();
            ssa::Type type = ssa::Primitive::I32; // TODO
            func_data.locals.push_back(lower_type(type));
            vregs2locals.insert({*instr.get_dest(), local_index});
        }
    }

    machine_func->set_target_data(func_data);
}

void WasmSSALowerer::lower_loadarg(ssa::Instruction &instr) {
    unsigned param_index = instr.get_operand(1).get_int_immediate().to_u64();
    unsigned local_index = vregs2locals.at(*instr.get_dest());

    emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(param_index)}});
    emit({WasmOpcode::LOCAL_SET, {mcode::Operand::from_int_immediate(local_index)}});
}

void WasmSSALowerer::lower_add(ssa::Instruction &instr) {
    bool is_64_bit = instr.get_operand(0).get_type() == ssa::Primitive::I64;
    lower_2_operand_numeric(instr, is_64_bit ? WasmOpcode::I64_ADD : WasmOpcode::I32_ADD);
}

void WasmSSALowerer::lower_sub(ssa::Instruction &instr) {
    bool is_64_bit = instr.get_operand(0).get_type() == ssa::Primitive::I64;
    lower_2_operand_numeric(instr, is_64_bit ? WasmOpcode::I64_SUB : WasmOpcode::I32_SUB);
}

void WasmSSALowerer::lower_mul(ssa::Instruction &instr) {
    bool is_64_bit = instr.get_operand(0).get_type() == ssa::Primitive::I64;
    lower_2_operand_numeric(instr, is_64_bit ? WasmOpcode::I64_MUL : WasmOpcode::I32_MUL);
}

void WasmSSALowerer::lower_sdiv(ssa::Instruction &instr) {
    bool is_64_bit = instr.get_operand(0).get_type() == ssa::Primitive::I64;
    lower_2_operand_numeric(instr, is_64_bit ? WasmOpcode::I64_DIV_S : WasmOpcode::I32_DIV_S);
}

void WasmSSALowerer::lower_srem(ssa::Instruction &instr) {
    bool is_64_bit = instr.get_operand(0).get_type() == ssa::Primitive::I64;
    lower_2_operand_numeric(instr, is_64_bit ? WasmOpcode::I64_REM_S : WasmOpcode::I32_REM_S);
}

void WasmSSALowerer::lower_udiv(ssa::Instruction &instr) {
    bool is_64_bit = instr.get_operand(0).get_type() == ssa::Primitive::I64;
    lower_2_operand_numeric(instr, is_64_bit ? WasmOpcode::I64_DIV_U : WasmOpcode::I32_DIV_U);
}

void WasmSSALowerer::lower_urem(ssa::Instruction &instr) {
    bool is_64_bit = instr.get_operand(0).get_type() == ssa::Primitive::I64;
    lower_2_operand_numeric(instr, is_64_bit ? WasmOpcode::I64_REM_U : WasmOpcode::I32_REM_U);
}

void WasmSSALowerer::lower_ret(ssa::Instruction &instr) {
    if (!instr.get_operands().empty()) {
        push_operand(instr.get_operand(0));
    }

    emit({WasmOpcode::END});
}

void WasmSSALowerer::lower_2_operand_numeric(ssa::Instruction &instr, mcode::Opcode m_opcode) {
    unsigned local_index = vregs2locals.at(*instr.get_dest());

    push_operand(instr.get_operand(0));
    push_operand(instr.get_operand(1));
    emit({m_opcode});
    emit({WasmOpcode::LOCAL_SET, {mcode::Operand::from_int_immediate(local_index)}});
}

void WasmSSALowerer::push_operand(ssa::Operand &operand) {
    if (operand.is_register()) {
        unsigned local_index = vregs2locals.at(operand.get_register());
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(local_index)}});
    } else if (operand.is_int_immediate()) {
        LargeInt immediate = operand.get_int_immediate();
        bool is_64_bit = operand.get_type() == ssa::Primitive::I64;
        mcode::Opcode opcode = is_64_bit ? WasmOpcode::I64_CONST : WasmOpcode::I32_CONST;
        emit({opcode, {mcode::Operand::from_int_immediate(immediate)}});
    } else if (operand.is_fp_immediate()) {
        double immediate = operand.get_fp_immediate();
        bool is_64_bit = operand.get_type() == ssa::Primitive::F64;
        mcode::Opcode opcode = is_64_bit ? WasmOpcode::F64_CONST : WasmOpcode::F32_CONST;
        emit({opcode, {mcode::Operand::from_fp_immediate(immediate)}});
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::CallingConvention *WasmSSALowerer::get_calling_convention(ssa::CallingConv /*calling_conv*/) {
    return (mcode::CallingConvention *)&MSABICallingConv::INSTANCE;
}

WasmType WasmSSALowerer::lower_type(ssa::Type type) {
    ASSERT(type.is_primitive() && type.get_array_length() == 1);

    switch (type.get_primitive()) {
        case ssa::Primitive::VOID: ASSERT_UNREACHABLE;
        case ssa::Primitive::I8:
        case ssa::Primitive::I16:
        case ssa::Primitive::I32: return WasmType::I32;
        case ssa::Primitive::I64: return WasmType::I64;
        case ssa::Primitive::F32: return WasmType::F32;
        case ssa::Primitive::F64: return WasmType::F64;
        case ssa::Primitive::ADDR: return WasmType::I32;
    }
}

} // namespace banjo::target
