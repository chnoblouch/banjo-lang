#ifndef BANJO_MCODE_FUNCTION_H
#define BANJO_MCODE_FUNCTION_H

#include "banjo/mcode/basic_block.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/ssa/type.hpp"

#include <any>
#include <string>
#include <vector>

namespace banjo::mcode {

class CallingConvention;

struct Parameter {
    ssa::Type type;
    std::variant<Register, StackSlotID> storage;
};

struct UnwindInfo {
    unsigned alloc_size;
};

struct Function {
    std::string name;
    LinkedList<BasicBlock> basic_blocks;
    std::vector<Parameter> parameters;
    CallingConvention *calling_conv = nullptr;
    StackFrame stack_frame;
    UnwindInfo unwind_info;
    std::any target_data;
    std::string debug_name;

    BasicBlockIter begin() { return basic_blocks.begin(); }
    BasicBlockIter end() { return basic_blocks.end(); }
};

} // namespace banjo::mcode

#endif
