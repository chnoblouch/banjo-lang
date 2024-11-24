#include "type.hpp"

#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/structure.hpp"

#include <cassert>

namespace banjo {

namespace ssa {

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
