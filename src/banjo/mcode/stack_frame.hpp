#ifndef MCODE_STACK_FRAME_H
#define MCODE_STACK_FRAME_H

#include "banjo/mcode/stack_slot.hpp"
#include <vector>

namespace banjo {

namespace mcode {

class Function;

typedef unsigned StackSlotID;

class StackFrame {

private:
    std::vector<StackSlot> stack_slots;
    std::vector<int> call_arg_slot_indices;
    std::vector<unsigned> reg_save_slot_indices;
    int size;
    int total_size;

public:
    std::vector<StackSlot> &get_stack_slots() { return stack_slots; }
    std::vector<int> &get_call_arg_slot_indices() { return call_arg_slot_indices; }
    std::vector<unsigned> &get_reg_save_slot_indices() { return reg_save_slot_indices; }

    StackSlot &get_stack_slot(StackSlotID id) { return stack_slots[id]; }
    int get_size() { return size; }
    int get_total_size() { return total_size; }

    StackSlotID new_stack_slot(StackSlot slot);
    void set_size(int size) { this->size = size; }
    void set_total_size(int total_size) { this->total_size = total_size; }
};

} // namespace mcode

} // namespace banjo

#endif
