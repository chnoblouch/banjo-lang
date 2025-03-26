#ifndef BANJO_TARGET_AARCH64_ENCODER_H
#define BANJO_TARGET_AARCH64_ENCODER_H

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/module.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace banjo {
namespace target {

class AArch64Encoder {

private:
    BinModule bin_mod;

    std::unordered_map<std::string, std::uint32_t> symbol_indices;

public:
    BinModule encode(mcode::Module &m_mod);

private:
    void encode_func(mcode::Function &func);
    void encode_instr(mcode::Instruction &instr);

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
