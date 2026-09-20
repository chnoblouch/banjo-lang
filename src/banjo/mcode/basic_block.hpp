#ifndef BANJO_MCODE_BASIC_BLOCK_H
#define BANJO_MCODE_BASIC_BLOCK_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/utils/linked_list.hpp"

#include <vector>

namespace banjo::mcode {

struct BasicBlock;

typedef LinkedListNode<BasicBlock> BasicBlockNode;
typedef LinkedListIter<BasicBlock> BasicBlockIter;

struct BasicBlock {
    LinkedList<Instruction> instrs;
    std::string label;
    std::vector<PhysicalReg> params;
    std::vector<BasicBlockIter> successors;

    std::string debug_label() { return label.empty() ? "<entry>" : label; }

    InstrIter append(Instruction instr);
    InstrIter insert_before(InstrIter iter, Instruction instr);
    InstrIter insert_after(InstrIter iter, Instruction instr);
    void remove(InstrIter iter);
    InstrIter replace(InstrIter iter, Instruction instr);

    InstrIter begin() { return instrs.begin(); }
    InstrIter end() { return instrs.end(); }
};

} // namespace banjo::mcode

#endif
