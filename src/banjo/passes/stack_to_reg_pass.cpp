#include "stack_to_reg_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/dead_code_elimination.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include <vector>

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
    slots.clear();
    blocks.clear();

    cfg = ssa::ControlFlowGraph::build(func);
    dom_tree = ssa::DominatorTree::build(cfg);

    for (ssa::BasicBlock &block : func.get_basic_blocks()) {
        for (ssa::Instruction &instr : block.get_instrs()) {
            find_stack_slots(instr);
        }
    }

    for (ssa::BasicBlockIter block = func.begin(); block != func.end(); ++block) {
        for (ssa::Instruction &instr : block->get_instrs()) {
            find_slot_uses(cfg.node_id(block), instr);
        }
    }

    for (auto iter = slots.begin(); iter != slots.end();) {
        if (iter->second.promotable) {
            ++iter;
        } else {
            iter = slots.erase(iter);
        }
    }

    ValueMap initial_values;

    for (auto &[reg, slot] : slots) {
        try_promote(reg, slot, initial_values);
    }

    update_uses(func.get_entry_block_iter(), initial_values);

    Precomputing::precompute_instrs(func);
    ssa::DeadCodeElimination{}.run(func);

    print_dump();
}

void StackToRegPass::find_stack_slots(ssa::Instruction &instr) {
    if (instr.get_opcode() != ssa::Opcode::ALLOCA) {
        return;
    }

    ssa::Type type = instr.get_operand(0).get_type();
    if (!get_target()->get_data_layout().fits_in_register(type)) {
        return;
    }

    StackSlotInfo slot{
        .type = type,
        .store_nodes{},
        .promotable = true,
    };

    ssa::VirtualRegister reg = *instr.get_dest();
    slots.insert({reg, slot});
}

void StackToRegPass::find_slot_uses(NodeID node, ssa::Instruction &instr) {
    ssa::Opcode opcode = instr.get_opcode();

    if (opcode == ssa::Opcode::LOAD) {
        ssa::Type type = instr.get_operand(0).get_type();
        ssa::Operand &src = instr.get_operand(1);

        if (src.is_register()) {
            analyze_reg_use(src.get_register(), node, opcode);

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
            analyze_reg_use(src.get_register(), node, opcode);
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
            analyze_reg_use(reg, node, opcode);
        });
    }
}

