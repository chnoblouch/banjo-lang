#ifndef BANJO_MCODE_FUNCTION_H
#define BANJO_MCODE_FUNCTION_H

#include "banjo/mcode/basic_block.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/ssa/type.hpp"

#include <any>
#include <string>
#include <utility>
#include <vector>

namespace banjo {

namespace mcode {

class CallingConvention;

struct Parameter {
    ssa::Type type;
    std::variant<Register, StackSlotID> storage;
};

struct UnwindInfo {
    unsigned alloc_size;
};

class Function {

private:
    std::string name;
    LinkedList<BasicBlock> basic_blocks;
    std::vector<Parameter> parameters;
    CallingConvention *calling_conv;
    StackFrame stack_frame;
    UnwindInfo unwind_info;
    std::any target_data;

public:
    Function(std::string name, CallingConvention *calling_conv);

    std::string get_name() { return name; }
    LinkedList<BasicBlock> &get_basic_blocks() { return basic_blocks; }
    std::vector<Parameter> &get_parameters() { return parameters; }
    CallingConvention *get_calling_conv() { return calling_conv; }
    StackFrame &get_stack_frame() { return stack_frame; }
    UnwindInfo &get_unwind_info() { return unwind_info; }
    const std::any &get_target_data() { return target_data; }

    void set_target_data(std::any target_data) { this->target_data = std::move(target_data); }

    BasicBlockIter begin() { return basic_blocks.begin(); }
    BasicBlockIter end() { return basic_blocks.end(); }
};

} // namespace mcode

} // namespace banjo

#endif
