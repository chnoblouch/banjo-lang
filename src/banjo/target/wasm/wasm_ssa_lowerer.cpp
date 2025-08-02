#include "wasm_ssa_lowerer.hpp"

#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/mcode/symbol.hpp"
#include "banjo/ssa/utils.hpp"
#include "banjo/target/wasm/wasm_calling_conv.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::target {

WasmSSALowerer::WasmSSALowerer(target::Target *target) : codegen::SSALowerer(target) {}

void WasmSSALowerer::init_module(ssa::Module &mod) {
    WasmModData mod_data;

    for (ssa::FunctionDecl *func_decl : mod.get_external_functions()) {
        mod_data.func_imports.push_back(
            WasmFuncImport{
                .mod = "env",
                .name = func_decl->name,
                .type = lower_func_type(func_decl->type),
            }
        );
    }

    machine_module.set_target_data(std::move(mod_data));
}

void WasmSSALowerer::analyze_func(ssa::Function &func) {
    WasmFuncData func_data;
    func_data.type = lower_func_type(func.type);

    vregs2locals.clear();
    stack_pointer_local = func_data.type.params.size();

    // Create a local for the stack pointer.
    func_data.locals.push_back(WasmType::I32);

    func_data.stack_pointer_local = stack_pointer_local;

    for (ssa::BasicBlock &block : func) {
        for (ssa::Instruction &instr : block) {
            if (!instr.get_dest() || instr.get_opcode() == ssa::Opcode::ALLOCA) {
                continue;
            }

            unsigned local_index = func.type.params.size() + func_data.locals.size();
            ssa::Type type = ssa::get_result_type(instr, ssa::Primitive::I32);
            func_data.locals.push_back(lower_type(type));
            vregs2locals.insert({*instr.get_dest(), local_index});
        }
    }

    machine_func->set_target_data(func_data);
}

void WasmSSALowerer::lower_load(ssa::Instruction &instr) {
    unsigned local_index = vregs2locals.at(*instr.get_dest());
    ssa::Type type = instr.get_operand(0).get_type();

    ASSERT(type.is_primitive() && type.get_array_length() == 1);

    mcode::Opcode load_opcode;

    switch (type.get_primitive()) {
        case ssa::Primitive::VOID: ASSERT_UNREACHABLE;
        case ssa::Primitive::I8: ASSERT_UNREACHABLE;
        case ssa::Primitive::I16: ASSERT_UNREACHABLE;
        case ssa::Primitive::I32: load_opcode = WasmOpcode::I32_LOAD; break;
        case ssa::Primitive::I64: load_opcode = WasmOpcode::I64_LOAD; break;
        case ssa::Primitive::F32: load_opcode = WasmOpcode::F32_LOAD; break;
        case ssa::Primitive::F64: load_opcode = WasmOpcode::F64_LOAD; break;
        case ssa::Primitive::ADDR: load_opcode = WasmOpcode::I32_LOAD; break;
    }

    AddrComponents addr = collect_addr(instr.get_operand(1));
    ssa::VirtualRegister base = addr.base.get_register();

    if (std::optional<mcode::StackSlotID> stack_slot = find_stack_slot(base)) {
        mcode::Operand::StackSlotOffset stack_slot_offset{*stack_slot, addr.const_offset};
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(stack_pointer_local)}});
        emit({load_opcode, {mcode::Operand::from_stack_slot_offset(stack_slot_offset)}});
    } else {
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(vregs2locals.at(base))}});
        emit({load_opcode, {mcode::Operand::from_int_immediate(addr.const_offset)}});
    }

    emit({WasmOpcode::LOCAL_SET, {mcode::Operand::from_int_immediate(local_index)}});
}

