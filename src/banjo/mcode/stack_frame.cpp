#include "stack_frame.hpp"

namespace banjo::mcode {

StackSlotID StackFrame::new_stack_slot(StackSlot slot) {
    stack_slots.push_back(slot);
    StackSlotID index = static_cast<StackSlotID>(stack_slots.size() - 1);

    if (slot.type == StackSlot::Type::CALL_ARG) {
        call_arg_slot_indices.push_back(index);
    }

    return index;
}

StackSlotID StackFrame::create_call_arg_slot(unsigned index, unsigned size, unsigned alignment) {
    if (call_arg_slot_indices.size() <= index) {
        mcode::StackSlot slot{
            .type = mcode::StackSlot::Type::CALL_ARG,
            .size = size,
            .alignment = alignment,
            .call_arg_index = index,
        };

        return new_stack_slot(slot);
    } else {
        return call_arg_slot_indices[index];
    }
}

unsigned StackFrame::offset_of(StackAddress stack_addr) {
    return stack_slots[stack_addr.slot].offset + stack_addr.offset;
}

} // namespace banjo::mcode
