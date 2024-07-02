#ifndef AARCH64_ADDRESS_H
#define AARCH64_ADDRESS_H

#include "mcode/register.hpp"

namespace banjo {

namespace target {

struct AArch64Address {

public:
    enum class Type {
        BASE,
        BASE_OFFSET_IMM,
        BASE_OFFSET_IMM_WRITE,
        BASE_OFFSET_REG,
    };

private:
    Type type;
    mcode::Register base = mcode::Register::from_virtual(-1);

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
    void set_offset_reg(mcode::Register offset_reg) { this->offset_reg = offset_reg; }

    friend bool operator==(const AArch64Address &lhs, const AArch64Address &rhs) {
        if (lhs.type != rhs.type) {
            return false;
        }

        switch (lhs.type) {
            case Type::BASE: return lhs.base == rhs.base;
            case Type::BASE_OFFSET_IMM:
            case Type::BASE_OFFSET_IMM_WRITE: return lhs.base == rhs.base && lhs.offset_imm == rhs.offset_imm;
            case Type::BASE_OFFSET_REG: return lhs.base == rhs.base && lhs.offset_reg != rhs.offset_reg;
        }
    }

    friend bool operator!=(const AArch64Address &lhs, const AArch64Address &rhs) { return !(lhs == rhs); }
};

} // namespace target

} // namespace banjo

#endif
