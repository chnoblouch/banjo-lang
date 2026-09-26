#include "stack_to_reg_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/dead_code_elimination.hpp"
#include "banjo/ssa/virtual_register.hpp"

namespace banjo::passes {

static ssa::Value create_undefined(ssa::Type type) {
    if (type.is_floating_point()) {
        return ssa::Value::from_fp_immediate(0.0, type);
    } else {
        return ssa::Value::from_int_immediate(0, type);
    }
}

StackToRegPass::StackToRegPass(target::Target *target)
  : Pass{"stack-to-reg", target},
    data_layout{target->get_data_layout()} {}

void StackToRegPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void StackToRegPass::run(ssa::Function &func) {
    this->func = &func;

    cfg = ssa::ControlFlowGraph::build(func);

    if (is_logging()) {
        log() << "--- CFG FOR " << func.name << " ---\n";
        cfg.dump(log());
        log() << '\n';
    }

    domtree = ssa::DominatorTree::build(cfg);

    if (is_logging()) {
        log() << "--- DOMINATOR TREE FOR " << func.name << " ---\n";
        domtree.dump(cfg, log());
        log() << '\n';
    }

    StackSlotMap slots = find_stack_slots();
    BlockMap blocks;
    ValueMap init_replacements;

    for (auto &[reg, slot] : slots) {
        if (slot.store_nodes.empty()) {
            ssa::Value replacement = create_undefined(slot.type);
            init_replacements[reg] = replacement;
            slot.cur_replacement = replacement;
            continue;
        }

        BitSet store_blocks_analyzed = slot.store_nodes;

        for (ssa::ControlFlowGraph::NodeID store_node : slot.store_nodes) {
            for (ssa::ControlFlowGraph::NodeID node : domtree.nodes[store_node].dominance_frontiers) {
                ssa::BasicBlockIter block = cfg.block(node);

                if (slot.nodes_having_val_as_param.get(node)) {
                    continue;
                }

                BitSet nodes_visited;
                if (!is_slot_loaded(slot, node, nodes_visited)) {
                    continue;
                }

                blocks[block].new_params.push_back({
                    .param_index = static_cast<unsigned>(block->get_param_regs().size()),
                    .stack_slot = reg,
                });

                ssa::VirtualRegister param_reg = func.next_virtual_reg();
                block->get_param_regs().push_back(param_reg);
                block->get_param_types().push_back(slot.type);

                slot.nodes_having_val_as_param.set(node);

                ssa::Value replacement = create_undefined(slot.type);
                init_replacements[reg] = replacement;
                slot.cur_replacement = replacement;

                if (!store_blocks_analyzed.get(node)) {
                    slot.store_nodes.set(node);
                    store_blocks_analyzed.set(block);
                }
            }
        }
    }

    if (is_logging()) {
        log() << '\n';
    }

    rename(func.begin(), slots, blocks, init_replacements);

    Precomputing::precompute_instrs(func);
    ssa::DeadCodeElimination{}.run(func);
}

StackToRegPass::StackSlotMap StackToRegPass::find_stack_slots() {
    StackSlotMap stack_slots;

    for (ssa::BasicBlock &block : *func) {
        for (ssa::Instruction &instr : block) {
            if (instr.get_opcode() != ssa::Opcode::ALLOCA) {
                continue;
            }

            ssa::Type type = instr.get_operand(0).get_type();
            if (!get_target()->get_data_layout().fits_in_register(type)) {
                continue;
            }

            StackSlotInfo slot{
                .type = type,
                .store_nodes{},
                .promotable = true,
            };

            ssa::VirtualRegister reg = *instr.get_dest();
            stack_slots.insert({reg, slot});
        }
    }

    for (ssa::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        for (ssa::Instruction &instr : block_iter->get_instrs()) {
            find_slot_uses(stack_slots, cfg.node_id(block_iter), instr);
        }
    }

    // Remove non-promotable registers.
    for (auto iter = stack_slots.begin(); iter != stack_slots.end();) {
        if (iter->second.promotable) {
            ++iter;
        } else {
            iter = stack_slots.erase(iter);
        }
    }

    return stack_slots;
}

void StackToRegPass::find_slot_uses(StackSlotMap &slots, NodeID node, ssa::Instruction &instr) {
    ssa::Opcode opcode = instr.get_opcode();

    if (opcode == ssa::Opcode::LOAD) {
        ssa::Type type = instr.get_operand(0).get_type();
        ssa::Operand &src = instr.get_operand(1);

        if (src.is_register()) {
            analyze_reg_use(slots, src.get_register(), node, opcode);

            auto iter = slots.find(src.get_register());
            if (iter == slots.end()) {
                return;
            }

            StackSlotInfo &slot = iter->second;

            // Can't promote if a type with a different size than allocated is
            // loaded from this slot.
            if (data_layout.get_size(slot.type) != data_layout.get_size(type)) {
                slot.promotable = false;
            }
        }
    } else if (opcode == ssa::Opcode::STORE) {
        ssa::Operand &src = instr.get_operand(0);
        ssa::Operand &dst = instr.get_operand(1);

        if (src.is_register()) {
            analyze_reg_use(slots, src.get_register(), node, opcode);
        }

        if (dst.is_register()) {
            auto iter = slots.find(dst.get_register());
            if (iter == slots.end()) {
                return;
            }

            StackSlotInfo &slot = iter->second;

            // Can't promote if a type with a different size than allocated is
            // stored to this slot.
            if (data_layout.get_size(slot.type) != data_layout.get_size(src.get_type())) {
                slot.promotable = false;
            }

            slot.store_nodes.set(node);
        }
    } else if (opcode != ssa::Opcode::ALLOCA) {
        PassUtils::iter_regs(instr.get_operands(), [&](ssa::VirtualRegister reg) {
            analyze_reg_use(slots, reg, node, opcode);
        });
    }
}

void StackToRegPass::analyze_reg_use(StackSlotMap &slots, ssa::VirtualRegister reg, NodeID node, ssa::Opcode opcode) {
    auto iter = slots.find(reg);
    if (iter == slots.end()) {
        return;
    }

    bool is_store_block = false;

    for (NodeID store_node : iter->second.store_nodes) {
        if (store_node == node) {
            is_store_block = true;
            break;
        }
    }

    if (!is_store_block) {
        iter->second.load_nodes.set(node);
    }

    // Can't promote registers whose address is stored somewhere.
    if (opcode != ssa::Opcode::LOAD) {
        iter->second.promotable = false;
    }
}

bool StackToRegPass::is_slot_loaded(StackSlotInfo &slot, NodeID node, BitSet &nodes_visited) {
    if (slot.load_nodes.get(node)) {
        return true;
    }

    nodes_visited.set(node);

    for (ssa::ControlFlowGraph::NodeID succ : cfg.nodes[node].successors) {
        if (nodes_visited.get(succ)) {
            continue;
        }

        if (is_slot_loaded(slot, succ, nodes_visited)) {
            return true;
        }
    }

    return false;
}

void StackToRegPass::rename(
    ssa::BasicBlockIter block_iter,
    StackSlotMap &slots,
    BlockMap &blocks,
    ValueMap cur_replacements
) {
    ssa::BasicBlock &block = *block_iter;

    if (block_iter->has_label()) {
        for (ParamInfo param : blocks[block_iter].new_params) {
            unsigned index = param.param_index;
            ssa::Value value = ssa::Value::from_register(block.get_param_regs()[index], block.get_param_types()[index]);
            cur_replacements[param.stack_slot] = value;
        }
    }

    for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        ssa::InstrIter prev = iter.get_prev();

        if (iter->get_opcode() == ssa::Opcode::ALLOCA && slots[*iter->get_dest()].promotable) {
            block.remove(iter);
            iter = prev;
        } else if (iter->get_opcode() == ssa::Opcode::LOAD) {
            rename_in_load(block, iter, slots, cur_replacements);
        } else if (iter->get_opcode() == ssa::Opcode::STORE) {
            rename_in_store(block, iter, slots, cur_replacements);
        } else if (iter->get_opcode() == ssa::Opcode::JMP) {
            update_branch_target(iter->get_operand(0), blocks, cur_replacements);
            replace_regs(iter->get_operands(), cur_replacements);
        } else if (iter->get_opcode() == ssa::Opcode::CJMP || iter->get_opcode() == ssa::Opcode::FCJMP) {
            update_branch_target(iter->get_operand(3), blocks, cur_replacements);
            update_branch_target(iter->get_operand(4), blocks, cur_replacements);
            replace_regs(iter->get_operands(), cur_replacements);
        } else {
            replace_regs(iter->get_operands(), cur_replacements);
        }
    }

