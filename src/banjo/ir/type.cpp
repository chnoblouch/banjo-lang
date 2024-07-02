#include "type.hpp"

#include "ir/primitive.hpp"
#include "ir/structure.hpp"

#include <cassert>

namespace banjo {

namespace ir {

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

} // namespace ir

} // namespace banjo
