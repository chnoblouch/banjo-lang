#include "type.hpp"

#include "ir/primitive.hpp"
#include "ir/structure.hpp"

#include <cassert>
#include <utility>

namespace ir {

Type::Type(Base value, int array_length) : value(std::move(value)), array_length(array_length) {}

Type::Type(Primitive primitive, int array_length /* = 1 */)
  : Type(Base{std::in_place_index<0>, primitive}, array_length) {}

Type::Type(ir::Structure *struct_, int array_length /* = 1 */)
  : Type(Base{std::in_place_index<1>, struct_}, array_length) {
    assert(struct_);
}

Type::Type(std::vector<Type> tuple_types, int array_length /* = 1 */)
  : Type(Base{std::in_place_index<2>, tuple_types}, array_length) {}

Type::Type() : Type(Base{std::in_place_index<0>, Primitive::VOID}, 1) {}

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
