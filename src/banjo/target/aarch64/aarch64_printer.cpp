#include "aarch64_printer.hpp"

#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/hash_map.hpp"
#include "banjo/utils/macros.hpp"

#include <string_view>

namespace banjo::target {

// clang-format off
static const HashMap<mcode::Opcode, std::string_view> OPCODES{
    {target::AArch64Opcode::MOV, "mov"},
    {target::AArch64Opcode::MOVZ, "movz"},
    {target::AArch64Opcode::MOVK, "movk"},
    {target::AArch64Opcode::LDR, "ldr"},
    {target::AArch64Opcode::LDRB, "ldrb"},
    {target::AArch64Opcode::LDRH, "ldrh"},
    {target::AArch64Opcode::STR, "str"},
    {target::AArch64Opcode::STRB, "strb"},
    {target::AArch64Opcode::STRH, "strh"},
    {target::AArch64Opcode::LDP, "ldp"},
    {target::AArch64Opcode::STP, "stp"},
    {target::AArch64Opcode::LDAR, "ldar"},
    {target::AArch64Opcode::LDARB, "ldarb"},
    {target::AArch64Opcode::LDARH, "ldarh"},
    {target::AArch64Opcode::LDAXR, "ldaxr"},
    {target::AArch64Opcode::LDAXRB, "ldaxrb"},
    {target::AArch64Opcode::LDAXRH, "ldaxrh"},
    {target::AArch64Opcode::STLR, "stlr"},
    {target::AArch64Opcode::STLRB, "stlrb"},
    {target::AArch64Opcode::STLRH, "stlrh"},
    {target::AArch64Opcode::STLXR, "stlxr"},
    {target::AArch64Opcode::STLXRB, "stlxrb"},
    {target::AArch64Opcode::STLXRH, "stlxrh"},
    {target::AArch64Opcode::ADD, "add"},
    {target::AArch64Opcode::SUB, "sub"},
    {target::AArch64Opcode::MUL, "mul"},
    {target::AArch64Opcode::MADD, "madd"},
    {target::AArch64Opcode::MSUB, "msub"},
    {target::AArch64Opcode::SDIV, "sdiv"},
    {target::AArch64Opcode::UDIV, "udiv"},
    {target::AArch64Opcode::AND, "and"},
    {target::AArch64Opcode::ORR, "orr"},
    {target::AArch64Opcode::EOR, "eor"},
    {target::AArch64Opcode::LSL, "lsl"},
    {target::AArch64Opcode::LSR, "lsr"},
    {target::AArch64Opcode::ASR, "asr"},
    {target::AArch64Opcode::CSEL, "csel"},
    {target::AArch64Opcode::FMOV, "fmov"},
    {target::AArch64Opcode::FADD, "fadd"},
    {target::AArch64Opcode::FSUB, "fsub"},
    {target::AArch64Opcode::FMUL, "fmul"},
    {target::AArch64Opcode::FDIV, "fdiv"},
    {target::AArch64Opcode::FCVT, "fcvt"},
    {target::AArch64Opcode::SCVTF, "scvtf"},
    {target::AArch64Opcode::UCVTF, "ucvtf"},
    {target::AArch64Opcode::FCVTZS, "fcvtzs"},
    {target::AArch64Opcode::FCVTZU, "fcvtzu"},
    {target::AArch64Opcode::FCSEL, "fcsel"},
    {target::AArch64Opcode::CMP, "cmp"},
    {target::AArch64Opcode::FCMP, "fcmp"},
    {target::AArch64Opcode::B, "b"},
    {target::AArch64Opcode::BR, "br"},
    {target::AArch64Opcode::B_EQ, "b.eq"},
    {target::AArch64Opcode::B_NE, "b.ne"},
    {target::AArch64Opcode::B_HS, "b.hs"},
    {target::AArch64Opcode::B_LO, "b.lo"},
    {target::AArch64Opcode::B_HI, "b.hi"},
    {target::AArch64Opcode::B_LS, "b.ls"},
    {target::AArch64Opcode::B_GE, "b.ge"},
    {target::AArch64Opcode::B_LT, "b.lt"},
    {target::AArch64Opcode::B_GT, "b.gt"},
    {target::AArch64Opcode::B_LE, "b.le"},
    {target::AArch64Opcode::BL, "bl"},
    {target::AArch64Opcode::BLR, "blr"},
    {target::AArch64Opcode::RET, "ret"},
    {target::AArch64Opcode::ADRP, "adrp"},
    {target::AArch64Opcode::UXTB, "uxtb"},
    {target::AArch64Opcode::UXTH, "uxth"},
    {target::AArch64Opcode::SXTB, "sxtb"},
    {target::AArch64Opcode::SXTH, "sxth"},
    {target::AArch64Opcode::SXTW, "sxtw"},
};
// clang-format on

// clang-format off
static const HashMap<mcode::PhysicalReg, std::string_view> REGISTERS_4{
    {target::AArch64Register::R0, "w0"},
    {target::AArch64Register::R1, "w1"},
    {target::AArch64Register::R2, "w2"},
    {target::AArch64Register::R3, "w3"},
    {target::AArch64Register::R4, "w4"},
    {target::AArch64Register::R5, "w5"},
    {target::AArch64Register::R6, "w6"},
    {target::AArch64Register::R7, "w7"},
    {target::AArch64Register::R8, "w8"},
    {target::AArch64Register::R9, "w9"},
    {target::AArch64Register::R10, "w10"},
    {target::AArch64Register::R11, "w11"},
    {target::AArch64Register::R12, "w12"},
    {target::AArch64Register::R13, "w13"},
    {target::AArch64Register::R14, "w14"},
    {target::AArch64Register::R15, "w15"},
    {target::AArch64Register::R16, "w16"},
    {target::AArch64Register::R17, "w17"},
    {target::AArch64Register::R18, "w18"},
    {target::AArch64Register::R19, "w19"},
    {target::AArch64Register::R20, "w20"},
    {target::AArch64Register::R21, "w21"},
    {target::AArch64Register::R22, "w22"},
    {target::AArch64Register::R23, "w23"},
    {target::AArch64Register::R24, "w24"},
    {target::AArch64Register::R25, "w25"},
    {target::AArch64Register::R26, "w26"},
    {target::AArch64Register::R27, "w27"},
    {target::AArch64Register::R28, "w28"},
    {target::AArch64Register::R29, "w29"},
    {target::AArch64Register::R30, "w30"},
    {target::AArch64Register::V0, "s0"},
    {target::AArch64Register::V1, "s1"},
    {target::AArch64Register::V2, "s2"},
    {target::AArch64Register::V3, "s3"},
    {target::AArch64Register::V4, "s4"},
    {target::AArch64Register::V5, "s5"},
    {target::AArch64Register::V6, "s6"},
    {target::AArch64Register::V7, "s7"},
    {target::AArch64Register::V8, "s8"},
    {target::AArch64Register::V9, "s9"},
    {target::AArch64Register::V10, "s10"},
    {target::AArch64Register::V11, "s11"},
    {target::AArch64Register::V12, "s12"},
    {target::AArch64Register::V13, "s13"},
    {target::AArch64Register::V14, "s14"},
    {target::AArch64Register::V15, "s15"},
    {target::AArch64Register::V16, "s16"},
    {target::AArch64Register::V17, "s17"},
    {target::AArch64Register::V18, "s18"},
    {target::AArch64Register::V19, "s19"},
    {target::AArch64Register::V20, "s20"},
    {target::AArch64Register::V21, "s21"},
    {target::AArch64Register::V22, "s22"},
    {target::AArch64Register::V23, "s23"},
    {target::AArch64Register::V24, "s24"},
    {target::AArch64Register::V25, "s25"},
    {target::AArch64Register::V26, "s26"},
    {target::AArch64Register::V27, "s27"},
    {target::AArch64Register::V28, "s28"},
    {target::AArch64Register::V29, "s29"},
    {target::AArch64Register::V30, "s30"},
};

// clang-format off
static const HashMap<mcode::PhysicalReg, std::string_view> REGISTERS_8{
    {target::AArch64Register::R0, "x0"},
    {target::AArch64Register::R1, "x1"},
    {target::AArch64Register::R2, "x2"},
    {target::AArch64Register::R3, "x3"},
    {target::AArch64Register::R4, "x4"},
    {target::AArch64Register::R5, "x5"},
    {target::AArch64Register::R6, "x6"},
    {target::AArch64Register::R7, "x7"},
    {target::AArch64Register::R8, "x8"},
    {target::AArch64Register::R9, "x9"},
    {target::AArch64Register::R10, "x10"},
    {target::AArch64Register::R11, "x11"},
    {target::AArch64Register::R12, "x12"},
    {target::AArch64Register::R13, "x13"},
    {target::AArch64Register::R14, "x14"},
    {target::AArch64Register::R15, "x15"},
    {target::AArch64Register::R16, "x16"},
    {target::AArch64Register::R17, "x17"},
    {target::AArch64Register::R18, "x18"},
    {target::AArch64Register::R19, "x19"},
    {target::AArch64Register::R20, "x20"},
    {target::AArch64Register::R21, "x21"},
    {target::AArch64Register::R22, "x22"},
    {target::AArch64Register::R23, "x23"},
    {target::AArch64Register::R24, "x24"},
    {target::AArch64Register::R25, "x25"},
    {target::AArch64Register::R26, "x26"},
    {target::AArch64Register::R27, "x27"},
    {target::AArch64Register::R28, "x28"},
    {target::AArch64Register::R29, "x29"},
    {target::AArch64Register::R30, "x30"},
    {target::AArch64Register::V0, "d0"},
    {target::AArch64Register::V1, "d1"},
    {target::AArch64Register::V2, "d2"},
    {target::AArch64Register::V3, "d3"},
    {target::AArch64Register::V4, "d4"},
    {target::AArch64Register::V5, "d5"},
    {target::AArch64Register::V6, "d6"},
    {target::AArch64Register::V7, "d7"},
    {target::AArch64Register::V8, "d8"},
    {target::AArch64Register::V9, "d9"},
    {target::AArch64Register::V10, "d10"},
    {target::AArch64Register::V11, "d11"},
    {target::AArch64Register::V12, "d12"},
    {target::AArch64Register::V13, "d13"},
    {target::AArch64Register::V14, "d14"},
    {target::AArch64Register::V15, "d15"},
    {target::AArch64Register::V16, "d16"},
    {target::AArch64Register::V17, "d17"},
    {target::AArch64Register::V18, "d18"},
    {target::AArch64Register::V19, "d19"},
    {target::AArch64Register::V20, "d20"},
    {target::AArch64Register::V21, "d21"},
    {target::AArch64Register::V22, "d22"},
    {target::AArch64Register::V23, "d23"},
    {target::AArch64Register::V24, "d24"},
    {target::AArch64Register::V25, "d25"},
    {target::AArch64Register::V26, "d26"},
    {target::AArch64Register::V27, "d27"},
    {target::AArch64Register::V28, "d28"},
    {target::AArch64Register::V29, "d29"},
    {target::AArch64Register::V30, "d30"},
    {target::AArch64Register::SP, "sp"},
};
// clang-format on

void AArch64Printer::print_opcode(mcode::Instruction &instr) {
    emit(OPCODES.find(instr.get_opcode()));
}

void AArch64Printer::print_operand(mcode::Instruction &instr, unsigned index) {
    mcode::Operand &operand = instr.get_operand(index);

    if (operand.is_int_immediate()) {
        emit(operand.get_int_immediate().to_string());
    } else if (operand.is_fp_immediate()) {
        emit(operand.get_fp_immediate());
    } else if (operand.is_register()) {
        print_register(operand.get_register(), operand.get_size());
    } else if (operand.is_aarch64_addr()) {
        print_address(operand.get_aarch64_addr());
    } else {
        emit("<operand>");
    }
}

void AArch64Printer::print_register(mcode::Register reg, unsigned size) {
    if (reg.is_virtual()) {
        print_virtual_reg(reg.get_virtual_reg(), size);
    } else if (reg.is_physical()) {
        if (size == 8) {
            emit(REGISTERS_8.find(reg.get_physical_reg()));
        } else {
            emit(REGISTERS_4.find(reg.get_physical_reg()));
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

void AArch64Printer::print_address(const AArch64Address &address) {
    emit('[');
    print_register(address.get_base(), 8);

    switch (address.get_type()) {
        case AArch64Address::Type::BASE: break;
        case AArch64Address::Type::BASE_OFFSET_IMM:
        case AArch64Address::Type::BASE_OFFSET_IMM_WRITE:
            emit(", ");
            emit(address.get_offset_imm());
            break;

        case AArch64Address::Type::BASE_OFFSET_STACK_ADDR: ASSERT_UNREACHABLE; // TODO
        case AArch64Address::Type::BASE_OFFSET_REG: ASSERT_UNREACHABLE;        // TODO
        case AArch64Address::Type::BASE_OFFSET_SYMBOL: ASSERT_UNREACHABLE;     // TODO
    }

    emit(']');

    if (address.get_type() == AArch64Address::Type::BASE_OFFSET_IMM_WRITE) {
        emit('!');
    }
}

} // namespace banjo::target