    ssa::ControlFlowGraph::NodeID cfg_node = cfg.node_id(block_iter);

    for (ssa::ControlFlowGraph::NodeID child : domtree.nodes[cfg_node].children) {
        ssa::BasicBlockIter child_block = cfg.block(child);
        rename(child_block, slots, blocks, cur_replacements);
    }
}

void StackToRegPass::rename_in_load(
    ssa::BasicBlock &block,
    ssa::InstrIter &instr,
    StackSlotMap &slots,
    ValueMap &cur_replacements
) {
    ssa::VirtualRegister dst = *instr->get_dest();
    ssa::Type type = instr->get_operand(0).get_type();
    ssa::Operand &addr = instr->get_operand(1);

    if (!addr.is_register()) {
        replace_regs(instr->get_operands(), cur_replacements);
        return;
    }

    ssa::VirtualRegister addr_reg = addr.get_register();

    if (!slots.contains(addr_reg) || !slots[addr_reg].promotable) {
        replace_regs(instr->get_operands(), cur_replacements);
        return;
    }

    ssa::Value value = cur_replacements[addr_reg];

    // TODO: Test this, and probably also insert a bitcast before loading.
    if (value.get_type().is_floating_point() != type.is_floating_point()) {
        ssa::VirtualRegister reg = func->next_virtual_reg();
        ssa::Operand type_operand = ssa::Operand::from_type(type);
        block.insert_before(instr, {ssa::Opcode::BITCAST, reg, {value, type_operand}});

        value = ssa::Operand::from_register(reg, type);
    }

    cur_replacements[dst] = value;

    ssa::InstrIter prev = instr.get_prev();
    block.remove(instr);
    instr = prev;
}

