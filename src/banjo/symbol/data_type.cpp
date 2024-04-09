#include "data_type.hpp"

#include "symbol/data_type_manager.hpp"
#include "symbol/structure.hpp"
#include "symbol/union.hpp"
#include "utils/macros.hpp"

#include <cassert>

namespace lang {

DataType::DataType() {}

DataType::DataType(PrimitiveType type) {
    set_to_primitive(type);
}

PrimitiveType DataType::get_primitive_type() const {
    return std::get<PrimitiveType>(value);
}

Structure *DataType::get_structure() const {
    return std::get<Structure *>(value);
}

Enumeration *DataType::get_enumeration() const {
    return std::get<Enumeration *>(value);
}

Union *DataType::get_union() const {
    return std::get<Union *>(value);
}

UnionCase *DataType::get_union_case() const {
    return std::get<UnionCase *>(value);
}

DataType *DataType::get_base_data_type() const {
    return std::get<DataType *>(value);
}

const FunctionType &DataType::get_function_type() const {
    return std::get<FunctionType>(value);
}

const StaticArrayType &DataType::get_static_array_type() const {
    return std::get<StaticArrayType>(value);
}

const Tuple &DataType::get_tuple() const {
    return std::get<Tuple>(value);
}

GenericType DataType::get_generic_type() const {
    return std::get<GenericType>(value);
}

void DataType::set_to_primitive(PrimitiveType primitive_type) {
    kind = Kind::PRIMITIVE;
    value = primitive_type;
}

void DataType::set_to_structure(Structure *structure) {
    assert(structure);

    kind = Kind::STRUCT;
    value = structure;
}

void DataType::set_to_enumeration(Enumeration *enumeration) {
    kind = Kind::ENUM;
    value = enumeration;
}

void DataType::set_to_union(Union *union_) {
    kind = Kind::UNION;
    value = union_;
}

void DataType::set_to_union_case(UnionCase *union_case) {
    kind = Kind::UNION_CASE;
    value = union_case;
}

void DataType::set_to_pointer(DataType *base_data_type) {
    kind = Kind::POINTER;
    value = base_data_type;
}

void DataType::set_to_array(DataType *base_data_type) {
    kind = Kind::ARRAY;
    value = base_data_type;
}

void DataType::set_to_function(FunctionType function_type) {
    kind = Kind::FUNCTION;
    value = function_type;
}

void DataType::set_to_static_array(StaticArrayType static_array_type) {
    kind = Kind::STATIC_ARRAY;
    value = static_array_type;
}

void DataType::set_to_tuple(Tuple tuple) {
    kind = Kind::TUPLE;
    value = tuple;
}

void DataType::set_to_closure(FunctionType closure_type) {
    kind = Kind::CLOSURE;
    value = closure_type;
}

void DataType::set_to_generic(GenericType generic_type) {
    kind = Kind::GENERIC;
    value = generic_type;
}

bool DataType::is_primitive_of_type(PrimitiveType primitive_type) {
    return kind == Kind::PRIMITIVE && get_primitive_type() == primitive_type;
}

bool DataType::is_signed_int() {
    if (kind != Kind::PRIMITIVE) {
        return false;
    }

    PrimitiveType p = get_primitive_type();
    return p == PrimitiveType::I8 || p == PrimitiveType::I16 || p == PrimitiveType::I32 || p == PrimitiveType::I64;
}

bool DataType::is_unsigned_int() {
    if (kind != Kind::PRIMITIVE) {
        return false;
    }

    PrimitiveType p = get_primitive_type();
    return p == PrimitiveType::U8 || p == PrimitiveType::U16 || p == PrimitiveType::U32 || p == PrimitiveType::U64;
}

bool DataType::is_floating_point() {
    return is_primitive_of_type(F32) || is_primitive_of_type(F64);
}

bool DataType::is_struct(const std::string &name) {
    return kind == Kind::STRUCT && get_structure()->get_name() == name;
}

bool DataType::has_methods() {
    return kind == Kind::STRUCT || kind == Kind::UNION;
}

MethodTable &DataType::get_method_table() {
    if (kind == Kind::STRUCT) {
        return get_structure()->get_method_table();
    } else if (kind == Kind::UNION) {
        return get_union()->get_method_table();
    } else {
        ASSERT_UNREACHABLE;
    }
}

DataType *DataType::ref(DataTypeManager &manager) {
    DataType *type = manager.new_data_type();
    type->set_to_pointer(this);
    return type;
}

bool DataType::equals(const DataType *other) const {
    if (kind != other->kind) {
        return false;
    }

    switch (kind) {
        case Kind::PRIMITIVE: return get_primitive_type() == other->get_primitive_type();
        case Kind::STRUCT: return get_structure() == other->get_structure();
        case Kind::ENUM: return get_enumeration() == other->get_enumeration();
        case Kind::UNION: return get_union() == other->get_union();
        case Kind::UNION_CASE: return get_union_case() == other->get_union_case();
        case Kind::POINTER:
        case Kind::ARRAY: return get_base_data_type()->equals(other->get_base_data_type());
        case Kind::FUNCTION:
        case Kind::CLOSURE: {
            FunctionType func_type_a = get_function_type();
            FunctionType func_type_b = other->get_function_type();

            if (!equal(func_type_a.param_types, func_type_b.param_types)) {
                return false;
            }

            return func_type_a.return_type->equals(func_type_b.return_type);
        }
        case Kind::STATIC_ARRAY: {
            DataType *base_type_a = get_static_array_type().base_type;
            DataType *base_type_b = other->get_static_array_type().base_type;
            unsigned length_a = get_static_array_type().length;
            unsigned length_b = other->get_static_array_type().length;
            return base_type_a->equals(base_type_b) && length_a == length_b;
        }
        case Kind::TUPLE: return equal(get_tuple().types, other->get_tuple().types);
        case Kind::GENERIC: return false;
    }
}

bool DataType::equal(
    const std::vector<DataType *> &a,
    const std::vector<DataType *> &b,
    unsigned a_start /* = 0 */,
    unsigned b_start /* = 0 */
) {
    if (a.size() - a_start != b.size() - b_start) {
        return false;
    }

    for (int i = 0; i < a.size() - a_start; i++) {
        if (!a[i + a_start]->equals(b[i + b_start])) {
            return false;
        }
    }

    return true;
}

} // namespace lang
