#ifndef BANJO_MCODE_STACK_SLOT_H
#define BANJO_MCODE_STACK_SLOT_H

namespace banjo {

namespace mcode {

class StackSlot {

public:
    enum class Type { GENERIC, ARG_STORE, CALL_ARG };

    static constexpr int INVALID_OFFSET = 0xFFFFFFFF;

private:
    Type type;
    int size;
    int alignment;

    int offset = INVALID_OFFSET;
    int call_arg_index;

public:
    StackSlot(Type type, int size, int alignment) : type(type), size(size), alignment(alignment) {}
    Type get_type() const { return type; }
    int get_size() const { return size; }
    int get_alignment() const { return alignment; }
    int get_offset() const { return offset; }
    int get_call_arg_index() const { return call_arg_index; }

    bool is_defined() const { return offset != INVALID_OFFSET; }

    void set_offset(int offset) { this->offset = offset; }
    void set_call_arg_index(int call_arg_index) { this->call_arg_index = call_arg_index; }
};

} // namespace mcode

} // namespace banjo

#endif
