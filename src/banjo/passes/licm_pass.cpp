#include "licm_pass.hpp"

#include "passes/pass_utils.hpp"

#include <iostream>
#include <unordered_set>

namespace banjo {

namespace passes {

LICMPass::LICMPass(target::Target *target) : Pass("licm", target) {}

void LICMPass::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(func);
    }
}

void LICMPass::run(ir::Function *func) {
    ir::ControlFlowGraph cfg(func);
    ir::DominatorTree domtree(cfg);
    ir::LoopAnalyzer analyzer(cfg, domtree);
    std::vector<ir::LoopAnalysis> loops = analyzer.analyze();

    if (!loops.empty()) {
        std::cout << func->get_name() << '\n' << std::string(16, '-') << '\n';
        analyzer.dump(std::cout);
    }

    for (const ir::LoopAnalysis &loop : loops) {
        run(loop, cfg, func);
    }
}

void LICMPass::run(const ir::LoopAnalysis &loop, ir::ControlFlowGraph &cfg, ir::Function *func) {
    if (loop.entries.size() != 1) {
        return;
    }

    std::unordered_set<ir::VirtualRegister> in_loop_defs;

    for (unsigned block_index : loop.body) {
        ir::BasicBlockIter block = cfg.get_node(block_index).block;

        for (ir::VirtualRegister param_reg : block->get_param_regs()) {
            in_loop_defs.insert(param_reg);
        }

        for (ir::Instruction &instr : block->get_instrs()) {
            if (instr.get_dest()) {
                in_loop_defs.insert(*instr.get_dest());
            }
        }
    }

    ir::BasicBlockIter entry_block = cfg.get_node(*loop.entries.begin()).block;
    bool changed = false;

    for (unsigned block_index : loop.body) {
        ir::BasicBlockIter block = cfg.get_node(block_index).block;

        for (ir::InstrIter iter = block->begin(); iter != block->end(); ++iter) {
            if (iter->get_opcode() == ir::Opcode::LOAD || iter->get_opcode() == ir::Opcode::STORE ||
                iter->get_opcode() == ir::Opcode::CALL || iter->get_opcode() == ir::Opcode::MEMBERPTR) {
                continue;
            }

            bool uses_in_loop_def = false;

            PassUtils::iter_regs(iter->get_operands(), [&in_loop_defs, &uses_in_loop_def](ir::VirtualRegister reg) {
                if (in_loop_defs.contains(reg)) {
                    uses_in_loop_def = true;
                }
            });

            if (!uses_in_loop_def && iter->get_dest()) {
                std::cout << "instr %" << *iter->get_dest() << " can be moved out of ";
                std::cout << cfg.get_node(loop.header).block->get_debug_label() << '\n';

                in_loop_defs.erase(*iter->get_dest());

                ir::InstrIter next_iter = iter.get_prev();
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

        ir::ControlFlowGraph::Node &header_node = cfg.get_node(loop.header);
        ir::BasicBlockIter header = header_node.block;
        std::string preheader_name = header->get_label() + ".preheader";
        ir::BasicBlockIter preheader = func->get_basic_blocks().insert_before(header, ir::BasicBlock(preheader_name));

        for (unsigned pred : header_node.predecessors) {

        }

        preheader->append(ir::Instruction(ir::Opcode::JMP, {ir::Operand::from_branch_target()}));

        return;
    }
    */
}

} // namespace passes

} // namespace banjo
