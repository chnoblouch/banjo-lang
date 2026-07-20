#ifndef BANJO_TARGET_X86_64_ADDRESS_H
#define BANJO_TARGET_X86_64_ADDRESS_H

#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_address.hpp"
#include "banjo/mcode/symbol.hpp"

#include <variant>
#include <optional>

namespace banjo::target {

struct X8664Address {
    struct RegOffset {
        mcode::Register reg;
        unsigned scale;

        RegOffset() : reg{mcode::Register::from_virtual(0)}, scale{1} {}
        RegOffset(mcode::Register reg) : reg{reg}, scale{1} {}
        RegOffset(mcode::Register reg, unsigned scale) : reg{reg}, scale{scale} {}

        bool operator==(const RegOffset &other) const = default;
        bool operator!=(const RegOffset &other) const = default;
    };

    typedef std::variant<mcode::Register, mcode::Symbol> Base;
    typedef std::variant<int, mcode::StackAddress> ConstOffset;

    Base base;
    ConstOffset offset_const;
    std::optional<RegOffset> offset_reg;

    mcode::Register get_base_reg() const { return std::get<0>(base); }
    const mcode::Symbol &get_base_symbol() const { return std::get<1>(base); }

    bool is_base_reg() const { return base.index() == 0; }
    bool is_base_symbol() const { return base.index() == 1; }

    int get_offset_imm() const { return std::get<0>(offset_const); }
    mcode::StackAddress get_offset_stack_addr() const { return std::get<1>(offset_const); }
    const RegOffset &get_offset_reg() const { return *offset_reg; }

    bool has_offset_imm() const { return offset_const.index() == 0; }
    bool has_offset_stack_addr() const { return offset_const.index() == 1; }
    bool has_offset_reg() const { return offset_reg.has_value(); }

    void set_base(mcode::Register base) { this->base = base; }
    void set_offset_reg(RegOffset offset_reg) { this->offset_reg = offset_reg; }

    bool operator==(const X8664Address &rhs) const = default;
    bool operator!=(const X8664Address &rhs) const = default;
};

} // namespace banjo::target

#endif
