#include "basic_block.hpp"

#include <utility>

namespace banjo {

namespace ir {

BasicBlock::BasicBlock(std::string label) : label(std::move(label)) {}

BasicBlock::BasicBlock() {}

bool BasicBlock::has_label() const {
    return !label.empty();
}

InstrIter BasicBlock::append(Instruction instr) {
    return instrs.append(std::move(instr));
}

InstrIter BasicBlock::insert_before(InstrIter iter, Instruction instr) {
    return instrs.insert_before(iter, std::move(instr));
}

InstrIter BasicBlock::insert_after(InstrIter iter, Instruction instr) {
    return instrs.insert_after(iter, std::move(instr));
}

void BasicBlock::remove(InstrIter iter) {
    instrs.remove(iter);
}

InstrIter BasicBlock::replace(InstrIter iter, Instruction instr) {
    return instrs.replace(iter, std::move(instr));
}

} // namespace ir

} // namespace banjo
