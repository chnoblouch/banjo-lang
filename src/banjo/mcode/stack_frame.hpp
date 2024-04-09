#ifndef MCODE_STACK_FRAME_H
#define MCODE_STACK_FRAME_H

#include "mcode/stack_slot.hpp"
#include <vector>

namespace mcode {

class Function;

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

    StackSlot &get_stack_slot(long index) { return stack_slots[index]; }
    int get_size() { return size; }
    int get_total_size() { return total_size; }

    long new_stack_slot(StackSlot slot);
    void set_size(int size) { this->size = size; }
    void set_total_size(int total_size) { this->total_size = total_size; }
};

} // namespace mcode

#endif
