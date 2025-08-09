#ifndef BANJO_MCODE_OPERAND_H
#define BANJO_MCODE_OPERAND_H

#include "banjo/mcode/indirect_address.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/mcode/symbol.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_condition.hpp"
#include "banjo/utils/large_int.hpp"

#include <string>
#include <utility>
#include <variant>

namespace banjo {

namespace mcode {

class Operand {

public:
    struct StackSlotOffset {
        StackSlotID slot_index;
        unsigned addend;

        StackSlotOffset(StackSlotID slot_index) : slot_index(slot_index), addend(0) {}
        StackSlotOffset(StackSlotID slot_index, unsigned addend) : slot_index(slot_index), addend(addend) {}

        friend bool operator==(const StackSlotOffset &lhs, const StackSlotOffset &rhs) = default;
        friend bool operator!=(const StackSlotOffset &lhs, const StackSlotOffset &rhs) = default;
    };

private:
    typedef std::variant<
        LargeInt,
        double,
        Register,
        StackSlotID,
        Symbol,
        std::string,
        Symbol,
        IndirectAddress,
        target::AArch64Address,
        StackSlotOffset,
        unsigned,
        target::AArch64Condition>
        InternalValue;

    InternalValue value;
    int size;

public:
    static Operand from_int_immediate(LargeInt immediate, int size = 0) {
        return Operand{InternalValue{std::in_place_index<0>, immediate}, size};
    }

    static Operand from_fp_immediate(double immediate, int size = 0) {
        return Operand{InternalValue{std::in_place_index<1>, immediate}, size};
    }

    static Operand from_register(Register reg, int size = 0) {
        return Operand{InternalValue{std::in_place_index<2>, reg}, size};
    }

    static Operand from_stack_slot(StackSlotID stack_slot, int size = 0) {
        return Operand{InternalValue{std::in_place_index<3>, stack_slot}, size};
    }

    static Operand from_symbol(Symbol symbol, int size = 0) {
        return Operand{InternalValue{std::in_place_index<4>, std::move(symbol)}, size};
    }

    static Operand from_label(std::string label, int size = 0) {
        return Operand{InternalValue{std::in_place_index<5>, std::move(label)}, size};
    }

    static Operand from_symbol_deref(Symbol symbol, int size = 0) {
        return Operand{InternalValue{std::in_place_index<6>, std::move(symbol)}, size};
    }

    static Operand from_addr(IndirectAddress addr, int size = 0) {
        return Operand{InternalValue{std::in_place_index<7>, addr}, size};
    }

    static Operand from_aarch64_addr(target::AArch64Address addr, int size = 0) {
        return Operand{InternalValue{std::in_place_index<8>, addr}, size};
    }

    static Operand from_stack_slot_offset(StackSlotOffset offset, int size = 0) {
        return Operand{InternalValue{std::in_place_index<9>, offset}, size};
    }

    static Operand from_aarch64_left_shift(unsigned left_shift, int size = 0) {
        return Operand{InternalValue{std::in_place_index<10>, left_shift}, size};
    }

    static Operand from_aarch64_condition(target::AArch64Condition condition, int size = 0) {
        return Operand{InternalValue{std::in_place_index<11>, condition}, size};
    }

    Operand() : value{std::in_place_index<0>, 0}, size(0) {};

private:
    Operand(InternalValue value, int size) : value(std::move(value)), size(size) {}

public:
    bool is_int_immediate() const { return value.index() == 0; }
    bool is_fp_immediate() const { return value.index() == 1; }
    bool is_register() const { return value.index() == 2; }
    bool is_stack_slot() const { return value.index() == 3; }
    bool is_symbol() const { return value.index() == 4; }
    bool is_label() const { return value.index() == 5; }
    bool is_symbol_deref() const { return value.index() == 6; }
    bool is_addr() const { return value.index() == 7; }
    bool is_aarch64_addr() const { return value.index() == 8; }
    bool is_stack_slot_offset() const { return value.index() == 9; }
    bool is_aarch64_left_shift() const { return value.index() == 10; }
    bool is_aarch64_condition() const { return value.index() == 11; }

    bool is_virtual_reg() const { return is_register() && get_register().is_virtual(); }
    bool is_physical_reg() const { return is_register() && get_register().is_physical(); }

    LargeInt get_int_immediate() const { return std::get<0>(value); }
    double get_fp_immediate() const { return std::get<1>(value); }
    Register get_register() const { return std::get<2>(value); }
    StackSlotID get_stack_slot() const { return std::get<3>(value); }
    Symbol get_symbol() const { return std::get<4>(value); }
    std::string get_label() const { return std::get<5>(value); }
    Symbol get_deref_symbol() const { return std::get<6>(value); }
    IndirectAddress &get_addr() { return std::get<7>(value); }
    const target::AArch64Address &get_aarch64_addr() const { return std::get<8>(value); }
    StackSlotOffset get_stack_slot_offset() const { return std::get<9>(value); }
    unsigned get_aarch64_left_shift() const { return std::get<10>(value); }
    target::AArch64Condition get_aarch64_condition() const { return std::get<11>(value); }

    VirtualReg get_virtual_reg() const { return get_register().get_virtual_reg(); }
    PhysicalReg get_physical_reg() const { return get_register().get_physical_reg(); }

    int get_size() const { return size; }
    void set_size(int size) { this->size = size; }

    Operand with_size(int size) {
        Operand copy(*this);
        copy.set_size(size);
        return copy;
    }

    friend bool operator==(const Operand &lhs, const Operand &rhs) { return lhs.value == rhs.value; }
    friend bool operator!=(const Operand &lhs, const Operand &rhs) { return !(lhs == rhs); }
};

typedef Operand Value;

} // namespace mcode

} // namespace banjo

#endif
