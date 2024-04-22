#include "type.hpp"

#include "ir/primitive.hpp"
#include "ir/structure.hpp"

#include <cassert>
#include <utility>

namespace ir {

Type::Type(Base value, int ptr_depth, int array_length)
  : value(std::move(value)),
    ptr_depth(ptr_depth),
    array_length(array_length) {
    assert(ptr_depth >= 0 && "pointer depth must not be negative");
}

Type::Type(Primitive primitive, int ptr_depth /* = 0 */, int array_length /* = 1 */)
  : Type(Base{std::in_place_index<0>, primitive}, ptr_depth, array_length) {}

Type::Type(ir::Structure *struct_, int ptr_depth /* = 0 */, int array_length /* = 1 */)
  : Type(Base{std::in_place_index<1>, struct_}, ptr_depth, array_length) {
    assert(struct_);
}

Type::Type(std::vector<Type> tuple_types, int ptr_depth /* = 0 */, int array_length /* = 1 */)
  : Type(Base{std::in_place_index<2>, tuple_types}, ptr_depth, array_length) {}

Type::Type() : Type(Base{std::in_place_index<0>, Primitive::VOID}, 0, 1) {}

bool Type::is_primitive(Primitive primitive) const {
    return is_primitive() && get_primitive() == primitive && is_base_only();
}

bool Type::is_floating_point() const {
    if (!is_primitive()) {
        return false;
    }

    return (get_primitive() == Primitive::F32 || get_primitive() == Primitive::F64) && is_base_only();
}

bool Type::is_integer() const {
    return is_base_only() && !is_floating_point();
}

bool Type::is_base_only() const {
    return ptr_depth == 0 && array_length == 1;
}

Type Type::ref() const {
    return {value, ptr_depth + 1, 1};
}

Type Type::deref() const {
    return {value, ptr_depth - 1, 1};
}

Type Type::base() const {
    return {value, ptr_depth, 1};
}

} // namespace ir