void StackToRegPass::rename_in_store(
    ssa::BasicBlock &block,
    ssa::InstrIter &instr,
    StackSlotMap &slots,
    ValueMap &cur_replacements
) {
    ssa::Operand &value = instr->get_operand(0);
    ssa::Operand &addr = instr->get_operand(1);

    if (!addr.is_register()) {
        replace_regs(instr->get_operands(), cur_replacements);
        return;
    }

    ssa::VirtualRegister addr_reg = instr->get_operand(1).get_register();

    if (!slots.contains(addr_reg) || !slots[addr_reg].promotable) {
        replace_regs(instr->get_operands(), cur_replacements);
        return;
    }

    if (value.is_register() && cur_replacements.contains(value.get_register())) {
        value = cur_replacements[value.get_register()];
    }

    cur_replacements[addr_reg] = value;

    ssa::InstrIter prev = instr.get_prev();
    block.remove(instr);
    instr = prev;
}

void StackToRegPass::replace_regs(std::vector<ssa::Operand> &operands, ValueMap cur_replacements) {
    PassUtils::iter_values(operands, [&cur_replacements](ssa::Value &value) {
        if (value.is_register() && cur_replacements.count(value.get_register())) {
            value = cur_replacements[value.get_register()];
        }
    });
}

void StackToRegPass::update_branch_target(ssa::Operand &operand, BlockMap &blocks, ValueMap cur_replacements) {
    ssa::BranchTarget &target = operand.get_branch_target();

    for (ParamInfo &param : blocks[target.block].new_params) {
        target.args.push_back(cur_replacements[param.stack_slot]);
    }
}

} // namespace banjo::passes
