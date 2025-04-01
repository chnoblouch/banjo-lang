#include "aarch64_encoder.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/mcode/stack_slot.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/bit_operations.hpp"
#include "banjo/utils/macros.hpp"

#include <cstdint>
#include <iostream>

#define WARN_UNIMPLEMENTED(name) std::cout << "warning: " << name << " is unimplemented\n";

namespace banjo {
namespace target {

void AArch64Encoder::encode_instr(mcode::Instruction &instr, mcode::Function *func, UnwindInfo & /*frame_info*/) {
    cur_func = func;

    switch (instr.get_opcode()) {
        case AArch64Opcode::MOV: encode_mov(instr); break;
        case AArch64Opcode::MOVZ: encode_movz(instr); break;
        case AArch64Opcode::MOVK: encode_movk(instr); break;
        case AArch64Opcode::LDR: encode_ldr(instr); break;
        case AArch64Opcode::LDRB: encode_ldrb(instr); break;
        case AArch64Opcode::LDRH: encode_ldrh(instr); break;
        case AArch64Opcode::STR: encode_str(instr); break;
        case AArch64Opcode::STRB: encode_strb(instr); break;
        case AArch64Opcode::STRH: encode_strh(instr); break;
        case AArch64Opcode::LDP: encode_ldp(instr); break;
        case AArch64Opcode::STP: encode_stp(instr); break;
        case AArch64Opcode::ADD: encode_add(instr); break;
        case AArch64Opcode::SUB: encode_sub(instr); break;
        case AArch64Opcode::MUL: encode_mul(instr); break;
        case AArch64Opcode::SDIV: encode_sdiv(instr); break;
        case AArch64Opcode::UDIV: encode_udiv(instr); break;
        case AArch64Opcode::AND: encode_and(instr); break;
        case AArch64Opcode::ORR: encode_orr(instr); break;
        case AArch64Opcode::EOR: encode_eor(instr); break;
        case AArch64Opcode::LSL: encode_lsl(instr); break;
        case AArch64Opcode::ASR: encode_asr(instr); break;
        case AArch64Opcode::CSEL: WARN_UNIMPLEMENTED("csel"); break;
        case AArch64Opcode::FMOV: encode_fmov(instr); break;
        case AArch64Opcode::FADD: WARN_UNIMPLEMENTED("fadd"); break;
        case AArch64Opcode::FSUB: WARN_UNIMPLEMENTED("fsub"); break;
        case AArch64Opcode::FMUL: WARN_UNIMPLEMENTED("fmul"); break;
        case AArch64Opcode::FDIV: WARN_UNIMPLEMENTED("fdiv"); break;
        case AArch64Opcode::FCVT: WARN_UNIMPLEMENTED("fcvt"); break;
        case AArch64Opcode::SCVTF: WARN_UNIMPLEMENTED("scvtf"); break;
        case AArch64Opcode::UCVTF: WARN_UNIMPLEMENTED("ucvtf"); break;
        case AArch64Opcode::FCVTZS: WARN_UNIMPLEMENTED("fcvtzs"); break;
        case AArch64Opcode::FCVTZU: WARN_UNIMPLEMENTED("fcvtzu"); break;
        case AArch64Opcode::FCSEL: WARN_UNIMPLEMENTED("fcsel"); break;
        case AArch64Opcode::CMP: encode_cmp(instr); break;
        case AArch64Opcode::FCMP: WARN_UNIMPLEMENTED("fcmp"); break;
        case AArch64Opcode::B: encode_b(instr); break;
        case AArch64Opcode::BR: encode_br(instr); break;
        case AArch64Opcode::B_EQ: encode_b_eq(instr); break;
        case AArch64Opcode::B_NE: encode_b_ne(instr); break;
        case AArch64Opcode::B_HS: encode_b_hs(instr); break;
        case AArch64Opcode::B_LO: encode_b_lo(instr); break;
        case AArch64Opcode::B_HI: encode_b_hi(instr); break;
        case AArch64Opcode::B_LS: encode_b_ls(instr); break;
        case AArch64Opcode::B_GE: encode_b_ge(instr); break;
        case AArch64Opcode::B_LT: encode_b_lt(instr); break;
        case AArch64Opcode::B_GT: encode_b_gt(instr); break;
        case AArch64Opcode::B_LE: encode_b_le(instr); break;
        case AArch64Opcode::BL: encode_bl(instr); break;
        case AArch64Opcode::BLR: encode_blr(instr); break;
        case AArch64Opcode::RET: encode_ret(instr); break;
        case AArch64Opcode::ADRP: encode_adrp(instr); break;
        case AArch64Opcode::UXTB: encode_uxtb(instr); break;
        case AArch64Opcode::UXTH: encode_uxth(instr); break;
        case AArch64Opcode::SXTB: encode_sxtb(instr); break;
        case AArch64Opcode::SXTH: encode_sxth(instr); break;
        case AArch64Opcode::SXTW: encode_sxtw(instr); break;
    }
}

void AArch64Encoder::encode_mov(mcode::Instruction &instr) {
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_src = instr.get_operand(1);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());

