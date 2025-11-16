#include "stack_to_reg_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/dead_code_elimination.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <unordered_set>

namespace banjo {

namespace passes {

StackToRegPass::StackToRegPass(target::Target *target) : Pass("stack-to-reg", target) {}

void StackToRegPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func);
    }
}

void StackToRegPass::run(ssa::Function *func) {
    ssa::ControlFlowGraph cfg(func);

    if (is_logging()) {
        log() << "--- CFG FOR " << func->name << " ---\n";
        cfg.dump(log());
        log() << '\n';
    }

    ssa::DominatorTree dt(cfg);

    if (is_logging()) {
        log() << "--- DOMINATOR TREE FOR " << func->name << " ---\n";
        dt.dump(log());
        log() << '\n';
    }

    StackSlotMap slots = find_stack_slots(func);
    BlockMap blocks;

    std::unordered_map<ssa::VirtualRegister, ssa::Value> init_replacements;

    for (auto &[reg, slot] : slots) {
        if (slot.store_blocks.empty()) {
            ssa::Value replacement = create_undefined(slot.type);
            init_replacements[reg] = replacement;
            slot.cur_replacement = replacement;
            continue;
        }

        std::unordered_set<ssa::BasicBlockIter> store_blocks_analyzed;
        for (ssa::BasicBlockIter store_block : slot.store_blocks) {
            store_blocks_analyzed.insert(store_block);
        }

        for (ssa::BasicBlockIter store_block_iter : slot.store_blocks) {
            for (unsigned index : dt.get_node(store_block_iter).dominance_frontiers) {
                ssa::ControlFlowGraph::Node &cfg_node = cfg.get_node(index);

                if (slot.blocks_having_val_as_param.contains(cfg_node.block)) {
                    continue;
                }

                std::unordered_set<unsigned> nodes_visited;
                if (!is_slot_loaded(slot, cfg, index, nodes_visited)) {
                    continue;
                }

                blocks[cfg_node.block].new_params.push_back({
                    .param_index = static_cast<unsigned>(cfg_node.block->get_param_regs().size()),
                    .stack_slot = reg,
                });

                ssa::VirtualRegister param_reg = 1000 + func->next_virtual_reg();
                cfg_node.block->get_param_regs().push_back(param_reg);
                cfg_node.block->get_param_types().push_back(slot.type);

                slot.blocks_having_val_as_param.insert(cfg_node.block);

                ssa::Value replacement = create_undefined(slot.type);
                init_replacements[reg] = replacement;
                slot.cur_replacement = replacement;

                if (!store_blocks_analyzed.contains(cfg_node.block)) {
                    slot.store_blocks.push_back(cfg_node.block);
                    store_blocks_analyzed.insert(cfg_node.block);
                }
            }
        }
    }

    if (is_logging()) {
        log() << '\n';
    }

    rename(func->begin(), slots, blocks, init_replacements, dt);

    Precomputing::precompute_instrs(*func);
    ssa::DeadCodeElimination().run(*func);
}

