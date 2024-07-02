#ifndef MCODE_ADDRESS_H
#define MCODE_ADDRESS_H

#include "banjo/mcode/register.hpp"

namespace banjo {

namespace mcode {

struct Address {

public:
    enum class Type {
        BASE_REG,
        BASE_SYMBOL,
        BASE_REG_OFF_IMM,
        BASE_REG_OFF_IMM_WRITE,
        BASE_REG_OFF_REG,
    };

    /*
    static constexpr int MASK_BASE = 0x03;
    static constexpr int MASK_OFFSET = 0x0C;
    static constexpr int MASK_WRITE_BACK = 0x10;

    static constexpr int BASE_REG = 0x00;
    static constexpr int BASE_SYMBOL = 0x01;

    static constexpr int OFFSET_NONE = 0x04;
    static constexpr int OFFSET_IMM = 0x05;
    static constexpr int OFFSET_REG = 0x06;
    */

private:
    Type type;

    union {
        Register base_reg = Register::from_virtual(-1);
    };

    union {
        int offset_imm;

        struct {
            Register offset_reg = Register::from_virtual(-1);
            unsigned char offset_scale;
        };
    };

public:
    Address(Register base) : type(Type::BASE_REG), base_reg(base) {}

    Address(Register base, int offset, bool write)
      : type(write ? Type::BASE_REG_OFF_IMM : Type::BASE_REG_OFF_IMM_WRITE),
        base_reg(base),
        offset_imm(offset) {}

    Address(Register base, Register offset, unsigned char scale)
      : type(Type::BASE_REG_OFF_REG),
        base_reg(base),
        offset_reg(offset),
        offset_scale(scale) {}

public:
    Type get_type() const { return type; }
    Register get_base() const { return base; }
    int get_offset_imm() const { return offset_imm; }
    Register get_offset_reg() const { return offset_reg; }

    void set_base(Register base) { this->base = base; }

    friend bool operator==(const Address &lhs, const Address &rhs) {
        if (lhs.type != rhs.type) {
            return false;
        }

        switch (lhs.type) {
            case Type::BASE_REG: return lhs.base_reg == rhs.base_reg;
            case Type::BASE_REG_OFF_IMM: return lhs.base_reg == rhs.base_reg && lhs.offset_imm == rhs.offset_imm;
            case Type::BASE_REG_OFF_IMM_WRITE: return lhs.base_reg == rhs.base_reg && lhs.offset_imm == rhs.offset_imm;
            case Type::BASE_REG_OFF_REG:
                return lhs.base_reg == rhs.base_reg && lhs.offset_reg == rhs.offset_reg &&
                       lhs.offset_scale == rhs.offset_scale;
        }
    }

    friend bool operator!=(const Address &lhs, const Address &rhs) { return !(lhs == rhs); }
};

} // namespace mcode

} // namespace banjo

#endif
