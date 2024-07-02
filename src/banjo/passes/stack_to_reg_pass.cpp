#include "stack_to_reg_pass.hpp"

#include "ir/dead_code_elimination.hpp"
#include "ir/virtual_register.hpp"
#include "pass_utils.hpp"
#include "passes/pass_utils.hpp"
#include "passes/precomputing.hpp"

#include <unordered_set>

namespace banjo {

namespace passes {

StackToRegPass::StackToRegPass(target::Target *target) : Pass("stack-to-reg", target) {}

void StackToRegPass::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(func, mod);
    }
}

void StackToRegPass::run(ir::Function *func, ir::Module &mod) {
    ir::ControlFlowGraph cfg(func);

    if (is_logging()) {
        get_logging_stream() << "--- CFG FOR " << func->get_name() << " ---" << std::endl;
        cfg.dump(get_logging_stream());
        get_logging_stream() << std::endl;
    }

    ir::DominatorTree dt(cfg);

    if (is_logging()) {
        get_logging_stream() << "--- DOMINATOR TREE FOR " << func->get_name() << " ---" << std::endl;
        dt.dump(get_logging_stream());
        get_logging_stream() << std::endl;
    }

    StackSlotMap stack_slots = find_stack_slots(func);
    BlockMap blocks;

    std::unordered_map<long, ir::Value> init_replacements;

    for (auto iter : stack_slots) {
        ir::VirtualRegister var = iter.first;
        StackSlotInfo &stack_slot = iter.second;

        if (stack_slot.def_blocks.empty()) {
            ir::Value replacement = stack_slot.type.is_floating_point()
                                        ? ir::Value::from_fp_immediate(0.0, stack_slot.type)
                                        : ir::Value::from_int_immediate(0, stack_slot.type);
            init_replacements[var] = replacement;
            stack_slot.cur_replacement = replacement;
            continue;
        }

        std::unordered_set<ir::BasicBlockIter> def_blocks_analyzed;
        for (ir::BasicBlockIter def_block : stack_slot.def_blocks) {
            def_blocks_analyzed.insert(def_block);
        }

        for (ir::BasicBlockIter def_block_iter : stack_slot.def_blocks) {
            for (unsigned index : dt.get_node(def_block_iter).dominance_frontiers) {
                ir::ControlFlowGraph::Node &cfg_node = cfg.get_node(index);

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

                ir::VirtualRegister param_reg = 1000 + func->next_virtual_reg();
                cfg_node.block->get_param_regs().push_back(param_reg);
                cfg_node.block->get_param_types().push_back(stack_slot.type);

                stack_slot.blocks_having_val_as_param.insert(cfg_node.block);

                ir::Value replacement = stack_slot.type.is_floating_point()
                                            ? ir::Value::from_fp_immediate(0.0, stack_slot.type)
                                            : ir::Value::from_int_immediate(0, stack_slot.type);
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
    ir::DeadCodeElimination().run(*func);
}

StackToRegPass::StackSlotMap StackToRegPass::find_stack_slots(ir::Function *func) {
    StackSlotMap stack_slots;

    for (ir::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        for (ir::Instruction &instr : block_iter->get_instrs()) {
            if (instr.get_opcode() == ir::Opcode::ALLOCA) {
                ir::VirtualRegister dest = *instr.get_dest();
                ir::Type type = instr.get_operand(0).get_type();

                if (get_target()->get_data_layout().fits_in_register(type)) {
                    stack_slots.insert({dest, StackSlotInfo{.type = type, .def_blocks = {}, .promotable = true}});
                }
            } else if (instr.get_opcode() == ir::Opcode::STORE) {
                if (instr.get_operand(1).is_register()) {
                    ir::Operand &dst = instr.get_operand(1);
                    if (dst.is_register() && stack_slots.contains(dst.get_register())) {
                        stack_slots[dst.get_register()].def_blocks.push_back(block_iter);
                    }
                }
            } else {
                PassUtils::iter_regs(
                    instr.get_operands(),
                    [&instr, &stack_slots, &block_iter](ir::VirtualRegister reg) {
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
                        if (instr.get_opcode() != ir::Opcode::LOAD) {
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
    ir::ControlFlowGraph &cfg,
    unsigned node_index,
    std::unordered_set<unsigned> &nodes_visited
) {
    ir::ControlFlowGraph::Node &node = cfg.get_node(node_index);

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
    ir::BasicBlockIter block_iter,
    StackSlotMap &stack_slots,
    BlockMap &blocks,
    std::unordered_map<long, ir::Value> cur_replacements,
    ir::DominatorTree &dominator_tree
) {
    ir::BasicBlock &block = *block_iter;

    if (block_iter->has_label()) {
        for (ParamInfo param : blocks[block_iter].new_params) {
            unsigned index = param.param_index;
            ir::Value value = ir::Value::from_register(block.get_param_regs()[index], block.get_param_types()[index]);
            cur_replacements[param.stack_slot] = value;
        }
    }

    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        ir::InstrIter prev = iter.get_prev();

        if (iter->get_opcode() == ir::Opcode::ALLOCA && stack_slots[*iter->get_dest()].promotable) {
            block.remove(iter);
            iter = prev;
        } else if (iter->get_opcode() == ir::Opcode::STORE && iter->get_operand(1).is_register()) {
            ir::VirtualRegister store_reg = iter->get_operand(1).get_register();

            if (stack_slots.contains(store_reg) && stack_slots[store_reg].promotable) {
                ir::Value &value = iter->get_operand(0);

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
        } else if (iter->get_opcode() == ir::Opcode::LOAD && iter->get_operand(1).is_register()) {
            ir::VirtualRegister dst = *iter->get_dest();
            ir::VirtualRegister load_reg = iter->get_operand(1).get_register();

            if (stack_slots.contains(load_reg) && stack_slots[load_reg].promotable) {
                cur_replacements[dst] = cur_replacements[load_reg];
                block.remove(iter);
                iter = prev;
            } else {
                replace_regs(iter->get_operands(), cur_replacements);
            }
        } else if (iter->get_opcode() == ir::Opcode::JMP) {
            update_branch_target(iter->get_operand(0), blocks, cur_replacements);
            replace_regs(iter->get_operands(), cur_replacements);
        } else if (iter->get_opcode() == ir::Opcode::CJMP || iter->get_opcode() == ir::Opcode::FCJMP) {
            update_branch_target(iter->get_operand(3), blocks, cur_replacements);
            update_branch_target(iter->get_operand(4), blocks, cur_replacements);
            replace_regs(iter->get_operands(), cur_replacements);
        } else {
            replace_regs(iter->get_operands(), cur_replacements);
        }
    }

    for (unsigned child_index : dominator_tree.get_node(block_iter).children_indices) {
        ir::BasicBlockIter child_block = dominator_tree.get_cfg().get_nodes()[child_index].block;
        rename(child_block, stack_slots, blocks, cur_replacements, dominator_tree);
    }
}

void StackToRegPass::replace_regs(
    std::vector<ir::Operand> &operands,
    std::unordered_map<long, ir::Value> cur_replacements
) {
    PassUtils::iter_values(operands, [&cur_replacements](ir::Value &value) {
        if (value.is_register() && cur_replacements.count(value.get_register())) {
            value = cur_replacements[value.get_register()];
        }
    });
}

void StackToRegPass::update_branch_target(
    ir::Operand &operand,
    BlockMap &blocks,
    std::unordered_map<long, ir::Value> cur_replacements
) {
    ir::BranchTarget &target = operand.get_branch_target();

    for (ParamInfo &param : blocks[target.block].new_params) {
        target.args.push_back(cur_replacements[param.stack_slot]);
    }
}

} // namespace passes

} // namespace banjo
