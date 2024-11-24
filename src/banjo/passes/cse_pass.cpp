#include "cse_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include <map>

#include <iostream>

namespace banjo {

namespace passes {

CSEPass::CSEPass(target::Target *target) : Pass("cse", target) {}

void CSEPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(func);
    }
}

void CSEPass::run(ssa::Function *func) {
    for (ssa::BasicBlock &block : func->get_basic_blocks()) {
        run(block, func);
    }
}

void CSEPass::run(ssa::BasicBlock &block, ssa::Function *func) {
    // FIXME: what if the types are different?

    std::map<std::pair<ssa::VirtualRegister, unsigned>, ssa::InstrIter> member_ptrs;
    std::map<ssa::VirtualRegister, ssa::InstrIter> loads;

    for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        if (iter->get_opcode() != ssa::Opcode::MEMBERPTR) {
            continue;
        }

        ssa::Operand &ptr = iter->get_operand(1);
        ssa::Operand &index = iter->get_operand(2);

        if (!ptr.is_register() || !index.is_immediate()) {
            continue;
        }

        std::pair<ssa::VirtualRegister, unsigned> key{ptr.get_register(), index.get_int_immediate().to_u64()};

        if (member_ptrs.contains(key)) {
            std::cout << "merging memberptr in " << func->get_name() << " (%" << *iter->get_dest() << ")" << std::endl;
            PassUtils::replace_in_block(block, *iter->get_dest(), *member_ptrs[key]->get_dest());
            iter = iter.get_prev();
            block.remove(iter.get_next());
        } else {
            member_ptrs.insert({key, iter});
        }
    }

    for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        // Stores or calls might modify values, so reload them conservatively.
        if (iter->get_opcode() == ssa::Opcode::STORE || iter->get_opcode() == ssa::Opcode::CALL) {
            loads.clear();
            continue;
        }

        if (iter->get_opcode() == ssa::Opcode::LOAD && iter->get_operand(1).is_register()) {
            ssa::VirtualRegister key = iter->get_operand(1).get_register();
            if (loads.contains(key)) {
                std::cout << "merging load in " << func->get_name() << " (%" << *iter->get_dest() << ")" << std::endl;
                PassUtils::replace_in_block(block, *iter->get_dest(), *loads[key]->get_dest());
                iter = iter.get_prev();
                block.remove(iter.get_next());
            } else {
                loads.insert({key, iter});
            }
        }
    }
}

} // namespace passes

} // namespace banjo
