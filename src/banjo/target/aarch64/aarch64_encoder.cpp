#include "aarch64_encoder.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"

#include <cstdint>

namespace banjo {
namespace target {

BinModule AArch64Encoder::encode(mcode::Module &m_mod) {
    for (mcode::Function *func : m_mod.get_functions()) {
        bin_mod.symbol_defs.push_back(
            BinSymbolDef{
                .name = func->get_name(),
                .kind = BinSymbolKind::TEXT_FUNC,
                .offset = static_cast<std::uint32_t>(bin_mod.text.get_size()),
                .global = true,
            }
        );

        encode_func(*func);
    }

    return bin_mod;
}

void AArch64Encoder::encode_func(mcode::Function &func) {
    for (mcode::BasicBlock &block : func.get_basic_blocks()) {
        for (mcode::Instruction &instr : block) {
            encode_instr(instr);
        }
    }
}

void AArch64Encoder::encode_instr(mcode::Instruction &instr) {
    switch (instr.get_opcode()) {
        case AArch64Opcode::ADD: encode_add(instr); break;
        case AArch64Opcode::RET: encode_ret(instr); break;
    }
}

void AArch64Encoder::encode_add(mcode::Instruction &instr) {
    WriteBuffer &buf = bin_mod.text;

    if (!instr.get_operand(1).is_register() || !instr.get_operand(2).is_register()) {
        return;
    }

    std::uint32_t r_dst = instr.get_operand(0).get_physical_reg();
    std::uint32_t r_lhs = instr.get_operand(1).get_physical_reg();
    std::uint32_t r_rhs = instr.get_operand(2).get_physical_reg();
    buf.write_u32(0x0B000000 | (r_rhs << 16) | (r_lhs << 5) | r_dst);
}

void AArch64Encoder::encode_ret(mcode::Instruction & /*instr*/) {
    WriteBuffer &buf = bin_mod.text;

    std::uint32_t r_target = 30;
    buf.write_u32(0xD65F0000 | (r_target << 5));
}

} // namespace target
} // namespace banjo
