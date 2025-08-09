#include "wasm_calling_conv.hpp"

#include "banjo/mcode/stack_frame.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/utils/utils.hpp"

#include <any>

namespace banjo::target {

WasmCallingConv WasmCallingConv::INSTANCE;

static const mcode::Operand STACK_POINTER_GLOBAL = mcode::Operand::from_symbol(mcode::Symbol{"__stack_pointer"});

void WasmCallingConv::lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr) {}

void WasmCallingConv::create_arg_store_region(mcode::StackFrame &frame, mcode::StackRegions &regions) {
    mcode::ArgStoreStackRegion &region = regions.arg_store_region;
    region.size = 0;

    for (mcode::StackSlotID i = 0; i < frame.get_stack_slots().size(); i++) {
        mcode::StackSlot &slot = frame.get_stack_slots()[i];

        if (!slot.is_defined() && slot.get_type() == mcode::StackSlot::Type::ARG_STORE) {
            region.size -= 8;
            region.offsets.insert({i, region.size});
        }
    }
}

void WasmCallingConv::create_call_arg_region(
    mcode::Function * /*func*/,
    mcode::StackFrame & /*frame*/,
    mcode::StackRegions &regions
) {
    regions.call_arg_region.size = 0;
}

void WasmCallingConv::create_implicit_region(
    mcode::Function * /*func*/,
    mcode::StackFrame & /*frame*/,
    mcode::StackRegions &regions
) {
    regions.implicit_region.saved_reg_size = 0;
    regions.implicit_region.size = 0;
}

int WasmCallingConv::get_alloca_size(mcode::StackRegions &regions) {
    int size = regions.generic_region.size + regions.arg_store_region.size;
    return Utils::align(size, 16);
}

std::vector<mcode::Instruction> WasmCallingConv::get_prolog(mcode::Function *func) {
    const WasmFuncData &func_data = std::any_cast<const WasmFuncData &>(func->get_target_data());
    unsigned alloca_size = func->get_stack_frame().get_size();

    if (alloca_size == 0) {
        return {};
    }

    return {
        {WasmOpcode::GLOBAL_GET, {STACK_POINTER_GLOBAL}},
        {WasmOpcode::I32_CONST, {mcode::Operand::from_int_immediate(alloca_size)}},
        {WasmOpcode::I32_SUB},
        {WasmOpcode::LOCAL_TEE, {mcode::Operand::from_int_immediate(func_data.stack_pointer_local)}},
        {WasmOpcode::GLOBAL_SET, {STACK_POINTER_GLOBAL}},
    };
}

std::vector<mcode::Instruction> WasmCallingConv::get_epilog(mcode::Function *func) {
    const WasmFuncData &func_data = std::any_cast<const WasmFuncData &>(func->get_target_data());
    unsigned alloca_size = func->get_stack_frame().get_size();

    if (alloca_size == 0) {
        return {};
    }

    return {
        {WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(func_data.stack_pointer_local)}},
        {WasmOpcode::I32_CONST, {mcode::Operand::from_int_immediate(alloca_size)}},
        {WasmOpcode::I32_ADD},
        {WasmOpcode::GLOBAL_SET, {STACK_POINTER_GLOBAL}},
    };
}

bool WasmCallingConv::is_func_exit(mcode::Opcode opcode) {
    return opcode == WasmOpcode::END_FUNCTION;
}

std::vector<mcode::ArgStorage> WasmCallingConv::get_arg_storage(const ssa::FunctionType &func_type) {
    std::vector<mcode::ArgStorage> result(func_type.params.size());

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        result[i] = mcode::ArgStorage{
            .in_reg = true,
            .reg = 0,
        };
    }

    return result;
}

int WasmCallingConv::get_implicit_stack_bytes(mcode::Function * /*func*/) {
    return 0;
}

} // namespace banjo::target
