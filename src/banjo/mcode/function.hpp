#ifndef MCODE_FUNCTION_H
#define MCODE_FUNCTION_H

#include "mcode/basic_block.hpp"
#include "mcode/calling_convention.hpp"
#include "mcode/parameter.hpp"
#include "mcode/stack_frame.hpp"

#include <string>
#include <vector>

namespace mcode {

struct UnwindInfo {
    unsigned int alloc_size;
};

class Function {

private:
    std::string name;
    LinkedList<BasicBlock> basic_blocks;
    std::vector<Parameter> parameters;
    CallingConvention *calling_conv;
    StackFrame stack_frame;
    UnwindInfo unwind_info;

public:
    Function(std::string name, CallingConvention *calling_conv);

    std::string get_name() { return name; }
    LinkedList<BasicBlock> &get_basic_blocks() { return basic_blocks; }
    std::vector<Parameter> &get_parameters() { return parameters; }
    CallingConvention *get_calling_conv() { return calling_conv; }
    StackFrame &get_stack_frame() { return stack_frame; }
    UnwindInfo &get_unwind_info() { return unwind_info; }

    BasicBlockIter begin() { return basic_blocks.begin(); }
    BasicBlockIter end() { return basic_blocks.end(); }
};

} // namespace mcode

#endif
