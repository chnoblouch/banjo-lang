#include "ir_lowerer.hpp"

#include "banjo/ir/control_flow_graph.hpp"
#include "banjo/utils/timing.hpp"

#include <iostream>
#include <unordered_map>

#define WARN_UNIMPLEMENTED(instruction) std::cerr << "warning: cannot lower instruction " << (instruction) << '\n';

namespace banjo {

namespace codegen {

IRLowerer::IRLowerer(target::Target *target) : target(target) {}

mcode::Module IRLowerer::lower_module(ir::Module &module_) {
    PROFILE_SCOPE("ir lowering");

    this->module_ = &module_;
    init_module(module_);

    lower_funcs();
    lower_globals();
    lower_external_funcs();
    lower_external_globals();
    lower_dll_exports();

    return std::move(machine_module);
}

void IRLowerer::lower_funcs() {
    for (ir::Function *func : module_->get_functions()) {
        this->func = func;

        mcode::CallingConvention *calling_conv = get_calling_convention(func->get_calling_conv());
        mcode::Function *machine_func = new mcode::Function(func->get_name(), calling_conv);
        this->machine_func = machine_func;

        context = {};

        std::vector<mcode::ArgStorage> storage = calling_conv->get_arg_storage(func->get_params());

        for (unsigned int i = 0; i < func->get_params().size(); i++) {
            mcode::Parameter param = lower_param(storage[i], machine_func);
            machine_func->get_parameters().push_back(param);
        }

        const LinkedList<ir::BasicBlock> &basic_blocks = func->get_basic_blocks();
        BlockMap block_map;

        context.reg_use_counts.clear();

        for (ir::BasicBlock &basic_block : *func) {
            for (ir::Instruction &instr : basic_block.get_instrs()) {
                for (ir::Operand &operand : instr.get_operands()) {
                    if (operand.is_register()) {
                        context.reg_use_counts[operand.get_register()]++;
                    }

                    if (operand.is_branch_target()) {
                        for (ir::Operand &arg : operand.get_branch_target().args) {
                            if (arg.is_register()) {
                                context.reg_use_counts[arg.get_register()]++;
                            }
                        }
                    }
                }

                if (instr.get_opcode() == ir::Opcode::ALLOCA) {
                    lower_alloca(instr);
                }
            }
        }

        for (ir::BasicBlockIter iter = basic_blocks.begin(); iter != basic_blocks.end(); ++iter) {
            basic_block_iter = iter;
            mcode::BasicBlock m_block = lower_basic_block(*iter);
            mcode::BasicBlockIter m_iter = machine_func->get_basic_blocks().append(m_block);
            block_map.insert({iter, m_iter});
        }

        store_graphs(*func, machine_func, block_map);

        machine_module.add(std::move(machine_func));
        if (func->is_global()) {
            machine_module.add_global_symbol(func->get_name());
        }
    }
}

mcode::Parameter IRLowerer::lower_param(mcode::ArgStorage storage, mcode::Function *machine_func) {
    if (storage.in_reg) {
        return mcode::Parameter(mcode::Register::from_physical(storage.reg), -1);
    } else {
        mcode::StackSlot slot(mcode::StackSlot::Type::GENERIC, 8, 1);
        long slot_index = machine_func->get_stack_frame().new_stack_slot(slot);
        return mcode::Parameter(mcode::Register::from_stack_slot(slot_index), slot_index);
    }
}

mcode::BasicBlock IRLowerer::lower_basic_block(ir::BasicBlock &basic_block) {
    mcode::BasicBlock machine_basic_block(basic_block.get_label(), machine_func);

    for (ir::VirtualRegister reg : basic_block.get_param_regs()) {
        machine_basic_block.get_params().push_back(reg);
    }

    this->machine_basic_block = &machine_basic_block;

    basic_block_context = {
        .basic_block = &machine_basic_block,
        .insertion_iter = machine_basic_block.begin(),
        .regs = {},
    };

    for (ir::InstrIter iter = basic_block.get_instrs().get_last_iter(); iter != basic_block.get_header(); --iter) {
        if (iter->get_dest() && context.reg_use_counts[*iter->get_dest()] == 0) {
            continue;
        }

        instr_iter = iter;
        basic_block_context.insertion_iter = machine_basic_block.begin();
        lower_instr(*iter);
    }

    return machine_basic_block;
}

void IRLowerer::store_graphs(ir::Function &ir_func, mcode::Function *machine_func, const BlockMap &block_map) {
    ir::ControlFlowGraph cfg(func);
    ir::DominatorTree domtree(cfg);

    for (unsigned i = 0; i < cfg.get_nodes().size(); i++) {
        ir::ControlFlowGraph::Node &cfg_node = cfg.get_node(i);
        ir::DominatorTree::Node &domtree_node = domtree.get_node(i);
        mcode::BasicBlockIter m_iter = block_map.at(cfg_node.block);

        for (unsigned pred : cfg_node.predecessors) {
            m_iter->get_predecessors().push_back(block_map.at(cfg.get_node(pred).block));
        }

        for (unsigned succ : cfg_node.successors) {
            m_iter->get_successors().push_back(block_map.at(cfg.get_node(succ).block));
        }

        ir::BasicBlockIter domtree_parent_iter = cfg.get_node(domtree_node.parent_index).block;
        m_iter->set_domtree_parent(block_map.at(domtree_parent_iter));

        for (unsigned domtree_child : domtree_node.children_indices) {
            ir::BasicBlockIter domtree_child_iter = cfg.get_node(domtree_child).block;
            m_iter->get_domtree_children().push_back(block_map.at(domtree_child_iter));
        }
    }
}

void IRLowerer::lower_instr(ir::Instruction &instr) {
    switch (instr.get_opcode()) {
        case ir::Opcode::LOAD: lower_load(instr); break;
        case ir::Opcode::STORE: lower_store(instr); break;
        case ir::Opcode::LOADARG: lower_loadarg(instr); break;
        case ir::Opcode::ADD: lower_add(instr); break;
        case ir::Opcode::SUB: lower_sub(instr); break;
        case ir::Opcode::MUL: lower_mul(instr); break;
        case ir::Opcode::SDIV: lower_sdiv(instr); break;
        case ir::Opcode::SREM: lower_srem(instr); break;
        case ir::Opcode::UDIV: lower_udiv(instr); break;
        case ir::Opcode::UREM: lower_urem(instr); break;
        case ir::Opcode::FADD: lower_fadd(instr); break;
        case ir::Opcode::FSUB: lower_fsub(instr); break;
        case ir::Opcode::FMUL: lower_fmul(instr); break;
        case ir::Opcode::FDIV: lower_fdiv(instr); break;
        case ir::Opcode::AND: lower_and(instr); break;
        case ir::Opcode::OR: lower_or(instr); break;
        case ir::Opcode::XOR: lower_xor(instr); break;
        case ir::Opcode::SHL: lower_shl(instr); break;
        case ir::Opcode::SHR: lower_shr(instr); break;
        case ir::Opcode::JMP: lower_jmp(instr); break;
        case ir::Opcode::CJMP: lower_cjmp(instr); break;
        case ir::Opcode::FCJMP: lower_fcjmp(instr); break;
        case ir::Opcode::SELECT: lower_select(instr); break;
        case ir::Opcode::CALL: lower_call(instr); break;
        case ir::Opcode::RET: lower_ret(instr); break;
        case ir::Opcode::UEXTEND: lower_uextend(instr); break;
        case ir::Opcode::SEXTEND: lower_sextend(instr); break;
        case ir::Opcode::TRUNCATE: lower_truncate(instr); break;
        case ir::Opcode::FPROMOTE: lower_fpromote(instr); break;
        case ir::Opcode::FDEMOTE: lower_fdemote(instr); break;
        case ir::Opcode::UTOF: lower_utof(instr); break;
        case ir::Opcode::STOF: lower_stof(instr); break;
        case ir::Opcode::FTOU: lower_ftou(instr); break;
        case ir::Opcode::FTOS: lower_ftos(instr); break;
        case ir::Opcode::OFFSETPTR: lower_offsetptr(instr); break;
        case ir::Opcode::MEMBERPTR: lower_memberptr(instr); break;
        case ir::Opcode::COPY: lower_copy(instr); break;
        case ir::Opcode::SQRT: lower_sqrt(instr); break;
        default: break;
    }
}

void IRLowerer::lower_globals() {
    for (ir::Global &global : module_->get_globals()) {
        mcode::Value val;
        if (global.get_initial_value().is_symbol()) {
            val = mcode::Value::from_symbol(global.get_initial_value().get_symbol_name());
        } else {
            val = lower_global_value(global.get_initial_value());
        }

        machine_module.add(mcode::Global(global.get_name(), val));

        if (global.is_external()) {
            machine_module.add_global_symbol(global.get_name());
        }
    }
}

void IRLowerer::lower_external_funcs() {
    for (ir::FunctionDecl &external_function : module_->get_external_functions()) {
        machine_module.add_external_symbol(external_function.get_name());
    }
}

void IRLowerer::lower_external_globals() {
    for (ir::GlobalDecl &external_global : module_->get_external_globals()) {
        machine_module.add_external_symbol(external_global.get_name());
    }
}

void IRLowerer::lower_dll_exports() {
    for (const std::string &dll_export : module_->get_dll_exports()) {
        machine_module.add_dll_export(dll_export);
    }
}

mcode::InstrIter IRLowerer::emit(mcode::Instruction instr) {
    return basic_block_context.basic_block->insert_before(basic_block_context.insertion_iter, std::move(instr));
}

mcode::Register IRLowerer::lower_reg(ir::VirtualRegister reg) {
    auto it = context.stack_regs.find(reg);

    if (it != context.stack_regs.end()) {
        return mcode::Register::from_stack_slot(it->second);
    } else {
        return mcode::Register::from_virtual(reg);
    }
}

unsigned IRLowerer::get_size(const ir::Type &type) {
    return target->get_data_layout().get_size(type);
}

unsigned IRLowerer::get_alignment(const ir::Type &type) {
    return target->get_data_layout().get_alignment(type);
}

unsigned IRLowerer::get_member_offset(ir::Structure *struct_, unsigned index) {
    return target->get_data_layout().get_member_offset(struct_, index);
}

unsigned IRLowerer::get_member_offset(const std::vector<ir::Type> &types, unsigned index) {
    return target->get_data_layout().get_member_offset(types, index);
}

mcode::Register IRLowerer::create_tmp_reg() {
    return mcode::Register::from_virtual(func->next_virtual_reg());
}

void IRLowerer::lower_alloca(ir::Instruction &instr) {
    unsigned size = std::max(get_size(instr.get_operand(0).get_type()), 8u);
    bool is_arg_store = instr.is_flag(ir::Instruction::FLAG_ARG_STORE);
    mcode::StackSlot::Type type = is_arg_store ? mcode::StackSlot::Type::ARG_STORE : mcode::StackSlot::Type::GENERIC;
    mcode::StackSlot slot(type, size, 1);

    long index = get_machine_func()->get_stack_frame().new_stack_slot(slot);
    context.stack_regs.insert({*instr.get_dest(), index});
}

void IRLowerer::lower_load(ir::Instruction &) {
    WARN_UNIMPLEMENTED("load");
}

void IRLowerer::lower_store(ir::Instruction &) {
    WARN_UNIMPLEMENTED("store");
}

void IRLowerer::lower_loadarg(ir::Instruction &) {
    WARN_UNIMPLEMENTED("loadarg");
}

void IRLowerer::lower_add(ir::Instruction &) {
    WARN_UNIMPLEMENTED("add");
}

void IRLowerer::lower_sub(ir::Instruction &) {
    WARN_UNIMPLEMENTED("sub");
}

void IRLowerer::lower_mul(ir::Instruction &) {
    WARN_UNIMPLEMENTED("mul");
}

void IRLowerer::lower_sdiv(ir::Instruction &) {
    WARN_UNIMPLEMENTED("sdiv");
}

void IRLowerer::lower_srem(ir::Instruction &) {
    WARN_UNIMPLEMENTED("srem");
}

void IRLowerer::lower_udiv(ir::Instruction &) {
    WARN_UNIMPLEMENTED("udiv");
}

void IRLowerer::lower_urem(ir::Instruction &) {
    WARN_UNIMPLEMENTED("urem");
}

void IRLowerer::lower_fadd(ir::Instruction &) {
    WARN_UNIMPLEMENTED("fadd");
}

void IRLowerer::lower_fsub(ir::Instruction &) {
    WARN_UNIMPLEMENTED("fsub");
}

void IRLowerer::lower_fmul(ir::Instruction &) {
    WARN_UNIMPLEMENTED("fmul");
}

void IRLowerer::lower_fdiv(ir::Instruction &) {
    WARN_UNIMPLEMENTED("fdiv");
}

void IRLowerer::lower_and(ir::Instruction &) {
    WARN_UNIMPLEMENTED("and");
}

void IRLowerer::lower_or(ir::Instruction &) {
    WARN_UNIMPLEMENTED("or");
}

void IRLowerer::lower_xor(ir::Instruction &) {
    WARN_UNIMPLEMENTED("xor");
}

void IRLowerer::lower_shl(ir::Instruction &) {
    WARN_UNIMPLEMENTED("shl");
}

void IRLowerer::lower_shr(ir::Instruction &) {
    WARN_UNIMPLEMENTED("shr");
}

void IRLowerer::lower_jmp(ir::Instruction &) {
    WARN_UNIMPLEMENTED("jmp");
}

void IRLowerer::lower_cjmp(ir::Instruction &) {
    WARN_UNIMPLEMENTED("cjmp");
}

void IRLowerer::lower_fcjmp(ir::Instruction &) {
    WARN_UNIMPLEMENTED("fcjmp");
}

void IRLowerer::lower_select(ir::Instruction &) {
    WARN_UNIMPLEMENTED("select");
}

void IRLowerer::lower_call(ir::Instruction &) {
    WARN_UNIMPLEMENTED("call");
}

void IRLowerer::lower_ret(ir::Instruction &) {
    WARN_UNIMPLEMENTED("ret");
}

void IRLowerer::lower_uextend(ir::Instruction &) {
    WARN_UNIMPLEMENTED("uextend");
}

void IRLowerer::lower_sextend(ir::Instruction &) {
    WARN_UNIMPLEMENTED("sextend");
}

void IRLowerer::lower_truncate(ir::Instruction &) {
    WARN_UNIMPLEMENTED("truncate");
}

void IRLowerer::lower_fpromote(ir::Instruction &) {
    WARN_UNIMPLEMENTED("fpromote");
}

void IRLowerer::lower_fdemote(ir::Instruction &) {
    WARN_UNIMPLEMENTED("fdemote");
}

void IRLowerer::lower_utof(ir::Instruction &) {
    WARN_UNIMPLEMENTED("utof");
}

void IRLowerer::lower_stof(ir::Instruction &) {
    WARN_UNIMPLEMENTED("stof");
}

void IRLowerer::lower_ftou(ir::Instruction &) {
    WARN_UNIMPLEMENTED("ftou");
}

void IRLowerer::lower_ftos(ir::Instruction &) {
    WARN_UNIMPLEMENTED("ftos");
}

void IRLowerer::lower_offsetptr(ir::Instruction &) {
    WARN_UNIMPLEMENTED("offsetptr");
}

void IRLowerer::lower_memberptr(ir::Instruction &) {
    WARN_UNIMPLEMENTED("memberptr");
}

void IRLowerer::lower_copy(ir::Instruction &instr) {
    ir::Operand func_operand = ir::Operand::from_extern_func("memcpy", ir::Primitive::VOID);
    ir::Operand dst_operand = instr.get_operand(0);
    ir::Operand src_operand = instr.get_operand(1);

    unsigned size = target->get_data_layout().get_size(instr.get_operand(2).get_type());
    ir::Operand size_operand = ir::Operand::from_int_immediate(size, target->get_data_layout().get_usize_type());

    ir::Instruction call_instr(ir::Opcode::CALL, {func_operand, dst_operand, src_operand, size_operand});
    lower_call(call_instr);
}

void IRLowerer::lower_sqrt(ir::Instruction &instr) {
    ir::Operand func_operand = ir::Operand::from_extern_func("sqrt", ir::Primitive::F32);
    ir::Operand input_operand = instr.get_operand(0);
    ir::VirtualRegister output_reg = *instr.get_dest();

    ir::Instruction call_instr(ir::Opcode::CALL, output_reg, {func_operand, input_operand});
    lower_call(call_instr);
}

ir::InstrIter IRLowerer::get_producer(ir::VirtualRegister reg) {
    ir::BasicBlock &cur_block = get_block();

    for (ir::InstrIter iter = cur_block.get_trailer().get_prev(); iter != cur_block.get_header(); --iter) {
        if (iter->get_dest() == reg) {
            return iter;
        }
    }

    return cur_block.end();
}

ir::InstrIter IRLowerer::get_producer_globally(ir::VirtualRegister reg) {
    ir::InstrIter iter = get_producer(reg);
    if (iter != basic_block_iter->end()) {
        return iter;
    }

    /*
    for (ir::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        if (block_iter == basic_block_iter) {
            continue;
        }

        for (ir::InstrIter iter = block_iter->begin(); iter != block_iter->end(); ++iter) {
            if (iter->get_dest() == reg) {
                return iter;
            }
        }
    }
    */

    return basic_block_iter->end();
}

unsigned IRLowerer::get_num_uses(ir::VirtualRegister reg) {
    return context.reg_use_counts[reg];
}

void IRLowerer::discard_use(ir::VirtualRegister reg) {
    context.reg_use_counts[reg] -= 1;
}

} // namespace codegen

} // namespace banjo
