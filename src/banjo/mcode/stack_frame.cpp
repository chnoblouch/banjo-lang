#include "stack_frame.hpp"

namespace banjo {

namespace mcode {

StackSlotID StackFrame::new_stack_slot(StackSlot slot) {
    stack_slots.push_back(slot);
    StackSlotID index = static_cast<StackSlotID>(stack_slots.size() - 1);

    if (slot.get_type() == StackSlot::Type::CALL_ARG) {
        call_arg_slot_indices.push_back(index);
    }

    return index;
}

StackSlotID StackFrame::create_call_arg_slot(unsigned index, unsigned size, unsigned alignment) {
    if (call_arg_slot_indices.size() <= index) {
        mcode::StackSlot stack_slot(mcode::StackSlot::Type::CALL_ARG, size, alignment);
        stack_slot.set_call_arg_index(index);
        return new_stack_slot(stack_slot);
    } else {
        return call_arg_slot_indices[index];
    }
}

} // namespace mcode

} // namespace banjo
