#ifndef X86_64_ADDRESS_H
#define X86_64_ADDRESS_H

#include "mcode/register.hpp"

namespace target {

struct X8664Address {

public:
    enum class Type { BASE_REG, BASE_SYMBOL, BASE_REG_OFFSET_REG, BASE_REG_OFFSET_IMM };

private:
    Type type;

    union {
        mcode::Register base_reg = mcode::Register::from_virtual(-1);
        std::string base_symbol;
    };

    union {
        int offset_imm;
        mcode::Register offset_reg = mcode::Register::from_virtual(-1);
    };

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

    static AArch64Address new_base_offset(mcode::Register base, mcode::Register offset_reg) {
        AArch64Address addr;
        addr.type = Type::BASE_OFFSET_REG;
        addr.base = base;
        addr.offset_reg = offset_reg;
        return addr;
    }

public:
    Type get_type() const { return type; }
    mcode::Register get_base() const { return base; }
    int get_offset_imm() const { return offset_imm; }
    mcode::Register get_offset_reg() const { return offset_reg; }

    void set_base(mcode::Register base) { this->base = base; }

    friend bool operator==(const X8664Address &lhs, const X8664Address &rhs) {
        if (lhs.type != rhs.type) {
            return false;
        }

        switch (lhs.type) {
            case Type::BASE_REG: return lhs.base_reg == rhs.base_reg;
            case Type::BASE_OFFSET_IMM:
            case Type::BASE_OFFSET_IMM_WRITE: return left.base == right.base && left.offset_imm == right.offset_imm;
            case Type::BASE_OFFSET_REG: return left.base == right.base && left.offset_reg != right.offset_reg;
        }
    }

    friend bool operator!=(const X8664Address &lhs, const X8664Address &rhs) { return !(lhs == rhs); }
};

} // namespace target

#endif