    if (m_src.is_register()) {
        // TODO: Move to/from SP.

        std::uint32_t r_src = encode_gp_reg(m_src.get_physical_reg());
        text.write_u32(0x2A0003E0 | (sf << 31) | (r_src << 16) | r_dst);
    } else if (m_src.is_int_immediate()) {
        std::uint64_t bits = m_src.get_int_immediate().to_bits();

        // TODO: Support `MOVN` instructions and use them in the SSA lowerer.

        bool inverted = false;
        if (m_src.get_int_immediate().is_negative()) {
            inverted = true;
            bits = ~bits;
        }

        unsigned shift = 0;
        if (bits >= (1ull << 48)) {
            shift = 48;
        } else if (bits >= (1ull << 32)) {
            shift = 32;
        } else if (bits >= (1ull << 16)) {
            shift = 16;
        } else {
            shift = 0;
        }

        std::uint32_t instr_template = inverted ? 0x12800000 : 0x52800000;
        std::uint32_t imm = encode_imm(bits, 16, shift);
        std::uint32_t hw = encode_imm(shift, 2, 4);
        text.write_u32(instr_template | (sf << 31) | (hw << 21) | (imm << 5) | r_dst);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_movz(mcode::Instruction &instr) {
    // TODO: This should probably merged with `mov`.
    encode_movz_family(instr, {0x52800000});
}

void AArch64Encoder::encode_movk(mcode::Instruction &instr) {
    encode_movz_family(instr, {0x72800000});
}

void AArch64Encoder::encode_ldr(mcode::Instruction &instr) {
    encode_ldr_family(instr, {0xB8400000, 0xBC400000});
}

void AArch64Encoder::encode_ldrb(mcode::Instruction &instr) {
    encode_ldrb_family(instr, {0, 0x38400000});
}

void AArch64Encoder::encode_ldrh(mcode::Instruction &instr) {
    encode_ldrb_family(instr, {1, 0x78400000});
}

void AArch64Encoder::encode_str(mcode::Instruction &instr) {
    encode_ldr_family(instr, {0xB8000000, 0xBC000000});
}

void AArch64Encoder::encode_strb(mcode::Instruction &instr) {
    encode_ldrb_family(instr, {0, 0x38000000});
}

void AArch64Encoder::encode_strh(mcode::Instruction &instr) {
    encode_ldrb_family(instr, {1, 0x78000000});
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

void AArch64Encoder::encode_mul(mcode::Instruction &instr) {
    encode_mul_family(instr, {0x1B007C00});
}

void AArch64Encoder::encode_sdiv(mcode::Instruction &instr) {
    encode_mul_family(instr, {0x1AC00C00});
}

void AArch64Encoder::encode_udiv(mcode::Instruction &instr) {
    encode_mul_family(instr, {0x1AC00800});
}

void AArch64Encoder::encode_and(mcode::Instruction &instr) {
    encode_and_family(instr, {0x0A000000});
}

void AArch64Encoder::encode_orr(mcode::Instruction &instr) {
    encode_and_family(instr, {0x2A000000});
}

void AArch64Encoder::encode_eor(mcode::Instruction &instr) {
    encode_and_family(instr, {0x4A000000});
}

void AArch64Encoder::encode_lsl(mcode::Instruction &instr) {
    encode_lsl_family(instr, {0x1AC02000});
}

void AArch64Encoder::encode_asr(mcode::Instruction &instr) {
    encode_lsl_family(instr, {0x1AC02800});
}

void AArch64Encoder::encode_fmov(mcode::Instruction &instr) {
    ASSERT(instr.get_operands().get_size() == 2);

    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_src = instr.get_operand(1);

    bool ftype = instr.get_operand(0).get_size() == 8;

    if (m_src.is_register()) {
        bool from_fp = is_fp_reg(m_src.get_physical_reg());
        bool to_fp = is_fp_reg(m_dst.get_physical_reg());

        if (from_fp && to_fp) {
            std::uint32_t r_dst = encode_fp_reg(m_dst.get_physical_reg());
            std::uint32_t r_src = encode_fp_reg(m_src.get_physical_reg());
            text.write_u32(0x1E204000 | (ftype << 22) | (r_src << 5 | r_dst));
        } else if (from_fp && !to_fp) {
            bool sf = m_dst.get_size() == 8;
            bool ftype = m_src.get_size() == 8;
            bool opcode = 0;
            std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
            std::uint32_t r_src = encode_fp_reg(m_src.get_physical_reg());
            text.write_u32(0x1E260000 | (sf << 31) | (ftype << 22) | (opcode << 16) | (r_src << 5) | r_dst);
        } else if (!from_fp && to_fp) {
            bool sf = m_src.get_size() == 8;
            bool ftype = m_src.get_size() == 8;
            bool opcode = 1;
            std::uint32_t r_dst = encode_fp_reg(m_dst.get_physical_reg());
            std::uint32_t r_src = encode_gp_reg(m_src.get_physical_reg());
            text.write_u32(0x1E260000 | (sf << 31) | (ftype << 22) | (opcode << 16) | (r_src << 5) | r_dst);
        } else {
            ASSERT_UNREACHABLE;
        }
    } else if (m_src.is_fp_immediate()) {
        std::uint32_t r_dst = encode_fp_reg(m_dst.get_physical_reg());

        std::uint32_t imm;
        if (ftype) {
            imm = encode_f64_imm(m_src.get_fp_immediate());
        } else {
            imm = encode_f32_imm(m_src.get_fp_immediate());
        }

        text.write_u32(0x1E201000 | (ftype << 22) | (imm << 13) | r_dst);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_cmp(mcode::Instruction &instr) {
    mcode::Operand &m_lhs = instr.get_operand(0);
    mcode::Operand &m_rhs = instr.get_operand(1);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_lhs = encode_gp_reg(m_lhs.get_physical_reg());

    if (m_rhs.is_register()) {
        std::uint32_t r_rhs = encode_gp_reg(m_rhs.get_physical_reg());
        text.write_u32(0x6B00001F | (sf << 31) | (r_rhs << 16) | (r_lhs << 5));
    } else if (m_rhs.is_int_immediate()) {
        std::uint32_t imm = encode_imm(m_rhs.get_int_immediate(), 12, 0);
        text.write_u32(0x7100001F | (sf << 31) | (imm << 10) | (r_lhs) << 5);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_b(mcode::Instruction &instr) {
    mcode::Operand &m_target = instr.get_operand(0);

    text.add_symbol_use(m_target.get_label(), BinSymbolUseKind::INTERNAL);
    text.write_u32(0x14000000);
}

void AArch64Encoder::encode_br(mcode::Instruction &instr) {
    mcode::Operand &m_target = instr.get_operand(0);

    std::uint32_t r_target = encode_gp_reg(m_target.get_physical_reg());
    text.write_u32(0xD61F0000 | (r_target << 5));
}

void AArch64Encoder::encode_b_eq(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x00});
}

void AArch64Encoder::encode_b_ne(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x01});
}

void AArch64Encoder::encode_b_hs(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x02});
}

void AArch64Encoder::encode_b_lo(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x03});
}

