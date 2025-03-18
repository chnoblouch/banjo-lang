#ifndef BANJO_TARGET_AARCH64_ENCODER_H
#define BANJO_TARGET_AARCH64_ENCODER_H

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/module.hpp"

#include <array>

namespace banjo {
namespace target {

class AArch64Encoder {

private:
    BinModule bin_mod;

public:
    BinModule encode(mcode::Module &m_mod);

private:
    void encode_func(mcode::Function &func);
    void encode_instr(mcode::Instruction &instr);

    void encode_add(mcode::Instruction &instr);
    void encode_sub(mcode::Instruction &instr);
    void encode_ret(mcode::Instruction &instr);

    void encode_add_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params);

    std::uint32_t encode_reg(mcode::PhysicalReg reg);
};

} // namespace target
} // namespace banjo

#endif
