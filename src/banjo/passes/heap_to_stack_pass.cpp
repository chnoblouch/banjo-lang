#include "heap_to_stack_pass.hpp"

#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#define DEBUG_LOG is_logging() && log()

constexpr unsigned MAX_ALLOC_SIZE = 256;

namespace banjo {

namespace passes {

HeapToStackPass::HeapToStackPass(target::Target *target) : Pass("heap-to-stack", target) {
    // enable_logging(std::cout);
}

void HeapToStackPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void HeapToStackPass::run(ssa::Function &func) {
    allocs.clear();
    cfg = ssa::ControlFlowGraph{&func};

    for (ssa::BasicBlockIter block = func.begin(); block != func.end(); ++block) {
        collect_heap_instrs(block);
    }

    if (is_logging() && !allocs.empty()) {
        DEBUG_LOG << "\nheap to stack on " << func.name << ":\n\n";

        for (auto &[reg, alloc] : allocs) {
            std::string malloc_block_label = alloc.alloc_instr.block->get_debug_label();
            DEBUG_LOG << "  candidate malloc %" << reg << " in block " << malloc_block_label << "\n";

            if (!alloc.free_instrs.empty()) {
                for (InstrInContext &free_instr : alloc.free_instrs) {
                    DEBUG_LOG << "    freed in block " << free_instr.block->get_debug_label() << "\n";
                }
            } else {
                DEBUG_LOG << "    couldn't find a free instruction\n";
            }

            DEBUG_LOG << "\n";
        }
    }

    std::unordered_set<ssa::BasicBlockIter> visited_blocks;

    for (auto &[reg, alloc] : allocs) {
        visited_blocks.clear();

        if (is_always_freed(alloc, alloc.alloc_instr.block, visited_blocks)) {
            promote(func, alloc);
        }
    }

    DEBUG_LOG << "\n";
}

void HeapToStackPass::collect_heap_instrs(ssa::BasicBlockIter block) {
    for (ssa::InstrIter iter = block->begin(); iter != block->end(); ++iter) {
        ssa::Instruction &instr = *iter;

        if (instr.get_opcode() != ssa::Opcode::CALL) {
            continue;
        }

        ssa::Operand &callee = instr.get_operand(0);
        if (!callee.is_extern_func()) {
            continue;
        }

        const std::string &name = callee.get_extern_func()->name;

        if (name == "malloc") {
            ssa::Operand &arg = instr.get_operand(1);

            if (!instr.get_dest() || !arg.is_int_immediate()) {
                continue;
            }

            if (arg.get_int_immediate() > MAX_ALLOC_SIZE) {
                continue;
            }

            ssa::VirtualRegister reg = *instr.get_dest();

            Allocation alloc{
                .alloc_instr{.block = block, .instr = iter},
                .free_instrs{},
            };

            allocs.insert({reg, alloc});
        } else if (name == "free") {
            ssa::Operand &arg = instr.get_operand(1);
            if (!arg.is_register()) {
                continue;
            }

            auto alloc_iter = allocs.find(arg.get_register());
            if (alloc_iter == allocs.end()) {
                continue;
            }

            alloc_iter->second.free_instrs.push_back(InstrInContext{.block = block, .instr = iter});
        }
    }
}

bool HeapToStackPass::is_always_freed(
    Allocation &alloc,
    ssa::BasicBlockIter block,
    std::unordered_set<ssa::BasicBlockIter> &visited_blocks
) {
    if (visited_blocks.contains(block)) {
        return true;
    }

    visited_blocks.insert(block);

    if (block->get_exit_iter()->get_opcode() == ssa::Opcode::RET) {
        for (const InstrInContext &free_instr : alloc.free_instrs) {
            if (free_instr.block == block) {
                return true;
            }
        }

        return false;
    }

    for (unsigned successor : cfg.get_node(block).successors) {
        if (!is_always_freed(alloc, cfg.get_node(successor).block, visited_blocks)) {
            return false;
        }
    }

    return true;
}

void HeapToStackPass::promote(ssa::Function &func, Allocation &alloc) {
    ssa::VirtualRegister reg = *alloc.alloc_instr.instr->get_dest();
    unsigned size = alloc.alloc_instr.instr->get_operand(1).get_int_immediate().to_u64();

    DEBUG_LOG << "  promoting %" << reg << "\n";

    ssa::Operand alloca_arg = ssa::Operand::from_type({ssa::Primitive::I8, size});
    ssa::Instruction alloca_instr{ssa::Opcode::ALLOCA, reg, {alloca_arg}};
    func.get_entry_block().insert_before(func.get_entry_block().get_entry_iter(), alloca_instr);

    alloc.alloc_instr.block->remove(alloc.alloc_instr.instr);

    for (InstrInContext &free_instr : alloc.free_instrs) {
        free_instr.block->remove(free_instr.instr);
    }
}

} // namespace passes

} // namespace banjo
