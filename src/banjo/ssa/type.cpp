#include "type.hpp"

#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/structure.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::ssa {

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
    if (array_length != 1 || !is_primitive()) {
        return false;
    }

    switch (get_primitive()) {
        case Primitive::I8:
        case Primitive::I16:
        case Primitive::I32:
        case Primitive::I64:
        case Primitive::U8:
        case Primitive::U16:
        case Primitive::U32:
        case Primitive::U64:
        case Primitive::ADDR: return true;
        case Primitive::VOID:
        case Primitive::F32:
        case Primitive::F64: return false;
    }
}

bool Type::is_struct_aggregate() const {
    return is_struct() && !struct_->is_union && array_length == 1;
}

} // namespace banjo::ssa
