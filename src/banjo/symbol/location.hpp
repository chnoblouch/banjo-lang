#ifndef LOCATION_H
#define LOCATION_H

#include "symbol/generics.hpp"
#include "symbol/symbol_ref.hpp"
#include "symbol/union.hpp"

#include <variant>
#include <vector>

namespace banjo {

namespace lang {

class LocalVariable;
class Parameter;
class GlobalVariable;
class Constant;
class Function;
class StructField;
class Structure;
class EnumVariant;
class UnionCase;
class UnionCaseField;
struct FunctionSignature;
class Expr;
class DataType;
struct SymbolGroup;

struct LocationElement {

public:
    // clang-format off
    typedef std::variant<
        LocalVariable *,
        Parameter *,
        GlobalVariable *,
        Constant *,
        Function *,
        StructField *,
        SymbolRef,
        EnumVariant *,
        UnionCase *,
        UnionCaseField *,
        int,
        FunctionSignature *,
        Expr *,
        GenericFunc *,
        GenericStruct *,
        SymbolGroup *,
        std::monostate
    > Value;
    // clang-format on

private:
    Value value;
    DataType *type;

public:
    LocationElement(LocalVariable *local, DataType *type) : value(local), type(type) {}
    LocationElement(Parameter *param, DataType *type) : value(param), type(type) {}
    LocationElement(GlobalVariable *global, DataType *type) : value(global), type(type) {}
    LocationElement(Constant *const_, DataType *type) : value(const_), type(type) {}
    LocationElement(Function *func, DataType *type) : value(func), type(type) {}
    LocationElement(StructField *field, DataType *type) : value(field), type(type) {}
    LocationElement(SymbolRef self_symbol, DataType *type) : value(self_symbol), type(type) {}
    LocationElement(EnumVariant *enum_variant, DataType *type) : value(enum_variant), type(type) {}
    LocationElement(UnionCase *union_case, DataType *type) : value(union_case), type(type) {}
    LocationElement(UnionCaseField *union_case_field, DataType *type) : value(union_case_field), type(type) {}
    LocationElement(int tuple_index, DataType *type) : value(tuple_index), type(type) {}
    LocationElement(FunctionSignature *proto_method, DataType *type) : value(proto_method), type(type) {}
    LocationElement(Expr *expr, DataType *type) : value(expr), type(type) {}
    LocationElement(GenericFunc *generic_func, DataType *type) : value(generic_func), type(type) {}
    LocationElement(GenericStruct *generic_struct, DataType *type) : value(generic_struct), type(type) {}
    LocationElement(SymbolGroup *group, DataType *type) : value(group), type(type) {}
    LocationElement(DataType *type) : value(), type(type) {}

    LocalVariable *get_local() const { return std::get<0>(value); }
    Parameter *get_param() const { return std::get<1>(value); }
    GlobalVariable *get_global() const { return std::get<2>(value); }
    Constant *get_const() const { return std::get<3>(value); }
    Function *get_func() const { return std::get<4>(value); }
    StructField *get_field() const { return std::get<5>(value); }
    const SymbolRef &get_self_symbol() const { return std::get<6>(value); }
    EnumVariant *get_enum_variant() const { return std::get<7>(value); }
    UnionCase *get_union_case() const { return std::get<8>(value); }
    UnionCaseField *get_union_case_field() const { return std::get<9>(value); }
    int get_tuple_index() const { return std::get<10>(value); }
    FunctionSignature *get_proto_method() const { return std::get<11>(value); }
    Expr *get_expr() const { return std::get<12>(value); }
    GenericFunc *get_generic_func() const { return std::get<13>(value); }
    GenericStruct *get_generic_struct() const { return std::get<14>(value); }
    SymbolGroup *get_symbol_group() const { return std::get<15>(value); }
    DataType *get_type() const { return type; }

    bool is_local() const { return value.index() == 0; }
    bool is_param() const { return value.index() == 1; }
    bool is_global() const { return value.index() == 2; }
    bool is_const() const { return value.index() == 3; }
    bool is_func() const { return value.index() == 4; }
    bool is_field() const { return value.index() == 5; }
    bool is_self() const { return value.index() == 6; }
    bool is_enum_variant() const { return value.index() == 7; }
    bool is_union_case() const { return value.index() == 8; }
    bool is_union_case_field() const { return value.index() == 9; }
    bool is_tuple_index() const { return value.index() == 10; }
    bool is_proto_method() const { return value.index() == 11; }
    bool is_expr() const { return value.index() == 12; }
    bool is_generic_func() const { return value.index() == 13; }
    bool is_generic_struct() const { return value.index() == 14; }
    bool is_symbol_group() const { return value.index() == 15; }
    bool is_type() const { return value.index() == 16; }
};

class Location {

private:
    std::vector<LocationElement> elements;

public:
    const std::vector<LocationElement> &get_elements() const { return elements; }
    void add_element(LocationElement element);
    void remove_last_element();
    void replace_last_element(LocationElement element);

    DataType *get_type() const;
    Function *get_func() const;
    bool has_root() const;
    const LocationElement &get_last_element() const;
};

} // namespace lang

} // namespace banjo

#endif