void AArch64Encoder::encode_b_hi(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x08});
}

void AArch64Encoder::encode_b_ls(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x09});
}

void AArch64Encoder::encode_b_ge(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x0A});
}

void AArch64Encoder::encode_b_lt(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x0B});
}

void AArch64Encoder::encode_b_gt(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x0C});
}

void AArch64Encoder::encode_b_le(mcode::Instruction &instr) {
    encode_b_cond_family(instr, {0x0D});
}

void AArch64Encoder::encode_bl(mcode::Instruction &instr) {
    mcode::Operand &m_callee = instr.get_operand(0);

    text.add_symbol_use(m_callee.get_symbol().name, BinSymbolUseKind::BRANCH26);
    text.write_u32(0x94000000);
}

void AArch64Encoder::encode_blr(mcode::Instruction &instr) {
    mcode::Operand &m_callee = instr.get_operand(0);

    std::uint32_t r_callee = encode_gp_reg(m_callee.get_physical_reg());
    text.write_u32(0xD63F0000 | (r_callee << 5));
}

void AArch64Encoder::encode_ret(mcode::Instruction & /*instr*/) {
    std::uint32_t r_x30 = 30;
    text.write_u32(0xD65F0000 | (r_x30 << 5));
}

void AArch64Encoder::encode_adrp(mcode::Instruction &instr) {
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_symbol = instr.get_operand(1);

    const mcode::Symbol &symbol = m_symbol.get_symbol();
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());

    text.add_symbol_use(symbol.name, lower_reloc(symbol.reloc));
    text.write_u32(0x90000000 | r_dst);
}

