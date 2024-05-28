#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include "symbol/generics.hpp"
#include "symbol/method_table.hpp"

#include <string>
#include <variant>
#include <vector>

namespace lang {

class DataType;
class Structure;
class Enumeration;
class Union;
class UnionCase;
class Protocol;
class DataTypeManager;
class Function;

enum PrimitiveType {
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
    BOOL,
    ADDR,
    VOID,
};

struct FunctionType {
    std::vector<DataType *> param_types;
    DataType *return_type = nullptr;

    friend bool operator==(const FunctionType &lhs, const FunctionType &rhs);
    friend bool operator!=(const FunctionType &lhs, const FunctionType &rhs) { return !(lhs == rhs); }
};

struct StaticArrayType {
    DataType *base_type;
    unsigned length;

    friend bool operator==(const StaticArrayType &lhs, const StaticArrayType &rhs);
    friend bool operator!=(const StaticArrayType &lhs, const StaticArrayType &rhs) { return !(lhs == rhs); }
};

struct Tuple {
    std::vector<DataType *> types;

    friend bool operator==(const Tuple &lhs, const Tuple &rhs);
    friend bool operator!=(const Tuple &lhs, const Tuple &rhs) { return !(lhs == rhs); }
};

struct GenericType {
    const std::vector<std::string> *generic_entity_params;
    int index;
};

class DataType {

public:
    enum class Kind {
        PRIMITIVE,
        STRUCT,
        ENUM,
        UNION,
        UNION_CASE,
        PROTO,
        POINTER,
        ARRAY,
        FUNCTION,
        STATIC_ARRAY,
        TUPLE,
        CLOSURE,
        GENERIC,
    };

private:
    Kind kind;
    std::variant<
        std::monostate,
        PrimitiveType,
        Structure *,
        Enumeration *,
        UnionCase *,
        Union *,
        Protocol *,
        DataType *,
        FunctionType,
        StaticArrayType,
        Tuple,
        GenericType>
        value;

public:
    DataType();
    DataType(PrimitiveType type);
    Kind get_kind() const { return kind; }
    PrimitiveType get_primitive_type() const;
    Structure *get_structure() const;
    Enumeration *get_enumeration() const;
    Union *get_union() const;
    UnionCase *get_union_case() const;
    Protocol *get_protocol() const;
    DataType *get_base_data_type() const;
    const FunctionType &get_function_type() const;
    const StaticArrayType &get_static_array_type() const;
    const Tuple &get_tuple() const;
    GenericType get_generic_type() const;

    void set_to_primitive(PrimitiveType primitive_type);
    void set_to_structure(Structure *structure);
    void set_to_enumeration(Enumeration *enumeration);
    void set_to_union(Union *union_);
    void set_to_union_case(UnionCase *union_case);
    void set_to_protocol(Protocol *protocol);
    void set_to_pointer(DataType *base_data_type);
    void set_to_array(DataType *base_data_type);
    void set_to_function(FunctionType function_type);
    void set_to_static_array(StaticArrayType static_array_type);
    void set_to_tuple(Tuple tuple);
    void set_to_closure(FunctionType closure_type);
    void set_to_generic(GenericType generic_type);

    bool is_primitive_of_type(PrimitiveType primitive_type);
    bool is_signed_int();
    bool is_unsigned_int();
    bool is_floating_point();
    bool is_struct(const std::string &name);
    bool is_ptr_to(Kind base_kind);
    bool has_methods();
    MethodTable &get_method_table();

    DataType *ref(DataTypeManager &manager);

    bool equals(const DataType *other) const;

    static bool equal(
        const std::vector<DataType *> &a,
        const std::vector<DataType *> &b,
        unsigned a_start = 0,
        unsigned b_start = 0
    );
};

} // namespace lang

#endif