void WasmSSALowerer::lower_store(ssa::Instruction &instr) {
    ssa::Operand &value = instr.get_operand(0);
    ssa::Type type = value.get_type();

    ASSERT(type.is_primitive() && type.get_array_length() == 1);

    mcode::Opcode store_opcode;

    switch (type.get_primitive()) {
        case ssa::Primitive::VOID: ASSERT_UNREACHABLE;
        case ssa::Primitive::I8: ASSERT_UNREACHABLE;
        case ssa::Primitive::I16: ASSERT_UNREACHABLE;
        case ssa::Primitive::I32: store_opcode = WasmOpcode::I32_STORE; break;
        case ssa::Primitive::I64: store_opcode = WasmOpcode::I64_STORE; break;
        case ssa::Primitive::F32: store_opcode = WasmOpcode::F32_STORE; break;
        case ssa::Primitive::F64: store_opcode = WasmOpcode::F64_STORE; break;
        case ssa::Primitive::ADDR: store_opcode = WasmOpcode::I32_STORE; break;
    }

    AddrComponents addr = collect_addr(instr.get_operand(1));
    ssa::VirtualRegister base = addr.base.get_register();

    if (std::optional<mcode::StackSlotID> stack_slot = find_stack_slot(base)) {
        mcode::Operand::StackSlotOffset stack_slot_offset{*stack_slot, addr.const_offset};
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(stack_pointer_local)}});
        push_operand(value);
        emit({store_opcode, {mcode::Operand::from_stack_slot_offset(stack_slot_offset)}});
    } else {
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(vregs2locals.at(base))}});
        push_operand(value);
        emit({store_opcode, {mcode::Operand::from_int_immediate(addr.const_offset)}});
    }
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

void WasmSSALowerer::lower_call(ssa::Instruction &instr) {
    ssa::Operand &callee = instr.get_operand(0);

    for (unsigned i = 1; i < instr.get_operands().size(); i++) {
        push_operand(instr.get_operand(i));
    }

    ssa::Type return_type;

    if (callee.is_func()) {
        ssa::Function &func = *callee.get_func();
        return_type = func.type.return_type;
        emit({WasmOpcode::CALL, {mcode::Operand::from_symbol(mcode::Symbol{func.name})}});
    } else if (callee.is_extern_func()) {
        ssa::FunctionDecl &extern_func = *callee.get_extern_func();
        return_type = extern_func.type.return_type;
        emit({WasmOpcode::CALL, {mcode::Operand::from_symbol(mcode::Symbol{extern_func.name})}});
    }

    if (return_type != ssa::Primitive::VOID) {
        emit({WasmOpcode::DROP});
    }
}

void WasmSSALowerer::lower_offsetptr(ssa::Instruction &instr) {
    unsigned local_index = vregs2locals.at(*instr.get_dest());
    ssa::Operand &offset = instr.get_operand(1);
    const ssa::Type &base_type = instr.get_operand(2).get_type();

    AddrComponents addr = collect_addr(instr.get_operand(0));

    ssa::VirtualRegister base = addr.base.get_register();
    unsigned const_offset = addr.const_offset + get_size(base_type) * offset.get_int_immediate().to_s64();

    if (std::optional<mcode::StackSlotID> stack_slot = find_stack_slot(base)) {
        mcode::Operand::StackSlotOffset stack_slot_offset{*stack_slot, const_offset};
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(stack_pointer_local)}});
        emit({WasmOpcode::I32_CONST, {mcode::Operand::from_stack_slot_offset(stack_slot_offset)}});
        emit({WasmOpcode::I32_ADD});
    } else {
        unsigned local_index = vregs2locals.at(base);
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(local_index)}});
        emit({WasmOpcode::I32_CONST, {mcode::Operand::from_stack_slot_offset(const_offset)}});
        emit({WasmOpcode::I32_ADD});
    }

    emit({WasmOpcode::LOCAL_SET, {mcode::Operand::from_int_immediate(local_index)}});
}

