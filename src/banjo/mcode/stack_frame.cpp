#include "stack_frame.hpp"

namespace mcode {

StackSlotID StackFrame::new_stack_slot(StackSlot slot) {
    stack_slots.push_back(slot);
    StackSlotID index = static_cast<StackSlotID>(stack_slots.size() - 1);

    if (slot.get_type() == StackSlot::Type::CALL_ARG) {
        call_arg_slot_indices.push_back(index);
    }

    return index;
}

} // namespace mcode
