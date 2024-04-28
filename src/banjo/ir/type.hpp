#ifndef IR_TYPE_H
#define IR_TYPE_H

#include "ir/primitive.hpp"

#include <variant>
#include <vector>

namespace ir {

class Structure;

class Type {

public:
    typedef std::variant<Primitive, Structure *, std::vector<Type>> Base;

private:
    Base value;
    int array_length;

private:
    Type(Base value, int array_length);

public:
    Type(Primitive primitive, int array_length = 1);
    Type(Structure *struct_, int array_length = 1);
    Type(std::vector<Type> tuple_types, int array_length = 1);
    Type();

    bool is_primitive() const { return value.index() == 0; }
    bool is_struct() const { return value.index() == 1; }
    bool is_tuple() const { return value.index() == 2; }

    Primitive get_primitive() const { return std::get<0>(value); }
    Structure *get_struct() const { return std::get<1>(value); }
    const std::vector<Type> &get_tuple_types() const { return std::get<2>(value); }
    int get_array_length() const { return array_length; }

    void set_array_length(int array_length) { this->array_length = array_length; }

    bool is_primitive(Primitive primitive) const;
    bool is_floating_point() const;
    bool is_integer() const;

    friend bool operator==(const Type &left, const Type &right) {
        return left.value == right.value && left.array_length == right.array_length;
    }

    friend bool operator!=(const Type &left, const Type &right) { return !(left == right); }
};

} // namespace ir

#endif
