#include "dead_code_elimination.hpp"

#include "banjo/ir/basic_block.hpp"
#include "banjo/ir/instruction.hpp"
#include "banjo/ir/operand.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/passes/pass_utils.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace ir {

void DeadCodeElimination::run(Function &func) {
    remove_unused_instrs(func);
    remove_unused_params(func);
}

void DeadCodeElimination::remove_unused_params(Function &func) {
    std::unordered_map<BasicBlockIter, BlockInfo> blocks;
    std::unordered_map<VirtualRegister, ParamInfo *> regs2params;

    BasicBlockIter begin = func.get_basic_blocks().begin().get_next();

    // Find all basic blocks and parameters.
    for (BasicBlockIter iter = begin; iter != func.get_basic_blocks().end(); ++iter) {
        BlockInfo info;

        for (unsigned i = 0; i < iter->get_param_regs().size(); i++) {
            info.params.push_back({
                .direct_src_params = {},
                .used = false,
            });
        }

        blocks.insert({iter, info});
    }

    // Create the mapping from registers to parameter info.
    for (BasicBlockIter iter = begin; iter != func.get_basic_blocks().end(); ++iter) {
        BlockInfo &info = blocks[iter];

        unsigned param_index = 0;
        for (VirtualRegister param : iter->get_param_regs()) {
            regs2params.insert({param, &info.params[param_index]});
            param_index += 1;
        }
    }

    // Look for arguments to branches that are themselves branch arguments.
    // This creates a kind of graph of all registers that are only passed between basic blocks,
    // but never used in actual instructions. These are the registers we want to eliminate.
    for (BasicBlockIter iter = func.get_basic_blocks().begin(); iter != func.get_basic_blocks().end(); ++iter) {
        Instruction &branch_instr = iter->get_instrs().get_last();

        for (Operand &operand : branch_instr.get_operands()) {
            if (!operand.is_branch_target()) {
                continue;
            }

            BranchTarget *target = &operand.get_branch_target();
            BlockInfo &target_info = blocks[target->block];
            target_info.preds.push_back(target);

            for (unsigned i = 0; i < target->args.size(); i++) {
                Value &arg = target->args[i];

                if (!arg.is_register()) {
                    continue;
                }

                auto arg_param_iter = regs2params.find(arg.get_register());
                if (arg_param_iter == regs2params.end()) {
                    continue;
                }

                target_info.params[i].direct_src_params.push_back(arg_param_iter->second);
            }
        }
    }

    // Mark all parameters as used that are directly or indirectly used in an instruction.
    for (BasicBlockIter iter = func.get_basic_blocks().begin(); iter != func.get_basic_blocks().end(); ++iter) {
        for (Instruction &instr : *iter) {
            for (Operand &operand : instr.get_operands()) {
                if (!operand.is_register()) {
                    continue;
                }

                auto param_iter = regs2params.find(operand.get_register());
                if (param_iter == regs2params.end()) {
                    continue;
                }

                mark_as_used(param_iter->second);
            }
        }
    }

    // Remove all parameters that are never used (neither directly nor indirectly).
    for (BasicBlockIter iter = begin; iter != func.get_basic_blocks().end(); ++iter) {
        const BlockInfo &info = blocks[iter];

        std::vector<VirtualRegister> new_param_regs;
        std::vector<Type> new_param_types;

        for (unsigned i = 0; i < iter->get_param_regs().size(); i++) {
            if (info.params[i].used) {
                new_param_regs.push_back(iter->get_param_regs()[i]);
                new_param_types.push_back(iter->get_param_types()[i]);
            }
        }

        iter->get_param_regs() = new_param_regs;
        iter->get_param_types() = new_param_types;

        for (BranchTarget *pred : info.preds) {
            std::vector<Operand> new_args;

            for (unsigned i = 0; i < pred->args.size(); i++) {
                if (info.params[i].used) {
                    new_args.push_back(pred->args[i]);
                }
            }

            pred->args = new_args;
        }
    }
}

void DeadCodeElimination::mark_as_used(ParamInfo *param_info) {
    param_info->used = true;

    for (ParamInfo *src_param : param_info->direct_src_params) {
        if (!src_param->used) {
            mark_as_used(src_param);
        }
    }
}

void DeadCodeElimination::remove_unused_instrs(Function &func) {
    std::unordered_map<ir::VirtualRegister, unsigned> reg_uses;

    for (BasicBlock &block : func) {
        for (Instruction &instr : block) {
            passes::PassUtils::iter_regs(instr.get_operands(), [&reg_uses](ir::VirtualRegister reg) {
                reg_uses[reg] += 1;
            });
        }
    }

    bool changed = true;

    while (changed) {
        changed = false;

        for (BasicBlock &block : func) {
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

} // namespace banjo
