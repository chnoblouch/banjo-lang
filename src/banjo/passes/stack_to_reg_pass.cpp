#include "stack_to_reg_pass.hpp"

#include "banjo/ssa/dead_code_elimination.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "pass_utils.hpp"
#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"

#include <unordered_set>

namespace banjo {

namespace passes {

StackToRegPass::StackToRegPass(target::Target *target) : Pass("stack-to-reg", target) {}

void StackToRegPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func, mod);
    }
}

void StackToRegPass::run(ssa::Function *func, ssa::Module &mod) {
    ssa::ControlFlowGraph cfg(func);

    if (is_logging()) {
        get_logging_stream() << "--- CFG FOR " << func->get_name() << " ---" << std::endl;
        cfg.dump(get_logging_stream());
        get_logging_stream() << std::endl;
    }

    ssa::DominatorTree dt(cfg);

    if (is_logging()) {
        get_logging_stream() << "--- DOMINATOR TREE FOR " << func->get_name() << " ---" << std::endl;
        dt.dump(get_logging_stream());
        get_logging_stream() << std::endl;
    }

    StackSlotMap stack_slots = find_stack_slots(func);
    BlockMap blocks;

    std::unordered_map<long, ssa::Value> init_replacements;

    for (auto iter : stack_slots) {
        ssa::VirtualRegister var = iter.first;
        StackSlotInfo &stack_slot = iter.second;

        if (stack_slot.def_blocks.empty()) {
            ssa::Value replacement = stack_slot.type.is_floating_point()
                                        ? ssa::Value::from_fp_immediate(0.0, stack_slot.type)
                                        : ssa::Value::from_int_immediate(0, stack_slot.type);
            init_replacements[var] = replacement;
            stack_slot.cur_replacement = replacement;
            continue;
        }

        std::unordered_set<ssa::BasicBlockIter> def_blocks_analyzed;
        for (ssa::BasicBlockIter def_block : stack_slot.def_blocks) {
            def_blocks_analyzed.insert(def_block);
        }

        for (ssa::BasicBlockIter def_block_iter : stack_slot.def_blocks) {
            for (unsigned index : dt.get_node(def_block_iter).dominance_frontiers) {
                ssa::ControlFlowGraph::Node &cfg_node = cfg.get_node(index);

                if (stack_slot.blocks_having_val_as_param.contains(cfg_node.block)) {
                    continue;
                }

                std::unordered_set<unsigned> nodes_visited;
                if (!is_stack_slot_used(stack_slot, cfg, index, nodes_visited)) {
                    continue;
                }

                blocks[cfg_node.block].new_params.push_back({
                    .param_index = (unsigned)cfg_node.block->get_param_regs().size(),
                    .stack_slot = var,
                });

                ssa::VirtualRegister param_reg = 1000 + func->next_virtual_reg();
                cfg_node.block->get_param_regs().push_back(param_reg);
                cfg_node.block->get_param_types().push_back(stack_slot.type);

                stack_slot.blocks_having_val_as_param.insert(cfg_node.block);

                ssa::Value replacement = stack_slot.type.is_floating_point()
                                            ? ssa::Value::from_fp_immediate(0.0, stack_slot.type)
                                            : ssa::Value::from_int_immediate(0, stack_slot.type);
                init_replacements[var] = replacement;
                stack_slot.cur_replacement = replacement;

                if (!def_blocks_analyzed.contains(cfg_node.block)) {
                    stack_slot.def_blocks.push_back(cfg_node.block);
                    def_blocks_analyzed.insert(cfg_node.block);
                }
            }
        }
    }

    if (is_logging()) {
        get_logging_stream() << std::endl;
    }

    rename(func->begin(), stack_slots, blocks, init_replacements, dt);

    Precomputing::precompute_instrs(*func);
    ssa::DeadCodeElimination().run(*func);
}

StackToRegPass::StackSlotMap StackToRegPass::find_stack_slots(ssa::Function *func) {
    StackSlotMap stack_slots;

    for (ssa::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        for (ssa::Instruction &instr : block_iter->get_instrs()) {
            if (instr.get_opcode() == ssa::Opcode::ALLOCA) {
                ssa::VirtualRegister dest = *instr.get_dest();
                ssa::Type type = instr.get_operand(0).get_type();

                if (get_target()->get_data_layout().fits_in_register(type)) {
                    stack_slots.insert({dest, StackSlotInfo{.type = type, .def_blocks = {}, .promotable = true}});
                }
            } else if (instr.get_opcode() == ssa::Opcode::STORE) {
                if (instr.get_operand(1).is_register()) {
                    ssa::Operand &dst = instr.get_operand(1);
                    if (dst.is_register() && stack_slots.contains(dst.get_register())) {
                        stack_slots[dst.get_register()].def_blocks.push_back(block_iter);
                    }
                }
            } else {
                PassUtils::iter_regs(
                    instr.get_operands(),
                    [&instr, &stack_slots, &block_iter](ssa::VirtualRegister reg) {
                        auto iter = stack_slots.find(reg);
                        if (iter == stack_slots.end()) {
                            return;
                        }

                        bool is_def = false;

                        for (auto a : iter->second.def_blocks) {
                            if (a == block_iter) {
                                is_def = true;
                                break;
                            }
                        }

                        if (!is_def) {
                            iter->second.use_blocks.insert(block_iter);
                        }

                        // Can't promote registers whose address is stored somewhere.
                        if (instr.get_opcode() != ssa::Opcode::LOAD) {
                            iter->second.promotable = false;
                        }
                    }
                );
            }
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

bool StackToRegPass::is_stack_slot_used(
    StackSlotInfo &stack_slot,
    ssa::ControlFlowGraph &cfg,
    unsigned node_index,
    std::unordered_set<unsigned> &nodes_visited
) {
    ssa::ControlFlowGraph::Node &node = cfg.get_node(node_index);

    if (stack_slot.use_blocks.contains(node.block)) {
        return true;
    }

    nodes_visited.insert(node_index);

    for (unsigned succ : node.successors) {
        if (nodes_visited.contains(succ)) {
            continue;
        }

        if (is_stack_slot_used(stack_slot, cfg, succ, nodes_visited)) {
            return true;
        }
    }

    return false;
}

void StackToRegPass::rename(
    ssa::BasicBlockIter block_iter,
    StackSlotMap &stack_slots,
    BlockMap &blocks,
    std::unordered_map<long, ssa::Value> cur_replacements,
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

        if (iter->get_opcode() == ssa::Opcode::ALLOCA && stack_slots[*iter->get_dest()].promotable) {
            block.remove(iter);
            iter = prev;
        } else if (iter->get_opcode() == ssa::Opcode::STORE && iter->get_operand(1).is_register()) {
            ssa::VirtualRegister store_reg = iter->get_operand(1).get_register();

            if (stack_slots.contains(store_reg) && stack_slots[store_reg].promotable) {
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

            if (stack_slots.contains(load_reg) && stack_slots[load_reg].promotable) {
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
        rename(child_block, stack_slots, blocks, cur_replacements, dominator_tree);
    }
}

void StackToRegPass::replace_regs(
    std::vector<ssa::Operand> &operands,
    std::unordered_map<long, ssa::Value> cur_replacements
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
    std::unordered_map<long, ssa::Value> cur_replacements
) {
    ssa::BranchTarget &target = operand.get_branch_target();

    for (ParamInfo &param : blocks[target.block].new_params) {
        target.args.push_back(cur_replacements[param.stack_slot]);
    }
}

} // namespace passes

} // namespace banjo
