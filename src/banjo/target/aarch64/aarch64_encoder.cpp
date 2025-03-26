#include "aarch64_encoder.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
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

    for (const mcode::Global &global : m_mod.get_globals()) {
        symbol_indices.insert({global.name, bin_mod.symbol_defs.size()});

        bin_mod.symbol_defs.push_back(
            BinSymbolDef{
                .name = "_" + global.name,
                .kind = BinSymbolKind::DATA_LABEL,
                .offset = static_cast<std::uint32_t>(bin_mod.data.get_size()),
                .global = m_mod.get_global_symbols().contains(global.name),
            }
        );

        if (auto value = std::get_if<mcode::Global::Integer>(&global.value)) {
            switch (global.size) {
                case 1: bin_mod.data.write_u8(value->to_bits()); break;
                case 2: bin_mod.data.write_u16(value->to_bits()); break;
                case 4: bin_mod.data.write_u32(value->to_bits()); break;
                case 8: bin_mod.data.write_u64(value->to_bits()); break;
                default: ASSERT_UNREACHABLE;
            }
        } else if (auto value = std::get_if<mcode::Global::FloatingPoint>(&global.value)) {
            switch (global.size) {
                case 4: bin_mod.data.write_f32(*value); break;
                case 8: bin_mod.data.write_f64(*value); break;
                default: ASSERT_UNREACHABLE;
            }
        } else if (auto value = std::get_if<mcode::Global::Bytes>(&global.value)) {
            bin_mod.data.write_data(value->data(), value->size());
        } else if (auto value = std::get_if<mcode::Global::String>(&global.value)) {
            bin_mod.data.write_data(value->data(), value->size());
        } else if (std::holds_alternative<mcode::Global::None>(global.value)) {
            bin_mod.data.write_zeroes(global.size);
        } else {
            ASSERT_UNREACHABLE;
        }
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
        case AArch64Opcode::MOV: encode_mov(instr); break;
        case AArch64Opcode::LDP: encode_ldp(instr); break;
        case AArch64Opcode::STP: encode_stp(instr); break;
        case AArch64Opcode::ADD: encode_add(instr); break;
        case AArch64Opcode::SUB: encode_sub(instr); break;
        case AArch64Opcode::BL: encode_bl(instr); break;
        case AArch64Opcode::RET: encode_ret(instr); break;
        case AArch64Opcode::ADRP: encode_adrp(instr); break;
    }
}

void AArch64Encoder::encode_mov(mcode::Instruction &instr) {
    WriteBuffer &buf = bin_mod.text;
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_src = instr.get_operand(1);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_reg(m_dst.get_physical_reg());
    std::uint32_t imm = encode_imm(m_src.get_int_immediate(), 16, 0);

    // TODO: Shifting

    buf.write_u32(0x52800000 | (sf << 31) | (imm << 5) | r_dst);
}

void AArch64Encoder::encode_ldp(mcode::Instruction &instr) {
    encode_ldp_family(instr, {0x28C00000, 0x29C00000});
}

void AArch64Encoder::encode_stp(mcode::Instruction &instr) {
    encode_ldp_family(instr, {0x28800000, 0x29800000});
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

void AArch64Encoder::encode_adrp(mcode::Instruction &instr) {
    WriteBuffer &buf = bin_mod.text;
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_symbol = instr.get_operand(1);

    std::uint32_t r_dst = encode_reg(m_dst.get_physical_reg());

    bin_mod.symbol_uses.push_back(
        BinSymbolUse{
            .address = static_cast<std::uint32_t>(buf.get_size()),
            .addend = 0,
            .symbol_index = symbol_indices[m_symbol.get_symbol().name],
            .kind = BinSymbolUseKind::PAGE21,
            .section = BinSectionKind::TEXT,
        }
    );

    buf.write_u32(0x90000000 | r_dst);
}

void AArch64Encoder::encode_ldp_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params) {
    WriteBuffer &buf = bin_mod.text;
    mcode::Operand &m_reg1 = instr.get_operand(0);
    mcode::Operand &m_reg2 = instr.get_operand(1);
    mcode::Operand &m_addr = instr.get_operand(2);

    const AArch64Address &addr = m_addr.get_aarch64_addr();

    bool sf = instr.get_operand(0).get_size() == 8;
    unsigned imm_shift = sf ? 3 : 2;
    std::uint32_t r_reg1 = encode_reg(m_reg1.get_physical_reg());
    std::uint32_t r_reg2 = encode_reg(m_reg2.get_physical_reg());

    if (addr.get_type() == AArch64Address::Type::BASE) {
        mcode::Operand &m_imm = instr.get_operand(3);

        std::uint32_t r_base = encode_reg(addr.get_base().get_physical_reg());
        std::uint32_t imm = encode_imm(m_imm.get_int_immediate(), 7, imm_shift);
        buf.write_u32(params[0] | (sf << 31) | (imm << 15) | (r_reg2 << 10) | (r_base << 5) | r_reg1);
    } else if (addr.get_type() == AArch64Address::Type::BASE_OFFSET_IMM_WRITE) {
        std::uint32_t r_base = encode_reg(addr.get_base().get_physical_reg());
        std::uint32_t imm = encode_imm(addr.get_offset_imm(), 7, imm_shift);
        buf.write_u32(params[1] | (sf << 31) | (imm << 15) | (r_reg2 << 10) | (r_base << 5) | r_reg1);
    } else {
        // There are other addressing modes, but the compiler currently never generates them.
        ASSERT_UNREACHABLE;
    }
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
    } else if (m_rhs.is_symbol()) {
        // TODO: This is only allowed with `add` instructions!

        ASSERT(sf == 1);

        bin_mod.symbol_uses.push_back(
            BinSymbolUse{
                .address = static_cast<std::uint32_t>(bin_mod.text.get_size()),
                .addend = 0,
                .symbol_index = symbol_indices[m_rhs.get_symbol().name],
                .kind = BinSymbolUseKind::PAGEOFF12,
                .section = BinSectionKind::TEXT,
            }
        );

        buf.write_u32(params[1] | (1 << 31) | (r_lhs << 5) | r_dst);
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

std::uint32_t AArch64Encoder::encode_imm(LargeInt imm, unsigned num_bits, unsigned shift) {
    std::uint32_t bits = static_cast<std::uint32_t>(imm.to_bits());
    std::uint32_t mask = (1 << num_bits) - 1;
    return (bits >> shift) & mask;
}

} // namespace target
} // namespace banjo
