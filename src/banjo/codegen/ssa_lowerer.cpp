#include "ssa_lowerer.hpp"

#include "banjo/mcode/basic_block.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/stack_address.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/timing.hpp"

namespace banjo::codegen {

SSALowerer::SSALowerer(target::Target *target) : target{target} {}

mcode::Module SSALowerer::lower_module(ssa::Module &mod) {
    PROFILE_SCOPE("ssa lowering");

    this->ssa_mod = &mod;
    init_module(mod);

    if (mod.get_addr_table()) {
        m_module.set_addr_table(
            mcode::AddrTable{
                .entries = mod.get_addr_table()->get_entries(),
            }
        );
    }

    for (ssa::FunctionDecl *external_func : mod.get_external_functions()) {
        if (external_func->name == "memcpy") {
            memcpy_func = external_func;
        } else if (external_func->name == "sqrt") {
            sqrt_func = external_func;
        }
    }

    for (ssa::FunctionDecl *external_function : mod.get_external_functions()) {
        m_module.add_external_symbol(external_function->name);
    }

    for (ssa::GlobalDecl *external_global : mod.get_external_globals()) {
        m_module.add_external_symbol(external_global->name);
    }

    for (ssa::Function *func : mod.get_functions()) {
        lower_func(*func);
    }

    for (ssa::Global *global : mod.get_globals()) {
        lower_global(*global);
    }

    for (const std::string &dll_export : mod.get_dll_exports()) {
        m_module.add_dll_export(dll_export);
    }

    return std::move(m_module);
}

void SSALowerer::lower_func(ssa::Function &func) {
    this->ssa_func = &func;

    mcode::Function *m_func = new mcode::Function{
        .name = func.name,
        .calling_conv = get_calling_convention(func.type.calling_conv),
        .debug_name = func.debug_name,
    };

    instr_ctx = {
        .func = m_func,
        .block = nullptr,
        .instr = nullptr,
    };

    stack_regs.clear();
    reg_use_counts.clear();

    std::vector<mcode::ArgStorage> storage = m_func->calling_conv->get_arg_storage(func.type);

    for (unsigned i = 0; i < func.type.params.size(); i++) {
        mcode::Parameter param = lower_param(func.type.params[i], storage[i], *m_func);
        m_func->parameters.push_back(param);
    }

    for (ssa::BasicBlock &basic_block : func) {
        for (ssa::Instruction &instr : basic_block.get_instrs()) {
            for (ssa::Operand &operand : instr.get_operands()) {
                if (operand.is_register()) {
                    reg_use_counts[operand.get_register()] += 1;
                }

                if (operand.is_branch_target()) {
                    for (ssa::Operand &arg : operand.get_branch_target().args) {
                        if (arg.is_register()) {
                            reg_use_counts[arg.get_register()] += 1;
                        }
                    }
                }
            }

            if (instr.get_opcode() == ssa::Opcode::ALLOCA) {
                lower_alloca(instr);
            }
        }
    }

    init_func(func);
    block_map.clear();

    for (ssa::BasicBlockIter ssa_block = func.begin(); ssa_block != func.end(); ++ssa_block) {
        create_block(ssa_block);
    }

    generate_blocks(func);
    m_module.add(m_func);

    if (func.global) {
        m_module.add_global_symbol(func.name);
    }
}

void SSALowerer::generate_blocks(ssa::Function &func) {
    for (ssa::BasicBlockIter ssa_block = func.begin(); ssa_block != func.end(); ++ssa_block) {
        generate_block(ssa_block, block_map.at(ssa_block));
    }
}

mcode::Parameter SSALowerer::lower_param(ssa::Type type, mcode::ArgStorage storage, mcode::Function &m_func) {
    if (storage.in_reg) {
        return mcode::Parameter{
            .type = type,
            .storage = mcode::Register::from_physical(storage.reg),
        };
    } else {
        mcode::StackSlot slot{
            .type = mcode::StackSlot::Type::GENERIC,
            .size = 8,
            .alignment = 8,
        };

        return mcode::Parameter{
            .type = type,
            .storage = m_func.stack_frame.new_stack_slot(slot),
        };
    }
}

void SSALowerer::create_block(ssa::BasicBlockIter ssa_block) {
    mcode::BasicBlock m_block{.label = ssa_block->get_label()};

    for (ssa::VirtualRegister reg : ssa_block->get_param_regs()) {
        m_block.params.push_back(reg);
    }

    mcode::BasicBlockIter m_block_iter = instr_ctx.func->basic_blocks.append(m_block);
    block_map.insert({ssa_block, m_block_iter});
}

void SSALowerer::generate_block(ssa::BasicBlockIter ssa_block, mcode::BasicBlockIter m_block) {
    this->ssa_block = ssa_block;

    for (ssa::InstrIter instr = ssa_block->get_instrs().get_last_iter(); instr != ssa_block->get_header(); --instr) {
        if (instr->get_dest() && get_num_uses(*instr->get_dest()) == 0 && !instr->has_side_effects()) {
            continue;
        }

        instr_ctx.instr = m_block->instrs.begin();
        instr_ctx.block = m_block;
        lower_instr(instr);
    }

    instr_ctx.instr = m_block->instrs.begin();
    instr_ctx.block = m_block;
    emit_block_prologue(*ssa_block);
}

void SSALowerer::lower_instr(ssa::InstrIter instr) {
    ssa_instr = instr;

    switch (instr->get_opcode()) {
        case ssa::Opcode::ALLOCA: break;
        case ssa::Opcode::LOAD: lower_load(*instr); break;
        case ssa::Opcode::STORE: lower_store(*instr); break;
        case ssa::Opcode::LOADARG: lower_loadarg(*instr); break;
        case ssa::Opcode::ADD: lower_add(*instr); break;
        case ssa::Opcode::SUB: lower_sub(*instr); break;
        case ssa::Opcode::MUL: lower_mul(*instr); break;
        case ssa::Opcode::SDIV: lower_sdiv(*instr); break;
        case ssa::Opcode::SREM: lower_srem(*instr); break;
        case ssa::Opcode::UDIV: lower_udiv(*instr); break;
        case ssa::Opcode::UREM: lower_urem(*instr); break;
        case ssa::Opcode::FADD: lower_fadd(*instr); break;
        case ssa::Opcode::FSUB: lower_fsub(*instr); break;
        case ssa::Opcode::FMUL: lower_fmul(*instr); break;
        case ssa::Opcode::FDIV: lower_fdiv(*instr); break;
        case ssa::Opcode::AND: lower_and(*instr); break;
        case ssa::Opcode::OR: lower_or(*instr); break;
        case ssa::Opcode::XOR: lower_xor(*instr); break;
        case ssa::Opcode::LSHL: lower_lshl(*instr); break;
        case ssa::Opcode::LSHR: lower_lshr(*instr); break;
        case ssa::Opcode::ASHR: lower_ashr(*instr); break;
        case ssa::Opcode::JMP: lower_jmp(*instr); break;
        case ssa::Opcode::CJMP: lower_cjmp(*instr); break;
        case ssa::Opcode::FCJMP: lower_fcjmp(*instr); break;
        case ssa::Opcode::SELECT: lower_select(*instr); break;
        case ssa::Opcode::CALL: lower_call(*instr); break;
        case ssa::Opcode::RET: lower_ret(*instr); break;
        case ssa::Opcode::UEXTEND: lower_uextend(*instr); break;
        case ssa::Opcode::SEXTEND: lower_sextend(*instr); break;
        case ssa::Opcode::TRUNCATE: lower_truncate(*instr); break;
        case ssa::Opcode::FPROMOTE: lower_fpromote(*instr); break;
        case ssa::Opcode::FDEMOTE: lower_fdemote(*instr); break;
        case ssa::Opcode::UTOF: lower_utof(*instr); break;
        case ssa::Opcode::STOF: lower_stof(*instr); break;
        case ssa::Opcode::FTOU: lower_ftou(*instr); break;
        case ssa::Opcode::FTOS: lower_ftos(*instr); break;
        case ssa::Opcode::ATOMIC_LOAD: lower_atomic_load(*instr); break;
        case ssa::Opcode::ATOMIC_STORE: lower_atomic_store(*instr); break;
        case ssa::Opcode::ATOMIC_ADD: lower_atomic_add(*instr); break;
        case ssa::Opcode::ATOMIC_SUB: lower_atomic_sub(*instr); break;
        case ssa::Opcode::ATOMIC_AND: lower_atomic_and(*instr); break;
        case ssa::Opcode::ATOMIC_OR: lower_atomic_or(*instr); break;
        case ssa::Opcode::ATOMIC_XOR: lower_atomic_xor(*instr); break;
        case ssa::Opcode::OFFSETPTR: lower_offsetptr(*instr); break;
        case ssa::Opcode::MEMBERPTR: lower_memberptr(*instr); break;
        case ssa::Opcode::COPY: lower_copy(*instr); break;
        case ssa::Opcode::SQRT: lower_sqrt(*instr); break;
        case ssa::Opcode::FRAME_ADDRESS: lower_frame_address(*instr); break;
    }
}

void SSALowerer::lower_global(ssa::Global &global) {
    mcode::Global m_global{
        .name = global.name,
        .size = get_size(global.type),
        .alignment = get_alignment(global.type),
        .value = {},
    };

    ssa::Global::Value &value = global.initial_value;

    if (std::holds_alternative<ssa::Global::None>(value)) {
        m_global.value = mcode::Global::None{};
    } else if (auto int_value = std::get_if<ssa::Global::Integer>(&value)) {
        m_global.value = *int_value;
    } else if (auto fp_value = std::get_if<ssa::Global::FloatingPoint>(&value)) {
        m_global.value = *fp_value;
    } else if (auto bytes = std::get_if<ssa::Global::Bytes>(&value)) {
        m_global.value = *bytes;
    } else if (auto string = std::get_if<ssa::Global::String>(&value)) {
        m_global.value = *string;
    } else if (auto func = std::get_if<ssa::Function *>(&value)) {
        m_global.value = mcode::Global::SymbolRef{.name = (*func)->name};
    } else if (auto global_ref = std::get_if<ssa::Global::GlobalRef>(&value)) {
        m_global.value = mcode::Global::SymbolRef{.name = global_ref->name};
    } else if (auto extern_func_ref = std::get_if<ssa::Global::ExternFuncRef>(&value)) {
        m_global.value = mcode::Global::SymbolRef{.name = extern_func_ref->name};
    } else if (auto extern_global_ref = std::get_if<ssa::Global::ExternGlobalRef>(&value)) {
        m_global.value = mcode::Global::SymbolRef{.name = extern_global_ref->name};
    } else {
        ASSERT_UNREACHABLE;
    }

    m_module.add(m_global);
}

mcode::InstrIter SSALowerer::emit(mcode::Instruction instr) {
    return instr_ctx.block->insert_before(instr_ctx.instr, std::move(instr));
}

mcode::BasicBlockIter SSALowerer::create_block() {
    mcode::BasicBlock block{.label = ssa_func->next_block_label()};
    return instr_ctx.func->basic_blocks.create_iter(std::move(block));
}

void SSALowerer::start_block(mcode::BasicBlockIter block) {
    mcode::InstrIter first_emitted_instr = instr_ctx.instr.get_prev();

    while (first_emitted_instr.get_next() != instr_ctx.block->end()) {
        mcode::InstrIter instr_to_move = first_emitted_instr.get_next();
        block->append(*instr_to_move);
        instr_ctx.block->remove(instr_to_move);
    }

    instr_ctx.func->basic_blocks.insert_after(instr_ctx.block, block);

    instr_ctx.block = block;
    instr_ctx.instr = block->begin();
}

std::optional<mcode::StackSlotID> SSALowerer::find_stack_slot(ssa::VirtualRegister reg) {
    auto iter = stack_regs.find(reg);

    if (iter != stack_regs.end()) {
        return iter->second;
    } else {
        return {};
    }
}

std::variant<mcode::Register, mcode::StackSlotID> SSALowerer::map_vreg(ssa::VirtualRegister reg) {
    auto iter = stack_regs.find(reg);

    if (iter != stack_regs.end()) {
        return iter->second;
    } else {
        return mcode::Register::from_virtual(reg);
    }
}

mcode::Register SSALowerer::map_vreg_as_reg(ssa::VirtualRegister reg) {
    ASSERT(!stack_regs.contains(reg));
    return mcode::Register::from_virtual(reg);
}

mcode::Operand SSALowerer::map_vreg_as_operand(ssa::VirtualRegister reg, unsigned size) {
    auto iter = stack_regs.find(reg);

    if (iter != stack_regs.end()) {
        return mcode::Operand::from_stack_slot(iter->second, size);
    } else {
        return mcode::Operand::from_register(mcode::Register::from_virtual(reg), size);
    }
}

mcode::Operand SSALowerer::map_vreg_dst(ssa::Instruction &instr, unsigned size) {
    return mcode::Operand::from_register(map_vreg_as_reg(*instr.get_dest()), size);
}

unsigned SSALowerer::get_size(const ssa::Type &type) {
    return target->get_data_layout().get_size(type);
}

unsigned SSALowerer::get_alignment(const ssa::Type &type) {
    return target->get_data_layout().get_alignment(type);
}

unsigned SSALowerer::get_member_offset(ssa::Structure *struct_, unsigned index) {
    return target->get_data_layout().get_member_offset(struct_, index);
}

mcode::Register SSALowerer::create_tmp_reg() {
    return mcode::Register::from_virtual(ssa_func->next_virtual_reg());
}

void SSALowerer::lower_alloca(ssa::Instruction &instr) {
    unsigned size = std::max(get_size(instr.get_operand(0).get_type()), 8u);
    bool is_arg_store = instr.get_attr() == ssa::Instruction::Attribute::ARG_STORE;
    mcode::StackSlot::Type type = is_arg_store ? mcode::StackSlot::Type::ARG_STORE : mcode::StackSlot::Type::GENERIC;
    mcode::StackSlot slot{.type = type, .size = size, .alignment = 1};

    mcode::StackSlotID index = instr_ctx.func->stack_frame.new_stack_slot(slot);
    stack_regs.insert({*instr.get_dest(), index});
}

void SSALowerer::lower_copy(ssa::Instruction &instr) {
    ASSERT(memcpy_func);

    ssa::Operand func_operand = ssa::Operand::from_extern_func(memcpy_func, ssa::Primitive::VOID);
    ssa::Operand dst_operand = instr.get_operand(0);
    ssa::Operand src_operand = instr.get_operand(1);

    unsigned size = target->get_data_layout().get_size(instr.get_operand(2).get_type());
    ssa::Operand size_operand = ssa::Operand::from_int_immediate(size, target->get_data_layout().get_usize_type());

    ssa::Instruction call_instr(ssa::Opcode::CALL, {func_operand, dst_operand, src_operand, size_operand});
    lower_call(call_instr);
}

void SSALowerer::lower_sqrt(ssa::Instruction &instr) {
    ASSERT(sqrt_func);

    ssa::Operand func_operand = ssa::Operand::from_extern_func(sqrt_func, ssa::Primitive::F32);
    ssa::Operand input_operand = instr.get_operand(0);
    ssa::VirtualRegister output_reg = *instr.get_dest();

    ssa::Instruction call_instr(ssa::Opcode::CALL, output_reg, {func_operand, input_operand});
    lower_call(call_instr);
}

ssa::InstrIter SSALowerer::get_producer(ssa::VirtualRegister reg) {
    ssa::BasicBlock &cur_block = get_block();

    for (ssa::InstrIter iter = cur_block.get_trailer().get_prev(); iter != cur_block.get_header(); --iter) {
        if (iter->get_dest() == reg) {
            return iter;
        }
    }

    return cur_block.end();
}

ssa::InstrIter SSALowerer::get_producer_globally(ssa::VirtualRegister reg) {
    ssa::InstrIter iter = get_producer(reg);
    if (iter != ssa_block->end()) {
        return iter;
    }

    /*
    for (ssa::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        if (block_iter == basic_block_iter) {
            continue;
        }

        for (ssa::InstrIter iter = block_iter->begin(); iter != block_iter->end(); ++iter) {
            if (iter->get_dest() == reg) {
                return iter;
            }
        }
    }
    */

    return nullptr;
}

unsigned SSALowerer::get_num_uses(ssa::VirtualRegister reg) {
    return reg_use_counts[reg];
}

void SSALowerer::discard_use(ssa::VirtualRegister reg) {
    reg_use_counts[reg] -= 1;
}

SSALowerer::AddrComponents SSALowerer::collect_addr(ssa::Operand &addr) {
    ssa::Operand *base = &addr;
    LargeInt const_offset = 0;
    std::optional<RegOffset> reg_offset;

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
                const_offset += LargeInt{get_size(base_type)} * offset.get_int_immediate();
                discard_use(*producer->get_dest());
            } else if (offset.is_register()) {
                // TODO: Temporary fix
                if (target->get_descr().get_architecture() == target::Architecture::WASM) {
                    break;
                }

                if (!reg_offset) {
                    mcode::Register reg = mcode::Register::from_virtual(offset.get_register());

                    base = &producer->get_operand(0);
                    reg_offset = RegOffset{.reg = reg, .scale = get_size(base_type)};
                    discard_use(*producer->get_dest());
                } else {
                    break;
                }
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
        .reg_offset = reg_offset,
    };
}

} // namespace banjo::codegen
