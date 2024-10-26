#include "heap_to_stack_pass.hpp"

#include "banjo/ir/instruction.hpp"
#include "banjo/ir/virtual_register.hpp"

#include <utility>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace passes {

HeapToStackPass::HeapToStackPass(target::Target *target) : Pass("heap-to-stack", target) {}

void HeapToStackPass::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void HeapToStackPass::run(ir::Function &func) {
    for (ir::BasicBlock &block : func) {
        run(block, func);
    }
}

void HeapToStackPass::run(ir::BasicBlock &block, ir::Function &func) {
    std::unordered_map<ir::VirtualRegister, ir::InstrIter> heap_allocs;
    std::vector<std::pair<ir::InstrIter, ir::InstrIter>> replaceable_heap_allocs;

    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        ir::Instruction &instr = *iter;

        if (instr.get_opcode() != ir::Opcode::CALL) {
            continue;
        }

        ir::Operand &callee = instr.get_operand(0);
        if (!callee.is_extern_func()) {
            continue;
        }

        const std::string &name = callee.get_extern_func_name();

        if (name == "malloc") {
            if (!instr.get_dest()) {
                continue;
            }

            ir::Operand &arg = instr.get_operand(1);
            if (!arg.is_int_immediate()) {
                continue;
            }

            heap_allocs.insert({*instr.get_dest(), iter});
        } else if (name == "free") {
            ir::Operand &arg = instr.get_operand(1);
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

        ir::Operand alloca_arg = ir::Operand::from_type(ir::Type(ir::Primitive::I8, size));
        ir::Instruction alloca_instr(ir::Opcode::ALLOCA, reg, {alloca_arg});
        func.get_entry_block().insert_before(func.get_entry_block().get_instrs().get_first_iter(), alloca_instr);

        block.remove(alloc_instr);
        block.remove(free_instr);
    }
}

} // namespace passes

} // namespace banjo
