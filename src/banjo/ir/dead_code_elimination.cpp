#include "dead_code_elimination.hpp"

#include "ir/control_flow_graph.hpp"
#include "passes/pass_utils.hpp"

#include <map>

namespace ir {

void DeadCodeElimination::run(Module &mod) {
    for (Function *func : mod.get_functions()) {
        run(func);
    }
}

void DeadCodeElimination::run(Function *func) {
    ControlFlowGraph cfg(func);

    bool changed = true;

    while (changed) {
        changed = false;

        for (BasicBlockIter iter = func->begin(); iter != func->end(); ++iter) {
            if (!cfg.contains(iter)) {
                iter = iter.get_next();
                func->get_basic_blocks().remove(iter.get_prev());
                changed = true;
            }
        }

        cfg = ControlFlowGraph(func);
    }

    std::map<ir::VirtualRegister, unsigned> reg_uses;

    for (BasicBlock &block : *func) {
        for (Instruction &instr : block) {
            passes::PassUtils::iter_regs(instr.get_operands(), [&reg_uses](ir::VirtualRegister reg) {
                reg_uses[reg] += 1;
            });
        }
    }

    changed = true;

    while (changed) {
        changed = false;

        for (BasicBlock &block : *func) {
            LinkedList<Instruction> &instrs = block.get_instrs();

            for (InstrIter iter = instrs.get_last_iter(); iter != instrs.get_first_iter().get_prev(); --iter) {
                if (iter->get_opcode() == ir::Opcode::CALL) {
                    continue;
                }

                if (iter->get_dest() && reg_uses[*iter->get_dest()] == 0) {
                    InstrIter next_iter = iter.get_next();

                    passes::PassUtils::iter_regs(iter->get_operands(), [&reg_uses](ir::VirtualRegister reg) {
                        reg_uses[reg] -= 1;
                    });
                    block.remove(iter);

                    iter = next_iter;
                    changed = true;
                }
            }
        }
    }
}

} // namespace ir
