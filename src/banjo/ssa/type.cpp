#include "type.hpp"

#include "banjo/ssa/primitive.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace ssa {

Type Type::sized(unsigned size) {
    switch (size) {
        case 0: return Primitive::VOID;
        case 1: return Primitive::U8;
        case 2: return Primitive::U16;
        case 3:
        case 4: return Primitive::U32;
        case 5:
        case 6:
        case 7:
        case 8: return Primitive::U64;
        default: ASSERT_UNREACHABLE;
    }
}

bool Type::is_primitive(Primitive primitive) const {
    return array_length == 1 && is_primitive() && get_primitive() == primitive;
}

bool Type::is_floating_point() const {
    if (array_length != 1 || !is_primitive()) {
        return false;
    }

    return get_primitive() == Primitive::F32 || get_primitive() == Primitive::F64;
}

bool Type::is_integer() const {
    return array_length == 1 && !is_floating_point();
}

} // namespace ssa

} // namespace banjo
