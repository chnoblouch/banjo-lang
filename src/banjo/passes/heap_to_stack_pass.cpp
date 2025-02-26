#include "heap_to_stack_pass.hpp"

#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <utility>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace passes {

HeapToStackPass::HeapToStackPass(target::Target *target) : Pass("heap-to-stack", target) {}

void HeapToStackPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void HeapToStackPass::run(ssa::Function &func) {
    for (ssa::BasicBlock &block : func) {
        run(block, func);
    }
}

void HeapToStackPass::run(ssa::BasicBlock &block, ssa::Function &func) {
    std::unordered_map<ssa::VirtualRegister, ssa::InstrIter> heap_allocs;
    std::vector<std::pair<ssa::InstrIter, ssa::InstrIter>> replaceable_heap_allocs;

    for (ssa::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        ssa::Instruction &instr = *iter;

        if (instr.get_opcode() != ssa::Opcode::CALL) {
            continue;
        }

        ssa::Operand &callee = instr.get_operand(0);
        if (!callee.is_extern_func()) {
            continue;
        }

        const std::string &name = callee.get_extern_func()->get_name();

        if (name == "malloc") {
            if (!instr.get_dest()) {
                continue;
            }

            ssa::Operand &arg = instr.get_operand(1);
            if (!arg.is_int_immediate()) {
                continue;
            }

            heap_allocs.insert({*instr.get_dest(), iter});
        } else if (name == "free") {
            ssa::Operand &arg = instr.get_operand(1);
            if (!arg.is_register()) {
                continue;
            }

            auto heap_alloc_iter = heap_allocs.find(arg.get_register());
            if (heap_alloc_iter == heap_allocs.end()) {
                continue;
            }

            replaceable_heap_allocs.push_back({heap_alloc_iter->second, iter});
        }
    }

    for (auto [alloc_instr, free_instr] : replaceable_heap_allocs) {
        unsigned reg = *alloc_instr->get_dest();
        unsigned size = alloc_instr->get_operand(1).get_int_immediate().to_u64();

        ssa::Operand alloca_arg = ssa::Operand::from_type(ssa::Type(ssa::Primitive::I8, size));
        ssa::Instruction alloca_instr(ssa::Opcode::ALLOCA, reg, {alloca_arg});
        func.get_entry_block().insert_before(func.get_entry_block().get_instrs().get_first_iter(), alloca_instr);

        block.remove(alloc_instr);
        block.remove(free_instr);
    }
}

} // namespace passes

} // namespace banjo
