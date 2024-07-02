#ifndef MCODE_REGISTER_H
#define MCODE_REGISTER_H

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
