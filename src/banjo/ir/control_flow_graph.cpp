#include "control_flow_graph.hpp"

#include <cassert>

namespace ir {

ControlFlowGraph::ControlFlowGraph(ir::Function *func) {
    create_nodes(func->get_entry_block_iter());
    entry_index = 0;
    sort_in_post_order();
}

void ControlFlowGraph::create_nodes(ir::BasicBlockIter block) {
    assert(block->get_size() != 0 && "block is empty");

    if (blocks2nodes.contains(block)) {
        return;
    }

    nodes.push_back(Node{.block = block});
    blocks2nodes.insert({block, nodes.size() - 1});

    ir::Instruction &last_instr = block->get_instrs().get_last();

    switch (last_instr.get_opcode()) {
        case ir::Opcode::JMP: create_edge(block, last_instr.get_operand(0).get_branch_target().block); break;
        case ir::Opcode::CJMP:
        case ir::Opcode::FCJMP:
            create_edge(block, last_instr.get_operand(3).get_branch_target().block);
            create_edge(block, last_instr.get_operand(4).get_branch_target().block);
            break;
        case ir::Opcode::RET: break;
        default: assert(!"block does not end in branch instruction"); break;
    }
}

void ControlFlowGraph::create_edge(ir::BasicBlockIter from, ir::BasicBlockIter to) {
    create_nodes(to);

    unsigned from_index = blocks2nodes[from];
    unsigned to_index = blocks2nodes[to];

    nodes[from_index].successors.push_back(to_index);
    nodes[to_index].predecessors.push_back(from_index);
}

void ControlFlowGraph::sort_in_post_order() {
    std::unordered_map<unsigned, unsigned> index_map;
    unsigned cur_new_index = 0;
    collect_nodes_in_post_order(entry_index, index_map, cur_new_index);

    assert(nodes.size() == index_map.size());

    std::vector<Node> nodes_sorted;
    nodes_sorted.resize(index_map.size());

    for (unsigned i = 0; i < index_map.size(); i++) {
        Node &node = nodes[i];

        for (unsigned &predecessor : node.predecessors) {
            predecessor = index_map[predecessor];
        }

        for (unsigned &successor : node.successors) {
            successor = index_map[successor];
        }

        unsigned new_index = index_map[i];
        nodes_sorted[new_index] = node;
        blocks2nodes[node.block] = new_index;
    }

    nodes = std::move(nodes_sorted);
    entry_index = nodes.size() - 1;
}

void ControlFlowGraph::collect_nodes_in_post_order(
    unsigned index,
    std::unordered_map<unsigned, unsigned> &index_map,
    unsigned &cur_new_index
) {
    // Insert a dummy value to mark this node as visited.
    index_map.insert({index, 0});

    for (unsigned successor : nodes[index].successors) {
        if (!index_map.contains(successor)) {
            collect_nodes_in_post_order(successor, index_map, cur_new_index);
        }
    }

    index_map[index] = cur_new_index++;
}

void ControlFlowGraph::dump(std::ostream &stream) {
    for (unsigned i = 0; i < nodes.size(); i++) {
        Node &node = nodes[i];

        stream << i << " ";

        const std::string &label = node.block->get_label();
        stream << (label.empty() ? "<entry>" : label) << ":\n";

        for (unsigned predecessor : node.predecessors) {
            const std::string &label = nodes[predecessor].block->get_label();
            stream << "  <- " << (label.empty() ? "<entry>" : label) << std::endl;
        }

        for (unsigned successor : node.successors) {
            const std::string &label = nodes[successor].block->get_label();
            stream << "  -> " << (label.empty() ? "<entry>" : label) << std::endl;
        }

        stream << std::endl;
    }
}

DominatorTree::DominatorTree(ControlFlowGraph &cfg) : cfg(cfg) {
    nodes.resize(cfg.get_nodes().size());
    for (unsigned i = 0; i < cfg.get_nodes().size(); i++) {
        nodes[i] = Node{.index = i};
    }

    compute_idoms();
    compute_dominance_frontiers();
}

void DominatorTree::compute_idoms() {
    std::vector<int> doms(cfg.get_nodes().size(), -1);
    doms[cfg.get_entry_index()] = cfg.get_entry_index();

    bool changed = true;
    while (changed) {
        changed = false;

        for (auto iter = nodes.rbegin(); iter != nodes.rend(); ++iter) {
            Node &node = *iter;

            if (node.index == cfg.get_entry_index()) {
                continue;
            }

            int new_idom = -1;
            for (unsigned pred : cfg.get_nodes()[node.index].predecessors) {
                if (doms[pred] != -1) {
                    new_idom = new_idom == -1 ? pred : intersect(new_idom, pred, doms);
                }
            }

            if (doms[node.index] != new_idom) {
                doms[node.index] = new_idom;
                changed = true;
            }
        }
    }

    for (unsigned i = 0; i < doms.size(); i++) {
        unsigned idom = doms[i];
        nodes[i].parent_index = idom;

        if (i != cfg.get_entry_index()) {
            nodes[idom].children_indices.push_back(i);
        }
    }
}

unsigned DominatorTree::intersect(unsigned b1, unsigned b2, const std::vector<int> &doms) {
    while (b1 != b2) {
        if (b1 < b2) {
            assert(doms[b1] != -1);
            b1 = doms[b1];
        } else {
            assert(doms[b2] != -1);
            b2 = doms[b2];
        }
    }

    return b1;
}

void DominatorTree::compute_dominance_frontiers() {
    for (unsigned i = 0; i < cfg.get_nodes().size(); i++) {
        const ControlFlowGraph::Node &node = cfg.get_nodes()[i];
        unsigned idom = nodes[i].parent_index;

        // Skip if this node is not a join point.
        if (node.predecessors.size() < 2) {
            continue;
        }

        for (unsigned pred : node.predecessors) {
            unsigned runner = pred;

            // Move up the dominance tree and add the node to the dominance frontier sets of the predecessors.
            while (runner != idom) {
                nodes[runner].dominance_frontiers.push_back(i);
                runner = nodes[runner].parent_index;
            }
        }
    }
}

std::vector<ControlFlowGraph::Node> DominatorTree::get_dominance_frontiers(ir::BasicBlockIter iter) {
    const std::vector<unsigned> &indices = get_node(iter).dominance_frontiers;

    std::vector<ControlFlowGraph::Node> cfg_nodes;
    cfg_nodes.resize(indices.size());

    for (unsigned i = 0; i < indices.size(); i++) {
        cfg_nodes[i] = cfg.get_nodes()[indices[i]];
    }

    return cfg_nodes;
}

void DominatorTree::dump(std::ostream &stream) {
    dump_tree(stream, nodes[cfg.get_entry_index()], 0);
}

void DominatorTree::dump_tree(std::ostream &stream, Node &node, unsigned indentation) {
    stream << std::string(2 * indentation, ' ') << " - ";
    stream << get_debug_label(cfg.get_nodes()[node.index].block);

    if (!node.dominance_frontiers.empty()) {
        stream << " [ ";

        for (unsigned i = 0; i < node.dominance_frontiers.size(); i++) {
            stream << get_debug_label(cfg.get_nodes()[node.dominance_frontiers[i]].block);
            if (i != node.dominance_frontiers.size() - 1) {
                stream << ", ";
            }
        }

        stream << " ]";
    }

    stream << '\n';

    for (unsigned child_index : node.children_indices) {
        dump_tree(stream, nodes[child_index], indentation + 1);
    }
}

std::string DominatorTree::get_debug_label(ir::BasicBlockIter block) {
    return block->get_label().empty() ? "<entry>" : block->get_label();
}

} // namespace ir
