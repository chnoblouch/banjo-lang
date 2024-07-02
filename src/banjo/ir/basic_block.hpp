#ifndef IR_BASIC_BLOCK_H
#define IR_BASIC_BLOCK_H

#include "banjo/ir/instruction.hpp"
#include "banjo/utils/linked_list.hpp"

#include <string>
#include <vector>

namespace banjo {

namespace ir {

class BasicBlock {

private:
    LinkedList<Instruction> instrs;
    std::vector<ir::VirtualRegister> param_regs;
    std::vector<ir::Type> param_types;
    std::string label;

public:
    BasicBlock(std::string label);
    BasicBlock();

    LinkedList<Instruction> &get_instrs() { return instrs; }
    std::vector<ir::VirtualRegister> &get_param_regs() { return param_regs; }
    std::vector<ir::Type> &get_param_types() { return param_types; }
    const std::string &get_label() const { return label; }
    std::string get_debug_label() const { return label.empty() ? "<entry>" : label; }
    bool has_label() const;

    InstrIter append(Instruction instr);
    InstrIter insert_before(InstrIter iter, Instruction instr);
    InstrIter insert_after(InstrIter iter, Instruction instr);
    void remove(InstrIter iter);
    InstrIter replace(InstrIter iter, Instruction instr);

    InstrIter get_header() const { return instrs.get_header(); }
    InstrIter get_trailer() const { return instrs.get_trailer(); }
    unsigned get_size() const { return instrs.get_size(); }

    InstrIter begin() { return instrs.begin(); }
    InstrIter end() { return instrs.end(); }
};

typedef LinkedListNode<BasicBlock> BasicBlockNode;
typedef LinkedListIter<BasicBlock> BasicBlockIter;

} // namespace ir

} // namespace banjo

#endif