void WasmSSALowerer::lower_memberptr(ssa::Instruction &instr) {
    unsigned local_index = vregs2locals.at(*instr.get_dest());
    ssa::Structure *struct_ = instr.get_operand(0).get_type().get_struct();
    unsigned member_index = instr.get_operand(2).get_int_immediate().to_u64();

    AddrComponents addr = collect_addr(instr.get_operand(1));

    ssa::VirtualRegister base = addr.base.get_register();
    unsigned const_offset = addr.const_offset + get_member_offset(struct_, member_index);

    if (std::optional<mcode::StackSlotID> stack_slot = find_stack_slot(base)) {
        mcode::Operand::StackSlotOffset stack_slot_offset{*stack_slot, const_offset};
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(stack_pointer_local)}});
        emit({WasmOpcode::I32_CONST, {mcode::Operand::from_stack_slot_offset(stack_slot_offset)}});
        emit({WasmOpcode::I32_ADD});
    } else {
        unsigned local_index = vregs2locals.at(base);
        emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(local_index)}});
        emit({WasmOpcode::I32_CONST, {mcode::Operand::from_stack_slot_offset(const_offset)}});
        emit({WasmOpcode::I32_ADD});
    }

    emit({WasmOpcode::LOCAL_SET, {mcode::Operand::from_int_immediate(local_index)}});
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
        if (std::optional<mcode::StackSlotID> stack_slot = find_stack_slot(operand.get_register())) {
            mcode::Operand::StackSlotOffset stack_slot_offset{*stack_slot};
            emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(stack_pointer_local)}});
            emit({WasmOpcode::I32_CONST, {mcode::Operand::from_stack_slot_offset(stack_slot_offset)}});
            emit({WasmOpcode::I32_ADD});
        } else {
            unsigned local_index = vregs2locals.at(operand.get_register());
            emit({WasmOpcode::LOCAL_GET, {mcode::Operand::from_int_immediate(local_index)}});
        }
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
    } else if (operand.is_global()) {
        mcode::Symbol symbol{operand.get_global()->name};
        emit({WasmOpcode::I32_CONST, {mcode::Operand::from_symbol(symbol)}});
    } else if (operand.is_symbol()) {
        emit({WasmOpcode::I32_CONST, {mcode::Operand::from_int_immediate(0)}});
    } else {
        ASSERT_UNREACHABLE;
    }
}

WasmSSALowerer::AddrComponents WasmSSALowerer::collect_addr(ssa::Operand &addr) {
    ssa::Operand *base = &addr;
    unsigned const_offset = 0;

    while (base->is_register()) {
        ssa::InstrIter producer = get_producer_globally(base->get_register());
        if (!producer) {
            break;
        }

        if (producer->get_opcode() == ssa::Opcode::OFFSETPTR) {
            ssa::Operand &offset = producer->get_operand(1);
            const ssa::Type &base_type = producer->get_operand(2).get_type();

            if (offset.is_int_immediate()) {
                base = &producer->get_operand(0);
                const_offset += get_size(base_type) * offset.get_int_immediate().to_s64();
                discard_use(*producer->get_dest());
            } else {
                break;
            }
        } else if (producer->get_opcode() == ssa::Opcode::MEMBERPTR) {
            ssa::Structure *struct_ = producer->get_operand(0).get_type().get_struct();
            unsigned member_index = producer->get_operand(2).get_int_immediate().to_u64();

            base = &producer->get_operand(1);
            const_offset += get_member_offset(struct_, member_index);
            discard_use(*producer->get_dest());
        } else {
            break;
        }
    }

    return AddrComponents{
        .base = *base,
        .const_offset = const_offset,
    };
}

mcode::CallingConvention *WasmSSALowerer::get_calling_convention(ssa::CallingConv /*calling_conv*/) {
    return static_cast<mcode::CallingConvention *>(&WasmCallingConv::INSTANCE);
}

WasmFuncType WasmSSALowerer::lower_func_type(ssa::FunctionType type) {
    std::vector<WasmType> m_params(type.params.size());

    for (unsigned i = 0; i < type.params.size(); i++) {
        m_params[i] = lower_type(type.params[i]);
    }

    std::optional<WasmType> m_result_type;

    if (!type.return_type.is_primitive(ssa::Primitive::VOID)) {
        m_result_type = lower_type(type.return_type);
    }

    return WasmFuncType{
        .params = m_params,
        .result_type = m_result_type,
    };
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
