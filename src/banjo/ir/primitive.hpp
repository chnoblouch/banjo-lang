#ifndef IR_PRIMITIVE_H
#define IR_PRIMITIVE_H

namespace banjo {

namespace ir {

enum class Primitive {
    VOID,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
    ADDR,
};

} // namespace ir

} // namespace banjo

#endif
