#ifndef BANJO_SSA_FUNCTION_H
#define BANJO_SSA_FUNCTION_H

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <string>

namespace banjo {

namespace ssa {

struct FunctionDecl {
    std::string name;
    FunctionType type;
    bool global = false;
};

class Function {

public:
    std::string name;
    FunctionType type;
    bool global = false;

    LinkedList<BasicBlock> basic_blocks;

    ssa::VirtualRegister last_virtual_reg = 0;
    int last_float_label_id = 0;

public:
    Function(std::string name, FunctionType type);

    LinkedList<BasicBlock> &get_basic_blocks() { return basic_blocks; }

    BasicBlock &get_entry_block() { return *basic_blocks.begin(); }
    BasicBlockIter get_entry_block_iter() { return basic_blocks.begin(); }

    BasicBlockIter create_block(std::string label);
    void append_block(BasicBlockIter block);
    void merge_blocks(BasicBlockIter first, BasicBlockIter second);
    BasicBlockIter split_block_after(BasicBlockIter block, InstrIter instr);
    BasicBlockIter find_basic_block(const std::string &label);
    VirtualRegister next_virtual_reg();
    std::string next_float_label();

    void set_next_reg(ssa::VirtualRegister reg) { last_virtual_reg = reg; }

    BasicBlockIter begin() { return basic_blocks.begin(); }
    BasicBlockIter end() { return basic_blocks.end(); }
};

} // namespace ssa

} // namespace banjo

#endif
