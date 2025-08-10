#ifndef BANJO_SSA_PRIMITIVE_H
#define BANJO_SSA_PRIMITIVE_H

namespace banjo {

namespace ssa {

enum class Primitive {
    VOID,
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    ADDR,
};

} // namespace ssa

} // namespace banjo

#endif
