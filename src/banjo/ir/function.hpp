#ifndef IR_FUNCTION_H
#define IR_FUNCTION_H

#include "ir/basic_block.hpp"
#include "ir/function_decl.hpp"
#include "ir/virtual_register.hpp"

#include <vector>

namespace ir {

class Function : public FunctionDecl {

private:
    LinkedList<BasicBlock> basic_blocks;

    ir::VirtualRegister last_virtual_reg = 0;
    int last_float_label_id = 0;

public:
    Function();
    Function(std::string name, std::vector<Type> params, Type return_type, CallingConv calling_conv);

    LinkedList<BasicBlock> &get_basic_blocks() { return basic_blocks; }

    BasicBlock &get_entry_block() { return *basic_blocks.begin(); }
    BasicBlockIter get_entry_block_iter() { return basic_blocks.begin(); }
    std::vector<ir::VirtualRegister> &get_param_regs();

    BasicBlockIter create_block(std::string label);
    void append_block(BasicBlockIter block);
    void merge_blocks(BasicBlockIter first, BasicBlockIter second);
    BasicBlockIter split_block_after(BasicBlockIter block, InstrIter instr);
    BasicBlockIter find_basic_block(const std::string &label);
    VirtualRegister next_virtual_reg();
    std::string next_float_label();

    BasicBlockIter begin() { return basic_blocks.begin(); }
    BasicBlockIter end() { return basic_blocks.end(); }
};

} // namespace ir

#endif
