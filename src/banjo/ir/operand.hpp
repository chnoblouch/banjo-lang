#ifndef IR_OPERAND_H
#define IR_OPERAND_H

#include "banjo/ir/comparison.hpp"
#include "banjo/ir/type.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/utils/large_int.hpp"
#include "banjo/utils/linked_list.hpp"

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace banjo {

namespace ir {

class Operand;
class Function;
class Global;
class BasicBlock;

struct BranchTarget {
    LinkedListIter<BasicBlock> block;
    std::vector<Operand> args;
};

class Operand {

private:
    std::variant<
        LargeInt,
        double,
        VirtualRegister,
        Function *,
        std::string,
        std::string,
        std::string,
        std::string,
        BranchTarget,
        Comparison,
        std::monostate,
        std::monostate>
        value{std::in_place_index<0>, 0};
    Type type{Primitive::VOID};

public:
    static Operand from_int_immediate(LargeInt immediate, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_int_immediate(immediate);
        operand.set_type(type);
        return operand;
    }

    static Operand from_fp_immediate(double immediate, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_fp_immediate(immediate);
        operand.set_type(type);
        return operand;
    }

    static Operand from_register(VirtualRegister reg, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_register(reg);
        operand.set_type(type);
        return operand;
    }

    static Operand from_func(Function *func, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_func(func);
        operand.set_type(type);
        return operand;
    }

    static Operand from_global(std::string global_name, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_global(global_name);
        operand.set_type(type);
        return operand;
    }

    static Operand from_extern_func(std::string extern_func_name, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_extern_func(extern_func_name);
        operand.set_type(type);
        return operand;
    }

    static Operand from_extern_global(std::string extern_global_name, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_extern_global(extern_global_name);
        operand.set_type(type);
        return operand;
    }

    static Operand from_string(std::string string, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_string(string);
        operand.set_type(type);
        return operand;
    }

    static Operand from_branch_target(BranchTarget branch_target, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_branch_target(branch_target);
        operand.set_type(type);
        return operand;
    }

    static Operand from_comparison(Comparison comparison, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_comparison(comparison);
        operand.set_type(type);
        return operand;
    }

    static Operand from_type(Type type) {
        Operand operand;
        operand.set_to_type(type);
        return operand;
    }

    static Operand undef(Type type) {
        Operand operand;
        operand.set_to_undef();
        operand.set_type(type);
        return operand;
    }

    Operand() {}

    bool is_int_immediate() const { return value.index() == 0; }
    bool is_fp_immediate() const { return value.index() == 1; }
    bool is_register() const { return value.index() == 2; }
    bool is_func() const { return value.index() == 3; }
    bool is_global() const { return value.index() == 4; }
    bool is_extern_func() const { return value.index() == 5; }
    bool is_extern_global() const { return value.index() == 6; }
    bool is_string() const { return value.index() == 7; }
    bool is_branch_target() const { return value.index() == 8; }
    bool is_comparison() const { return value.index() == 9; }
    bool is_type() const { return value.index() == 10; }
    bool is_undef() const { return value.index() == 11; }

    LargeInt get_int_immediate() const { return std::get<0>(value); }
    double get_fp_immediate() const { return std::get<1>(value); }
    VirtualRegister get_register() const { return std::get<2>(value); }
    Function *get_func() const { return std::get<3>(value); }
    const std::string &get_global_name() const { return std::get<4>(value); }
    const std::string &get_extern_func_name() const { return std::get<5>(value); }
    const std::string &get_extern_global_name() const { return std::get<6>(value); }
    std::string get_string() const { return std::get<7>(value); }
    BranchTarget &get_branch_target() { return std::get<8>(value); }
    Comparison get_comparison() const { return std::get<9>(value); }

    bool is_immediate() const { return is_int_immediate() || is_fp_immediate(); }
    bool is_register(VirtualRegister reg) const { return is_register() && get_register() == reg; }

    void set_to_int_immediate(LargeInt immediate) { value.emplace<0>(immediate); }
    void set_to_fp_immediate(double immediate) { value.emplace<1>(immediate); }
    void set_to_register(VirtualRegister reg) { value.emplace<2>(reg); }
    void set_to_func(Function *func) { value.emplace<3>(func); }
    void set_to_global(std::string global_name) { value.emplace<4>(global_name); }
    void set_to_extern_func(std::string extern_func_name) { value.emplace<5>(extern_func_name); }
    void set_to_extern_global(std::string extern_global_name) { value.emplace<6>(extern_global_name); }
    void set_to_string(std::string string) { value.emplace<7>(string); }
    void set_to_branch_target(BranchTarget branch_target) { value.emplace<8>(branch_target); }
    void set_to_comparison(Comparison comparison) { value.emplace<9>(comparison); }

    void set_to_type(Type type) {
        value.emplace<10>();
        this->type = type;
    }

    void set_to_undef() { value.emplace<11>(); }

    Type get_type() const { return type; }
    void set_type(Type type) { this->type = std::move(type); }

    Operand with_type(Type type) const {
        ir::Operand copy = *this;
        copy.set_type(std::move(type));
        return copy;
    }

    bool is_symbol() const { return is_func() || is_global() || is_extern_func() || is_extern_global(); }
    const std::string &get_symbol_name() const;

    friend bool operator==(const Operand &lhs, const Operand &rhs) {
        if (lhs.value.index() != rhs.value.index()) {
            return false;
        }

        switch (lhs.value.index()) {
            case 0: return std::get<0>(lhs.value) == std::get<0>(rhs.value);
            case 1: return std::get<1>(lhs.value) == std::get<1>(rhs.value);
            case 2: return std::get<2>(lhs.value) == std::get<2>(rhs.value);
            case 3: return std::get<3>(lhs.value) == std::get<3>(rhs.value);
            case 4: return std::get<4>(lhs.value) == std::get<4>(rhs.value);
            case 5: return std::get<5>(lhs.value) == std::get<5>(rhs.value);
            case 6: return std::get<6>(lhs.value) == std::get<6>(rhs.value);
            case 7: return std::get<7>(lhs.value) == std::get<7>(rhs.value);
            case 8:
                return std::get<8>(lhs.value).block == std::get<8>(rhs.value).block &&
                       std::get<8>(lhs.value).args == std::get<8>(rhs.value).args;
            case 9: return std::get<9>(lhs.value) == std::get<9>(rhs.value);
            case 10: return std::get<10>(lhs.value) == std::get<10>(rhs.value);
            case 11: return std::get<11>(lhs.value) == std::get<11>(rhs.value);
            default: return false;
        }
    }

    friend bool operator!=(const Operand &lhs, const Operand &rhs) { return !(lhs == rhs); }
};

typedef Operand Value;

} // namespace ir

} // namespace banjo

#endif
