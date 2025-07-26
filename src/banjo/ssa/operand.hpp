#ifndef BANJO_SSA_OPERAND_H
#define BANJO_SSA_OPERAND_H

#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/large_int.hpp"
#include "banjo/utils/linked_list.hpp"

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace banjo {

namespace ssa {

class Operand;
class Function;
struct Global;
struct FunctionDecl;
struct GlobalDecl;
class BasicBlock;

struct BranchTarget {
    LinkedListIter<BasicBlock> block;
    std::vector<Operand> args;

    friend bool operator==(const BranchTarget &lhs, const BranchTarget &rhs) = default;
    friend bool operator!=(const BranchTarget &lhs, const BranchTarget &rhs) = default;
};

class Operand {

private:
    std::variant<
        LargeInt,
        double,
        VirtualRegister,
        Function *,
        Global *,
        FunctionDecl *,
        GlobalDecl *,
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

    static Operand from_global(Global *global, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_global(global);
        operand.set_type(type);
        return operand;
    }

    static Operand from_extern_func(FunctionDecl *extern_func, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_extern_func(extern_func);
        operand.set_type(type);
        return operand;
    }

    static Operand from_extern_global(GlobalDecl *extern_global, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_extern_global(extern_global);
        operand.set_type(type);
        return operand;
    }

    static Operand from_branch_target(BranchTarget branch_target, Type type = Type(Primitive::VOID)) {
        Operand operand;
        operand.set_to_branch_target(std::move(branch_target));
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
    bool is_branch_target() const { return value.index() == 7; }
    bool is_comparison() const { return value.index() == 8; }
    bool is_type() const { return value.index() == 9; }
    bool is_undef() const { return value.index() == 10; }

    LargeInt get_int_immediate() const { return std::get<0>(value); }
    double get_fp_immediate() const { return std::get<1>(value); }
    VirtualRegister get_register() const { return std::get<2>(value); }
    Function *get_func() const { return std::get<3>(value); }
    Global *get_global() const { return std::get<4>(value); }
    FunctionDecl *get_extern_func() const { return std::get<5>(value); }
    GlobalDecl *get_extern_global() const { return std::get<6>(value); }
    BranchTarget &get_branch_target() { return std::get<7>(value); }
    Comparison get_comparison() const { return std::get<8>(value); }

    bool is_immediate() const { return is_int_immediate() || is_fp_immediate(); }
    bool is_register(VirtualRegister reg) const { return is_register() && get_register() == reg; }

    void set_to_int_immediate(LargeInt immediate) { value.emplace<0>(immediate); }
    void set_to_fp_immediate(double immediate) { value.emplace<1>(immediate); }
    void set_to_register(VirtualRegister reg) { value.emplace<2>(reg); }
    void set_to_func(Function *func) { value.emplace<3>(func); }
    void set_to_global(Global *global) { value.emplace<4>(global); }
    void set_to_extern_func(FunctionDecl *extern_func) { value.emplace<5>(extern_func); }
    void set_to_extern_global(GlobalDecl *extern_global) { value.emplace<6>(extern_global); }
    void set_to_branch_target(BranchTarget branch_target) { value.emplace<7>(branch_target); }
    void set_to_comparison(Comparison comparison) { value.emplace<8>(comparison); }

    void set_to_type(Type type) {
        value.emplace<9>();
        this->type = type;
    }

    void set_to_undef() { value.emplace<10>(); }

    Type get_type() const { return type; }
    void set_type(Type type) { this->type = type; }

    Operand with_type(Type type) const {
        ssa::Operand copy = *this;
        copy.set_type(type);
        return copy;
    }

    bool is_symbol() const { return is_func() || is_global() || is_extern_func() || is_extern_global(); }
    const std::string &get_symbol_name() const;

    friend bool operator==(const Operand &lhs, const Operand &rhs) { return lhs.value == rhs.value; }
    friend bool operator!=(const Operand &lhs, const Operand &rhs) { return !(lhs == rhs); }
};

typedef Operand Value;

} // namespace ssa

} // namespace banjo

#endif
