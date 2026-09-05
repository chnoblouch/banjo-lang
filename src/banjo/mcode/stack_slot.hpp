#ifndef BANJO_MCODE_STACK_SLOT_H
#define BANJO_MCODE_STACK_SLOT_H

namespace banjo::mcode {

struct StackSlot {
    enum class Type {
        GENERIC,
        ARG_STORE,
        CALL_ARG,
    };

    static constexpr unsigned INVALID_OFFSET = 0xFFFFFFFF;

    Type type;
    unsigned size;
    unsigned alignment;

    unsigned offset = INVALID_OFFSET;
    unsigned call_arg_index = 0;

    bool is_defined() const { return offset != INVALID_OFFSET; }
};

} // namespace banjo::mcode

#endif
