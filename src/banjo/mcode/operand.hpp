#ifndef MCODE_OPERAND_H
#define MCODE_OPERAND_H

#include "banjo/mcode/indirect_address.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_condition.hpp"

#include <string>
#include <variant>

namespace banjo {

namespace mcode {

enum class Relocation {
    NONE,
    GOT,
    PLT,
    LO12,
};

enum class Directive {
    NONE,
    PAGE,
    PAGEOFF,
};

struct Symbol {
    std::string name;
    Relocation reloc;
    Directive directive;

    Symbol(std::string name) : name(name), reloc(Relocation::NONE), directive(Directive::NONE) {}

    Symbol(std::string name, Relocation reloc) : name(name), reloc(reloc), directive(Directive::NONE) {}

    Symbol(std::string name, Directive directive) : name(name), reloc(Relocation::NONE), directive(directive) {}

    Symbol(std::string name, Relocation reloc, Directive directive) : name(name), reloc(reloc), directive(directive) {}

    friend bool operator==(const Symbol &lhs, const Symbol &rhs) {
        return lhs.name == rhs.name && lhs.reloc == rhs.reloc && lhs.directive == rhs.directive;
    }

    friend bool operator!=(const Symbol &lhs, const Symbol &rhs) { return !(lhs == rhs); }
};

class Operand {

public:
    struct StackSlotOffset {
        long slot_index;
        unsigned additional_offset;

        friend bool operator==(const StackSlotOffset &lhs, const StackSlotOffset &rhs) {
            return lhs.slot_index == rhs.slot_index && lhs.additional_offset == rhs.additional_offset;
        }

        friend bool operator!=(const StackSlotOffset &lhs, const StackSlotOffset &rhs) { return !(lhs == rhs); }
    };

private:
    std::variant<
        std::string,
        double,
        Register,
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
    static Operand from_immediate(std::string immediate, int size = 0) {
        Operand operand;
        operand.set_to_immediate(immediate);
        operand.set_size(size);
        return operand;
    }

    static Operand from_fp(float fp, int size = 0) {
        Operand operand;
        operand.set_to_fp(fp);
        operand.set_size(size);
        return operand;
    }

    static Operand from_register(Register reg, int size = 0) {
        Operand operand;
        operand.set_to_register(reg);
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

    bool is_immediate() const { return value.index() == 0; }
    bool is_fp() const { return value.index() == 1; }
    bool is_register() const { return value.index() == 2; }
    bool is_symbol() const { return value.index() == 3; }
    bool is_label() const { return value.index() == 4; }
    bool is_symbol_deref() const { return value.index() == 5; }
    bool is_addr() const { return value.index() == 6; }
    bool is_aarch64_addr() const { return value.index() == 7; }
    bool is_stack_slot_offset() const { return value.index() == 8; }
    bool is_aarch64_left_shift() const { return value.index() == 9; }
    bool is_aarch64_condition() const { return value.index() == 10; }

    bool is_virtual_reg() const { return is_register() && get_register().is_virtual_reg(); }
    bool is_physical_reg() const { return is_register() && get_register().is_physical_reg(); }
    bool is_stack_slot() const { return is_register() && get_register().is_stack_slot(); }

    std::string get_immediate() const { return std::get<0>(value); }
    double get_fp() const { return std::get<1>(value); }
    Register get_register() const { return std::get<2>(value); }
    Symbol get_symbol() const { return std::get<3>(value); }
    std::string get_label() const { return std::get<4>(value); }
    Symbol get_deref_symbol() const { return std::get<5>(value); }
    IndirectAddress &get_addr() { return std::get<6>(value); }
    const target::AArch64Address &get_aarch64_addr() const { return std::get<7>(value); }
    StackSlotOffset get_stack_slot_offset() const { return std::get<8>(value); }
    unsigned get_aarch64_left_shift() const { return std::get<9>(value); }
    target::AArch64Condition get_aarch64_condition() const { return std::get<10>(value); }

    VirtualReg get_virtual_reg() const { return get_register().get_virtual_reg(); }
    PhysicalReg get_physical_reg() const { return get_register().get_physical_reg(); }
    long get_stack_slot() const { return get_register().get_stack_slot(); }

    void set_to_immediate(std::string immediate) { value.emplace<0>(immediate); }
    void set_to_fp(double fp) { value.emplace<1>(fp); }
    void set_to_register(Register reg) { value.emplace<2>(reg); }
    void set_to_symbol(Symbol symbol) { value.emplace<3>(symbol); }
    void set_to_label(std::string label) { value.emplace<4>(label); }
    void set_to_symbol_deref(Symbol symbol) { value.emplace<5>(symbol); }
    void set_to_addr(IndirectAddress addr) { value.emplace<6>(addr); }
    void set_to_aarch64_addr(target::AArch64Address addr) { value.emplace<7>(addr); }
    void set_to_stack_slot_offset(StackSlotOffset offset) { value.emplace<8>(offset); }
    void set_to_aarch64_left_shift(unsigned left_shift) { value.emplace<9>(left_shift); }
    void set_to_aarch64_condition(target::AArch64Condition condition) { value.emplace<10>(condition); }

    void set_to_virtual_reg(long virtual_reg) { set_to_register(Register::from_virtual(virtual_reg)); }
    void set_to_physical_reg(long physical_reg) { set_to_register(Register::from_physical(physical_reg)); }
    void set_to_stack_slot(long stack_slot) { set_to_register(Register::from_stack_slot(stack_slot)); }

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
