#ifndef IR_TYPE_H
#define IR_TYPE_H

#include "ir/primitive.hpp"
#include "utils/macros.hpp"

#include <cassert>

namespace ir {

class Structure;

class Type {

private:
    union {
        Primitive primitive;
        Structure *struct_;
    };

    unsigned kind;
    unsigned array_length;

public:
    Type(Primitive primitive, unsigned array_length = 1) : primitive(primitive), kind(0), array_length(array_length) {}

    Type(Structure *struct_, unsigned array_length = 1) : struct_(struct_), kind(1), array_length(array_length) {
        assert(struct_);
    }

    Type() : Type(Primitive::VOID) {}

    bool is_primitive() const { return kind == 0; }
    bool is_struct() const { return kind == 1; }

    Primitive get_primitive() const {
        assert(is_primitive());
        return primitive;
    }

    Structure *get_struct() const {
        assert(is_struct());
        return struct_;
    }

    unsigned get_array_length() const { return array_length; }
    void set_array_length(unsigned array_length) { this->array_length = array_length; }

    bool is_primitive(Primitive primitive) const;
    bool is_floating_point() const;
    bool is_integer() const;

    friend bool operator==(const Type &left, const Type &right) {
        if (left.kind != right.kind || left.array_length != right.array_length) {
            return false;
        }

        if (left.kind == 0) {
            return left.primitive == right.primitive;
        } else if (left.kind == 1) {
            return left.struct_ == right.struct_;
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    friend bool operator!=(const Type &left, const Type &right) { return !(left == right); }
};

} // namespace ir

#endif
