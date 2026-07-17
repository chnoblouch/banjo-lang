#ifndef BANJO_TARGET_AARCH64_ADDRESS_H
#define BANJO_TARGET_AARCH64_ADDRESS_H

#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_address.hpp"
#include "banjo/mcode/symbol.hpp"

#include <utility>

namespace banjo {

namespace target {

struct AArch64Address {

public:
    enum class Type {
        BASE,
        BASE_OFFSET_IMM,
        BASE_OFFSET_IMM_WRITE,
        BASE_OFFSET_STACK_ADDR,
        BASE_OFFSET_REG,
        BASE_OFFSET_SYMBOL,
    };

    struct RegOffset {
        mcode::Register reg;
        unsigned shift;

        RegOffset() : reg{mcode::Register::from_virtual(0)}, shift{0} {}
        RegOffset(mcode::Register reg) : reg{reg}, shift{0} {}
        RegOffset(mcode::Register reg, unsigned shift) : reg{reg}, shift{shift} {}

        bool operator==(const RegOffset &other) const = default;
        bool operator!=(const RegOffset &other) const = default;
    };

private:
    Type type;
    mcode::Register base;

    int offset_imm;
    mcode::StackAddress offset_stack_addr{0, 0};
    RegOffset offset_reg;
    mcode::Symbol offset_symbol = mcode::Symbol("");

public:
    static AArch64Address new_base(mcode::Register base) {
        AArch64Address addr;
        addr.type = Type::BASE;
        addr.base = base;
        return addr;
    }

    static AArch64Address new_base_offset(mcode::Register base, int offset_imm) {
        AArch64Address addr;
        addr.type = Type::BASE_OFFSET_IMM;
        addr.base = base;
        addr.offset_imm = offset_imm;
        return addr;
    }

    static AArch64Address new_base_offset_write(mcode::Register base, int offset_imm) {
        AArch64Address addr;
        addr.type = Type::BASE_OFFSET_IMM_WRITE;
        addr.base = base;
        addr.offset_imm = offset_imm;
        return addr;
    }

    static AArch64Address new_base_offset(mcode::Register base, mcode::StackAddress stack_addr) {
        AArch64Address addr;
        addr.type = Type::BASE_OFFSET_STACK_ADDR;
        addr.base = base;
        addr.offset_stack_addr = stack_addr;
        return addr;
    }

    static AArch64Address new_base_offset(mcode::Register base, RegOffset offset_reg) {
        AArch64Address addr;
        addr.type = Type::BASE_OFFSET_REG;
        addr.base = base;
        addr.offset_reg = offset_reg;
        return addr;
    }

    static AArch64Address new_base_offset(mcode::Register base, mcode::Symbol offset_symbol) {
        AArch64Address addr;
        addr.type = Type::BASE_OFFSET_SYMBOL;
        addr.base = base;
        addr.offset_symbol = std::move(offset_symbol);
        return addr;
    }

public:
    Type get_type() const { return type; }
    mcode::Register get_base() const { return base; }
    int get_offset_imm() const { return offset_imm; }
    mcode::StackAddress get_offset_stack_addr() const { return offset_stack_addr; }
    const RegOffset &get_offset_reg() const { return offset_reg; }
    const mcode::Symbol &get_offset_symbol() const { return offset_symbol; }

    void set_base(mcode::Register base) { this->base = base; }
    void set_offset_reg(RegOffset offset_reg) { this->offset_reg = offset_reg; }

    friend bool operator==(const AArch64Address &lhs, const AArch64Address &rhs) {
        if (lhs.type != rhs.type || lhs.base != rhs.base) {
            return false;
        }

        switch (lhs.type) {
            case Type::BASE: return true;
            case Type::BASE_OFFSET_IMM:
            case Type::BASE_OFFSET_IMM_WRITE: return lhs.offset_imm == rhs.offset_imm;
            case Type::BASE_OFFSET_STACK_ADDR: return lhs.offset_stack_addr == rhs.offset_stack_addr;
            case Type::BASE_OFFSET_REG: return lhs.offset_reg == rhs.offset_reg;
            case Type::BASE_OFFSET_SYMBOL: return lhs.offset_symbol == rhs.offset_symbol;
        }
    }

    friend bool operator!=(const AArch64Address &lhs, const AArch64Address &rhs) { return !(lhs == rhs); }
};

} // namespace target

} // namespace banjo

#endif
