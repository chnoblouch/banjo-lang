#ifndef MCODE_REGISTER_H
#define MCODE_REGISTER_H

#include "banjo/utils/bit_set.hpp"

#include <unordered_set> // IWYU pragma: keep

namespace banjo {

namespace mcode {

class BasicBlock;

typedef unsigned VirtualReg;
typedef unsigned PhysicalReg;

class Register {

private:
    unsigned value;

public:
    static Register from_virtual(VirtualReg reg) { return Register((reg << 1) | 0); }
    static Register from_physical(PhysicalReg reg) { return Register((reg << 1) | 1); }

    Register() : value(0xFFFFFFFF) {}

private:
    Register(unsigned value) : value(value) {}

public:
    unsigned internal_value() const { return value; }

    bool is_virtual() const { return (value & 1) == 0; }
    bool is_physical() const { return (value & 1) == 1; }

    VirtualReg get_virtual_reg() const { return value >> 1; }
    PhysicalReg get_physical_reg() const { return value >> 1; }

    friend bool operator==(const Register &lhs, const Register &rhs) { return lhs.value == rhs.value; }
    friend bool operator!=(const Register &lhs, const Register &rhs) { return !(lhs == rhs); }

    friend class RegisterSet;
};

class RegisterSet {

public:
    class Iterator {

    private:
        BitSet::Iterator iter;

    public:
        Iterator(BitSet::Iterator iter) : iter(iter) {}
        Register operator*() const { return Register(*iter); }

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
    bool contains(Register reg) const { return set.get(reg.value); }
    unsigned size() const { return set.size(); }

    void insert(Register reg) { set.set(reg.value); }
    void remove(Register reg) { set.clear(reg.value); }
    void union_with(const RegisterSet &other) { set.union_with(other.set); }

    Iterator begin() const { return Iterator(set.begin()); }
    Iterator end() const { return Iterator(set.end()); }
};

} // namespace mcode

} // namespace banjo

template <>
struct std::hash<banjo::mcode::Register> {
    std::size_t operator()(const banjo::mcode::Register &reg) const noexcept {
        return static_cast<std::size_t>(reg.internal_value());
    }
};

#endif