StackToRegPass::StackSlotMap StackToRegPass::find_stack_slots(ssa::Function *func) {
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
                .store_blocks = {},
                .promotable = true,
            };

            ssa::VirtualRegister reg = *instr.get_dest();
            stack_slots.insert({reg, slot});
        }
    }

    for (ssa::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        for (ssa::Instruction &instr : block_iter->get_instrs()) {
            find_slot_uses(stack_slots, block_iter, instr);
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

void StackToRegPass::find_slot_uses(StackSlotMap &slots, ssa::BasicBlockIter block, ssa::Instruction &instr) {
    ssa::Opcode opcode = instr.get_opcode();

    if (opcode == ssa::Opcode::LOAD) {
        ssa::Type type = instr.get_operand(0).get_type();
        ssa::Operand &src = instr.get_operand(1);

        if (src.is_register()) {
            analyze_reg_use(slots, src.get_register(), block, opcode);

            auto iter = slots.find(src.get_register());
            if (iter == slots.end()) {
                return;
            }

            StackSlotInfo &slot = iter->second;

            // Can't promote if a different type than allocated is loaded from this slot.
            if (slot.type != type) {
                slot.promotable = false;
            }
        }
    } else if (opcode == ssa::Opcode::STORE) {
        ssa::Operand &src = instr.get_operand(0);
        ssa::Operand &dst = instr.get_operand(1);

        if (src.is_register()) {
            analyze_reg_use(slots, src.get_register(), block, opcode);
        }

        if (dst.is_register()) {
            auto iter = slots.find(dst.get_register());
            if (iter == slots.end()) {
                return;
            }

            StackSlotInfo &slot = iter->second;

            // Can't promote if a different type than allocated is stored into this slot.
            if (slot.type != src.get_type()) {
                slot.promotable = false;
            }

            slot.store_blocks.push_back(block);
        }
    } else if (opcode != ssa::Opcode::ALLOCA) {
        PassUtils::iter_regs(instr.get_operands(), [this, &slots, opcode, block](ssa::VirtualRegister reg) {
            analyze_reg_use(slots, reg, block, opcode);
        });
    }
}

void StackToRegPass::analyze_reg_use(
    StackSlotMap &slots,
    ssa::VirtualRegister reg,
    ssa::BasicBlockIter block,
    ssa::Opcode opcode
) {
    auto iter = slots.find(reg);
    if (iter == slots.end()) {
        return;
    }

    bool is_store_block = false;

    for (ssa::BasicBlockIter store_block : iter->second.store_blocks) {
        if (store_block == block) {
            is_store_block = true;
            break;
        }
    }

    if (!is_store_block) {
        iter->second.load_blocks.insert(block);
    }

    // Can't promote registers whose address is stored somewhere.
    if (opcode != ssa::Opcode::LOAD) {
        iter->second.promotable = false;
    }
}

bool StackToRegPass::is_slot_loaded(
    StackSlotInfo &slot,
    ssa::ControlFlowGraph &cfg,
    unsigned node_index,
    std::unordered_set<unsigned> &nodes_visited
) {
    ssa::ControlFlowGraph::Node &node = cfg.get_node(node_index);

    if (slot.load_blocks.contains(node.block)) {
        return true;
    }

    nodes_visited.insert(node_index);

    for (unsigned succ : node.successors) {
        if (nodes_visited.contains(succ)) {
            continue;
        }

        if (is_slot_loaded(slot, cfg, succ, nodes_visited)) {
            return true;
        }
    }

    return false;
}

ssa::Value StackToRegPass::create_undefined(ssa::Type type) {
    if (type.is_floating_point()) {
        return ssa::Value::from_fp_immediate(0.0, type);
    } else {
        return ssa::Value::from_int_immediate(0, type);
    }
}

void StackToRegPass::rename(
    ssa::BasicBlockIter block_iter,
    StackSlotMap &slots,
    BlockMap &blocks,
    std::unordered_map<ssa::VirtualRegister, ssa::Value> cur_replacements,
    ssa::DominatorTree &dominator_tree
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
        } else if (iter->get_opcode() == ssa::Opcode::STORE && iter->get_operand(1).is_register()) {
            ssa::VirtualRegister store_reg = iter->get_operand(1).get_register();

            if (slots.contains(store_reg) && slots[store_reg].promotable) {
                ssa::Value &value = iter->get_operand(0);

                if (value.is_register() && cur_replacements.contains(value.get_register())) {
                    cur_replacements[store_reg] = cur_replacements[value.get_register()];
                } else {
                    cur_replacements[store_reg] = value;
                }

                block.remove(iter);
                iter = prev;
            } else {
                replace_regs(iter->get_operands(), cur_replacements);
            }
        } else if (iter->get_opcode() == ssa::Opcode::LOAD && iter->get_operand(1).is_register()) {
            ssa::VirtualRegister dst = *iter->get_dest();
            ssa::VirtualRegister load_reg = iter->get_operand(1).get_register();

            if (slots.contains(load_reg) && slots[load_reg].promotable) {
                cur_replacements[dst] = cur_replacements[load_reg];
                block.remove(iter);
                iter = prev;
            } else {
                replace_regs(iter->get_operands(), cur_replacements);
            }
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

    for (unsigned child_index : dominator_tree.get_node(block_iter).children_indices) {
        ssa::BasicBlockIter child_block = dominator_tree.get_cfg().get_nodes()[child_index].block;
        rename(child_block, slots, blocks, cur_replacements, dominator_tree);
    }
}

void StackToRegPass::replace_regs(
    std::vector<ssa::Operand> &operands,
    std::unordered_map<ssa::VirtualRegister, ssa::Value> cur_replacements
) {
    PassUtils::iter_values(operands, [&cur_replacements](ssa::Value &value) {
        if (value.is_register() && cur_replacements.count(value.get_register())) {
            value = cur_replacements[value.get_register()];
        }
    });
}

void StackToRegPass::update_branch_target(
    ssa::Operand &operand,
    BlockMap &blocks,
    std::unordered_map<ssa::VirtualRegister, ssa::Value> cur_replacements
) {
    ssa::BranchTarget &target = operand.get_branch_target();

    for (ParamInfo &param : blocks[target.block].new_params) {
        target.args.push_back(cur_replacements[param.stack_slot]);
    }
}

} // namespace passes

} // namespace banjo
