#include "licm_pass.hpp"

#include "banjo/passes/pass_utils.hpp"

#include <iostream>
#include <unordered_set>

namespace banjo {

namespace passes {

LICMPass::LICMPass(target::Target *target) : Pass("licm", target) {}

void LICMPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func);
    }
}

void LICMPass::run(ssa::Function *func) {
    ssa::ControlFlowGraph cfg(func);
    ssa::DominatorTree domtree(cfg);
    ssa::LoopAnalyzer analyzer(cfg, domtree);
    std::vector<ssa::LoopAnalysis> loops = analyzer.analyze();

    if (!loops.empty()) {
        std::cout << func->get_name() << '\n' << std::string(16, '-') << '\n';
        analyzer.dump(std::cout);
    }

    for (const ssa::LoopAnalysis &loop : loops) {
        run(loop, cfg, func);
    }
}

void LICMPass::run(const ssa::LoopAnalysis &loop, ssa::ControlFlowGraph &cfg, ssa::Function *func) {
    if (loop.entries.size() != 1) {
        return;
    }

    std::unordered_set<ssa::VirtualRegister> in_loop_defs;

    for (unsigned block_index : loop.body) {
        ssa::BasicBlockIter block = cfg.get_node(block_index).block;

        for (ssa::VirtualRegister param_reg : block->get_param_regs()) {
            in_loop_defs.insert(param_reg);
        }

        for (ssa::Instruction &instr : block->get_instrs()) {
            if (instr.get_dest()) {
                in_loop_defs.insert(*instr.get_dest());
            }
        }
    }

    ssa::BasicBlockIter entry_block = cfg.get_node(*loop.entries.begin()).block;
    bool changed = false;

    for (unsigned block_index : loop.body) {
        ssa::BasicBlockIter block = cfg.get_node(block_index).block;

        for (ssa::InstrIter iter = block->begin(); iter != block->end(); ++iter) {
            if (iter->get_opcode() == ssa::Opcode::LOAD || iter->get_opcode() == ssa::Opcode::STORE ||
                iter->get_opcode() == ssa::Opcode::CALL || iter->get_opcode() == ssa::Opcode::MEMBERPTR) {
                continue;
            }

            bool uses_in_loop_def = false;

            PassUtils::iter_regs(iter->get_operands(), [&in_loop_defs, &uses_in_loop_def](ssa::VirtualRegister reg) {
                if (in_loop_defs.contains(reg)) {
                    uses_in_loop_def = true;
                }
            });

            if (!uses_in_loop_def && iter->get_dest()) {
                std::cout << "instr %" << *iter->get_dest() << " can be moved out of ";
                std::cout << cfg.get_node(loop.header).block->get_debug_label() << '\n';

                in_loop_defs.erase(*iter->get_dest());

                ssa::InstrIter next_iter = iter.get_prev();
                entry_block->get_instrs().insert_before(entry_block->get_instrs().get_last_iter(), *iter);
                block->remove(iter);
                iter = next_iter;

                changed = true;
            }
        }
    }

    if (changed) {
        run(loop, cfg, func);
    }

    /*
    if (cfg.get_node(loop.header).predecessors.size() != 1 || false) {
        std::cout << "inserting loop preheader\n";

        ssa::ControlFlowGraph::Node &header_node = cfg.get_node(loop.header);
        ssa::BasicBlockIter header = header_node.block;
        std::string preheader_name = header->get_label() + ".preheader";
        ssa::BasicBlockIter preheader = func->get_basic_blocks().insert_before(header, ssa::BasicBlock(preheader_name));

        for (unsigned pred : header_node.predecessors) {

        }

        preheader->append(ssa::Instruction(ssa::Opcode::JMP, {ssa::Operand::from_branch_target()}));

        return;
    }
    */
}

} // namespace passes

} // namespace banjo
