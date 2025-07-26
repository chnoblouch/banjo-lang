#ifndef BANJO_SSA_BASIC_BLOCK_H
#define BANJO_SSA_BASIC_BLOCK_H

#include "banjo/ssa/instruction.hpp"
#include "banjo/utils/linked_list.hpp"

#include <string>
#include <vector>

namespace banjo {

namespace ssa {

class BasicBlock {

private:
    LinkedList<Instruction> instrs;
    std::vector<ssa::VirtualRegister> param_regs;
    std::vector<ssa::Type> param_types;
    std::string label;

public:
    BasicBlock(std::string label);
    BasicBlock();

    LinkedList<Instruction> &get_instrs() { return instrs; }
    std::vector<ssa::VirtualRegister> &get_param_regs() { return param_regs; }
    std::vector<ssa::Type> &get_param_types() { return param_types; }
    const std::string &get_label() const { return label; }
    std::string get_debug_label() const { return label.empty() ? "<entry>" : label; }
    bool has_label() const;

    InstrIter append(Instruction instr);
    InstrIter insert_before(InstrIter iter, Instruction instr);
    InstrIter insert_after(InstrIter iter, Instruction instr);
    void remove(InstrIter iter);
    InstrIter replace(InstrIter iter, Instruction instr);

    const Instruction &get_entry() const { return instrs.get_first(); }
    InstrIter get_entry_iter() const { return instrs.get_first_iter(); }
    const Instruction &get_exit() const { return instrs.get_last(); }
    InstrIter get_exit_iter() const { return instrs.get_last_iter(); }

    InstrIter get_header() const { return instrs.get_header(); }
    InstrIter get_trailer() const { return instrs.get_trailer(); }
    unsigned get_size() const { return instrs.get_size(); }

    InstrIter begin() { return instrs.begin(); }
    InstrIter end() { return instrs.end(); }

    bool is_branching() const;
};

typedef LinkedListNode<BasicBlock> BasicBlockNode;
typedef LinkedListIter<BasicBlock> BasicBlockIter;

} // namespace ssa

} // namespace banjo

#endif