void StackToRegPass::analyze_reg_use(ssa::VirtualRegister reg, NodeID node, ssa::Opcode opcode) {
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

void StackToRegPass::try_promote(ssa::VirtualRegister reg, StackSlotInfo &slot, ValueMap &initial_values) {
    if (slot.store_nodes.empty()) {
        initial_values[reg] = create_undefined(slot.type);
        return;
    }

    std::vector<ssa::ControlFlowGraph::NodeID> store_nodes_to_analyze;
    BitSet store_nodes_analyzed;

    for (ssa::ControlFlowGraph::NodeID store_node : slot.store_nodes) {
        store_nodes_to_analyze.push_back(store_node);
    }

    for (unsigned i = 0; i < store_nodes_to_analyze.size(); i++) {
        ssa::ControlFlowGraph::NodeID store_node = store_nodes_to_analyze[i];

        for (ssa::ControlFlowGraph::NodeID node : dom_tree.nodes[store_node].dominance_frontiers) {
            if (store_nodes_analyzed.get(node) || slot.nodes_having_val_as_param.get(node)) {
                continue;
            }

            BitSet nodes_visited;
            if (!is_slot_loaded(slot, node, nodes_visited)) {
                continue;
            }

            ssa::BasicBlockIter block = cfg.block(node);

            unsigned param_index = static_cast<unsigned>(block->get_param_regs().size());
            blocks[block].new_params.push_back({.param_index = param_index, .stack_slot = reg});

            block->get_param_regs().push_back(func->next_virtual_reg());
            block->get_param_types().push_back(slot.type);

            slot.nodes_having_val_as_param.set(node);
            initial_values[reg] = create_undefined(slot.type);

            slot.store_nodes.set(node);
            store_nodes_analyzed.set(node);
            store_nodes_to_analyze.push_back(node);
        }
    }
}

void StackToRegPass::update_uses(ssa::BasicBlockIter block_iter, ValueMap current_values) {
    ssa::BasicBlock &block = *block_iter;

    for (ParamInfo param : blocks[block_iter].new_params) {
        unsigned index = param.param_index;
        ssa::Value value = ssa::Value::from_register(block.get_param_regs()[index], block.get_param_types()[index]);
        current_values[param.stack_slot] = value;
    }

    for (ssa::InstrIter instr = block.begin(); instr != block.end(); ++instr) {
        ssa::InstrIter prev = instr.get_prev();

        if (instr->get_opcode() == ssa::Opcode::ALLOCA && slots[*instr->get_dest()].promotable) {
            block.remove(instr);
            instr = prev;
        } else if (instr->get_opcode() == ssa::Opcode::LOAD) {
            update_uses_in_load(block, instr, current_values);
        } else if (instr->get_opcode() == ssa::Opcode::STORE) {
            update_uses_in_store(block, instr, current_values);
        } else if (instr->get_opcode() == ssa::Opcode::JMP) {
            update_branch_target(instr->get_operand(0), current_values);
            replace_regs(instr->get_operands(), current_values);
        } else if (instr->get_opcode() == ssa::Opcode::CJMP || instr->get_opcode() == ssa::Opcode::FCJMP) {
            update_branch_target(instr->get_operand(3), current_values);
            update_branch_target(instr->get_operand(4), current_values);
            replace_regs(instr->get_operands(), current_values);
        } else {
            replace_regs(instr->get_operands(), current_values);
        }
    }

    ssa::ControlFlowGraph::NodeID cfg_node = cfg.node_id(block_iter);

    for (ssa::ControlFlowGraph::NodeID child : dom_tree.nodes[cfg_node].children) {
        ssa::BasicBlockIter child_block = cfg.block(child);
        update_uses(child_block, current_values);
    }
}

void StackToRegPass::update_uses_in_load(ssa::BasicBlock &block, ssa::InstrIter &instr, ValueMap &current_values) {
    ssa::VirtualRegister dst = *instr->get_dest();
    ssa::Type type = instr->get_operand(0).get_type();
    ssa::Operand &addr = instr->get_operand(1);

    if (!addr.is_register()) {
        replace_regs(instr->get_operands(), current_values);
        return;
    }

    ssa::VirtualRegister addr_reg = addr.get_register();

    if (!slots.contains(addr_reg) || !slots[addr_reg].promotable) {
        replace_regs(instr->get_operands(), current_values);
        return;
    }

    ssa::Value value = current_values[addr_reg];

    // TODO: Test this, and probably also insert a bitcast before loading.
    if (value.get_type().is_floating_point() != type.is_floating_point()) {
        ssa::VirtualRegister reg = func->next_virtual_reg();
        ssa::Operand type_operand = ssa::Operand::from_type(type);
        block.insert_before(instr, {ssa::Opcode::BITCAST, reg, {value, type_operand}});

        value = ssa::Operand::from_register(reg, type);
    }

    current_values[dst] = value;

    ssa::InstrIter prev = instr.get_prev();
    block.remove(instr);
    instr = prev;
}

void StackToRegPass::update_uses_in_store(ssa::BasicBlock &block, ssa::InstrIter &instr, ValueMap &current_values) {
    ssa::Operand &value = instr->get_operand(0);
    ssa::Operand &addr = instr->get_operand(1);

    if (!addr.is_register()) {
        replace_regs(instr->get_operands(), current_values);
        return;
    }

    ssa::VirtualRegister addr_reg = instr->get_operand(1).get_register();

    if (!slots.contains(addr_reg) || !slots[addr_reg].promotable) {
        replace_regs(instr->get_operands(), current_values);
        return;
    }

    if (value.is_register() && current_values.contains(value.get_register())) {
        value = current_values[value.get_register()];
    }

    current_values[addr_reg] = value;

    ssa::InstrIter prev = instr.get_prev();
    block.remove(instr);
    instr = prev;
}

void StackToRegPass::replace_regs(std::vector<ssa::Operand> &operands, ValueMap current_values) {
    PassUtils::iter_values(operands, [&](ssa::Value &value) {
        if (value.is_register() && current_values.contains(value.get_register())) {
            value = current_values[value.get_register()];
        }
    });
}

void StackToRegPass::update_branch_target(ssa::Operand &operand, ValueMap current_values) {
    ssa::BranchTarget &target = operand.get_branch_target();

    for (ParamInfo &param : blocks[target.block].new_params) {
        target.args.push_back(current_values[param.stack_slot]);
    }
}

void StackToRegPass::print_dump() {
    if (!is_logging()) {
        return;
    }

    std::ostream &stream = log();

    stream << "func " << func->name << ":\n\n";
    stream << "stack slots: [\n";

    for (const auto &[reg, slot] : slots) {
        stream << "  {\n";
        stream << "    register: %" << reg << "\n";
        stream << "    promotable: " << (slot.promotable ? "yes" : "no") << "\n";

        if (!slot.store_nodes.empty()) {
            stream << "    store blocks:";

            for (ssa::ControlFlowGraph::NodeID node : slot.store_nodes) {
                stream << " " << cfg.block(node)->get_debug_label();
            }

            stream << "\n";
        }

        if (!slot.load_nodes.empty()) {
            stream << "    load blocks:";

            for (ssa::ControlFlowGraph::NodeID node : slot.load_nodes) {
                stream << " " << cfg.block(node)->get_debug_label();
            }

            stream << "\n";
        }

        stream << "  }\n";
    }

    stream << "]\n\n";

    stream << "dominator tree:\n";
    dom_tree.dump(cfg, stream);
    stream << "\n\n";
}

} // namespace banjo::passes
