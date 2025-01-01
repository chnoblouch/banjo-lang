#ifndef MCODE_REGISTER_H
#define MCODE_REGISTER_H

#include "banjo/utils/bit_set.hpp"
#include "banjo/utils/macros.hpp"

#include <unordered_set>

namespace banjo {

namespace mcode {

class BasicBlock;

typedef unsigned VirtualReg;
typedef unsigned PhysicalReg;

class Register {

private:
    enum class Type {
        VIRTUAL_REG,
        PHYSICAL_REG,
        STACK_SLOT,
    };

    Type type;
    long value;

public:
    static Register from_virtual(long id) { return Register(Type::VIRTUAL_REG, id); }
    static Register from_physical(long id) { return Register(Type::PHYSICAL_REG, id); }
    static Register from_stack_slot(long slot) { return Register(Type::STACK_SLOT, slot); }

    Register(Type type, long value) : type(type), value(value) {}

    Type get_type() const { return type; }
    bool is_virtual_reg() const { return type == Type::VIRTUAL_REG; }
    bool is_physical_reg() const { return type == Type::PHYSICAL_REG; }
    bool is_stack_slot() const { return type == Type::STACK_SLOT; }

    VirtualReg get_virtual_reg() const { return static_cast<VirtualReg>(value); }
    PhysicalReg get_physical_reg() const { return static_cast<PhysicalReg>(value); }
    long get_stack_slot() const { return value; }

    bool is_virtual_reg(long id) const { return is_virtual_reg() && value == id; }
    bool is_physical_reg(long id) const { return is_physical_reg() && value == id; }
    bool is_stack_slot(long slot) const { return is_stack_slot() && value == slot; }

    friend bool operator==(const Register &left, const Register &right) {
        return left.type == right.type && left.value == right.value;
    }

    friend bool operator!=(const Register &left, const Register &right) { return !(left == right); }

    friend class RegisterSet;
};

class RegisterSet {

public:
    class Iterator {

    private:
        BitSet::Iterator iter;

    public:
        Iterator(BitSet::Iterator iter) : iter(iter) {}

        Register operator*() const {
            unsigned position = *iter;

            if (position & 1) {
                return Register::from_physical(position >> 1);
            } else {
                return Register::from_virtual(position >> 1);
            }
        }

        Iterator &operator++() {
            ++iter;
            return *this;
        }

        bool operator==(const Iterator &other) const { return iter == other.iter; }
        bool operator!=(const Iterator &other) const { return iter != other.iter; }
    };

private:
    BitSet set;

public:
    bool contains(Register reg) const { return set.get(get_bit_position(reg)); }
    unsigned size() const { return set.size(); }

    void insert(Register reg) { set.set(get_bit_position(reg)); }
    void remove(Register reg) { set.clear(get_bit_position(reg)); }
    void union_with(const RegisterSet &other) { set.union_with(other.set); }

    Iterator begin() const { return Iterator(set.begin()); }
    Iterator end() const { return Iterator(set.end()); }

private:
    static unsigned get_bit_position(Register reg) {
        if (reg.is_virtual_reg()) {
            return (reg.value << 1) | 0;
        } else if (reg.is_physical_reg()) {
            return (reg.value << 1) | 1;
        } else {
            ASSERT_UNREACHABLE;
        }
    }
};

} // namespace mcode

} // namespace banjo

template <>
struct std::hash<banjo::mcode::Register> {
    std::size_t operator()(const banjo::mcode::Register &reg) const noexcept {
        return (std::size_t)(reg.get_type()) << 32 | (std::size_t)reg.get_type();
    }
};

#endif
