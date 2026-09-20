#include "function.hpp"

#include <utility>

namespace banjo::ssa {

Function::Function(std::string name, FunctionType type) : name{std::move(name)}, type{std::move(type)} {
    basic_blocks.append(BasicBlock());
}

BasicBlockIter Function::create_block(std::string label) {
    return basic_blocks.create_iter(std::move(label));
}

void Function::append_block(BasicBlockIter block) {
    basic_blocks.append(block);
}

void Function::merge_blocks(BasicBlockIter first, BasicBlockIter second) {
    for (ssa::Instruction &instr : second->get_instrs()) {
        first->append(instr);
    }

    basic_blocks.remove(second);
}

BasicBlockIter Function::split_block_after(BasicBlockIter block, InstrIter instr) {
    // TODO: this could be optimized by moving instruction iterators instead of copying them.

    ssa::BasicBlockIter new_block = basic_blocks.insert_after(block, BasicBlock{next_block_label()});

    for (ssa::InstrIter iter = instr.get_next(); iter != block->end();) {
        new_block->append(*iter);
        iter = iter.get_next();
        block->remove(iter.get_prev());
    }

    return new_block;
}

BasicBlockIter Function::insert_after(BasicBlockIter block) {
    return basic_blocks.insert_after(block, BasicBlock{next_block_label()});
}

BasicBlockIter Function::find_basic_block(const std::string &label) {
    for (BasicBlockIter iter = basic_blocks.begin(); iter != basic_blocks.end(); ++iter) {
        if (iter->get_label() == label) {
            return iter;
        }
    }

    return basic_blocks.end();
}

VirtualRegister Function::next_virtual_reg() {
    return VirtualRegister(last_virtual_reg++);
}

std::string Function::next_block_label() {
    return "block." + std::to_string(block_id++);
}

} // namespace banjo::ssa
