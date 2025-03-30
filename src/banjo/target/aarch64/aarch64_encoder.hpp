#ifndef BANJO_TARGET_AARCH64_ENCODER_H
#define BANJO_TARGET_AARCH64_ENCODER_H

#include "banjo/emit/binary_builder.hpp"
#include "banjo/mcode/instruction.hpp"

#include <array>
#include <cstdint>

namespace banjo {
namespace target {

class AArch64Encoder final : public BinaryBuilder {

private:
    enum class AddressingMode : std::uint8_t {
        OFFSET_CONST,
        OFFSET_REG,
        POST_INDEX,
        PRE_INDEX,
    };

    struct Address {
        AddressingMode mode;
        mcode::Register base;

        union {
            LargeInt offset_const;
            mcode::Register offset_reg;
        };
    };

    mcode::Function *cur_func;

    void encode_instr(mcode::Instruction &instr, mcode::Function *func, UnwindInfo &frame_info) override;

    void encode_mov(mcode::Instruction &instr);
    void encode_movz(mcode::Instruction &instr);
    void encode_movk(mcode::Instruction &instr);
    void encode_ldr(mcode::Instruction &instr);
    void encode_ldrb(mcode::Instruction &instr);
    void encode_ldrh(mcode::Instruction &instr);
    void encode_str(mcode::Instruction &instr);
    void encode_strb(mcode::Instruction &instr);
    void encode_strh(mcode::Instruction &instr);
    void encode_ldp(mcode::Instruction &instr);
    void encode_stp(mcode::Instruction &instr);
    void encode_add(mcode::Instruction &instr);
    void encode_sub(mcode::Instruction &instr);
    void encode_mul(mcode::Instruction &instr);
    void encode_sdiv(mcode::Instruction &instr);
    void encode_udiv(mcode::Instruction &instr);
    void encode_b(mcode::Instruction &instr);
    void encode_br(mcode::Instruction &instr);
    void encode_b_eq(mcode::Instruction &instr);
    void encode_b_ne(mcode::Instruction &instr);
    void encode_b_hs(mcode::Instruction &instr);
    void encode_b_lo(mcode::Instruction &instr);
    void encode_b_hi(mcode::Instruction &instr);
    void encode_b_ls(mcode::Instruction &instr);
    void encode_b_ge(mcode::Instruction &instr);
    void encode_b_lt(mcode::Instruction &instr);
    void encode_b_gt(mcode::Instruction &instr);
    void encode_b_le(mcode::Instruction &instr);
    void encode_bl(mcode::Instruction &instr);
    void encode_blr(mcode::Instruction &instr);
    void encode_ret(mcode::Instruction &instr);
    void encode_adrp(mcode::Instruction &instr);
    void encode_uxtb(mcode::Instruction &instr);
    void encode_uxth(mcode::Instruction &instr);
    void encode_sxtb(mcode::Instruction &instr);
    void encode_sxth(mcode::Instruction &instr);
    void encode_sxtw(mcode::Instruction &instr);

    void encode_movz_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params);
    void encode_ldr_family(mcode::Instruction &instr, std::array<std::uint32_t, 4> params);
    void encode_ldrb_family(mcode::Instruction &instr, std::array<std::uint32_t, 5> params);
    void encode_ldp_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params);
    void encode_add_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params);
    void encode_mul_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params);
    void encode_b_cond_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params);
    void encode_ubfm_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params);
    void encode_sbfm_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params);

    std::uint32_t encode_reg(mcode::PhysicalReg reg);
    std::uint32_t encode_imm(LargeInt imm, unsigned num_bits, unsigned shift);
    Address lower_addr(mcode::Operand &operand, mcode::Operand *post_operand);

    void resolve_internal_symbols() override;
    void resolve_symbol(SectionBuilder::SectionSlice &slice, SymbolUse &use);
};

} // namespace target
} // namespace banjo

#endif
