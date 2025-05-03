#include "ssa_lowerer.hpp"

#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/timing.hpp"

#include <iostream>

#define WARN_UNIMPLEMENTED(instruction) std::cerr << "warning: cannot lower instruction " << (instruction) << '\n';

namespace banjo {

namespace codegen {

SSALowerer::SSALowerer(target::Target *target) : target(target) {}

mcode::Module SSALowerer::lower_module(ssa::Module &module_) {
    PROFILE_SCOPE("ssa lowering");

    this->module_ = &module_;
    init_module(module_);

    if (module_.get_addr_table()) {
        machine_module.set_addr_table(
            mcode::AddrTable{
                .entries = module_.get_addr_table()->get_entries(),
            }
        );
    }

    for (ssa::FunctionDecl *external_func : module_.get_external_functions()) {
        if (external_func->name == "memcpy") {
            memcpy_func = external_func;
        } else if (external_func->name == "sqrt") {
            sqrt_func = external_func;
        }
    }

    lower_external_funcs();
    lower_external_globals();
    lower_funcs();
    lower_globals();
    lower_dll_exports();

    return std::move(machine_module);
}

void SSALowerer::lower_funcs() {
    for (ssa::Function *func : module_->get_functions()) {
        this->func = func;

        mcode::CallingConvention *calling_conv = get_calling_convention(func->type.calling_conv);
        mcode::Function *machine_func = new mcode::Function(func->name, calling_conv);
        this->machine_func = machine_func;

        context = {};

        std::vector<mcode::ArgStorage> storage = calling_conv->get_arg_storage(func->type);

        for (unsigned i = 0; i < func->type.params.size(); i++) {
            mcode::Parameter param = lower_param(func->type.params[i], storage[i], *machine_func);
            machine_func->get_parameters().push_back(param);
        }

        const LinkedList<ssa::BasicBlock> &basic_blocks = func->get_basic_blocks();
        BlockMap block_map;

        context.reg_use_counts.clear();

        for (ssa::BasicBlock &basic_block : *func) {
            for (ssa::Instruction &instr : basic_block.get_instrs()) {
                for (ssa::Operand &operand : instr.get_operands()) {
                    if (operand.is_register()) {
                        context.reg_use_counts[operand.get_register()]++;
                    }

                    if (operand.is_branch_target()) {
                        for (ssa::Operand &arg : operand.get_branch_target().args) {
                            if (arg.is_register()) {
                                context.reg_use_counts[arg.get_register()]++;
                            }
                        }
                    }
                }

                if (instr.get_opcode() == ssa::Opcode::ALLOCA) {
                    lower_alloca(instr);
                }
            }
        }

        for (ssa::BasicBlockIter iter = basic_blocks.begin(); iter != basic_blocks.end(); ++iter) {
            basic_block_iter = iter;
            mcode::BasicBlock m_block = lower_basic_block(*iter);
            mcode::BasicBlockIter m_iter = machine_func->get_basic_blocks().append(m_block);
            block_map.insert({iter, m_iter});
        }

        store_graphs(*func, machine_func, block_map);

        machine_module.add(machine_func);

        if (func->global) {
            machine_module.add_global_symbol(func->name);
        }
    }
}

mcode::Parameter SSALowerer::lower_param(ssa::Type type, mcode::ArgStorage storage, mcode::Function &m_func) {
    if (storage.in_reg) {
        return mcode::Parameter{
            .type = type,
            .storage = mcode::Register::from_physical(storage.reg),
        };
    } else {
        mcode::StackSlot slot(mcode::StackSlot::Type::GENERIC, 8, 1);

        return mcode::Parameter{
            .type = type,
            .storage = m_func.get_stack_frame().new_stack_slot(slot),
        };
    }
}

mcode::BasicBlock SSALowerer::lower_basic_block(ssa::BasicBlock &basic_block) {
    mcode::BasicBlock machine_basic_block(basic_block.get_label(), machine_func);

    for (ssa::VirtualRegister reg : basic_block.get_param_regs()) {
        machine_basic_block.get_params().push_back(reg);
    }

    this->machine_basic_block = &machine_basic_block;

    basic_block_context = {
        .basic_block = &machine_basic_block,
        .insertion_iter = machine_basic_block.begin(),
        .regs = {},
    };

    for (ssa::InstrIter iter = basic_block.get_instrs().get_last_iter(); iter != basic_block.get_header(); --iter) {
        if (iter->get_dest() && context.reg_use_counts[*iter->get_dest()] == 0) {
            continue;
        }

        instr_iter = iter;
        basic_block_context.insertion_iter = machine_basic_block.begin();
        lower_instr(*iter);
    }

    return machine_basic_block;
}

void SSALowerer::store_graphs(ssa::Function &ir_func, mcode::Function *machine_func, const BlockMap &block_map) {
    ssa::ControlFlowGraph cfg(func);
    ssa::DominatorTree domtree(cfg);

    for (unsigned i = 0; i < cfg.get_nodes().size(); i++) {
        ssa::ControlFlowGraph::Node &cfg_node = cfg.get_node(i);
        ssa::DominatorTree::Node &domtree_node = domtree.get_node(i);
        mcode::BasicBlockIter m_iter = block_map.at(cfg_node.block);

        for (unsigned pred : cfg_node.predecessors) {
            m_iter->get_predecessors().push_back(block_map.at(cfg.get_node(pred).block));
        }

        for (unsigned succ : cfg_node.successors) {
            m_iter->get_successors().push_back(block_map.at(cfg.get_node(succ).block));
        }

        ssa::BasicBlockIter domtree_parent_iter = cfg.get_node(domtree_node.parent_index).block;
        m_iter->set_domtree_parent(block_map.at(domtree_parent_iter));

        for (unsigned domtree_child : domtree_node.children_indices) {
            ssa::BasicBlockIter domtree_child_iter = cfg.get_node(domtree_child).block;
            m_iter->get_domtree_children().push_back(block_map.at(domtree_child_iter));
        }
    }
}

void SSALowerer::lower_instr(ssa::Instruction &instr) {
    switch (instr.get_opcode()) {
        case ssa::Opcode::LOAD: lower_load(instr); break;
        case ssa::Opcode::STORE: lower_store(instr); break;
        case ssa::Opcode::LOADARG: lower_loadarg(instr); break;
        case ssa::Opcode::ADD: lower_add(instr); break;
        case ssa::Opcode::SUB: lower_sub(instr); break;
        case ssa::Opcode::MUL: lower_mul(instr); break;
        case ssa::Opcode::SDIV: lower_sdiv(instr); break;
        case ssa::Opcode::SREM: lower_srem(instr); break;
        case ssa::Opcode::UDIV: lower_udiv(instr); break;
        case ssa::Opcode::UREM: lower_urem(instr); break;
        case ssa::Opcode::FADD: lower_fadd(instr); break;
        case ssa::Opcode::FSUB: lower_fsub(instr); break;
        case ssa::Opcode::FMUL: lower_fmul(instr); break;
        case ssa::Opcode::FDIV: lower_fdiv(instr); break;
        case ssa::Opcode::AND: lower_and(instr); break;
        case ssa::Opcode::OR: lower_or(instr); break;
        case ssa::Opcode::XOR: lower_xor(instr); break;
        case ssa::Opcode::SHL: lower_shl(instr); break;
        case ssa::Opcode::SHR: lower_shr(instr); break;
        case ssa::Opcode::JMP: lower_jmp(instr); break;
        case ssa::Opcode::CJMP: lower_cjmp(instr); break;
        case ssa::Opcode::FCJMP: lower_fcjmp(instr); break;
        case ssa::Opcode::SELECT: lower_select(instr); break;
        case ssa::Opcode::CALL: lower_call(instr); break;
        case ssa::Opcode::RET: lower_ret(instr); break;
        case ssa::Opcode::UEXTEND: lower_uextend(instr); break;
        case ssa::Opcode::SEXTEND: lower_sextend(instr); break;
        case ssa::Opcode::TRUNCATE: lower_truncate(instr); break;
        case ssa::Opcode::FPROMOTE: lower_fpromote(instr); break;
        case ssa::Opcode::FDEMOTE: lower_fdemote(instr); break;
        case ssa::Opcode::UTOF: lower_utof(instr); break;
        case ssa::Opcode::STOF: lower_stof(instr); break;
        case ssa::Opcode::FTOU: lower_ftou(instr); break;
        case ssa::Opcode::FTOS: lower_ftos(instr); break;
        case ssa::Opcode::OFFSETPTR: lower_offsetptr(instr); break;
        case ssa::Opcode::MEMBERPTR: lower_memberptr(instr); break;
        case ssa::Opcode::COPY: lower_copy(instr); break;
        case ssa::Opcode::SQRT: lower_sqrt(instr); break;
        default: break;
    }
}

void SSALowerer::lower_globals() {
    for (ssa::Global *global : module_->get_globals()) {
        mcode::Global m_global{
            .name = global->name,
            .size = get_size(global->type),
            .alignment = get_alignment(global->type),
            .value = {},
        };

        ssa::Global::Value &value = global->initial_value;

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

        machine_module.add(m_global);

        if (global->external) {
            machine_module.add_global_symbol(m_global.name);
        }
    }
}

void SSALowerer::lower_external_funcs() {
    for (ssa::FunctionDecl *external_function : module_->get_external_functions()) {
        machine_module.add_external_symbol(external_function->name);
    }
}

void SSALowerer::lower_external_globals() {
    for (ssa::GlobalDecl *external_global : module_->get_external_globals()) {
        machine_module.add_external_symbol(external_global->name);
    }
}

void SSALowerer::lower_dll_exports() {
    for (const std::string &dll_export : module_->get_dll_exports()) {
        machine_module.add_dll_export(dll_export);
    }
}

mcode::InstrIter SSALowerer::emit(mcode::Instruction instr) {
    return basic_block_context.basic_block->insert_before(basic_block_context.insertion_iter, std::move(instr));
}

std::variant<mcode::Register, mcode::StackSlotID> SSALowerer::map_vreg(ssa::VirtualRegister reg) {
    auto iter = context.stack_regs.find(reg);

    if (iter != context.stack_regs.end()) {
        return iter->second;
    } else {
        return mcode::Register::from_virtual(reg);
    }
}

mcode::Register SSALowerer::map_vreg_as_reg(ssa::VirtualRegister reg) {
    ASSERT(!context.stack_regs.contains(reg));
    return mcode::Register::from_virtual(reg);
}

mcode::Operand SSALowerer::map_vreg_as_operand(ssa::VirtualRegister reg, unsigned size) {
    auto iter = context.stack_regs.find(reg);

    if (iter != context.stack_regs.end()) {
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
    return mcode::Register::from_virtual(func->next_virtual_reg());
}

void SSALowerer::lower_alloca(ssa::Instruction &instr) {
    unsigned size = std::max(get_size(instr.get_operand(0).get_type()), 8u);
    bool is_arg_store = instr.get_attr() == ssa::Instruction::Attribute::ARG_STORE;
    mcode::StackSlot::Type type = is_arg_store ? mcode::StackSlot::Type::ARG_STORE : mcode::StackSlot::Type::GENERIC;
    mcode::StackSlot slot(type, size, 1);

    long index = get_machine_func()->get_stack_frame().new_stack_slot(slot);
    context.stack_regs.insert({*instr.get_dest(), index});
}

void SSALowerer::lower_load(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("load");
}

void SSALowerer::lower_store(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("store");
}

void SSALowerer::lower_loadarg(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("loadarg");
}

void SSALowerer::lower_add(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("add");
}

void SSALowerer::lower_sub(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("sub");
}

void SSALowerer::lower_mul(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("mul");
}

void SSALowerer::lower_sdiv(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("sdiv");
}

void SSALowerer::lower_srem(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("srem");
}

void SSALowerer::lower_udiv(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("udiv");
}

void SSALowerer::lower_urem(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("urem");
}

void SSALowerer::lower_fadd(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("fadd");
}

void SSALowerer::lower_fsub(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("fsub");
}

void SSALowerer::lower_fmul(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("fmul");
}

void SSALowerer::lower_fdiv(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("fdiv");
}

void SSALowerer::lower_and(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("and");
}

void SSALowerer::lower_or(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("or");
}

void SSALowerer::lower_xor(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("xor");
}

void SSALowerer::lower_shl(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("shl");
}

void SSALowerer::lower_shr(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("shr");
}

void SSALowerer::lower_jmp(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("jmp");
}

void SSALowerer::lower_cjmp(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("cjmp");
}

void SSALowerer::lower_fcjmp(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("fcjmp");
}

void SSALowerer::lower_select(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("select");
}

void SSALowerer::lower_call(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("call");
}

void SSALowerer::lower_ret(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("ret");
}

void SSALowerer::lower_uextend(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("uextend");
}

void SSALowerer::lower_sextend(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("sextend");
}

void SSALowerer::lower_truncate(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("truncate");
}

void SSALowerer::lower_fpromote(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("fpromote");
}

void SSALowerer::lower_fdemote(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("fdemote");
}

void SSALowerer::lower_utof(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("utof");
}

void SSALowerer::lower_stof(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("stof");
}

void SSALowerer::lower_ftou(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("ftou");
}

void SSALowerer::lower_ftos(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("ftos");
}

void SSALowerer::lower_offsetptr(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("offsetptr");
}

void SSALowerer::lower_memberptr(ssa::Instruction &) {
    WARN_UNIMPLEMENTED("memberptr");
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
    if (iter != basic_block_iter->end()) {
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

    return basic_block_iter->end();
}

unsigned SSALowerer::get_num_uses(ssa::VirtualRegister reg) {
    return context.reg_use_counts[reg];
}

void SSALowerer::discard_use(ssa::VirtualRegister reg) {
    context.reg_use_counts[reg] -= 1;
}

} // namespace codegen

} // namespace banjo
