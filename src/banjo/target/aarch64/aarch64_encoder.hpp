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
    void encode_instr(mcode::Instruction &instr, mcode::Function *func, UnwindInfo &frame_info) override;

    void encode_mov(mcode::Instruction &instr);
    void encode_ldp(mcode::Instruction &instr);
    void encode_stp(mcode::Instruction &instr);
    void encode_add(mcode::Instruction &instr);
    void encode_sub(mcode::Instruction &instr);
    void encode_bl(mcode::Instruction &instr);
    void encode_ret(mcode::Instruction &instr);
    void encode_adrp(mcode::Instruction &instr);

    void encode_ldp_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params);
    void encode_add_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params);

    std::uint32_t encode_reg(mcode::PhysicalReg reg);
    std::uint32_t encode_imm(LargeInt imm, unsigned num_bits, unsigned shift);
};

} // namespace target
} // namespace banjo

#endif
