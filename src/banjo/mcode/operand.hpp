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
    std::variant<
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
        value;
    int size;

public:
    static Operand from_int_immediate(LargeInt immediate, int size = 0) {
        Operand operand;
        operand.set_to_int_immediate(immediate);
        operand.set_size(size);
        return operand;
    }

    static Operand from_fp_immediate(double immediate, int size = 0) {
        Operand operand;
        operand.set_to_fp_immediate(immediate);
        operand.set_size(size);
        return operand;
    }

    static Operand from_register(Register reg, int size = 0) {
        Operand operand;
        operand.set_to_register(reg);
        operand.set_size(size);
        return operand;
    }

    static Operand from_stack_slot(StackSlotID stack_slot, int size = 0) {
        Operand operand;
        operand.set_to_stack_slot(stack_slot);
        operand.set_size(size);
        return operand;
    }

    static Operand from_symbol(Symbol symbol, int size = 0) {
        Operand operand;
        operand.set_to_symbol(symbol);
        operand.set_size(size);
        return operand;
    }

    static Operand from_label(std::string label, int size = 0) {
        Operand operand;
        operand.set_to_label(label);
        operand.set_size(size);
        return operand;
    }

    static Operand from_symbol_deref(Symbol symbol, int size = 0) {
        Operand operand;
        operand.set_to_symbol_deref(symbol);
        operand.set_size(size);
        return operand;
    }

    static Operand from_addr(IndirectAddress addr, int size = 0) {
        Operand operand;
        operand.set_to_addr(addr);
        operand.set_size(size);
        return operand;
    }

    static Operand from_aarch64_addr(target::AArch64Address addr, int size = 0) {
        Operand operand;
        operand.set_to_aarch64_addr(addr);
        operand.set_size(size);
        return operand;
    }

    static Operand from_stack_slot_offset(StackSlotOffset offset, int size = 0) {
        Operand operand;
        operand.set_to_stack_slot_offset(offset);
        operand.set_size(size);
        return operand;
    }

    static Operand from_aarch64_left_shift(unsigned left_shift, int size = 0) {
        Operand operand;
        operand.set_to_aarch64_left_shift(left_shift);
        operand.set_size(size);
        return operand;
    }

    static Operand from_aarch64_condition(target::AArch64Condition condition, int size = 0) {
        Operand operand;
        operand.set_to_aarch64_condition(condition);
        operand.set_size(size);
        return operand;
    }

    Operand() : value{std::in_place_index<0>, "???"}, size(0) {};

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

    void set_to_int_immediate(LargeInt immediate) { value.emplace<0>(immediate); }
    void set_to_fp_immediate(double immediate) { value.emplace<1>(immediate); }
    void set_to_register(Register reg) { value.emplace<2>(reg); }
    void set_to_stack_slot(StackSlotID stack_slot) { value.emplace<3>(stack_slot); }
    void set_to_symbol(Symbol symbol) { value.emplace<4>(symbol); }
    void set_to_label(std::string label) { value.emplace<5>(label); }
    void set_to_symbol_deref(Symbol symbol) { value.emplace<6>(symbol); }
    void set_to_addr(IndirectAddress addr) { value.emplace<7>(addr); }
    void set_to_aarch64_addr(target::AArch64Address addr) { value.emplace<8>(addr); }
    void set_to_stack_slot_offset(StackSlotOffset offset) { value.emplace<9>(offset); }
    void set_to_aarch64_left_shift(unsigned left_shift) { value.emplace<10>(left_shift); }
    void set_to_aarch64_condition(target::AArch64Condition condition) { value.emplace<11>(condition); }

    void set_to_virtual_reg(long virtual_reg) { set_to_register(Register::from_virtual(virtual_reg)); }
    void set_to_physical_reg(long physical_reg) { set_to_register(Register::from_physical(physical_reg)); }

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
