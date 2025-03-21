#include "aarch64_encoder.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/macros.hpp"

#include <cstdint>

namespace banjo {
namespace target {

BinModule AArch64Encoder::encode(mcode::Module &m_mod) {
    for (const std::string &external_symbol : m_mod.get_external_symbols()) {
        symbol_indices.insert({external_symbol, bin_mod.symbol_defs.size()});

        bin_mod.symbol_defs.push_back(
            BinSymbolDef{
                .name = "_" + external_symbol,
                .kind = BinSymbolKind::UNKNOWN,
                .offset = 0,
                .global = true,
            }
        );
    }

    for (mcode::Function *func : m_mod.get_functions()) {
        bin_mod.symbol_defs.push_back(
            BinSymbolDef{
                .name = "_" + func->get_name(),
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
        case AArch64Opcode::SUB: encode_sub(instr); break;
        case AArch64Opcode::BL: encode_bl(instr); break;
        case AArch64Opcode::RET: encode_ret(instr); break;
    }
}

void AArch64Encoder::encode_add(mcode::Instruction &instr) {
    encode_add_family(instr, {0x0B000000, 0x11000000});
}

void AArch64Encoder::encode_sub(mcode::Instruction &instr) {
    encode_add_family(instr, {0x4B000000, 0x51000000});
}

void AArch64Encoder::encode_bl(mcode::Instruction &instr) {
    WriteBuffer &buf = bin_mod.text;
    mcode::Operand &m_callee = instr.get_operand(0);

    bin_mod.symbol_uses.push_back(
        BinSymbolUse{
            .address = static_cast<std::uint32_t>(buf.get_size()),
            .addend = 0,
            .symbol_index = symbol_indices[m_callee.get_symbol().name],
            .kind = BinSymbolUseKind::BRANCH26,
            .section = BinSectionKind::TEXT,
        }
    );

    buf.write_u32(0x94000000);
}

void AArch64Encoder::encode_ret(mcode::Instruction & /*instr*/) {
    WriteBuffer &buf = bin_mod.text;

    std::uint32_t r_target = 30;
    buf.write_u32(0xD65F0000 | (r_target << 5));
}

void AArch64Encoder::encode_add_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params) {
    WriteBuffer &buf = bin_mod.text;
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_lhs = instr.get_operand(1);
    mcode::Operand &m_rhs = instr.get_operand(2);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_reg(m_dst.get_physical_reg());
    std::uint32_t r_lhs = encode_reg(m_lhs.get_physical_reg());

    if (m_rhs.is_register()) {
        std::uint32_t r_rhs = encode_reg(m_rhs.get_physical_reg());
        buf.write_u32(params[0] | (sf << 31) | (r_rhs << 16) | (r_lhs << 5) | r_dst);
    } else if (m_rhs.is_int_immediate()) {
        bool shifted = false;

        if (instr.get_operands().get_size() == 4) {
            mcode::Operand &m_shift = instr.get_operand(3);
            unsigned shift = m_shift.get_aarch64_left_shift();
            ASSERT(shift == 0 || shift == 12);
            shifted = shift == 12;
        }

        std::uint32_t imm = m_rhs.get_int_immediate().to_bits();
        ASSERT(imm <= 0xFFF);
        buf.write_u32(params[1] | (sf << 31) | (shifted << 22) | (imm << 10) | (r_lhs << 5) | r_dst);
    } else {
        ASSERT_UNREACHABLE;
    }
}

std::uint32_t AArch64Encoder::encode_reg(mcode::PhysicalReg reg) {
    if (reg >= AArch64Register::R0 && reg <= AArch64Register::R30) {
        return reg;
    } else if (reg >= AArch64Register::V0 && reg <= AArch64Register::V30) {
        return reg - AArch64Register::V0;
    } else if (reg == AArch64Register::SP) {
        return 31;
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace target
} // namespace banjo
