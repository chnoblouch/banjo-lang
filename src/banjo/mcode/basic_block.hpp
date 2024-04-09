#ifndef MCODE_BASIC_BLOCK_H
#define MCODE_BASIC_BLOCK_H

#include "mcode/instruction.hpp"
#include "utils/linked_list.hpp"

#include <vector>

namespace mcode {

class Function;
class BasicBlock;

typedef LinkedListNode<BasicBlock> BasicBlockNode;
typedef LinkedListIter<BasicBlock> BasicBlockIter;

class BasicBlock {

private:
    LinkedList<Instruction> instrs;
    std::string label;
    std::vector<PhysicalReg> params;
    std::vector<BasicBlockIter> predecessors;
    std::vector<BasicBlockIter> successors;
    BasicBlockIter domtree_parent;
    std::vector<BasicBlockIter> domtree_children;
    Function *func;

public:
    BasicBlock();
    BasicBlock(std::string label, Function *func);
    BasicBlock(Function *func);

    LinkedList<Instruction> &get_instrs() { return instrs; }
    std::string get_label() { return label; }
    std::vector<PhysicalReg> &get_params() { return params; }
    std::vector<BasicBlockIter> &get_predecessors() { return predecessors; }
    std::vector<BasicBlockIter> &get_successors() { return successors; }
    BasicBlockIter get_domtree_parent() { return domtree_parent; }
    std::vector<BasicBlockIter> &get_domtree_children() { return domtree_children; }
    Function *get_func() { return func; }

    InstrIter append(Instruction instr);
    InstrIter insert_before(InstrIter iter, Instruction instr);
    InstrIter insert_after(InstrIter iter, Instruction instr);
    void remove(InstrIter iter);
    InstrIter replace(InstrIter iter, Instruction instr);

    void set_domtree_parent(BasicBlockIter iter) { domtree_parent = iter; }

    InstrIter begin() { return instrs.begin(); }
    InstrIter end() { return instrs.end(); }
};

} // namespace mcode

#endif