void AArch64Encoder::encode_uxtb(mcode::Instruction &instr) {
    encode_ubfm_family(instr, {0x53001C00});
}

void AArch64Encoder::encode_uxth(mcode::Instruction &instr) {
    encode_ubfm_family(instr, {0x53003C00});
}

void AArch64Encoder::encode_sxtb(mcode::Instruction &instr) {
    encode_sbfm_family(instr, {0x13001C00});
}

void AArch64Encoder::encode_sxth(mcode::Instruction &instr) {
    encode_sbfm_family(instr, {0x13003C00});
}

void AArch64Encoder::encode_sxtw(mcode::Instruction &instr) {
    encode_sbfm_family(instr, {0x13007C00});
}

void AArch64Encoder::encode_movz_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params) {
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_src = instr.get_operand(1);
    mcode::Operand *m_shift = instr.try_get_operand(2);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
    unsigned shift = m_shift ? m_shift->get_aarch64_left_shift() : 0;

    std::uint32_t imm = encode_imm(m_src.get_int_immediate(), 16, 0);
    std::uint32_t hw = encode_imm(shift, 2, 4);
    text.write_u32(params[0] | (sf << 31) | (hw << 21) | (imm << 5) | r_dst);
}

void AArch64Encoder::encode_ldr_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params) {
    mcode::Operand &m_reg = instr.get_operand(0);
    mcode::Operand &m_addr = instr.get_operand(1);
    mcode::Operand *m_post_offset = instr.try_get_operand(2);

    Address addr = lower_addr(m_addr, m_post_offset);
    unsigned imm_shift = instr.get_operand(0).get_size() == 8 ? 3 : 2;
    std::uint32_t addr_bits = encode_addr(addr, imm_shift);

    if (is_gp_reg(m_reg.get_physical_reg())) {
        bool sf = instr.get_operand(0).get_size() == 8;
        std::uint32_t r_reg = encode_gp_reg(m_reg.get_physical_reg());
        text.write_u32(params[0] | (sf << 30) | addr_bits | r_reg);
    } else if (is_fp_reg(m_reg.get_physical_reg())) {
        bool ftype = instr.get_operand(0).get_size() == 8;
        std::uint32_t r_reg = encode_fp_reg(m_reg.get_physical_reg());
        text.write_u32(params[1] | (ftype << 30) | addr_bits | r_reg);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_ldrb_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params) {
    mcode::Operand &m_reg = instr.get_operand(0);
    mcode::Operand &m_addr = instr.get_operand(1);
    mcode::Operand *m_post_offset = instr.try_get_operand(2);

    std::uint32_t r_reg = encode_gp_reg(m_reg.get_physical_reg());

    Address addr = lower_addr(m_addr, m_post_offset);
    unsigned imm_shift = params[0];
    std::uint32_t addr_bits = encode_addr(addr, imm_shift);

    text.write_u32(params[1] | addr_bits | r_reg);
}

void AArch64Encoder::encode_ldp_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params) {
    mcode::Operand &m_reg1 = instr.get_operand(0);
    mcode::Operand &m_reg2 = instr.get_operand(1);
    mcode::Operand &m_dst = instr.get_operand(2);
    mcode::Operand *m_post_offset = instr.try_get_operand(3);

    bool sf = instr.get_operand(0).get_size() == 8;
    unsigned imm_shift = sf ? 3 : 2;
    std::uint32_t r_reg1 = encode_gp_reg(m_reg1.get_physical_reg());
    std::uint32_t r_reg2 = encode_gp_reg(m_reg2.get_physical_reg());
    Address addr = lower_addr(m_dst, m_post_offset);

    if (addr.mode == AddressingMode::POST_INDEX) {
        std::uint32_t r_base = encode_gp_reg(addr.base.get_physical_reg());
        std::uint32_t imm = encode_imm(addr.offset_const, 7, imm_shift);
        text.write_u32(params[0] | (sf << 31) | (imm << 15) | (r_reg2 << 10) | (r_base << 5) | r_reg1);
    } else if (addr.mode == AddressingMode::PRE_INDEX) {
        std::uint32_t r_base = encode_gp_reg(addr.base.get_physical_reg());
        std::uint32_t imm = encode_imm(addr.offset_const, 7, imm_shift);
        text.write_u32(params[1] | (sf << 31) | (imm << 15) | (r_reg2 << 10) | (r_base << 5) | r_reg1);
    } else {
        // There are other addressing modes, but the compiler currently never generates them.
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_add_family(mcode::Instruction &instr, std::array<std::uint32_t, 2> params) {
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_lhs = instr.get_operand(1);
    mcode::Operand &m_rhs = instr.get_operand(2);
    mcode::Operand *m_shift = instr.try_get_operand(3);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
    std::uint32_t r_lhs = encode_gp_reg(m_lhs.get_physical_reg());

    if (m_rhs.is_register()) {
        std::uint32_t r_rhs = encode_gp_reg(m_rhs.get_physical_reg());
        text.write_u32(params[0] | (sf << 31) | (r_rhs << 16) | (r_lhs << 5) | r_dst);
    } else if (m_rhs.is_int_immediate()) {
        bool shifted = false;

        if (m_shift) {
            unsigned shift = m_shift->get_aarch64_left_shift();
            ASSERT(shift == 0 || shift == 12);
            shifted = shift == 12;
        }

        std::uint32_t imm = encode_imm(m_rhs.get_int_immediate(), 12, 0);
        text.write_u32(params[1] | (sf << 31) | (shifted << 22) | (imm << 10) | (r_lhs << 5) | r_dst);
    } else if (m_rhs.is_symbol()) {
        // TODO: This is only allowed with `add` instructions!

        const mcode::Symbol &symbol = m_rhs.get_symbol();
        ASSERT(sf == 1);

        text.add_symbol_use(symbol.name, lower_reloc(symbol.reloc));
        text.write_u32(params[1] | (1 << 31) | (r_lhs << 5) | r_dst);
    } else if (m_rhs.is_stack_slot_offset()) {
        mcode::Operand::StackSlotOffset offset = m_rhs.get_stack_slot_offset();

        mcode::StackFrame &stack_frame = cur_func->get_stack_frame();
        mcode::StackSlot &slot = stack_frame.get_stack_slot(offset.slot_index);
        std::uint64_t total_offset = slot.get_offset() + offset.additional_offset;

        std::uint32_t imm = encode_imm(total_offset, 12, 0);
        text.write_u32(params[1] | (sf << 31) | (imm << 10) | (r_lhs << 5) | r_dst);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_mul_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params) {
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_lhs = instr.get_operand(1);
    mcode::Operand &m_rhs = instr.get_operand(2);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
    std::uint32_t r_lhs = encode_gp_reg(m_lhs.get_physical_reg());
    std::uint32_t r_rhs = encode_gp_reg(m_rhs.get_physical_reg());

    text.write_u32(params[0] | (sf << 31) | (r_rhs << 16) | (r_lhs << 5) | r_dst);
}

void AArch64Encoder::encode_and_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params) {
    ASSERT(instr.get_operands().get_size() == 3);

    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_lhs = instr.get_operand(1);
    mcode::Operand &m_rhs = instr.get_operand(2);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
    std::uint32_t r_lhs = encode_gp_reg(m_lhs.get_physical_reg());

    if (m_rhs.is_register()) {
        std::uint32_t r_rhs = encode_gp_reg(m_rhs.get_physical_reg());
        text.write_u32(params[0] | (sf << 31) | (r_rhs << 16) | (r_lhs << 5) | r_dst);
    } else if (m_rhs.is_int_immediate()) {
        // TODO: Bitmask immediates
        ASSERT_MESSAGE(false, "cannot encode bitmask immediates");
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_lsl_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params) {
    ASSERT(instr.get_operands().get_size() == 3);

    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_lhs = instr.get_operand(1);
    mcode::Operand &m_rhs = instr.get_operand(2);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
    std::uint32_t r_lhs = encode_gp_reg(m_lhs.get_physical_reg());

    if (m_rhs.is_register()) {
        std::uint32_t r_rhs = encode_gp_reg(m_rhs.get_physical_reg());
        text.write_u32(params[0] | (sf << 31) | (r_rhs << 16) | (r_lhs << 5) | r_dst);
    } else if (m_rhs.is_int_immediate()) {
        // TODO: Bitmask immediates
        ASSERT_MESSAGE(false, "cannot encode bitmask immediates");
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Encoder::encode_b_cond_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params) {
    mcode::Operand &m_target = instr.get_operand(0);

    text.add_symbol_use(m_target.get_label(), BinSymbolUseKind::INTERNAL);
    text.write_u32(0x54000000 | params[0]);
}

void AArch64Encoder::encode_ubfm_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params) {
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_src = instr.get_operand(1);

    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
    std::uint32_t r_src = encode_gp_reg(m_src.get_physical_reg());

    text.write_u32(params[0] | (r_src << 5) | r_dst);
}

void AArch64Encoder::encode_sbfm_family(mcode::Instruction &instr, std::array<std::uint32_t, 1> params) {
    mcode::Operand &m_dst = instr.get_operand(0);
    mcode::Operand &m_src = instr.get_operand(1);

    bool sf = instr.get_operand(0).get_size() == 8;
    std::uint32_t r_dst = encode_gp_reg(m_dst.get_physical_reg());
    std::uint32_t r_src = encode_gp_reg(m_src.get_physical_reg());

    text.write_u32(params[0] | (sf << 31) | (sf << 22) | (r_src << 5) | r_dst);
}

std::uint32_t AArch64Encoder::encode_gp_reg(mcode::PhysicalReg reg) {
    if (reg >= AArch64Register::R0 && reg <= AArch64Register::R30) {
        return reg;
    } else if (reg == AArch64Register::SP) {
        return 31;
    } else {
        ASSERT_UNREACHABLE;
    }
}

std::uint32_t AArch64Encoder::encode_fp_reg(mcode::PhysicalReg reg) {
    if (reg >= AArch64Register::V0 && reg <= AArch64Register::V30) {
        return reg - AArch64Register::V0;
    } else {
        ASSERT_UNREACHABLE;
    }
}

std::uint32_t AArch64Encoder::encode_imm(LargeInt imm, unsigned num_bits, unsigned shift) {
    std::uint64_t bits = imm.to_bits();
    ASSERT((bits & ((1ull << shift) - 1)) == 0);
    std::uint32_t mask = (1ull << num_bits) - 1;
    return static_cast<std::uint32_t>(bits >> shift) & mask;
}

std::uint32_t AArch64Encoder::encode_f32_imm(float value) {
    std::uint32_t bits = BitOperations::get_bits_32(value);

    ASSERT((bits & 0x0007FFFF) == 0);
    std::uint32_t b_bits = (bits & 0x7E000000) >> 25;
    ASSERT(b_bits == 0b100000 || b_bits == 0b011111);

    return ((bits & 0x80000000) >> 24) | ((bits & 0x03F80000) >> 19);
}

std::uint32_t AArch64Encoder::encode_f64_imm(double value) {
    std::uint64_t bits = BitOperations::get_bits_64(value);

    ASSERT((bits & 0x0000FFFFFFFFFFFF) == 0);
    std::uint32_t b_bits = (bits & 0x7FC0000000000000) >> 54;
    ASSERT(b_bits == 0b100000000 || b_bits == 0b011111111);

    return ((bits & 0x8000000000000000) >> 56) | ((bits & 0x007F000000000000) >> 48);
}

std::uint32_t AArch64Encoder::encode_addr(Address &addr, unsigned imm_shift) {
    std::uint32_t r_base = encode_gp_reg(addr.base.get_physical_reg());

    if (addr.mode == AddressingMode::OFFSET_CONST) {
        if (addr.offset_const >= 0 && (addr.offset_const & ((1 << imm_shift) - 1)) == 0) {
            std::uint32_t imm = encode_imm(addr.offset_const, 12, imm_shift);
            return 0x01000000 | (imm << 10) | (r_base << 5);
        } else {
            // TODO: Move `ldur*` and `stur*` into separate instructions.
            std::uint32_t imm = encode_imm(addr.offset_const, 9, 0);
            return 0x00000000 | (imm << 12) | (r_base << 5);
        }
    } else if (addr.mode == AddressingMode::OFFSET_SYMBOL) {
        text.add_symbol_use(addr.offset_symbol.name, lower_reloc(addr.offset_symbol.reloc));
        return 0x01000000 | (r_base << 5);
    } else if (addr.mode == AddressingMode::POST_INDEX) {
        std::uint32_t imm = encode_imm(addr.offset_const, 9, 0);
        return 0x00000400 | (imm << 12) | (r_base << 5);
    } else if (addr.mode == AddressingMode::PRE_INDEX) {
        std::uint32_t imm = encode_imm(addr.offset_const, 9, 0);
        return 0x00000C00 | (imm << 12) | (r_base << 5);
    } else {
        ASSERT_UNREACHABLE;
    }
}

AArch64Encoder::Address AArch64Encoder::lower_addr(mcode::Operand &operand, mcode::Operand *post_operand) {
    if (operand.is_stack_slot()) {
        mcode::StackFrame &stack_frame = cur_func->get_stack_frame();
        mcode::StackSlot &slot = stack_frame.get_stack_slot(operand.get_stack_slot());

        return Address{
            .mode = AddressingMode::OFFSET_CONST,
            .base = mcode::Register::from_physical(AArch64Register::SP),
            .offset_const = slot.get_offset(),
        };
    } else if (operand.is_aarch64_addr()) {
        const target::AArch64Address &addr = operand.get_aarch64_addr();

        if (addr.get_type() == AArch64Address::Type::BASE) {
            if (post_operand) {
                LargeInt offset = post_operand->get_int_immediate();

                return Address{
                    .mode = AddressingMode::POST_INDEX,
                    .base = addr.get_base(),
                    .offset_const = offset,
                };
            } else {
                return Address{
                    .mode = AddressingMode::OFFSET_CONST,
                    .base = addr.get_base(),
                    .offset_const = 0,
                };
            }
        } else if (addr.get_type() == AArch64Address::Type::BASE_OFFSET_IMM) {
            LargeInt offset = addr.get_offset_imm();

            ASSERT(!post_operand);

            return Address{
                .mode = AddressingMode::OFFSET_CONST,
                .base = addr.get_base(),
                .offset_const = offset,
            };
        } else if (addr.get_type() == AArch64Address::Type::BASE_OFFSET_IMM_WRITE) {
            LargeInt offset = addr.get_offset_imm();

            ASSERT(!post_operand);

            return Address{
                .mode = AddressingMode::PRE_INDEX,
                .base = addr.get_base(),
                .offset_const = offset,
            };
        } else if (addr.get_type() == AArch64Address::Type::BASE_OFFSET_REG) {
            return Address{
                .mode = AddressingMode::OFFSET_CONST,
                .base = addr.get_base(),
                .offset_reg = addr.get_offset_reg(),
            };
        } else if (addr.get_type() == AArch64Address::Type::BASE_OFFSET_SYMBOL) {
            return Address{
                .mode = AddressingMode::OFFSET_SYMBOL,
                .base = addr.get_base(),
                .offset_symbol = addr.get_offset_symbol(),
            };
        } else {
            ASSERT_UNREACHABLE;
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

BinSymbolUseKind AArch64Encoder::lower_reloc(mcode::Relocation reloc) {
    switch (reloc) {
        case mcode::Relocation::PAGE21: return BinSymbolUseKind::PAGE21;
        case mcode::Relocation::PAGEOFF12: return BinSymbolUseKind::PAGEOFF12;
        case mcode::Relocation::GOT_LOAD_PAGE21: return BinSymbolUseKind::GOT_LOAD_PAGE21;
        case mcode::Relocation::GOT_LOAD_PAGEOFF12: return BinSymbolUseKind::GOT_LOAD_PAGEOFF12;
        default: ASSERT_UNREACHABLE;
    }
}

bool AArch64Encoder::is_gp_reg(mcode::PhysicalReg reg) {
    return (reg >= AArch64Register::R0 && reg <= AArch64Register::R30) || reg == AArch64Register::SP;
}

bool AArch64Encoder::is_fp_reg(mcode::PhysicalReg reg) {
    return reg >= AArch64Register::V0 && reg <= AArch64Register::V30;
}

void AArch64Encoder::resolve_internal_symbols() {
    for (SectionBuilder::SectionSlice &slice : text.get_slices()) {
        for (SymbolUse &use : slice.uses) {
            resolve_symbol(slice, use);
        }
    }
}

void AArch64Encoder::resolve_symbol(SectionBuilder::SectionSlice &slice, SymbolUse &use) {
    SymbolDef &def = defs[use.index];

    if (def.kind != BinSymbolKind::TEXT_LABEL && def.kind != BinSymbolKind::TEXT_FUNC) {
        return;
    }

    slice.buffer.seek(use.local_offset);
    std::uint32_t instr_template = slice.buffer.read_u32();
    slice.buffer.seek(use.local_offset);

    std::uint8_t first_byte = instr_template >> 24;
    std::uint32_t displacement = compute_displacement(slice, use);

    if (first_byte == 0x14 || first_byte == 0x94) {
        // `B` and `BL` instructions encode displacements with 26 bits.
        std::uint32_t imm = encode_imm(displacement, 26, 2);
        slice.buffer.write_u32(instr_template | imm);
    } else if (first_byte == 0x54) {
        // `B.cond` instructions encode displacements with 19 bits.
        std::uint32_t imm = encode_imm(displacement, 19, 2);
        slice.buffer.write_u32(instr_template | (imm << 5));
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace target
} // namespace banjo
